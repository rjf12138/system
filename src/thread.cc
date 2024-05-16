#include "system.h"

namespace os {

Thread::Thread(void)
:is_init_(false),
thread_id_(0)
{
    this->set_stream_func(LOG_LEVEL_TRACE, g_msg_to_stream_trace);
    this->set_stream_func(LOG_LEVEL_DEBUG, g_msg_to_stream_debug);
    this->set_stream_func(LOG_LEVEL_INFO, g_msg_to_stream_info);
    this->set_stream_func(LOG_LEVEL_WARN, g_msg_to_stream_warn);
    this->set_stream_func(LOG_LEVEL_ERROR, g_msg_to_stream_error);
    this->set_stream_func(LOG_LEVEL_FATAL, g_msg_to_stream_fatal);
}

Thread::~Thread(void) 
{
    if (is_init_ == true) {
        pthread_attr_destroy(&attr_);
    }
}

void* 
Thread::create_func(void* arg)
{
    if (arg == NULL) {
        return NULL;
    }

    Thread *self = reinterpret_cast<Thread*>(arg);
    self->run_handler();

    return arg;
}

int 
Thread::init(void)
{
    this->start_handler();
    is_init_ = true;
#ifdef __RJF_LINUX__
    pthread_attr_init(&attr_);
    pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_DETACHED);
    int ret = ::pthread_create(&thread_id_, &attr_, create_func, reinterpret_cast<void*>(this));
    if (ret != 0) {
        LOG_ERROR("pthread_create(): %s", strerror(ret));
        return -1;
    }
#endif

    return 0;
}

thread_id_t 
Thread::current_thread_id(void)
{
#ifdef __RJF_LINUX__
    return pthread_self();
#endif
}

int Thread::run_handler(void) { return 0;}
int Thread::stop_handler(void) {return 0; }
int Thread::start_handler(void) {return 0; }

////////////////////////////////////// WorkThread ///////////////////////////////////////
WorkThread::WorkThread(ThreadPool *thread_pool, int idle_life)
: idle_life_(idle_life), 
thread_pool_(thread_pool),
state_(WorkThread_WAITING)
{
    start_idle_life_ = time(NULL);
#ifdef __RJF_LINUX__
    pthread_cond_init(&thread_cond_, NULL);
#endif
}

WorkThread::~WorkThread(void)
{
#ifdef __RJF_LINUX__
    pthread_cond_destroy(&thread_cond_);
#endif
}

void 
WorkThread::thread_cond_wait(void)
{
#ifdef __RJF_LINUX__
    pthread_cond_wait(&thread_cond_, mutex_.get_mutex());
#endif
}

void 
WorkThread::thread_cond_signal(void)
{
#ifdef __RJF_LINUX__
    pthread_cond_signal(&thread_cond_);
#endif
}

int 
WorkThread::run_handler(void)
{
    while (state_ != WorkThread_WAITINGEXIT) {
        mutex_.lock();
        while (state_ == WorkThread_WAITING) {
            thread_cond_wait();
        }
        mutex_.unlock();
        if (state_ == WorkThread_RUNNING) {
            while (thread_pool_->get_task(task_) > 0) {
                task_.state = THREAD_TASK_HANDLE;
                task_.work_func(task_.thread_arg);
                task_.state = THREAD_TASK_COMPLETE;
            }
            // 没有任务就休眠
            this->pause();
        }
    }
    state_ = WorkThread_EXIT;
    return 0;
}

int
WorkThread::stop_handler(void)
{
    if (state_ == WorkThread_EXIT || state_ == WorkThread_WAITINGEXIT) {
        return 0;
    }
    // 设置线程为退出状态
    int old_state = state_;
    state_ = WorkThread_WAITINGEXIT;
    // 运行中的线程，调用由任务发布者设置的任务退出函数
    if (old_state == WorkThread_RUNNING) {
        if (task_.exit_task != nullptr) {
            task_.exit_task(task_.exit_arg);
        }
    } else if (old_state == WorkThread_WAITING) { // 唤醒空闲线程，然后走结束线程的流程
        this->resume();
    }

    return 0;
}

int 
WorkThread::start_handler(void)
{
    state_ = WorkThread_WAITING;

    return 0;
}

int 
WorkThread::pause(void)
{
    mutex_.lock();
    if (state_ == WorkThread_RUNNING) {
        state_ = WorkThread_WAITING;
        start_idle_life_ = time(NULL);
    }
    mutex_.unlock();

    return 0;
}

int 
WorkThread::resume(void)
{
    mutex_.lock();
    if (state_ == WorkThread_WAITING) {
        state_ = WorkThread_RUNNING;
    }
    mutex_.unlock();
    thread_cond_signal();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ThreadPool::ThreadPool(void)
: exit_(false)
{
    thread_pool_config_.threads_num = 1;
    thread_pool_config_.max_waiting_task = 500;
    thread_pool_config_.threadpool_exit_action = SHUTDOWN_ALL_THREAD_IMMEDIATELY;

    // 运行所有线程
    thread_mutex_.lock();
    for (std::size_t i = 0; i < thread_pool_config_.threads_num; ++i) {
        WorkThread *work_thread = new WorkThread(this);
        work_thread->init();
        threads_[work_thread->get_thread_id()] = work_thread;
    }
    thread_mutex_.unlock();
    // 运行线程池
    this->init();
}

ThreadPool::~ThreadPool(void)
{
    this->stop_handler();
    for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
        delete iter->second;
    }
}

int 
ThreadPool::run_handler(void)
{
    mtime_t curr = Time::now();
    while (!exit_) {
        if (Time::now() - curr > 2000) {
            ajust_threads_num();
            curr = Time::now();
        }

        if (tasks_.size() > 0) {
            wakeup_random_thread();
        }

        Time::sleep(500);
    }

    return 0;
}

int ThreadPool::stop_handler(void)
{
    exit_ = true;
    this->shutdown_all_threads();
    return 0;
}

int 
ThreadPool::start_handler(void)
{
    exit_ = false;
    return 0;
}

int 
ThreadPool::add_task(Task &task)
{
    // 关闭线程池时，停止任务的添加
    if (exit_) {
        LOG_INFO("Thread pool waiting for stop, can't add task");
        return 0;
    }

    task_mutex_.lock();
    if (tasks_.size() >= static_cast<int>(thread_pool_config_.max_waiting_task)) {
        LOG_INFO("The number of tasks in the cache has reached the maximum.[size: %d, max: %d]", tasks_.size(), thread_pool_config_.max_waiting_task);
        task_mutex_.unlock();
        return 0;
    }

    int ret = tasks_.push(task);
    task_mutex_.unlock();

    return ret;
}

int 
ThreadPool::add_priority_task(Task &task)
{
    // 关闭线程池时，停止任务的添加
    if (exit_) {
        LOG_INFO("Thread pool waiting for stop, can't add task");
        return 0;
    }

    task_mutex_.lock();
    if (priority_tasks_.size() >= static_cast<int>(thread_pool_config_.max_waiting_task)) {
        LOG_INFO("The number of priority_tasks in the cache has reached the maximum.[size: %d, max: %d]", priority_tasks_.size(), thread_pool_config_.max_waiting_task);
        task_mutex_.unlock();
        return 0;
    }

    int ret = priority_tasks_.push(task);
    task_mutex_.unlock();

    return ret;
}

int 
ThreadPool::get_task(Task &task)
{
    int ret = 0;
    task_mutex_.lock();
    if (priority_tasks_.size() > 0) {
        ret = priority_tasks_.pop(task);
    } else {
        ret = tasks_.pop(task);
    }
    task_mutex_.unlock();

    return ret;
}

void
ThreadPool::wakeup_random_thread(void)
{
    thread_mutex_.lock();
    for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
        if (iter->second->get_current_state() != WorkThread_WAITING) {
            continue;
        }
        iter->second->resume();
    }
    thread_mutex_.unlock();
}

int 
ThreadPool::ajust_threads_num(void)
{
    thread_mutex_.lock();
    if (threads_.size() < thread_pool_config_.threads_num) {
        std::size_t start_threads = thread_pool_config_.threads_num - threads_.size();
        for (std::size_t i = 0; i < start_threads; ++i) {
            WorkThread *work_thread = new WorkThread(this);
            work_thread->init();
            threads_[work_thread->get_thread_id()] = work_thread;
        }
    } else if (threads_.size() > thread_pool_config_.threads_num) {
        std::size_t stop_threads = thread_pool_config_.threads_num - threads_.size();
        for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
            if (stop_threads > 0) {
                if (iter->second->get_current_state() == WorkThread_WAITING) {
                    WorkThread *del_work_thread_ptr = iter->second;
                    iter = threads_.erase(iter);
                    del_work_thread_ptr->stop_handler();
                    delete del_work_thread_ptr;
                }
            }
        }
    }
    thread_mutex_.unlock();
    return 0;
}

int 
ThreadPool::set_threadpool_config(const ThreadPoolConfig &config)
{
    if (config.threads_num <= 1 || config.threads_num > MAX_THREADS_NUM) {
        LOG_WARN("thread num is out of range --- %d", config.threads_num);
    } else {
        thread_pool_config_.threads_num = config.threads_num;
    }

    if (config.max_waiting_task > 50) {
        thread_pool_config_.max_waiting_task = config.max_waiting_task;
    }

    if (config.threadpool_exit_action == WAIT_FOR_ALL_TASKS_FINISHED ) {
        thread_pool_config_.threadpool_exit_action = WAIT_FOR_ALL_TASKS_FINISHED;
    }

    return 0;
}

int 
ThreadPool::shutdown_all_threads(void)
{
    int action = thread_pool_config_.threadpool_exit_action;
    LOG_INFO("Action: %s", action == WAIT_FOR_ALL_TASKS_FINISHED ? "WAIT_FOR_ALL_TASKS_FINISHED":"SHUTDOWN_ALL_THREAD_IMMEDIATELY");
    
    if (action == WAIT_FOR_ALL_TASKS_FINISHED) {
        while (true) {
            int running_thread_cnt = 0;
            for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
                if (iter->second->get_current_state() == WorkThread_WAITING) {
                    ++running_thread_cnt;
                }
            }
            LOG_INFO("Wait for task finish, Working threads num: %d", running_thread_cnt);
            if (running_thread_cnt <= 0) {
                break;
            }
        }
    }

    for (auto iter = threads_.begin(); iter != threads_.end(); ) {
        auto stop_iter = iter++;
        stop_iter->second->stop_handler();
        
        while (stop_iter->second->get_current_state() != WorkThread_EXIT) {
            Time::sleep(10);
        }
    }

    return 0;
}


ThreadPoolRunningInfo 
ThreadPool::get_running_info(void)
{
    ThreadPoolRunningInfo running_info;
    running_info.config = thread_pool_config_;
    running_info.curr_status = (exit_ == false ? "running":"stopping"); // true 表示退出

    if (thread_pool_config_.threadpool_exit_action == WAIT_FOR_ALL_TASKS_FINISHED) {
        running_info.exit_action =  "WAIT_FOR_ALL_TASKS_FINISHED";
    } else if (thread_pool_config_.threadpool_exit_action == SHUTDOWN_ALL_THREAD_IMMEDIATELY) {
        running_info.exit_action =  "SHUTDOWN_ALL_THREAD_IMMEDIATELY";
    } else {
        running_info.exit_action =  "UNKNOWN_ACTION";
    }

    running_info.running_threads_num = 0;
    running_info.idle_threads_num = 0;
    for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
        ++running_info.running_threads_num;
        ++running_info.idle_threads_num;
    }
    running_info.waiting_tasks = static_cast<int>(tasks_.size());
    running_info.waiting_prio_tasks = static_cast<int>(priority_tasks_.size());

    return running_info;
}

void 
ThreadPool::show_threadpool_info(void)
{
    os::Task task;
    task.work_func = print_threadpool_info;
    task.thread_arg = this;

    add_task(task);
}

void*
ThreadPool::print_threadpool_info(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    ThreadPool* thread_pool_ptr = reinterpret_cast<ThreadPool*>(arg);
    while (thread_pool_ptr->exit_ == false) {
        std::ostringstream ostr;
        ThreadPoolRunningInfo info = thread_pool_ptr->get_running_info();

        ostr << "==================thread pool config==========================" << std::endl;
        ostr << "max-threads: " << thread_pool_ptr->thread_pool_config_.threads_num << std::endl;
        ostr << "max-waiting-tasks: " << thread_pool_ptr->thread_pool_config_.max_waiting_task << std::endl;
        ostr << "action when threadpool exit: " << info.exit_action << std::endl;
        ostr << "==================thread pool realtime data===================" << std::endl;
        ostr << "running threads: " << info.running_threads_num << std::endl;
        ostr << "idle threads: " << info.idle_threads_num << std::endl;
        ostr << "waiting tasks: " << thread_pool_ptr->tasks_.size() << std::endl;
        ostr << "waiting prio tasks: " << thread_pool_ptr->priority_tasks_.size() << std::endl;
        ostr << "thread pool state: " << info.curr_status << std::endl;
        ostr << "===========================end================================" << std::endl;
        LOG_GLOBAL_TRACE("\n%s", ostr.str().c_str());

        Time::sleep(2000);
    }
    
    return nullptr;
}

}
