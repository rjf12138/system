#include "system.h"

namespace os {

Mutex::Mutex(void) 
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
Mutex::lock(void)
{
#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_lock(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_lock(): %s", strerror(ret));
        return -1;
    }
#endif

    return 0;
}

int 
Mutex::trylock(void)
{
#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_trylock(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_trylock(): %s", strerror(ret));
        return -1;
    }
#endif

    return 0;
}

int 
Mutex::unlock(void)
{
#ifdef __RJF_LINUX__
    int ret = ::pthread_mutex_unlock(mutex_ptr_);
    if (ret != 0) {
        LOG_ERROR("pthread_mutex_unlock(): %s", strerror(ret));
        return -1;
    }
#endif

    return 0;
}

}