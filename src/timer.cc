#include "system.h"

namespace util {

Timer::Timer(void)
:exit_(false),
loop_gap_(5),
timer_id_(0)
{
    this->init();
}

Timer::~Timer(void)
{

}

int 
Timer::run_handler(void)
{
    while(exit_ == false) {
        if (timer_heap_.size() > 0 && timer_heap_[0].expire_time > time_.now()) {
            TimerEvent_t event;
            mutex_.lock();
            int ret = timer_heap_.pop(event);
            mutex_.unlock();
            if (ret > 0) {
                if (event.TimeEvent_callback != nullptr) {
                    event.TimeEvent_callback(event.TimeEvent_arg);
                    if (event.attr == TimerEventAttr_ReAdd) {
                        this->add(event);
                    }
                }
            }
        }
        Time::sleep(loop_gap_);
    }
    return 0;
}

int 
Timer::stop_handler(void)
{
    exit_ = true;
    return 0;
}

// 添加定时器,错误返回-1， 成功返回一个定时器id
int 
Timer::add(TimerEvent_t &event)
{
    if (event.wait_time <= loop_gap_) {
        return -1;
    }

    event.id = timer_id_++;
    event.expire_time = time_.now() + event.wait_time;
    mutex_.lock();
    timer_heap_.push(event);
    mutex_.unlock();

    return 0;
}

int 
Timer::cancel(int id)
{
    int ret = 0;
    for (int i = 0; i < timer_id_; ++i) {
        if (timer_heap_[i].id == id) {
            mutex_.lock();
            ret = timer_heap_.remove(i);
            mutex_.unlock();
        }
    }
    return ret;
}

}