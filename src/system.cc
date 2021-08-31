#include "system.h"
#ifdef __RJF_LINUX__
#include <sys/time.h>
#endif

// 放不是实现单一功能的函数
namespace util {

// 设置system_utils内所有函数/类成员的消息回调
msg_to_stream_callback g_msg_to_stream_trace = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_debug = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_info = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_warn = output_to_stderr;
msg_to_stream_callback g_msg_to_stream_error = output_to_stderr;
msg_to_stream_callback g_msg_to_stream_fatal = output_to_stderr;

void set_systemcall_message_output_callback(InfoLevel level, msg_to_stream_callback func)
{
     switch (level) {
        case LOG_LEVEL_TRACE:
            g_msg_to_stream_trace = func; break;
        case LOG_LEVEL_DEBUG:
            g_msg_to_stream_debug = func; break;
        case LOG_LEVEL_INFO:
            g_msg_to_stream_info = func; break;
        case LOG_LEVEL_WARN:
            g_msg_to_stream_warn = func; break;
        case LOG_LEVEL_ERROR:
            g_msg_to_stream_error = func; break;
        case LOG_LEVEL_FATAL:
            g_msg_to_stream_fatal = func; break;
        default: 
        {
            output_to_stderr("MsgRecord::set_stream_func: Unknown option!");
        } break;
    }
}


int os_sleep(uint64_t millisecond)
{
    struct timespec req, rem;
    req.tv_sec = millisecond / 1000;
    req.tv_nsec = (millisecond % 1000) * 1000 * 1000;
    while (::nanosleep(&req, &rem) < 0){
        if (errno == EINTR) {
            ::nanosleep(&rem, NULL);
        } else {
            return -1;
        }
    }
    return 0;
}


std::string convert_to_string(uint64_t curr_time_milli)
{
    // 获取毫秒数以及time_t精确到秒
    uint32_t milli_time = curr_time_milli % 1000;
    time_t curr_time_sec =  curr_time_milli / 1000;

    // 获取时间的结构
    struct tm* struc_time = localtime(&curr_time_sec);
    char buf[256] = {0};
    if (strftime(buf, 256, "%Y-%m-%d %H:%M:%S", struc_time) == 0) {
        perror("strftime");
        return "";
    }

    std::ostringstream ostr;
    ostr << buf << ":" << milli_time;

    return ostr.str();
}

uint64_t convert_to_uint64(std::string conv_s_time)
{
    std::size_t index = conv_s_time.find_last_of(":");
    if (index == std::string::npos) {
        return 0;
    }

    struct tm struct_time;
    memset(&struct_time, 0, sizeof(struct_time));
    char *ptr = strptime(conv_s_time.c_str(), "%Y-%m-%d %H:%M:%S", &struct_time);
    ++ptr;
    // printf("%d-%d-%d %d:%d:%d\n", 
    //         struct_time.tm_year, 
    //         struct_time.tm_mon, 
    //         struct_time.tm_mday, 
    //         struct_time.tm_hour,
    //         struct_time.tm_min,
    //         struct_time.tm_sec);
    uint64_t milli_time = atol(ptr);
    time_t sec_time = mktime(&struct_time);
    if (sec_time == -1) {
        perror("mktime");
        return 0;
    }

    return sec_time * 1000 + milli_time;
}

uint64_t get_current_uint64_time(void)
{
    // 获取当前时间：毫秒级
    struct timeval struct_time_mill;
    gettimeofday(&struct_time_mill,NULL);
    
    return struct_time_mill.tv_sec * 1000 + struct_time_mill.tv_usec / 1000;
}

std::string get_current_string_time(void)
{
    return convert_to_string(get_current_uint64_time());
}

std::string add_string_time(std::string time, uint32_t days, uint32_t hours, uint32_t mins, uint32_t secs)
{
    return convert_to_string(add_uint64_time(convert_to_uint64(time), days, hours, mins, secs));
}

uint64_t add_uint64_time(uint64_t time, uint32_t days, uint32_t hours, uint32_t mins, uint32_t secs)
{
    return time + (days * 24 * 60 * 60 + hours * 60 * 60 + mins * 60 + secs) * 1000; // 最终要转换成增加的毫秒数
}

std::string sub_string_time(std::string time, uint32_t days, uint32_t hours, uint32_t mins, uint32_t secs)
{
    return convert_to_string(sub_uint64_time(convert_to_uint64(time), days, hours, mins, secs));
}

uint64_t sub_uint64_time(uint64_t time, uint32_t days, uint32_t hours, uint32_t mins, uint32_t secs)
{
    uint64_t sub_time = (days * 24 * 60 * 60 + hours * 60 * 60 + mins * 60 + secs) * 1000;
    if (sub_time >= time) {
        return 0;
    }

    return time - sub_time;
}

}