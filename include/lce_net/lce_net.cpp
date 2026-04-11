#include "lce_net.h"

#if defined(_WINDOWS64) || defined(_WIN32)
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <cstring>

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

bool LceNetResolveIpv4(
    const char* host,
    int port,
    char* outIp,
    std::size_t outIpSize)
{
    if (host == nullptr ||
        host[0] == '\0' ||
        outIp == nullptr ||
        outIpSize == 0 ||
        port < 0 ||
        port > 65535)
    {
        return false;
    }

    outIp[0] = '\0';

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char portText[16] = {};
#if defined(_WINDOWS64) || defined(_WIN32)
    _snprintf_s(portText, sizeof(portText), _TRUNCATE, "%d", port);
#else
    std::snprintf(portText, sizeof(portText), "%d", port);
#endif

    addrinfo* result = nullptr;
    if (getaddrinfo(host, portText, &hints, &result) != 0 || result == nullptr)
    {
        return false;
    }

    bool resolved = false;
    for (addrinfo* current = result; current != nullptr; current = current->ai_next)
    {
        if (current->ai_family != AF_INET)
        {
            continue;
        }

        const sockaddr_in* address =
            reinterpret_cast<const sockaddr_in*>(current->ai_addr);
        if (inet_ntop(AF_INET, &address->sin_addr, outIp, outIpSize) != nullptr)
        {
            resolved = true;
            break;
        }
    }

    freeaddrinfo(result);
    return resolved;
}

LceSocketHandle LceNetOpenTcpSocket()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    const SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return socketHandle == INVALID_SOCKET
        ? LCE_INVALID_SOCKET
        : static_cast<LceSocketHandle>(socketHandle);
#else
    const int socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return socketHandle < 0
        ? LCE_INVALID_SOCKET
        : static_cast<LceSocketHandle>(socketHandle);
#endif
}

LceSocketHandle LceNetOpenUdpSocket()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    const SOCKET socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    return socketHandle == INVALID_SOCKET
        ? LCE_INVALID_SOCKET
        : static_cast<LceSocketHandle>(socketHandle);
#else
    const int socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    return socketHandle < 0
        ? LCE_INVALID_SOCKET
        : static_cast<LceSocketHandle>(socketHandle);
#endif
}

bool LceNetSetSocketNonBlocking(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    u_long mode = enabled ? 1UL : 0UL;
    return ioctlsocket(
        static_cast<SOCKET>(socketHandle),
        FIONBIO,
        &mode) == 0;
#else
    const int flags = fcntl(static_cast<int>(socketHandle), F_GETFL, 0);
    if (flags < 0)
    {
        return false;
    }

    const int nextFlags = enabled
        ? (flags | O_NONBLOCK)
        : (flags & ~O_NONBLOCK);
    return fcntl(
        static_cast<int>(socketHandle),
        F_SETFL,
        nextFlags) == 0;
#endif
}

bool LceNetSetSocketNoDelay(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int value = enabled ? 1 : 0;
#if defined(_WINDOWS64) || defined(_WIN32)
    return setsockopt(
        static_cast<SOCKET>(socketHandle),
        IPPROTO_TCP,
        TCP_NODELAY,
        reinterpret_cast<const char*>(&value),
        sizeof(value)) == 0;
#else
    return setsockopt(
        static_cast<int>(socketHandle),
        IPPROTO_TCP,
        TCP_NODELAY,
        &value,
        sizeof(value)) == 0;
#endif
}

bool LceNetSetSocketBroadcast(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int value = enabled ? 1 : 0;
#if defined(_WINDOWS64) || defined(_WIN32)
    return setsockopt(
        static_cast<SOCKET>(socketHandle),
        SOL_SOCKET,
        SO_BROADCAST,
        reinterpret_cast<const char*>(&value),
        sizeof(value)) == 0;
#else
    return setsockopt(
        static_cast<int>(socketHandle),
        SOL_SOCKET,
        SO_BROADCAST,
        &value,
        sizeof(value)) == 0;
#endif
}

bool LceNetSetSocketReuseAddress(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int value = enabled ? 1 : 0;
#if defined(_WINDOWS64) || defined(_WIN32)
    return setsockopt(
        static_cast<SOCKET>(socketHandle),
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&value),
        sizeof(value)) == 0;
#else
    return setsockopt(
        static_cast<int>(socketHandle),
        SOL_SOCKET,
        SO_REUSEADDR,
        &value,
        sizeof(value)) == 0;
#endif
}

bool LceNetSetSocketRecvTimeout(LceSocketHandle socketHandle, int timeoutMs)
{
    if (socketHandle == LCE_INVALID_SOCKET || timeoutMs < 0)
    {
        return false;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    const DWORD timeout = static_cast<DWORD>(timeoutMs);
    return setsockopt(
        static_cast<SOCKET>(socketHandle),
        SOL_SOCKET,
        SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeout),
        sizeof(timeout)) == 0;
#else
    const timeval timeout = {
        timeoutMs / 1000,
        static_cast<suseconds_t>((timeoutMs % 1000) * 1000)
    };
    return setsockopt(
        static_cast<int>(socketHandle),
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)) == 0;
#endif
}

bool LceNetBindIpv4(LceSocketHandle socketHandle, const char* bindIp, int port)
{
    if (socketHandle == LCE_INVALID_SOCKET || port < 0 || port > 65535)
    {
        return false;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<std::uint16_t>(port));
    if (bindIp == nullptr || bindIp[0] == '\0')
    {
        address.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else if (inet_pton(AF_INET, bindIp, &address.sin_addr) != 1)
    {
        return false;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    return ::bind(
        static_cast<SOCKET>(socketHandle),
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
#else
    return ::bind(
        static_cast<int>(socketHandle),
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
#endif
}

int LceNetGetBoundPort(LceSocketHandle socketHandle)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return -1;
    }

    sockaddr_in address = {};
#if defined(_WINDOWS64) || defined(_WIN32)
    int addressLength = sizeof(address);
    if (getsockname(
            static_cast<SOCKET>(socketHandle),
            reinterpret_cast<sockaddr*>(&address),
            &addressLength) != 0)
    {
        return -1;
    }
#else
    socklen_t addressLength = sizeof(address);
    if (getsockname(
            static_cast<int>(socketHandle),
            reinterpret_cast<sockaddr*>(&address),
            &addressLength) != 0)
    {
        return -1;
    }
#endif

    return static_cast<int>(ntohs(address.sin_port));
}

bool LceNetListen(LceSocketHandle socketHandle, int backlog)
{
    if (socketHandle == LCE_INVALID_SOCKET || backlog <= 0)
    {
        return false;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    return listen(static_cast<SOCKET>(socketHandle), backlog) == 0;
#else
    return listen(static_cast<int>(socketHandle), backlog) == 0;
#endif
}

bool LceNetConnectIpv4(LceSocketHandle socketHandle, const char* ip, int port)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        ip == nullptr ||
        ip[0] == '\0' ||
        port < 0 ||
        port > 65535)
    {
        return false;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<std::uint16_t>(port));
    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1)
    {
        return false;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    return connect(
        static_cast<SOCKET>(socketHandle),
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
#else
    return connect(
        static_cast<int>(socketHandle),
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
#endif
}

LceSocketHandle LceNetAcceptIpv4(
    LceSocketHandle socketHandle,
    char* outIp,
    std::size_t outIpSize,
    int* outPort)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return LCE_INVALID_SOCKET;
    }

    if (outIp != nullptr && outIpSize > 0)
    {
        outIp[0] = '\0';
    }
    if (outPort != nullptr)
    {
        *outPort = 0;
    }

    sockaddr_in address = {};
#if defined(_WINDOWS64) || defined(_WIN32)
    int addressLength = sizeof(address);
    const SOCKET accepted = accept(
        static_cast<SOCKET>(socketHandle),
        reinterpret_cast<sockaddr*>(&address),
        &addressLength);
    if (accepted == INVALID_SOCKET)
    {
        return LCE_INVALID_SOCKET;
    }
#else
    socklen_t addressLength = sizeof(address);
    const int accepted = accept(
        static_cast<int>(socketHandle),
        reinterpret_cast<sockaddr*>(&address),
        &addressLength);
    if (accepted < 0)
    {
        return LCE_INVALID_SOCKET;
    }
#endif

    if (outIp != nullptr && outIpSize > 0)
    {
        inet_ntop(AF_INET, &address.sin_addr, outIp, outIpSize);
    }
    if (outPort != nullptr)
    {
        *outPort = static_cast<int>(ntohs(address.sin_port));
    }

    return static_cast<LceSocketHandle>(accepted);
}

int LceNetSend(LceSocketHandle socketHandle, const void* data, int dataSize)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        data == nullptr ||
        dataSize <= 0)
    {
        return -1;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    return send(
        static_cast<SOCKET>(socketHandle),
        static_cast<const char*>(data),
        dataSize,
        0);
#else
    return static_cast<int>(send(
        static_cast<int>(socketHandle),
        data,
        static_cast<std::size_t>(dataSize),
        0));
#endif
}

int LceNetRecv(LceSocketHandle socketHandle, void* buffer, int bufferSize)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        buffer == nullptr ||
        bufferSize <= 0)
    {
        return -1;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    return recv(
        static_cast<SOCKET>(socketHandle),
        static_cast<char*>(buffer),
        bufferSize,
        0);
#else
    return static_cast<int>(recv(
        static_cast<int>(socketHandle),
        buffer,
        static_cast<std::size_t>(bufferSize),
        0));
#endif
}

bool LceNetSendAll(LceSocketHandle socketHandle, const void* data, int dataSize)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        data == nullptr ||
        dataSize <= 0)
    {
        return false;
    }

    int totalSent = 0;
    while (totalSent < dataSize)
    {
        const int sent = LceNetSend(
            socketHandle,
            static_cast<const char*>(data) + totalSent,
            dataSize - totalSent);
        if (sent <= 0)
        {
            return false;
        }
        totalSent += sent;
    }

    return true;
}

bool LceNetRecvAll(LceSocketHandle socketHandle, void* buffer, int bufferSize)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        buffer == nullptr ||
        bufferSize <= 0)
    {
        return false;
    }

    int totalRead = 0;
    while (totalRead < bufferSize)
    {
        const int bytesRead = LceNetRecv(
            socketHandle,
            static_cast<char*>(buffer) + totalRead,
            bufferSize - totalRead);
        if (bytesRead <= 0)
        {
            return false;
        }
        totalRead += bytesRead;
    }

    return true;
}

int LceNetSendToIpv4(
    LceSocketHandle socketHandle,
    const char* ip,
    int port,
    const void* data,
    int dataSize)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        ip == nullptr ||
        ip[0] == '\0' ||
        data == nullptr ||
        dataSize <= 0 ||
        port < 0 ||
        port > 65535)
    {
        return -1;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<std::uint16_t>(port));
    if (inet_pton(AF_INET, ip, &address.sin_addr) != 1)
    {
        return -1;
    }

#if defined(_WINDOWS64) || defined(_WIN32)
    return sendto(
        static_cast<SOCKET>(socketHandle),
        static_cast<const char*>(data),
        dataSize,
        0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address));
#else
    return static_cast<int>(sendto(
        static_cast<int>(socketHandle),
        data,
        static_cast<std::size_t>(dataSize),
        0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)));
#endif
}

int LceNetRecvFromIpv4(
    LceSocketHandle socketHandle,
    void* buffer,
    int bufferSize,
    char* outIp,
    std::size_t outIpSize,
    int* outPort)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        buffer == nullptr ||
        bufferSize <= 0)
    {
        return -1;
    }

    if (outIp != nullptr && outIpSize > 0)
    {
        outIp[0] = '\0';
    }
    if (outPort != nullptr)
    {
        *outPort = 0;
    }

    sockaddr_in senderAddress = {};
#if defined(_WINDOWS64) || defined(_WIN32)
    int senderLength = sizeof(senderAddress);
    const int bytesRead = recvfrom(
        static_cast<SOCKET>(socketHandle),
        static_cast<char*>(buffer),
        bufferSize,
        0,
        reinterpret_cast<sockaddr*>(&senderAddress),
        &senderLength);
#else
    socklen_t senderLength = sizeof(senderAddress);
    const int bytesRead = static_cast<int>(recvfrom(
        static_cast<int>(socketHandle),
        buffer,
        static_cast<std::size_t>(bufferSize),
        0,
        reinterpret_cast<sockaddr*>(&senderAddress),
        &senderLength));
#endif
    if (bytesRead < 0)
    {
        return -1;
    }

    if (outIp != nullptr && outIpSize > 0)
    {
        inet_ntop(AF_INET, &senderAddress.sin_addr, outIp, outIpSize);
    }
    if (outPort != nullptr)
    {
        *outPort = static_cast<int>(ntohs(senderAddress.sin_port));
    }

    return bytesRead;
}
