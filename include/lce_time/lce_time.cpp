#include "lce_time.h"

#if defined(_WINDOWS64) || defined(_WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <time.h>
#endif

namespace
{
#if defined(_WINDOWS64) || defined(_WIN32)
std::uint64_t QueryPerformanceCounterNanoseconds()
{
    static LARGE_INTEGER s_frequency = { 0 };
    if (s_frequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&s_frequency);
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return static_cast<std::uint64_t>(
        static_cast<double>(counter.QuadPart) * 1000000000.0 /
        static_cast<double>(s_frequency.QuadPart));
}

std::uint64_t FileTimeToUnixMilliseconds(const FILETIME& fileTime)
{
    ULARGE_INTEGER value = {};
    value.HighPart = fileTime.dwHighDateTime;
    value.LowPart = fileTime.dwLowDateTime;

    static const std::uint64_t kWindowsToUnixEpoch100Ns =
        116444736000000000ULL;
    return (value.QuadPart - kWindowsToUnixEpoch100Ns) / 10000ULL;
}
#else
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
#endif
}

std::uint64_t LceGetMonotonicMilliseconds()
{
    return LceGetMonotonicNanoseconds() / 1000000ULL;
}

std::uint64_t LceGetMonotonicNanoseconds()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    return QueryPerformanceCounterNanoseconds();
#else
    return GetClockNanoseconds(CLOCK_MONOTONIC);
#endif
}

std::uint64_t LceGetUnixTimeMilliseconds()
{
#if defined(_WINDOWS64) || defined(_WIN32)
    FILETIME fileTime = {};
    GetSystemTimeAsFileTime(&fileTime);
    return FileTimeToUnixMilliseconds(fileTime);
#else
    return GetClockNanoseconds(CLOCK_REALTIME) / 1000000ULL;
#endif
}

void LceSleepMilliseconds(std::uint32_t milliseconds)
{
#if defined(_WINDOWS64) || defined(_WIN32)
    Sleep(milliseconds);
#else
    timespec request = {};
    request.tv_sec = milliseconds / 1000U;
    request.tv_nsec = static_cast<long>(milliseconds % 1000U) * 1000000L;

    timespec remaining = {};
    while (nanosleep(&request, &remaining) != 0 && errno == EINTR)
    {
        request = remaining;
    }
#endif
}
