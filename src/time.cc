#include "system.h"

namespace os {
Time::Time(void)
{
    now();
}

Time::~Time(void)
{

}

uint64_t 
Time::now(void)
{
    // 获取当前时间：毫秒级
    struct timeval struct_time_mill;
    gettimeofday(&struct_time_mill,NULL);
    
    time_ = struct_time_mill.tv_sec * 1000 + struct_time_mill.tv_usec / 1000;
    return time_;
}

std::string 
Time::format(bool mills_enable, const char *fmt)
{
    return convert_to(time_, mills_enable, fmt);
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
Time::convert_to(const std::string &ti)
{
    std::size_t index = ti.find_last_of(":");
    if (index == std::string::npos) {
        return 0;
    }

    struct tm struct_time;
    memset(&struct_time, 0, sizeof(struct_time));
    char *ptr = strptime(ti.c_str(), "%Y-%m-%d %H:%M:%S", &struct_time);
    // printf("%d-%d-%d %d:%d:%d\n", 
    //         struct_time.tm_year, 
    //         struct_time.tm_mon, 
    //         struct_time.tm_mday, 
    //         struct_time.tm_hour,
    //         struct_time.tm_min,
    //         struct_time.tm_sec);
    if (ptr != NULL || *ptr != '\0') {
        uint64_t milli_time = atol(++ptr);
        time_t sec_time = mktime(&struct_time);
        if (sec_time == -1) {
            perror("mktime");
            return 0;
        }
        return sec_time * 1000 + milli_time;
    }

    return 0;
}

std::string 
Time::convert_to(const mtime_t &ti, bool mills_enable, const char *fmt)
{
    // 获取毫秒数以及time_t精确到秒
    uint32_t milli_time = ti % 1000;
    time_t curr_time_sec = ti / 1000;

    // 获取时间的结构
    struct tm* struc_time = localtime(&curr_time_sec);
    char buf[256] = {0};
    if (strftime(buf, 256, fmt, struc_time) == 0) {
        perror("strftime");
        return "";
    }

    if (mills_enable == true) {
        std::ostringstream ostr;
        ostr << buf << ":" << milli_time;
        return ostr.str();
    }

    return buf;
}

}