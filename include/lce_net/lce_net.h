#pragma once

#include <cstdint>

using LceSocketHandle = std::intptr_t;
static constexpr LceSocketHandle LCE_INVALID_SOCKET = -1;

bool LceNetInitialize();
void LceNetShutdown();
int LceNetGetLastError();
int LceNetCloseSocket(LceSocketHandle socketHandle);
bool LceNetStringIsIpLiteral(const char* text);
