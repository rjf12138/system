#include "system.h"

namespace os {
Time::Time(void)
{
    now();
}

Time::~Time(void)
{

}

void 
Time::set_time(mtime_t tm)
{
    time_ = tm;
}

mtime_t 
Time::now(void)
{
    // 获取当前时间：毫秒级
    struct timeval struct_time_mill;
    gettimeofday(&struct_time_mill,NULL);
    
    mtime_t curr_time = struct_time_mill.tv_sec * 1000 + struct_time_mill.tv_usec / 1000;
    return curr_time;
}

stime_t 
Time::snow(void)
{
    return convert_to(time(NULL));
}

std::string 
Time::format(bool mills_enable, const char *fmt)
{
    return convert_to(Time::now(), mills_enable, fmt);
}

void Time::add(const stime_t &t)
{
    time_ += (t.days * 24 * 60 * 60 + t.hours * 60 * 60 + t.mins * 60 + t.secs) * 1000;
}

void Time::sub(const stime_t &t)
{
    time_ -= (t.days * 24 * 60 * 60 + t.hours * 60 * 60 + t.mins * 60 + t.secs) * 1000;
}

int
Time::sleep(uint32_t mills)
{
    struct timespec req, rem;
    req.tv_sec = mills / 1000;
    req.tv_nsec = (mills % 1000) * 1000 * 1000;
    while (::nanosleep(&req, &rem) < 0){
        if (errno == EINTR) {
            req = rem;
            continue;
        } else {
            return -1;
        }
    }
    return 0;
}

mtime_t 
Time::convert_to(const std::string &ti, bool mills_enable, const char *fmt)
{
    if (mills_enable == true) {
        std::size_t index = ti.find_last_of(":");
        if (index == std::string::npos) {
            return -1;
        }
    }

    struct tm struct_time;
    memset(&struct_time, 0, sizeof(struct_time));
    char *ptr = strptime(ti.c_str(), fmt, &struct_time);
    // printf("%d-%d-%d %d:%d:%d\n", 
    //         struct_time.tm_year, 
    //         struct_time.tm_mon, 
    //         struct_time.tm_mday, 
    //         struct_time.tm_hour,
    //         struct_time.tm_min,
    //         struct_time.tm_sec);
    if (ptr != NULL || *ptr != '\0') {
        time_t sec_time = mktime(&struct_time);
        if (sec_time == -1) {
            LOG_GLOBAL_ERROR("mktime: %s", strerror(errno));
            return -1;
        }
        if (mills_enable == true) {
            mtime_t milli_time = atol(++ptr);
            return sec_time * 1000 + milli_time;
        } else {
            return sec_time;
        }
    }

    return 0;
}

std::string 
Time::convert_to(const mtime_t &ti, bool mills_enable, const char *fmt)
{
    // 获取毫秒数以及time_t精确到秒
    mtime_t milli_time = ti % 1000;
    time_t curr_time_sec = ti / 1000;

    // 获取时间的结构
    struct tm* struc_time = localtime(&curr_time_sec);
    char buf[256] = {0};
    if (strftime(buf, 256, fmt, struc_time) == 0) {
        LOG_GLOBAL_ERROR("mktime: %s", strerror(errno));
        return "";
    }

    if (mills_enable == true) {
        std::ostringstream ostr;
        ostr << buf << ":" << milli_time;
        return ostr.str();
    }

    return buf;
}

stime_t 
Time::convert_to(const time_t &ti)
{
    stime_t sti;

    // 获取时间的结构
    struct tm* struc_time = localtime(&ti);
    sti.year = struc_time->tm_year + 1900;
    sti.month = struc_time->tm_mon + 1;
    sti.days = struc_time->tm_mday;
    sti.hours = struc_time->tm_hour;
    sti.mins = struc_time->tm_min;
    sti.secs = struc_time->tm_sec;
    sti.wday = struc_time->tm_wday == 0 ? 7 : struc_time->tm_wday;

    return sti;
}

}