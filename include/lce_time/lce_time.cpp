#include "lce_time.h"

#include <errno.h>
#include <time.h>

namespace
{
std::uint64_t TimespecToNanoseconds(const timespec& value)
{
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ULL +
        static_cast<std::uint64_t>(value.tv_nsec);
}

std::uint64_t GetClockNanoseconds(clockid_t clockId)
{
    timespec value = {};
    if (clock_gettime(clockId, &value) != 0)
    {
        return 0;
    }

    return TimespecToNanoseconds(value);
}
}

std::uint64_t LceGetMonotonicMilliseconds()
{
    return LceGetMonotonicNanoseconds() / 1000000ULL;
}

std::uint64_t LceGetMonotonicNanoseconds()
{
    return GetClockNanoseconds(CLOCK_MONOTONIC);
}

std::uint64_t LceGetUnixTimeMilliseconds()
{
    return GetClockNanoseconds(CLOCK_REALTIME) / 1000000ULL;
}

void LceSleepMilliseconds(std::uint32_t milliseconds)
{
    timespec request = {};
    request.tv_sec = milliseconds / 1000U;
    request.tv_nsec = static_cast<long>(milliseconds % 1000U) * 1000000L;

    timespec remaining = {};
    while (nanosleep(&request, &remaining) != 0 && errno == EINTR)
    {
        request = remaining;
    }
}
