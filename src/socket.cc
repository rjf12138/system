#include "system.h"

namespace os {
SocketTCP::SocketTCP(void)
: is_enable_(false)
{

}

SocketTCP::SocketTCP(std::string ip, uint16_t port)
: is_enable_(false)
{
    this->create_socket(ip, port);
}

SocketTCP::~SocketTCP()
{
    this->close();
}

int 
SocketTCP::close(void)
{
    if (is_enable_) {
        is_enable_ = false;
        int ret = ::close(socket_);
        if (ret < 0) {
            LOG_ERROR("close failed: %s", strerror(errno));
            return -1;
        }
    }

    return 0;
}

int 
SocketTCP::shutdown(int how)
{
    if (is_enable_) {
        int ret = ::shutdown(socket_, how);
        if (ret < 0) {
            is_enable_ = false;
            LOG_ERROR("shutdown failed: %s", strerror(errno));
            return -1;
        }
    }

    return 0;
}

int 
SocketTCP::setnonblocking(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }
    
    int old_option = fcntl(socket_, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    int ret = ::fcntl(socket_, F_SETFL, new_option);
    if (ret < 0) {
        LOG_ERROR("fcntl: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int 
SocketTCP::set_reuse_addr(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    int enable = 1;
    int ret = ::setsockopt(socket_,SOL_SOCKET,SO_REUSEADDR,(void*)&enable,sizeof(enable));
    if (ret < 0) {
        LOG_ERROR("setsockopt: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int 
SocketTCP::get_socket(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }
    
    return socket_;
}

int 
SocketTCP::get_ip_info(std::string &ip, uint16_t &port)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    ip = ip_;
    port = port_;

    return 0;
}

int 
SocketTCP::set_socket(int clisock, struct sockaddr_in *cliaddr, socklen_t *addrlen)
{
    if (is_enable_ == true) {
        LOG_WARN("set_socket failed, there's already opened a socket.");
        return -1;
    }

    if (cliaddr == nullptr || addrlen == nullptr) {
        return -1;
    }

    char buf[128] = {0};
    const char *ret = ::inet_ntop(AF_INET, &(cliaddr->sin_addr), buf, *addrlen);
    if (ret == nullptr) {
        LOG_ERROR("inet_ntop: %s", strerror(errno));
        return -1;
    }

    ip_ = ret;
    port_ = ::ntohs(cliaddr->sin_port);
    socket_ = clisock;
    is_enable_ = true;

    return 0;
}

int 
SocketTCP::create_socket(std::string ip, uint16_t port)
{
    if (is_enable_ == true) {
        LOG_WARN("create_socket failed, there's already opened a socket.");
        return -1;
    }

    ip_ = ip;
    port_ = port;
    memset(&addr_, 0, sizeof(struct sockaddr_in));

    int ret = ::inet_pton(AF_INET, ip_.c_str(), &addr_.sin_addr);
    if (ret  == 0) {
        LOG_WARN("Incorrect format of IP address： %s", ip_.c_str());
        return -1;
    } else if (ret < 0) {
        LOG_ERROR("inet_pton: %s", strerror(errno));
        return -1;
    }

    
    addr_.sin_family = AF_INET;
    addr_.sin_port = ::htons(port_);

    // 创建套接字
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        LOG_ERROR("socket: %s", strerror(errno));
        return -1;
    }

    is_enable_ = true;

    return 0;
}

// 这里listen()合并了 bind()和listen
int 
SocketTCP::listen(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    int ret = ::bind(socket_, (sockaddr*)&addr_, sizeof(addr_));
    if (ret < 0) {
        LOG_ERROR("bind: %s", strerror(errno));
        this->close();
        return -1;
    }

    ret = ::listen(socket_, 5);
    if (ret < 0) {
        LOG_ERROR("listen: %s", strerror(errno));
        this->close();
        return -1;
    }

    return 0;
}

int SocketTCP::connect(void)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    int ret = ::connect(socket_, (sockaddr*)&addr_, sizeof(addr_));
    if (ret < 0) {
        LOG_ERROR("connect: %s. socket: %d", strerror(errno), socket_);
        this->close();
        return -1;
    }

    return 0;
}

int SocketTCP::accept(int &clisock, struct sockaddr *cliaddr, socklen_t *addrlen)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    int ret = ::accept(socket_, cliaddr, addrlen);
    if (ret < 0) {
        LOG_ERROR("accept: %s", strerror(errno));
        this->close();
        return -1; 
    }
    clisock = ret;

    return 0;
}
// 存在问题需要修改
int 
SocketTCP::recv(ByteBuffer &buff, int flags)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    const int buffer_size = 2048;
    char buffer[buffer_size] = {0};
    ssize_t ret = 0;
    do {
        ret = ::recv(socket_, buffer, 2048, flags);
        if (ret < 0) {
            if (errno != EINTR) {
                LOG_ERROR("recv: %s", strerror(errno));
                break;
            }
            continue;
        } 
        buff.write_bytes(buffer, ret);
        if (ret == buffer_size) {
            continue;
        }
    } while (false);
   
    return buff.data_size();
}

int 
SocketTCP::send(ByteBuffer &buff, int flags)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    ssize_t send_size = 0;
    do {
        int ret = ::send(socket_, buff.get_read_buffer_ptr(), buff.get_cont_read_size(), flags);
        if (ret < 0) {
            if (errno != EINTR) {
                LOG_ERROR("send: %s", strerror(errno));
                break;
            }
            continue;
        }
        send_size += ret;
        buff.update_read_pos(ret);
    }  while (buff.data_size() > 0);
   
    return send_size;
}

}