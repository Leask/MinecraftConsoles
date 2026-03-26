#pragma once

#include <cstdint>

bool LceStdinIsAvailable();
int LceWaitForStdinReadable(std::uint32_t waitMs);
bool LceReadStdinByte(char* outByte);
