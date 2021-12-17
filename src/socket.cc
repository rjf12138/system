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

std::string 
SocketTCP::get_ip_info(void)
{
    return ip_ + ":" + std::to_string(port_);
}

bool 
SocketTCP::get_socket_state(void) const
{
    return is_enable_;
}

bool 
SocketTCP::check_ip_addr(std::string addr)
{
    int a,b,c,d;
    if ((sscanf(addr.c_str(), "%d.%d.%d.%d",&a,&b,&c,&d) == 4)
            && (a >= 0 && a <= 255)
            && (b >= 0 && b <= 255)
            && (c >= 0 && c <= 255)
            && (d >= 0 && d <= 255))
    {
        return true;
    }
    return false;
}

int
SocketTCP::get_addr_by_hostname(std::string hostname, std::string &addr)
{
    struct addrinfo     *ailist, *aip;
    struct addrinfo     hint;
    struct sockaddr_in  *sinp;
    char                abuf[INET_ADDRSTRLEN];

    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = 0;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;
    
    int ret = getaddrinfo(hostname.c_str(), NULL, &hint, &ailist);
    if (ret != 0) {
        LOG_WARN("getaddrinfo: %s", gai_strerror(ret));
        return -1;
    }

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        if (aip->ai_family == AF_INET && aip->ai_protocol == IPPROTO_TCP) {
            sinp = (struct sockaddr_in*)aip->ai_addr;
            addr = inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
            if (addr.length() == 0 || addr == "127.0.0.1") {
                LOG_WARN("Cant get addr by hostname[%s]", hostname.c_str());
                return -1;
            }
            break;
        }
    }

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

    char buf[256] = {0};
    const char *ret = ::inet_ntop(AF_INET, &cliaddr->sin_addr, buf, *addrlen);
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

    if (check_ip_addr(ip) == false) {
        std::string addr;
        int ret = get_addr_by_hostname(ip, addr);
        if (ret < 0 && addr.length() > 0) {
            return -1;
        }
        ip = addr;
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
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;
            }

            if (errno != EINTR) {
                LOG_ERROR("recv: %s", strerror(errno));
                this->close();
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

    const ssize_t send_once_size = 8192;
    ssize_t send_size = 0;
    do {
        int ready_data_size = buff.get_cont_read_size() > send_once_size ? send_once_size : buff.get_cont_read_size();
        int ret = ::send(socket_, buff.get_read_buffer_ptr(), buff.get_cont_read_size(), flags);
        if (ret < 0) {
            // 在非阻塞模式下,send函数的过程仅仅是将数据拷贝到协议栈的缓存区而已,
            // 如果缓存区可用空间不够,则尽能力的拷贝,返回成功拷贝的大小;
            // 如缓存区可用空间为0,则返回-1,同时设置errno为EAGAIN.
            if (errno == EAGAIN || errno == EINTR) {
                Time::sleep(10);
                continue;
            }

            LOG_ERROR("send: %s", strerror(errno));
            break;
        }
        send_size += ret;
        buff.update_read_pos(ret);
    }  while (buff.data_size() > 0);
   
    return send_size;
}

//////////////////////////// Debug code ///////////////////////////////////////////////////
void 
SocketTCP::print_family(struct addrinfo *aip)
{
    printf(" family ");
    switch (aip->ai_family) {
        case AF_INET:
            printf("inet"); break;
        case AF_INET6:
            printf("inet6"); break;
        case AF_UNIX:
            printf("unix"); break;
        case AF_UNSPEC:
            printf("unspecified"); break;
        default:
            printf("unknown");
    }
}
void 
SocketTCP::print_type(struct addrinfo *aip)
{
    printf(" type ");
    switch (aip->ai_socktype) {
        case SOCK_STREAM:
            printf("stream"); break;
        case SOCK_DGRAM:
            printf("datagram"); break;
        case SOCK_SEQPACKET:
            printf("seqpacket"); break;
        case SOCK_RAW:
            printf("raw"); break;
        default:
            printf("unknown (%d)", aip->ai_socktype);
    }
}
void 
SocketTCP::print_protocol(struct addrinfo *aip)
{
    printf(" protocol ");
    switch (aip->ai_protocol) {
        case 0:
            printf("default"); break;
        case IPPROTO_TCP:
            printf("TCP"); break;
        case IPPROTO_UDP:
            printf("UDP"); break;
        case IPPROTO_RAW:
            printf("raw"); break;
        default:
            printf("unknown (%d)", aip->ai_protocol);
    }
}

void 
SocketTCP::print_flags(struct addrinfo *aip)
{
    printf(" flags ");
    if (aip->ai_flags == 0) {
        printf(" 0");
    } else {
        if (aip->ai_flags & AI_PASSIVE)  {
            printf(" passive");
        } 
        if (aip->ai_flags & AI_CANONNAME) {
            printf(" canon");
        } 
        if (aip->ai_flags & AI_NUMERICHOST) {
            printf(" numhost");
        }
        if (aip->ai_flags & AI_NUMERICSERV) {
            printf(" numserv");
        }
        if (aip->ai_flags & AI_V4MAPPED) {
            printf(" v4mapped");
        }
        if (aip->ai_flags &AI_ALL) {
            printf(" all");
        }
    }
}

void 
SocketTCP::test_getaddr(std::string str)
{
    struct addrinfo     *ailist, *aip;
    struct addrinfo     hint;
    struct sockaddr_in  *sinp;
    const char          *addr;
    int                 err;
    char                abuf[INET_ADDRSTRLEN];

    hint.ai_flags = AI_CANONNAME;
    hint.ai_family = 0;
    hint.ai_socktype = 0;
    hint.ai_protocol = 0;
    hint.ai_addrlen = 0;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(str.c_str(), NULL, &hint, &ailist)) != 0) {
        perror("getaddrinfo");
    }

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        print_flags(aip);
        print_family(aip);
        print_type(aip);
        print_protocol(aip);
        printf("\n\thost %s", aip->ai_canonname ? aip->ai_canonname : "-");
        if (aip->ai_family == AF_INET) {
            sinp = (struct sockaddr_in*)aip->ai_addr;
            addr = inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
            printf(" address %s", addr ? addr : "unknown");
            printf(" port %d", ntohs(sinp->sin_port));
        }
        printf("\n");
    }
}

}