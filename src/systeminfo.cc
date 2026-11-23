#include "system.h"

namespace os {
SystemInfo::SystemInfo(void)
{

}

SystemInfo::~SystemInfo(void)
{

}

int 
SystemInfo::get_nprocs(void)
{
#ifdef __RJF_LINUX__
    return ::get_nprocs();
#elif defined(__RJF_WINDOWS__)
    return 0;
#else
    return 0;
#endif
}

int 
SystemInfo::get_nprocs_conf(void)
{
#ifdef __RJF_LINUX__
    return ::get_nprocs_conf();
#elif defined(__RJF_WINDOWS__)
    return 0;
#else
    return 0;
#endif
}

}