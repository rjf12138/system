#include "system.h"

using namespace os;

void* test_thread_fun(void *arg);

int main(int argc, char** argv)
{
    ThreadPool pool;
    pool.show_threadpool_info();
    for (int i = 455; i < 465; ++i) {
        int *p = new int;
        *p = i;
        Task task;
        task.thread_arg = p;
        task.work_func = test_thread_fun;

        pool.add_task(task); 
    }

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
    for (int i = 0; i < 10; ++i) {
        printf("%d----%d\n", *data, i);
        Time::sleep(500);
    }
    delete data;
    return nullptr;
}