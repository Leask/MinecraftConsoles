#include "NativeDesktopClientSaveControl.h"

#include <atomic>

namespace
{
    std::atomic<bool> g_savesDisabled{false};
}

bool NativeDesktopSavesAreDisabled()
{
    return g_savesDisabled.load(std::memory_order_acquire);
}

void NativeDesktopSetSavesDisabled(bool disabled)
{
    g_savesDisabled.store(disabled, std::memory_order_release);
}

void NativeDesktopTickSaves()
{
}
