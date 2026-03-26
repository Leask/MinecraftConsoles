#pragma once

#include <cstdint>

#include "lce_lan.h"

using LceSocketHandle = std::intptr_t;
static constexpr LceSocketHandle LCE_INVALID_SOCKET = -1;

bool LceNetInitialize();
void LceNetShutdown();
int LceNetGetLastError();
int LceNetCloseSocket(LceSocketHandle socketHandle);
bool LceNetStringIsIpLiteral(const char* text);
