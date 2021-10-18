#include "system.h"

namespace os {

Thread::Thread(void) 
{
    this->set_stream_func(LOG_LEVEL_TRACE, g_msg_to_stream_trace);
    this->set_stream_func(LOG_LEVEL_DEBUG, g_msg_to_stream_debug);
    this->set_stream_func(LOG_LEVEL_INFO, g_msg_to_stream_info);
    this->set_stream_func(LOG_LEVEL_WARN, g_msg_to_stream_warn);
    this->set_stream_func(LOG_LEVEL_ERROR, g_msg_to_stream_error);
    this->set_stream_func(LOG_LEVEL_FATAL, g_msg_to_stream_fatal);
}

Thread::~Thread(void) { }

void* 
Thread::create_func(void* arg)
{
    if (arg == NULL) {
        return NULL;
    }

    Thread *self = (Thread*)arg;
    self->run_handler();

    return arg;
}

int 
Thread::init(void)
{
    this->start_handler();
#ifdef __RJF_LINUX__
    int ret = ::pthread_create(&thread_id_, NULL, create_func, (void*)this);
    if (ret != 0) {
        LOG_ERROR("pthread_create(): %s", strerror(ret));
        return -1;
    }
#endif

    return 0;
}

int Thread::run_handler(void) { return 0;}
int Thread::stop_handler(void) {return 0; }
int Thread::start_handler(void) {return 0; }

int 
Thread::wait_thread(void)
{
#ifdef __RJF_LINUX__
    int ret = ::pthread_join(thread_id_, NULL);
    if (ret != 0) {
        LOG_ERROR("pthread_join(): %s", strerror(ret));
        return -1;
    }
#endif

    return 0;
}

////////////////////////////////////// WorkThread ///////////////////////////////////////
WorkThread::WorkThread(ThreadPool *thread_pool, int idle_life)
: idle_life_(idle_life), thread_pool_(thread_pool)
{
    start_idle_life_ = time(NULL);
    thread_id_ = (int64_t)this;
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
    while (state_ != WorkThread_EXIT) {
        mutex_.lock();
        while (state_ == WorkThread_WAITING) {
            thread_cond_wait();
        }
        mutex_.unlock();
        if (state_ == WorkThread_RUNNING) {
            if (thread_pool_->get_task(task_) > 0) {
                task_.state = THREAD_TASK_HANDLE;
                task_.work_func(task_.thread_arg);
                task_.state = THREAD_TASK_COMPLETE;
            }
        }

        this->pause();
    }
    // 从线程池中删除关闭的线程信息
    thread_pool_->remove_thread((int64_t)this);

    return 0;
}

int
WorkThread::stop_handler(void)
{
    // 设置线程为退出状态
    int old_state = state_;
    state_ = WorkThread_EXIT;
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
    if (state_ == WorkThread_RUNNING) {
        mutex_.lock();
        state_ = WorkThread_WAITING;
        start_idle_life_ = time(NULL);
        mutex_.unlock();
        thread_pool_->thread_move_to_idle_map((int64_t)this);
    }

    return 0;
}

int 
WorkThread::resume(void)
{
    if (state_ == WorkThread_WAITING) {
        state_ = WorkThread_RUNNING;
        thread_pool_->thread_move_to_running_map((int64_t)this);
    }

    mutex_.lock();
    thread_cond_signal();
    mutex_.unlock();

    return 0;
}

int 
WorkThread::idle_timeout(void)
{
    if (start_idle_life_ + idle_life_ < time(NULL)) {
        return true;
    }

    return false;
}

int 
WorkThread::reset_idle_life(void) 
{
    start_idle_life_ = time(NULL); 

    return start_idle_life_;
}

///////////////////////////////////////////////////////////////////////////////////
ThreadPool::ThreadPool(void)
: exit_(false),
 state_(WorkThread_WAITING)
{
    int min_cores = SystemInfo::get_nprocs() / 2;
    // 根据CPU数和核心数来设置线程的最大最小值
    thread_pool_config_.min_thread_num = (min_cores <= 0 ? 1 : min_cores);
    thread_pool_config_.max_thread_num = thread_pool_config_.min_thread_num * 2;

    thread_pool_config_.idle_thread_life = 30;
    thread_pool_config_.max_waiting_task = 500;
    thread_pool_config_.threadpool_exit_action = SHUTDOWN_ALL_THREAD_IMMEDIATELY;

    this->manage_work_threads(true);
}

ThreadPool::~ThreadPool(void)
{

}

int 
ThreadPool::run_handler(void)
{
    while (!exit_) {
        this->manage_work_threads(false);
    }

    return 0;
}

int ThreadPool::stop_handler(void)
{
    this->shutdown_all_threads();
    exit_ = true;
    state_ = WorkThread_EXIT;
    return 0;
}

int 
ThreadPool::start_handler(void)
{
    exit_ = false;
    state_ = WorkThread_RUNNING;
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

    if (tasks_.size() >= thread_pool_config_.max_waiting_task) {
        LOG_INFO("The number of tasks in the cache has reached the maximum.[size: %d, max: %d]", tasks_.size(), thread_pool_config_.max_waiting_task);
        return 0;
    }
    task_mutex_.lock();
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

    if (priority_tasks_.size() >= thread_pool_config_.max_waiting_task) {
        LOG_INFO("The number of priority_tasks in the cache has reached the maximum.[size: %d, max: %d]", priority_tasks_.size(), thread_pool_config_.max_waiting_task);
        return 0;
    }

    task_mutex_.lock();
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

int 
ThreadPool::remove_thread(int64_t thread_id)
{
    thread_mutex_.lock();
    auto remove_iter = idle_threads_.find(thread_id);
    if (remove_iter != idle_threads_.end()) {
        if (remove_iter->second != nullptr) {
            delete remove_iter->second;
        }
        idle_threads_.erase(remove_iter);
    }

    remove_iter = runing_threads_.find(thread_id);
    if (remove_iter != runing_threads_.end()) {
        if (remove_iter->second != nullptr) {
            delete remove_iter->second;
        }
        runing_threads_.erase(remove_iter);
    }
    thread_mutex_.unlock();

    return 0;
}

int 
ThreadPool::manage_work_threads(bool is_init)
{
    if (is_init) {
        // 初始化最小运行线程数
        for (std::size_t i = 0; i < thread_pool_config_.min_thread_num; ++i) {
            WorkThread *work_thread = new WorkThread(this);
            work_thread->init();
            idle_threads_[(int64_t)work_thread] = work_thread;
        }
        // 运行线程池
        this->init();
        return 0;
    }

    if (tasks_.size() > 0 ) {
        int awake_threads = tasks_.size() / 2 + 1;
        for (int i = 0; i < awake_threads; ++i) {
            if (idle_threads_.size() > 0) {
                idle_threads_.begin()->second->resume();
            } else {
                if (runing_threads_.size() < thread_pool_config_.max_thread_num) {
                    WorkThread *work_thread = new WorkThread(this);
                    work_thread->init();
                    idle_threads_[(int64_t)work_thread] = work_thread;
                    --i;
                    continue;
                } else {
                    // 可分配的线程数已经达到最大
                    LOG_WARN("run out of threads: running threads: %d, max threads: %d", runing_threads_.size(), thread_pool_config_.max_thread_num);
                    break;
                }
            }
        }
    } else {
        // 没有任务休眠 5 ms
        Time::sleep(5);
    }

    bool is_close = true;
    std::size_t workthread_cnt = idle_threads_.size() + runing_threads_.size();
    for (auto iter = idle_threads_.begin(); iter != idle_threads_.end();) {
        if (workthread_cnt <= thread_pool_config_.min_thread_num) { // 线程不能小于设置的最小线程数
            is_close = false;
        }

        auto stop_iter = iter++; // stop_handler 可能会改变idle_threads从而影响到当前的iter,先提前自增
        bool timeout = stop_iter->second->idle_timeout();
        if (timeout) {
            if (is_close) {
                stop_iter->second->stop_handler(); // 关闭多余的超时空闲线程
                workthread_cnt--;
            } else {
                stop_iter->second->reset_idle_life();
            }
        }
        is_close = true;
    }

    return 0;
}

int 
ThreadPool::thread_move_to_idle_map(int64_t thread_id)
{
    thread_mutex_.lock();
    auto iter = runing_threads_.find(thread_id);
    if (iter == runing_threads_.end()) {
        LOG_WARN("Can't find thread(thread_id: %ld) at runing_threads", thread_id);
        thread_mutex_.unlock();
        return -1;
    }

    if (idle_threads_.find(thread_id) != idle_threads_.end()) {
        LOG_WARN("There is exists a thread(thread_id: %ld) at idle_threads", thread_id);
        thread_mutex_.unlock();
        return -1;
    }

    idle_threads_[thread_id] = iter->second;
    runing_threads_.erase(iter);

    thread_mutex_.unlock();

    return 0;
}

int 
ThreadPool::thread_move_to_running_map(int64_t thread_id)
{
    thread_mutex_.lock();
    auto iter = idle_threads_.find(thread_id);
    if (iter == idle_threads_.end()) {
        LOG_WARN("Can't find thread(thread_id: %ld) at idle_threads", thread_id);
        thread_mutex_.unlock();
        return -1;
    }

    if (runing_threads_.find(thread_id) != runing_threads_.end()) {
        LOG_WARN("There is exists a thread(thread_id: %ld) at runing_threads", thread_id);
        thread_mutex_.unlock();
        return -1;
    }

    runing_threads_[thread_id] = iter->second;
    idle_threads_.erase(iter);

    thread_mutex_.unlock();

    return 0;
}

int 
ThreadPool::set_threadpool_config(const ThreadPoolConfig &config)
{

    if (config.min_thread_num < 0 || config.min_thread_num > MAX_THREADS_NUM) {
        LOG_WARN("min thread num is out of range --- %d", config.min_thread_num);
    } else {
        thread_pool_config_.min_thread_num =  config.min_thread_num;
    }

    if (config.max_thread_num < config.min_thread_num || config.max_thread_num > MAX_THREADS_NUM) {
        LOG_WARN("max thread num is out of range --- %d", config.max_thread_num);
    } else {
        thread_pool_config_.max_thread_num =  config.max_thread_num;
    }

    if (thread_pool_config_.idle_thread_life <= 0 || thread_pool_config_.idle_thread_life > MAX_THREAD_IDLE_LIFE) {
        LOG_WARN("thread life is out of range --- %d", thread_pool_config_.idle_thread_life);
    } else {
        thread_pool_config_.idle_thread_life = config.idle_thread_life;
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
        // 等待任务完成
        while (runing_threads_.size() > 0) {
            LOG_INFO("Wait for task finish, Working threads num: %d", runing_threads_.size());
            Time::sleep(1000);
        }
    } else {
        // 停止所有正在处理任务的线程
        for (auto iter = runing_threads_.begin(); iter != runing_threads_.end();) {
            auto stop_iter = iter++;
            stop_iter->second->stop_handler();
        }
    }

    while (runing_threads_.size() > 0) {
        LOG_INFO("shutdown threads waiting: %d threads still running", runing_threads_.size());
        Time::sleep(500);
    }

    for (auto iter = idle_threads_.begin(); iter != idle_threads_.end(); ) {
        auto stop_iter = iter++;
        stop_iter->second->stop_handler();
    }

    return 0;
}


ThreadPoolRunningInfo 
ThreadPool::get_running_info(void)
{
    ThreadPoolRunningInfo running_info;
    running_info.config = thread_pool_config_;
    running_info.curr_status = (state_ == false ? "running":"stopping"); // true 表示退出

    if (thread_pool_config_.threadpool_exit_action == WAIT_FOR_ALL_TASKS_FINISHED) {
        running_info.exit_action =  "WAIT_FOR_ALL_TASKS_FINISHED";
    } else if (thread_pool_config_.threadpool_exit_action == SHUTDOWN_ALL_THREAD_IMMEDIATELY) {
        running_info.exit_action =  "SHUTDOWN_ALL_THREAD_IMMEDIATELY";
    } else {
        running_info.exit_action =  "UNKNOWN_ACTION";
    }
    running_info.running_threads_num = runing_threads_.size();
    running_info.idle_threads_num = idle_threads_.size();
    running_info.waiting_tasks = tasks_.size();
    running_info.waiting_prio_tasks = priority_tasks_.size();

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
        ostr << "max-threads: " << thread_pool_ptr->thread_pool_config_.max_thread_num << std::endl;
        ostr << "min-threads: " << thread_pool_ptr->thread_pool_config_.min_thread_num << std::endl;
        ostr << "max-waiting-tasks: " << thread_pool_ptr->thread_pool_config_.max_waiting_task << std::endl;
        ostr << "idle-thread-life: " << thread_pool_ptr->thread_pool_config_.idle_thread_life << std::endl;
        ostr << "action when threadpool exit: " << info.exit_action << std::endl;
        ostr << "==================thread pool realtime data===================" << std::endl;
        ostr << "running threads: " << thread_pool_ptr->runing_threads_.size() << std::endl;
        ostr << "idle threads: " << thread_pool_ptr->idle_threads_.size() << std::endl;
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
