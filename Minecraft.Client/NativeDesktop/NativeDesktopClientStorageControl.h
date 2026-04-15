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
