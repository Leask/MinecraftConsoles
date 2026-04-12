#include "lce_net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>

namespace
{
    int ToNativeSocket(LceSocketHandle socketHandle)
    {
        return static_cast<int>(socketHandle);
    }
}

bool LceNetInitialize()
{
    return true;
}

void LceNetShutdown()
{
}

int LceNetGetLastError()
{
    return errno;
}

int LceNetCloseSocket(LceSocketHandle socketHandle)
{
    return close(ToNativeSocket(socketHandle));
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
    std::snprintf(portText, sizeof(portText), "%d", port);

    addrinfo* result = nullptr;
    if (getaddrinfo(host, portText, &hints, &result) != 0 || result == nullptr)
    {
        return false;
    }

    bool resolved = false;
    for (addrinfo* current = result; current != nullptr;
        current = current->ai_next)
    {
        if (current->ai_family != AF_INET)
        {
            continue;
        }

        const sockaddr_in* address =
            reinterpret_cast<const sockaddr_in*>(current->ai_addr);
        if (inet_ntop(
                AF_INET,
                &address->sin_addr,
                outIp,
                outIpSize) != nullptr)
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
    const int socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return socketHandle < 0
        ? LCE_INVALID_SOCKET
        : static_cast<LceSocketHandle>(socketHandle);
}

LceSocketHandle LceNetOpenUdpSocket()
{
    const int socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    return socketHandle < 0
        ? LCE_INVALID_SOCKET
        : static_cast<LceSocketHandle>(socketHandle);
}

bool LceNetSetSocketNonBlocking(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int flags = fcntl(ToNativeSocket(socketHandle), F_GETFL, 0);
    if (flags < 0)
    {
        return false;
    }

    const int nextFlags = enabled
        ? (flags | O_NONBLOCK)
        : (flags & ~O_NONBLOCK);
    return fcntl(ToNativeSocket(socketHandle), F_SETFL, nextFlags) == 0;
}

bool LceNetSetSocketNoDelay(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int value = enabled ? 1 : 0;
    return setsockopt(
        ToNativeSocket(socketHandle),
        IPPROTO_TCP,
        TCP_NODELAY,
        &value,
        sizeof(value)) == 0;
}

bool LceNetSetSocketBroadcast(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int value = enabled ? 1 : 0;
    return setsockopt(
        ToNativeSocket(socketHandle),
        SOL_SOCKET,
        SO_BROADCAST,
        &value,
        sizeof(value)) == 0;
}

bool LceNetSetSocketReuseAddress(LceSocketHandle socketHandle, bool enabled)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return false;
    }

    const int value = enabled ? 1 : 0;
    return setsockopt(
        ToNativeSocket(socketHandle),
        SOL_SOCKET,
        SO_REUSEADDR,
        &value,
        sizeof(value)) == 0;
}

bool LceNetSetSocketRecvTimeout(LceSocketHandle socketHandle, int timeoutMs)
{
    if (socketHandle == LCE_INVALID_SOCKET || timeoutMs < 0)
    {
        return false;
    }

    const timeval timeout = {
        timeoutMs / 1000,
        static_cast<suseconds_t>((timeoutMs % 1000) * 1000)
    };
    return setsockopt(
        ToNativeSocket(socketHandle),
        SOL_SOCKET,
        SO_RCVTIMEO,
        &timeout,
        sizeof(timeout)) == 0;
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

    return ::bind(
        ToNativeSocket(socketHandle),
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
}

int LceNetGetBoundPort(LceSocketHandle socketHandle)
{
    if (socketHandle == LCE_INVALID_SOCKET)
    {
        return -1;
    }

    sockaddr_in address = {};
    socklen_t addressLength = sizeof(address);
    if (getsockname(
            ToNativeSocket(socketHandle),
            reinterpret_cast<sockaddr*>(&address),
            &addressLength) != 0)
    {
        return -1;
    }

    return static_cast<int>(ntohs(address.sin_port));
}

bool LceNetListen(LceSocketHandle socketHandle, int backlog)
{
    if (socketHandle == LCE_INVALID_SOCKET || backlog <= 0)
    {
        return false;
    }

    return listen(ToNativeSocket(socketHandle), backlog) == 0;
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

    return connect(
        ToNativeSocket(socketHandle),
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
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
    socklen_t addressLength = sizeof(address);
    const int accepted = accept(
        ToNativeSocket(socketHandle),
        reinterpret_cast<sockaddr*>(&address),
        &addressLength);
    if (accepted < 0)
    {
        return LCE_INVALID_SOCKET;
    }

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

    return static_cast<int>(send(
        ToNativeSocket(socketHandle),
        data,
        static_cast<std::size_t>(dataSize),
        0));
}

int LceNetRecv(LceSocketHandle socketHandle, void* buffer, int bufferSize)
{
    if (socketHandle == LCE_INVALID_SOCKET ||
        buffer == nullptr ||
        bufferSize <= 0)
    {
        return -1;
    }

    return static_cast<int>(recv(
        ToNativeSocket(socketHandle),
        buffer,
        static_cast<std::size_t>(bufferSize),
        0));
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

    return static_cast<int>(sendto(
        ToNativeSocket(socketHandle),
        data,
        static_cast<std::size_t>(dataSize),
        0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)));
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
    socklen_t senderLength = sizeof(senderAddress);
    const int bytesRead = static_cast<int>(recvfrom(
        ToNativeSocket(socketHandle),
        buffer,
        static_cast<std::size_t>(bufferSize),
        0,
        reinterpret_cast<sockaddr*>(&senderAddress),
        &senderLength));
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
