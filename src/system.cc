#include "system.h"

#include <vector>

#ifdef __RJF_LINUX__
#include <sys/time.h>
#endif

using namespace basic;

// 放不是实现单一功能的函数
namespace os {

// 设置system_utils内所有函数/类成员的消息回调
msg_to_stream_callback g_msg_to_stream_trace = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_debug = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_info = output_to_stdout;
msg_to_stream_callback g_msg_to_stream_warn = output_to_stderr;
msg_to_stream_callback g_msg_to_stream_error = output_to_stderr;
msg_to_stream_callback g_msg_to_stream_fatal = output_to_stderr;

void set_systemcall_message_output_callback(InfoLevel level, msg_to_stream_callback func)
{
     if (func == nullptr) {
         LOG_GLOBAL_ERROR("set_systemcall_message_output_callback: func is nullptr");
         return;
     }
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

int exe_shell_cmd(std::string &ret, const char *format, ...)
{
	if (format == nullptr) {
		return -1;
	}

	va_list aptr;
	va_start(aptr, format);
	va_list aptr_copy;
	va_copy(aptr_copy, aptr);
	int needed = vsnprintf(nullptr, 0, format, aptr);
	va_end(aptr);
	if (needed < 0) {
		va_end(aptr_copy);
		return -1;
	}

	std::vector<char> cmd(static_cast<size_t>(needed) + 1, 0);
	vsnprintf(cmd.data(), cmd.size(), format, aptr_copy);
	va_end(aptr_copy);

	ret = "";
	FILE* fpipe = popen(cmd.data(), "r");
	if (!fpipe) {
	  LOG_GLOBAL_ERROR("CMD:%s\rpopen:[%s]\r\n", cmd.data(), strerror(errno));
	  return -1;
	}

	//将数据流读取到buf中
	const int N_MAX_READ_BUF = 2048;
	char result[N_MAX_READ_BUF] = {0};
	ssize_t read_len = 0;
	do {
		read_len = fread(result, 1, N_MAX_READ_BUF, fpipe);
		if (read_len > 0) {
		  ret.append(result, read_len);
		} else {
		  break;
		}
	} while (read_len == N_MAX_READ_BUF);

	int close_ret = pclose(fpipe);
	if (close_ret == -1) {
		LOG_GLOBAL_ERROR("pclose: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int exe_shell_cmd_to_stdin(const char *format, ...)
{
	if (format == nullptr) {
		return -1;
	}

	va_list aptr;
	va_start(aptr, format);
	va_list aptr_copy;
	va_copy(aptr_copy, aptr);
	int needed = vsnprintf(nullptr, 0, format, aptr);
	va_end(aptr);
	if (needed < 0) {
		va_end(aptr_copy);
		return -1;
	}

	std::vector<char> cmd(static_cast<size_t>(needed) + 1, 0);
	vsnprintf(cmd.data(), cmd.size(), format, aptr_copy);
	va_end(aptr_copy);

	FILE* fpipe = popen(cmd.data(), "r");
	if (!fpipe) {
	  LOG_GLOBAL_ERROR("CMD:%s\rpopen:[%s]\r\n", cmd.data(), strerror(errno));
	  return -1;
	}

	//将数据流读取到buf中
	const int N_MAX_READ_BUF = 2048;
	char result[N_MAX_READ_BUF] = {0};
	size_t read_len = 0;
	do {
		read_len = fread(result, 1, N_MAX_READ_BUF, fpipe);
		if (read_len > 0) {
		  fwrite(result, 1, read_len, stdout);
		} else {
		  break;
		}
	} while (read_len == N_MAX_READ_BUF);

	int close_ret = pclose(fpipe);
	if (close_ret == -1) {
		LOG_GLOBAL_ERROR("pclose: %s", strerror(errno));
		return -1;
	}

	return 0;
}

}