#pragma once

#include <cstddef>
#include <cstdint>

#include "lce_lan.h"

using LceSocketHandle = std::intptr_t;
static constexpr LceSocketHandle LCE_INVALID_SOCKET = -1;

bool LceNetInitialize();
void LceNetShutdown();
int LceNetGetLastError();
int LceNetCloseSocket(LceSocketHandle socketHandle);
bool LceNetStringIsIpLiteral(const char* text);
bool LceNetResolveIpv4(
    const char* host,
    int port,
    char* outIp,
    std::size_t outIpSize);
LceSocketHandle LceNetOpenTcpSocket();
LceSocketHandle LceNetOpenUdpSocket();
bool LceNetSetSocketNoDelay(LceSocketHandle socketHandle, bool enabled);
bool LceNetSetSocketBroadcast(LceSocketHandle socketHandle, bool enabled);
bool LceNetSetSocketReuseAddress(LceSocketHandle socketHandle, bool enabled);
bool LceNetSetSocketRecvTimeout(LceSocketHandle socketHandle, int timeoutMs);
bool LceNetBindIpv4(LceSocketHandle socketHandle, const char* bindIp, int port);
int LceNetGetBoundPort(LceSocketHandle socketHandle);
bool LceNetListen(LceSocketHandle socketHandle, int backlog);
bool LceNetConnectIpv4(LceSocketHandle socketHandle, const char* ip, int port);
LceSocketHandle LceNetAcceptIpv4(
    LceSocketHandle socketHandle,
    char* outIp,
    std::size_t outIpSize,
    int* outPort);
int LceNetSend(LceSocketHandle socketHandle, const void* data, int dataSize);
int LceNetRecv(LceSocketHandle socketHandle, void* buffer, int bufferSize);
bool LceNetSendAll(LceSocketHandle socketHandle, const void* data, int dataSize);
bool LceNetRecvAll(LceSocketHandle socketHandle, void* buffer, int bufferSize);
int LceNetSendToIpv4(
    LceSocketHandle socketHandle,
    const char* ip,
    int port,
    const void* data,
    int dataSize);
int LceNetRecvFromIpv4(
    LceSocketHandle socketHandle,
    void* buffer,
    int bufferSize,
    char* outIp,
    std::size_t outIpSize,
    int* outPort);
