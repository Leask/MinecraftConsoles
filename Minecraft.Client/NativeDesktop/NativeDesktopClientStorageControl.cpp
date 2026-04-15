#include "../stdafx.h"

#include "NativeDesktopClientStorageControl.h"

#include <atomic>

namespace
{
    std::atomic<bool> g_systemUIDisplaying{false};
    constexpr unsigned int kNativeDesktopSaveDeviceSlots = 4;
    constexpr unsigned int kNativeDesktopSaveDeviceMask =
        (1u << kNativeDesktopSaveDeviceSlots) - 1u;
    std::atomic<unsigned int> g_saveDeviceSelectedMask{
        kNativeDesktopSaveDeviceMask};

    bool NativeDesktopSaveDevicePadInRange(unsigned int pad)
    {
        return pad < kNativeDesktopSaveDeviceSlots;
    }
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

void* NativeDesktopGetGameDefinedProfileData(int pad)
{
    return ProfileManager.GetGameDefinedProfileData(pad);
}

void NativeDesktopWriteProfile(int pad, bool gameDefinedDataChanged,
                               bool override5MinuteTimer)
{
    ProfileManager.WriteToProfile(
        pad,
        gameDefinedDataChanged,
        override5MinuteTimer);
}

void NativeDesktopForceQueuedProfileWrites(int pad)
{
    ProfileManager.ForceQueuedProfileWrites(pad);
}

void NativeDesktopSetSaveDeviceSelected(unsigned int pad, bool selected)
{
    if (!NativeDesktopSaveDevicePadInRange(pad))
    {
        return;
    }

    const unsigned int bit = 1u << pad;
    if (selected)
    {
        g_saveDeviceSelectedMask.fetch_or(bit, std::memory_order_acq_rel);
        return;
    }

    g_saveDeviceSelectedMask.fetch_and(~bit, std::memory_order_acq_rel);
}

bool NativeDesktopGetSaveDeviceSelected(unsigned int pad)
{
    if (!NativeDesktopSaveDevicePadInRange(pad))
    {
        return true;
    }

    const unsigned int bit = 1u << pad;
    const unsigned int selected =
        g_saveDeviceSelectedMask.load(std::memory_order_acquire);
    return (selected & bit) != 0;
}
