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
LceSocketHandle LceNetOpenUdpSocket();
bool LceNetSetSocketBroadcast(LceSocketHandle socketHandle, bool enabled);
bool LceNetSetSocketReuseAddress(LceSocketHandle socketHandle, bool enabled);
bool LceNetSetSocketRecvTimeout(LceSocketHandle socketHandle, int timeoutMs);
bool LceNetBindIpv4(LceSocketHandle socketHandle, const char* bindIp, int port);
int LceNetGetBoundPort(LceSocketHandle socketHandle);
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
