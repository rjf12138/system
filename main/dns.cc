#include "system.h"

using namespace os;

int main(void)
{
    std::string host = "fundgz.1234567.com.cn";
    SocketTCP stcp;

    std::string addr;
    stcp.get_addr_by_hostname(host, addr);

    std::cout << "addr: " << addr << std::endl;

    return 0;
}