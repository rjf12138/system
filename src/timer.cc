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
            if (timer_heap_.pop(event) > 0) {
                if (event.TimeEvent_callback != nullptr) {
                    event.TimeEvent_callback(event.TimeEvent_arg);
                }
            }
            mutex_.unlock();
        }
        Time::sleep(loop_gap_);
    }
    return 0;
}

int 
Timer::stop_handler(void)
{
    exit_ = true;
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
    for (int i = 0; i < timer_id_)
}

}