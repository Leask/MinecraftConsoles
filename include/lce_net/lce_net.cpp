#include "lce_net.h"

#if defined(_WINDOWS64) || defined(_WIN32)
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

bool LceNetInitialize()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    WSADATA wsaData = {};
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void LceNetShutdown()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    WSACleanup();
#endif
}

int LceNetGetLastError()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    return WSAGetLastError();
#else
    return errno;
#endif
}

int LceNetCloseSocket(LceSocketHandle socketHandle)
{
#if defined(_WINDOWS64) || defined(_WIN32)
    return closesocket(static_cast<SOCKET>(socketHandle));
#else
    return close(static_cast<int>(socketHandle));
#endif
}

bool LceNetStringIsIpLiteral(const char* text)
{
    if (text == nullptr || text[0] == '\0')
    {
        return false;
    }

    in_addr ipv4 = {};
    if (inet_pton(AF_INET, text, &ipv4) == 1)
    {
        return true;
    }

    in6_addr ipv6 = {};
    return inet_pton(AF_INET6, text, &ipv6) == 1;
}
