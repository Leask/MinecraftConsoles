#include "NativeDesktopClientStorageControl.h"

#include <atomic>

namespace
{
    std::atomic<bool> g_systemUIDisplaying{false};
}

void NativeDesktopRequestStorageExit(void (*complete)())
{
    if (complete != nullptr)
    {
        complete();
    }
}

void NativeDesktopSetSystemUIDisplaying(bool displaying)
{
    g_systemUIDisplaying.store(displaying, std::memory_order_release);
}

bool NativeDesktopIsSystemUIDisplaying()
{
    return g_systemUIDisplaying.load(std::memory_order_acquire);
}
