#include "system.h"

namespace os {

File::File(void) 
: file_open_flag_(false)
{
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

    int fd = -1;
    // 检查文件是否存在
    int ret = ::access(file_path.c_str(), F_OK);
    if (ret == -1) {
        fd = ::open(file_path.c_str(), flag  | O_CREAT, file_right);
    } else {
        fd = ::open(file_path.c_str(), flag);
    }

    if (fd == -1) {
        LOG_ERROR("open: %s", strerror(errno));
        return -1;
    }
    // 获取文件信息
    struct stat info;
    ret = ::stat(file_path.c_str(), &info);
    if (ret == -1)
    {
        LOG_ERROR("stat: %s", strerror(errno));
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
    if (this->check_fd(fd) == -1) {
        return -1;
    }
    this->close_file();

    fd_ = fd;
    int ret = ::fstat(fd_, &file_info_);
    if (ret == -1) {
        file_open_flag_ = false;
        LOG_ERROR("fstat: %s", strerror(errno));
        return -1;
    }
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
    if (file_open_flag_ == true && open_on_exit_) {
        file_open_flag_ = false;
        if (close(fd_) < 0) {
            LOG_ERROR("close(fd_): %s", strerror(errno));
        }
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
    int ret = ftruncate64(fd_, 0);
    if (ret < 0) {
        return -1;
    }
    // 将文件偏移量设为0，防止出现文件空洞
    this->seek(0, SEEK_SET);

    return 0;
}

off_t 
File::seek(off_t offset, int whence)
{
    off_t pos = lseek(fd_, offset, whence);
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

    if (this->file_size() <= 0) { // 空文件直接返回
        return 0;
    }

    buff.resize(buf_size);
    size_t remain_size = buf_size;
    do {
        ssize_t ret = ::read(fd_, buff.get_write_buffer_ptr(), buff.get_cont_write_size());
        if (ret < 0) {
            LOG_ERROR("read: %s", strerror(errno));
            return -1;
        }
        buff.update_write_pos(ret);

        remain_size -= ret;
        if (ret == 0 || buff.idle_size() == 0) {
            break;
        }
    }  while (remain_size > 0);
   
    return buff.data_size();
}

ssize_t 
File::read_from_pos(ByteBuffer &buff, size_t buf_size, off_t pos, int whence)
{
    if (!file_open_flag_) {
        LOG_ERROR("read_from_pos: haven't open any file!");
        return -1;
    }

    if (this->seek(pos, whence) <= 0) {
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
        return 0;
    }

    if (buff.data_size() <= 0) {
        return 0;
    }
    
    size_t remain_size = buf_size;
    do {
        ssize_t ret = ::write(fd_, buff.get_read_buffer_ptr(), buff.get_cont_read_size());
        if (ret == -1) {
            LOG_ERROR("write: %s", strerror(errno));
            return -1;
        }
        buff.update_read_pos(ret);

        remain_size -= ret;
        if (ret == 0 || buff.idle_size() == 0) {
            break;
        }
    }  while (remain_size > 0);
   
    return buf_size;
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
    const int buf_size = 4096;
    char *tmp_buf = new  char[buf_size];
    memset(tmp_buf, 0, buf_size);

    buff.read_bytes(tmp_buf, buf_size);

    va_list arg_ptr;
    va_start(arg_ptr,fmt);
    int read_size = vsscanf(tmp_buf, fmt, arg_ptr);
    va_end(arg_ptr);

    delete[] tmp_buf;

    return read_size;
}

ssize_t 
File::write_buf_fmt(ByteBuffer &buff, const char *fmt, ...)
{
    const int buf_size = 8192;
    char *tmp_buf = new  char[buf_size];
    memset(tmp_buf, 0, buf_size);

    va_list arg_ptr;
    va_start(arg_ptr,fmt);
    int write_size = vsnprintf(tmp_buf, buf_size, fmt, arg_ptr);
    va_end(arg_ptr);

    buff.write_bytes(tmp_buf, write_size);
    delete[] tmp_buf;

    return write_size;
}

ssize_t 
File::write_file_fmt(const char *fmt, ...)
{
    const int buf_size = 8192;
    char *tmp_buf = new  char[buf_size];
    memset(tmp_buf, 0, buf_size);

    va_list arg_ptr;
    va_start(arg_ptr,fmt);
    int write_size = vsnprintf(tmp_buf, buf_size, fmt, arg_ptr);
    va_end(arg_ptr);

    return ::write(fd_, tmp_buf, write_size);

}

ssize_t 
File::copy(File &file)
{
    if (!file.file_open_flag_ || !this->file_open_flag_) {
        LOG_ERROR("copy: haven't open any file!");
        return -1;
    }

    this->clear_file();
    basic::ByteBuffer buffer;
    do
    {
        ssize_t size = file.read(buffer, 8192);
        if (size <= 0) {
            break;
        }
        this->write(buffer, buffer.data_size());
        buffer.clear();
    } while (true);

    fchmod(fd_, file.file_info_.st_mode);

    return file.file_size();
}

}