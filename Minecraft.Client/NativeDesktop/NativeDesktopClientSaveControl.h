#pragma once

bool NativeDesktopSavesAreDisabled();
void NativeDesktopSetSavesDisabled(bool disabled);
bool NativeDesktopDoesSaveExist();
void NativeDesktopSetSaveExists(bool exists);
bool NativeDesktopSavesAreIdle();
void NativeDesktopResetSaveData();
void NativeDesktopSetSaveTitle(const wchar_t* title);
void NativeDesktopSetSaveTitleExtraFileSuffix(const wchar_t* suffix);
bool NativeDesktopGetSaveUniqueNumber(int* value);
unsigned int NativeDesktopGetSaveDataSize();
void NativeDesktopCopySaveData(void* data, unsigned int* bytes);
void* NativeDesktopAllocateSaveData(unsigned int bytes);
void NativeDesktopSetSaveDataSize(unsigned int bytes);
void NativeDesktopTickSaves();
