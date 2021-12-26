#include "system.h"

using namespace os;

static int value = 0;
static bool start = false;
Mutex mutex;

void* test_thread_fun(void *arg);

int main(int argc, char** argv)
{
    ThreadPool pool;
    ThreadPoolConfig config = pool.get_threadpool_config();
    config.threads_num = 2;
    pool.set_threadpool_config(config);
    
    // pool.show_threadpool_info();
    for (int i = 0; i < config.threads_num; ++i) {
        int *p = &value;
        Task task;
        task.thread_arg = p;
        task.work_func = test_thread_fun;

        pool.add_task(task); 
    }

    // 等待所有线程运行起来
    while (true) {
        os::ThreadPoolRunningInfo info = pool.get_running_info();
        if (info.idle_threads_num + info.running_threads_num == config.threads_num) {
            break;
        }
        Time::sleep(5);
    }

    start = true;
    while (true) {
        char ch = getchar();
        if (ch == 'q') {
            break;
        }
    }
    std::cout << "Exit.." << std::endl;
    return 0;
}

void* test_thread_fun(void *arg)
{
    if (arg == nullptr) {
        return nullptr;
    }

    int *data = (int*)arg;
    while (start == false) {
        Time::sleep(2);
    }
    
    // 会长时间阻塞
    int value = 0;
    *data = 0;
    bool ret = Atomic<int>::compare_and_swap(data, value, value + 1);
    // bool ret = __sync_bool_compare_and_swap(data, value, value + 1);
    std::cout << "ret: " << (ret?"true":"false") << " data: " << *data << " value: " << value <<std::endl;
    return nullptr;
}