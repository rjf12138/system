#include "system.h"

#include <algorithm>
#include <poll.h>

namespace os {
SocketTCP::SocketTCP(void)
: is_enable_(false),
socket_(-1),
port_(0)
{

}

SocketTCP::SocketTCP(std::string ip, uint16_t port)
: is_enable_(false),
socket_(-1),
port_(0)
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
    if (is_enable_ == false) {
        return 0;
    }

    int fd = socket_;
    socket_ = -1;
    is_enable_ = false;
    if (::close(fd) < 0) {
        LOG_ERROR("close failed: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int 
SocketTCP::shutdown(int how)
{
    if (is_enable_ == false) {
        return 0;
    }

    if (::shutdown(socket_, how) < 0) {
        LOG_ERROR("shutdown failed: %s", strerror(errno));
        return -1;
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
    
    int old_option = ::fcntl(socket_, F_GETFL);
    if (old_option < 0) {
        LOG_ERROR("fcntl(F_GETFL): %s", strerror(errno));
        return -1;
    }
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
    int ret = ::setsockopt(socket_,SOL_SOCKET,SO_REUSEADDR, &enable, sizeof(int));
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
    if (is_enable_ == false) {
        return "";
    }
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
    addr.clear();

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *ailist = nullptr;
    int ret = ::getaddrinfo(hostname.c_str(), nullptr, &hints, &ailist);
    if (ret != 0) {
        LOG_WARN("getaddrinfo[%s]: %s", hostname.c_str(), gai_strerror(ret));
        return -1;
    }

    int result = -1;
    for (struct addrinfo *aip = ailist; aip != nullptr; aip = aip->ai_next) {
        if (aip->ai_family != AF_INET || aip->ai_addr == nullptr) {
            continue;
        }

        char abuf[INET_ADDRSTRLEN] = {0};
        const char *ip = ::inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in*>(aip->ai_addr)->sin_addr,
                                     abuf, sizeof(abuf));
        if (ip == nullptr) {
            LOG_ERROR("inet_ntop: %s", strerror(errno));
            continue;
        }
        addr = ip;
        result = 0;
        break;
    }

    ::freeaddrinfo(ailist);
    if (result != 0) {
        LOG_WARN("Cant get IPv4 addr by hostname[%s]", hostname.c_str());
    }
    return result;
}

int 
SocketTCP::set_socket(int clisock, struct sockaddr_in *cliaddr, socklen_t *addrlen)
{
    if (is_enable_ == true) {
        LOG_WARN("set_socket failed, there's already opened a socket.");
        return -1;
    }
    if (clisock < 0 || cliaddr == nullptr || addrlen == nullptr) {
        return -1;
    }

    char buf[INET_ADDRSTRLEN] = {0};
    const char *ret = ::inet_ntop(AF_INET, &cliaddr->sin_addr, buf, sizeof(buf));
    if (ret == nullptr) {
        LOG_ERROR("inet_ntop: %s", strerror(errno));
        return -1;
    }

    ip_ = ret;
    port_ = ::ntohs(cliaddr->sin_port);
    addr_ = *cliaddr;
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
        if (ret < 0 || addr.empty()) {
            return -1;
        }
        ip = addr;
    }

    ip_ = ip;
    port_ = port;
    memset(&addr_, 0, sizeof(struct sockaddr_in));

    int ret = ::inet_pton(AF_INET, ip_.c_str(), &addr_.sin_addr);
    if (ret  == 0) {
        LOG_WARN("Incorrect format of IP address: %s", ip_.c_str());
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

    int ret = ::bind(socket_, reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_));
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

    int ret = ::connect(socket_, reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_));
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

    int ret = -1;
    do {
        ret = ::accept(socket_, cliaddr, addrlen);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        // 监听 socket 不应因为一次临时错误被关闭，交给调用方决定是否退出
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG_WARN("accept would block");
        } else {
            LOG_ERROR("accept: %s", strerror(errno));
        }
        return -1;
    }
    clisock = ret;

    return 0;
}

ssize_t 
SocketTCP::recv(ByteBuffer &buff, int flags)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    int old_flags = ::fcntl(socket_, F_GETFL);
    bool nonblocking = (old_flags != -1 && (old_flags & O_NONBLOCK) != 0);

    const int buffer_size = 2048;
    char buffer[buffer_size] = {0};
    ssize_t recv_size = 0;
    while (true) {
        ssize_t ret = ::recv(socket_, buffer, sizeof(buffer), flags);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                struct pollfd pfd;
                pfd.fd = socket_;
                pfd.events = POLLIN;
                pfd.revents = 0;
                int poll_ret = ::poll(&pfd, 1, 1000);
                if (poll_ret > 0) {
                    continue;
                }
                break;
            }
            LOG_ERROR("recv: %s", strerror(errno));
            this->close();
            break;
        }
        if (ret == 0) {
            break; // 对端关闭
        }

        buff.write_bytes(buffer, static_cast<size_t>(ret));
        recv_size += ret;

        // 阻塞 socket 一次 recv 后返回，避免在调用方未预期时一直等待
        if (nonblocking == false) {
            break;
        }
    }

    return recv_size;
}

ssize_t 
SocketTCP::send(ByteBuffer &buff, int flags)
{
    if (is_enable_ == false) {
        LOG_WARN("Please create socket first.");
        return -1;
    }

    if (buff.data_size() <= 0) {
        return 0;
    }

    int send_flags = flags;
#ifdef MSG_NOSIGNAL
    send_flags |= MSG_NOSIGNAL;
#endif

    ssize_t send_size = 0;
    while (buff.data_size() > 0) {
        size_t chunk = static_cast<size_t>(buff.get_cont_read_size());
        if (chunk == 0) {
            break;
        }

        ssize_t ret = ::send(socket_, buff.get_read_buffer_ptr(), chunk, send_flags);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = socket_;
                pfd.events = POLLOUT;
                pfd.revents = 0;
                int poll_ret = ::poll(&pfd, 1, 1000);
                if (poll_ret > 0) {
                    continue;
                }
                break;
            }
            LOG_ERROR("send: %s", strerror(errno));
            break;
        }
        if (ret == 0) {
            break;
        }

        send_size += ret;
        buff.update_read_pos(ret);
    }

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
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;

    struct addrinfo *ailist = nullptr;
    int err = ::getaddrinfo(str.c_str(), nullptr, &hints, &ailist);
    if (err != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(err));
        return;
    }

    for (struct addrinfo *aip = ailist; aip != nullptr; aip = aip->ai_next) {
        print_flags(aip);
        print_family(aip);
        print_type(aip);
        print_protocol(aip);
        printf("\n\thost %s", aip->ai_canonname ? aip->ai_canonname : "-");
        if (aip->ai_family == AF_INET && aip->ai_addr != nullptr) {
            struct sockaddr_in *sinp = reinterpret_cast<struct sockaddr_in*>(aip->ai_addr);
            char abuf[INET_ADDRSTRLEN] = {0};
            const char *addr = ::inet_ntop(AF_INET, &sinp->sin_addr, abuf, sizeof(abuf));
            printf(" address %s", addr ? addr : "unknown");
            printf(" port %d", ntohs(sinp->sin_port));
        }
        printf("\n");
    }

    ::freeaddrinfo(ailist);
}

}