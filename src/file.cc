#include "system.h"

#include <algorithm>
#include <cstdarg>
#include <vector>

namespace os {

File::File(void) 
: fd_(-1),
open_on_exit_(false),
file_open_flag_(false),
max_lines_(0)
{
    memset(&file_info_, 0, sizeof(file_info_));
    this->set_stream_func(LOG_LEVEL_TRACE, g_msg_to_stream_trace);
    this->set_stream_func(LOG_LEVEL_DEBUG, g_msg_to_stream_debug);
    this->set_stream_func(LOG_LEVEL_INFO, g_msg_to_stream_info);
    this->set_stream_func(LOG_LEVEL_WARN, g_msg_to_stream_warn);
    this->set_stream_func(LOG_LEVEL_ERROR, g_msg_to_stream_error);
    this->set_stream_func(LOG_LEVEL_FATAL, g_msg_to_stream_fatal);
}

File::~File(void)
{
    this->close_file();
}

int 
File::open(const std::string file_path, int flag, int file_right)
{
    if (file_path.empty()) {
        LOG_WARN("file_path is empty");
        return -1;
    }

    int open_flag = flag;
    // 检查文件是否存在（保留原有语义：不存在时创建）
    if (::access(file_path.c_str(), F_OK) == -1) {
        open_flag |= O_CREAT;
    }

    int fd = ::open(file_path.c_str(), open_flag, file_right);
    if (fd == -1) {
        LOG_ERROR("open: %s", strerror(errno));
        return -1;
    }

    // 直接对 fd 取信息，避免 access/open/stat 之间的路径竞态
    struct stat info;
    if (::fstat(fd, &info) == -1) {
        LOG_ERROR("fstat: %s", strerror(errno));
        ::close(fd);
        return -1;
    }

    // 如果之前有打开文件，先关闭之前的文件
    this->close_file();
    fd_ = fd;
    file_info_ = info;
    file_open_flag_ = true;
    open_on_exit_ = true;

    return 0;
}

int 
File::set_fd(int fd, bool open_on_exit)
{
    if (fd < 0 || this->check_fd(fd) == -1) {
        return -1;
    }
    this->close_file();

    fd_ = fd;
    if (::fstat(fd_, &file_info_) == -1) {
        file_open_flag_ = false;
        fd_ = -1;
        LOG_ERROR("fstat: %s", strerror(errno));
        return -1;
    }
    file_open_flag_ = true;
    open_on_exit_ = open_on_exit;

    return 0;
}

int 
File::fileinfo(struct stat &file_info)
{
    if (file_open_flag_ == true) {
        file_info = file_info_;
    } else {
        LOG_ERROR("haven't open any file");
        return -1;
    }

    return 0;
}

int 
File::close_file(void)
{
    if (file_open_flag_ == false) {
        return 0;
    }

    int fd = fd_;
    bool should_close = open_on_exit_;
    file_open_flag_ = false;
    fd_ = -1;
    open_on_exit_ = false;

    if (should_close && ::close(fd) < 0) {
        LOG_ERROR("close(fd_): %s", strerror(errno));
        return -1;
    }

    return 0;
}

int 
File::check_fd(int fd)
{
    struct stat info;
    int ret = fstat(fd, &info);
    if (ret == -1) {
        LOG_ERROR("fstat: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int
File::clear_file(void)
{
    if (!file_open_flag_) {
        LOG_ERROR("clear_file: haven't open any file!");
        return -1;
    }

    if (::ftruncate64(fd_, 0) < 0) {
        LOG_ERROR("ftruncate64: %s", strerror(errno));
        return -1;
    }
    // 将文件偏移量设为0，防止出现文件空洞
    return this->seek(0, SEEK_SET) < 0 ? -1 : 0;
}

off_t 
File::seek(off_t offset, int whence)
{
    if (!file_open_flag_) {
        LOG_ERROR("seek: haven't open any file!");
        return -1;
    }

    off_t pos = ::lseek(fd_, offset, whence);
    if (pos == -1) {
        LOG_ERROR("lseek: %s", strerror(errno));
        return -1;
    }

    return pos;
}

off_t 
File::curr_pos(void)
{
    return this->seek(0, SEEK_CUR);
}

ssize_t 
File::read(ByteBuffer &buff, size_t buf_size)
{
    if (!file_open_flag_) {
        LOG_WARN("File::read: haven't open any file!");
        return -1;
    }

    if (buf_size == 0) {
        return 0;
    }

    buff.resize(buf_size);
    size_t remain_size = buf_size;
    ssize_t read_size = 0;
    while (remain_size > 0) {
        size_t chunk = static_cast<size_t>(buff.get_cont_write_size());
        if (chunk > remain_size) {
            chunk = remain_size;
        }
        if (chunk == 0) {
            break;
        }

        ssize_t ret = ::read(fd_, buff.get_write_buffer_ptr(), chunk);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("read: %s", strerror(errno));
            return read_size > 0 ? read_size : -1;
        }
        if (ret == 0) {
            break;
        }

        buff.update_write_pos(ret);
        read_size += ret;
        remain_size -= static_cast<size_t>(ret);
    }

    return read_size;
}

ssize_t 
File::read_from_pos(ByteBuffer &buff, size_t buf_size, off_t pos, int whence)
{
    if (!file_open_flag_) {
        LOG_ERROR("read_from_pos: haven't open any file!");
        return -1;
    }

    // seek 成功时可能返回 0（文件开头），只有负值才是失败
    if (this->seek(pos, whence) < 0) {
        return -1;
    }

    ssize_t ret_size = this->read(buff, buf_size);
    return ret_size;
}


ssize_t 
File::write(ByteBuffer &buff, size_t buf_size)
{
    if (!file_open_flag_) {
        LOG_ERROR("write: haven't open any file!");
        return -1;
    }

    if (buff.data_size() <= 0 || buf_size == 0) {
        return 0;
    }

    // buf_size 是本次最多写入的字节数，不能超过缓冲区剩余数据量
    size_t remain_size = std::min(buf_size, static_cast<size_t>(buff.data_size()));
    ssize_t write_size = 0;
    while (remain_size > 0) {
        size_t chunk = static_cast<size_t>(buff.get_cont_read_size());
        if (chunk > remain_size) {
            chunk = remain_size;
        }
        if (chunk == 0) {
            break;
        }

        ssize_t ret = ::write(fd_, buff.get_read_buffer_ptr(), chunk);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("write: %s", strerror(errno));
            return write_size > 0 ? write_size : -1;
        }
        if (ret == 0) {
            break;
        }

        buff.update_read_pos(ret);
        write_size += ret;
        remain_size -= static_cast<size_t>(ret);
    }

    return write_size;
}

ssize_t 
File::write_to_pos(ByteBuffer &buff, size_t buf_size ,off_t pos, int whence)
{
    if (!file_open_flag_) {
        LOG_ERROR("write_to_pos: haven't open any file!");
        return -1;
    }

    if (this->seek(pos, whence) == -1) {
        LOG_ERROR("seek: %s!\n", strerror(errno));
        return -1;
    }

    ssize_t ret_size = this->write(buff, buf_size);
    return ret_size;
}


ssize_t 
File::read_buf_fmt(ByteBuffer &buff, const char *fmt, ...)
{
    if (fmt == nullptr) {
        return -1;
    }

    const size_t buf_size = 4096;
    std::vector<char> tmp_buf(buf_size, 0);

    // 最多读取 buf_size - 1 字节，保证 vsscanf 看到的字符串以 NUL 结尾
    size_t read_len = std::min(static_cast<size_t>(buff.data_size()), buf_size - 1);
    buff.read_bytes(tmp_buf.data(), read_len);

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    int read_size = vsscanf(tmp_buf.data(), fmt, arg_ptr);
    va_end(arg_ptr);

    return read_size;
}

ssize_t 
File::write_buf_fmt(ByteBuffer &buff, const char *fmt, ...)
{
    if (fmt == nullptr) {
        return -1;
    }

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    va_list arg_copy;
    va_copy(arg_copy, arg_ptr);
    int needed = vsnprintf(nullptr, 0, fmt, arg_ptr);
    va_end(arg_ptr);

    if (needed < 0) {
        va_end(arg_copy);
        return -1;
    }

    std::vector<char> tmp_buf(static_cast<size_t>(needed) + 1, 0);
    int write_size = vsnprintf(tmp_buf.data(), tmp_buf.size(), fmt, arg_copy);
    va_end(arg_copy);

    if (write_size < 0) {
        return -1;
    }

    buff.write_bytes(tmp_buf.data(), static_cast<size_t>(write_size));
    return write_size;
}

ssize_t 
File::write_file_fmt(const char *fmt, ...)
{
    if (!file_open_flag_) {
        LOG_ERROR("write_file_fmt: haven't open any file!");
        return -1;
    }
    if (fmt == nullptr) {
        return -1;
    }

    va_list arg_ptr;
    va_start(arg_ptr, fmt);
    va_list arg_copy;
    va_copy(arg_copy, arg_ptr);
    int needed = vsnprintf(nullptr, 0, fmt, arg_ptr);
    va_end(arg_ptr);

    if (needed < 0) {
        va_end(arg_copy);
        return -1;
    }

    std::vector<char> tmp_buf(static_cast<size_t>(needed) + 1, 0);
    int format_size = vsnprintf(tmp_buf.data(), tmp_buf.size(), fmt, arg_copy);
    va_end(arg_copy);

    if (format_size < 0) {
        return -1;
    }

    size_t remain_size = static_cast<size_t>(format_size);
    size_t write_size = 0;
    while (remain_size > 0) {
        ssize_t ret = ::write(fd_, tmp_buf.data() + write_size, remain_size);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            LOG_ERROR("write: %s", strerror(errno));
            return write_size > 0 ? static_cast<ssize_t>(write_size) : -1;
        }
        if (ret == 0) {
            break;
        }
        write_size += static_cast<size_t>(ret);
        remain_size -= static_cast<size_t>(ret);
    }

    return static_cast<ssize_t>(write_size);
}

ssize_t 
File::copy(File &file)
{
    if (!file.file_open_flag_ || !this->file_open_flag_) {
        LOG_ERROR("copy: haven't open any file!");
        return -1;
    }
    if (fd_ == file.fd_) {
        LOG_ERROR("copy: source and destination are the same file descriptor!");
        return -1;
    }

    if (this->clear_file() != 0) {
        return -1;
    }

    basic::ByteBuffer buffer;
    ssize_t total_size = 0;
    while (true) {
        buffer.clear();
        ssize_t read_size = file.read(buffer, 8192);
        if (read_size < 0) {
            return -1;
        }
        if (read_size == 0) {
            break;
        }

        ssize_t write_size = this->write(buffer, static_cast<size_t>(read_size));
        if (write_size != read_size) {
            LOG_ERROR("copy: write failed, read: %zd, write: %zd", read_size, write_size);
            return -1;
        }
        total_size += write_size;
    }

    if (fchmod(fd_, file.file_info_.st_mode) != 0) {
        LOG_ERROR("copy: fchmod: %s", strerror(errno));
        return -1;
    }

    return total_size;
}

}