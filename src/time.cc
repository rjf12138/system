#include "system.h"

namespace os {
Time::Time(void)
: time_(now())
{
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
    mtime_t delta = (static_cast<mtime_t>(t.days) * 24 * 60 * 60
                   + static_cast<mtime_t>(t.hours) * 60 * 60
                   + static_cast<mtime_t>(t.mins) * 60
                   + static_cast<mtime_t>(t.secs)) * 1000;
    time_ += delta;
}

void Time::sub(const stime_t &t)
{
    mtime_t delta = (static_cast<mtime_t>(t.days) * 24 * 60 * 60
                   + static_cast<mtime_t>(t.hours) * 60 * 60
                   + static_cast<mtime_t>(t.mins) * 60
                   + static_cast<mtime_t>(t.secs)) * 1000;
    time_ -= delta;
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
    if (fmt == nullptr || ti.empty()) {
        return 0;
    }

    struct tm struct_time;
    memset(&struct_time, 0, sizeof(struct_time));
    char *ptr = strptime(ti.c_str(), fmt, &struct_time);
    if (ptr == nullptr) {
        return 0;
    }

    time_t sec_time = mktime(&struct_time);
    if (sec_time == -1) {
        LOG_GLOBAL_ERROR("mktime: %s", strerror(errno));
        return 0;
    }

    if (mills_enable == false) {
        return static_cast<mtime_t>(sec_time);
    }

    // 默认格式不包含毫秒；如果输入后面带了 ":<毫秒>"，再解析毫秒
    if (*ptr != ':') {
        return static_cast<mtime_t>(sec_time) * 1000;
    }

    char *end = nullptr;
    long milli_time = strtol(ptr + 1, &end, 10);
    if (end == ptr + 1 || *end != '\0' || milli_time < 0 || milli_time > 999) {
        return 0;
    }

    return static_cast<mtime_t>(sec_time) * 1000 + static_cast<mtime_t>(milli_time);
}

std::string 
Time::convert_to(const mtime_t &ti, bool mills_enable, const char *fmt)
{
    if (fmt == nullptr) {
        fmt = DEFAULT_TIME_FMT;
    }

    // 获取毫秒数以及time_t精确到秒
    mtime_t milli_time = ti % 1000;
    time_t curr_time_sec = static_cast<time_t>(ti / 1000);

    // localtime 使用静态缓冲区，多线程下不安全；使用 localtime_r
    struct tm struc_time;
    memset(&struc_time, 0, sizeof(struc_time));
    if (localtime_r(&curr_time_sec, &struc_time) == nullptr) {
        LOG_GLOBAL_ERROR("localtime_r failed");
        return "";
    }

    char buf[256] = {0};
    if (strftime(buf, sizeof(buf), fmt, &struc_time) == 0) {
        LOG_GLOBAL_ERROR("strftime failed");
        return "";
    }

    if (mills_enable == true) {
        char milli_buf[8] = {0};
        snprintf(milli_buf, sizeof(milli_buf), ":%03u", static_cast<unsigned int>(milli_time));
        return std::string(buf) + milli_buf;
    }

    return buf;
}

stime_t 
Time::convert_to(const time_t &ti)
{
    stime_t sti;

    // localtime 使用静态缓冲区，多线程下不安全；使用 localtime_r
    time_t time_value = ti;
    struct tm struc_time;
    memset(&struc_time, 0, sizeof(struc_time));
    if (localtime_r(&time_value, &struc_time) == nullptr) {
        return sti;
    }

    sti.year = struc_time.tm_year + 1900;
    sti.month = struc_time.tm_mon + 1;
    sti.days = struc_time.tm_mday;
    sti.hours = struc_time.tm_hour;
    sti.mins = struc_time.tm_min;
    sti.secs = struc_time.tm_sec;
    sti.wday = struc_time.tm_wday == 0 ? 7 : struc_time.tm_wday;

    return sti;
}

}