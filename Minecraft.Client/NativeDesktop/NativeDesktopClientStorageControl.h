#pragma once

void NativeDesktopRequestStorageExit(void (*complete)());
void NativeDesktopSetSystemUIDisplaying(bool displaying);
bool NativeDesktopIsSystemUIDisplaying();
void* NativeDesktopGetGameDefinedProfileData(int pad);
void NativeDesktopWriteProfile(int pad, bool gameDefinedDataChanged,
                               bool override5MinuteTimer);
void NativeDesktopForceQueuedProfileWrites(int pad);
void NativeDesktopSetSaveDeviceSelected(unsigned int pad, bool selected);
bool NativeDesktopGetSaveDeviceSelected(unsigned int pad);

template <typename Callback, typename Param>
bool NativeDesktopSelectSaveDevice(
    Callback callback,
    Param param,
    bool showUI = false)
{
    (void)callback;
    (void)param;
    (void)showUI;
    return true;
}
