//------------------------------------------------------------------------------
#include "platform.h"
//------------------------------------------------------------------------------
#include <stdio.h>
//------------------------------------------------------------------------------
#ifdef IS_LINUX

void Sleep(unsigned long ms)
{
    timespec tw = {ms / 1000UL, ms % 1000UL * 1000000UL};
    timespec tr;
    nanosleep(&tw, &tr);
}

#endif

bool enable_nonblocking_mode(SOCKET s, const bool value)
{
#ifdef IS_LINUX
    return fcntl(s, F_SETFL, value? O_NONBLOCK : O_RDWR) != -1;
#else
    unsigned long _value = value;
    return ioctlsocket(s, FIONBIO, &_value) != SOCKET_ERROR;
#endif
}

#ifdef IS_LINUX

#include <errno.h>

int socket_subsys_error(void)
{
    return errno;
}

#else

int socket_subsys_error(void)
{
    return WSAGetLastError();
}

#endif

bool socket_subsys_init(void)
{
#ifdef IS_LINUX
    return true;
#else
    WSADATA wsa_data;
    return !WSAStartup(0x101, &wsa_data) && wsa_data.wVersion == 0x101;
#endif
}
void socket_subsys_uninit(void)
{
#ifdef IS_LINUX
#else
    WSACleanup();
#endif
}
