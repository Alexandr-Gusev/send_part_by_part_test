//------------------------------------------------------------------------------
#ifndef platform_h
#define platform_h
//------------------------------------------------------------------------------
//#define IS_LINUX

#ifdef IS_LINUX

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <dirent.h>

#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#define SOCKADDR_IN sockaddr_in
#define SOCKADDR sockaddr
#define SD_BOTH 2
#define WSAEAGAIN EAGAIN
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAENOBUFS ENOBUFS

#else

#include <winsock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include <direct.h>
#include <io.h>

#ifndef S_IFDIR
#define S_IFDIR  0x4000  /* directory */
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)  ((m) & S_IFDIR)
#endif

#define S_IRWXU 0
#define S_IRWXG 0
#define S_IRWXO 0

#define WSAEAGAIN WSAEWOULDBLOCK

#endif

#include <sys/stat.h>
//------------------------------------------------------------------------------
#ifdef IS_LINUX

void Sleep(unsigned long ms);

#endif

bool enable_nonblocking_mode(SOCKET s, const bool value);

bool socket_subsys_init(void);
void socket_subsys_uninit(void);
int socket_subsys_error(void);
//------------------------------------------------------------------------------
#endif //platform_h
