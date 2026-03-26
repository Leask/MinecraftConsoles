#pragma once

#include <cstdint>

std::uint64_t LceGetMonotonicMilliseconds();
std::uint64_t LceGetMonotonicNanoseconds();
std::uint64_t LceGetUnixTimeMilliseconds();
void LceSleepMilliseconds(std::uint32_t milliseconds);
