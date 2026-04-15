#pragma once

void NativeDesktopRequestStorageExit(void (*complete)());
void NativeDesktopSetSystemUIDisplaying(bool displaying);
bool NativeDesktopIsSystemUIDisplaying();
void NativeDesktopSetSaveDeviceSelected(unsigned int pad, bool selected);
bool NativeDesktopGetSaveDeviceSelected(unsigned int pad);
