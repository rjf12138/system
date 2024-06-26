#include "system.h"

namespace os {

Mutex::Mutex(bool is_show_lock_info)
    : is_show_lock_info_(is_show_lock_info),
    is_locked_(false)
{
    this->set_stream_func(LOG_LEVEL_TRACE, g_msg_to_stream_trace);
    this->set_stream_func(LOG_LEVEL_DEBUG, g_msg_to_stream_debug);
    this->set_stream_func(LOG_LEVEL_INFO, g_msg_to_stream_info);
    this->set_stream_func(LOG_LEVEL_WARN, g_msg_to_stream_warn);
    this->set_stream_func(LOG_LEVEL_ERROR, g_msg_to_stream_error);
    this->set_stream_func(LOG_LEVEL_FATAL, g_msg_to_stream_fatal);
    
    mutex_ptr_ = new pthread_mutex_t;
#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_init(mutex_ptr_, NULL);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_init(): %s", strerror(ret));
    }
#endif
}

Mutex::~Mutex(void)
{
#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_destroy(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_destroy(): %s", strerror(ret));
    }
#endif
    delete mutex_ptr_;
}

int 
Mutex::lock(const std::string& func_pos, int pos)
{
    if (is_show_lock_info_) {
        LOG_INFO("file pos: %s:%d, lock status: %s, lock_id: %d", func_pos.c_str() + func_pos.find_last_of('/') + 1, pos, is_locked_ ? "waiting unlock" : "locking", lock_id_);
    }

#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_lock(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_lock(): %s", strerror(ret));
        return -1;
    }
#endif

    if (is_show_lock_info_) {
        is_locked_ = true;
        lock_id_ = os::Time::now();
    }

    return 0;
}

int 
Mutex::trylock(const std::string& func_pos, int pos)
{
    if (is_show_lock_info_) {
        LOG_INFO("file pos: %s:%d, lock status: %s, lock_id: %d", func_pos.c_str() + func_pos.find_last_of('/') + 1, pos, is_locked_ ? "waiting unlock" : "locking", lock_id_);
    }

#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_trylock(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_trylock(): %s", strerror(ret));
        return -1;
    }
#endif
    if (is_show_lock_info_) {
        is_locked_ = true;
        lock_id_ = os::Time::now();
    }

    return 0;
}

int 
Mutex::unlock(const std::string& func_pos, int pos)
{
    if (is_show_lock_info_) {
        LOG_INFO("file pos: %s:%d, lock status: %s, lock_id: %d", func_pos.c_str() + func_pos.find_last_of('/') + 1, pos, is_locked_ ? "unlocking" : "not locked", lock_id_);
    }

#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_unlock(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_unlock(): %s", strerror(ret));
        return -1;
    }
#endif

    if (is_show_lock_info_) {
        is_locked_ = false;
        lock_id_ = 0;
    }

    return 0;
}

// 本质就是CPU提供了一个CMPXCHG的指令可以让我们的比较操作和置换操作在一句指令内完成
// 因为指令是粒度最小的，所以就是原子操作。既然是原子操作，就不用加锁了。但是为了多线程
// 下操作成功，必须使用while循环多次retry。
// 虽然没有像mutex一样sleep-waiting有上下文切换开销，但是CAS会陷入busy-waiting,
// 一直忙等，类似自旋锁，造成CPU升高。需要评估锁的粒度后使用。
/*
    https://www.felixcloutier.com/x86/cmpxchg:
    (* Accumulator = AL, AX, EAX, or RAX depending on whether a byte, word,
    doubleword, or quadward comparison is being performed *)
    TEMP <- DEST
    IF accumulator = TEMP (比较)
    THEN
        ZF <- 1;
        DEST <- SRC;
    ELSE
        ZF <- 0;
        accumulator <- TEMP;
        DEST <- TEMP;
    FI

    Compares the value in the AL, AX, EAX, or RAX register with the first operand (destination operand). 
    If the two values are equal, the second operand (source operand) is loaded into the destination operand. 
    Otherwise, the destination operand is loaded into the AL, AX, EAX or RAX register. RAX register is available only in 64-bit mode.
*/
// bool 
// Mutex::compare_and_swap(void *reg, void *old_value, void *new_value)
// {

// }

}