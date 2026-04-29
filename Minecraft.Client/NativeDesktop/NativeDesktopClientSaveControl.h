#pragma once

#include <cstddef>
#include <ctime>

typedef int (*NativeDesktopLoadSaveDataCallback)(
    void* param,
    bool isCorrupt,
    bool isOwner);
typedef int (*NativeDesktopLoadSaveDataThumbnailCallback)(
    void* param,
    unsigned char* thumbnailData,
    unsigned int thumbnailBytes);
typedef int (*NativeDesktopDeleteSaveDataCallback)(
    void* param,
    bool success);

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
int NativeDesktopGetSaveCount();
bool NativeDesktopGetSaveInfo(
    int index,
    char* utf8Title,
    std::size_t utf8TitleCount,
    char* utf8Filename,
    std::size_t utf8FilenameCount,
    wchar_t* utf16Title,
    std::size_t utf16TitleCount,
    wchar_t* utf16Filename,
    std::size_t utf16FilenameCount,
    std::time_t* modifiedTime,
    unsigned char** thumbnailData,
    unsigned int* thumbnailBytes,
    int* blocksUsed);
int NativeDesktopLoadSaveDataByIndex(
    int index,
    NativeDesktopLoadSaveDataCallback callback,
    void* param);
int NativeDesktopLoadSaveDataThumbnailByIndex(
    int index,
    NativeDesktopLoadSaveDataThumbnailCallback callback,
    void* param,
    bool force);
int NativeDesktopDeleteSaveDataByIndex(
    int index,
    NativeDesktopDeleteSaveDataCallback callback,
    void* param);
void NativeDesktopSetSaveImages(
    const unsigned char* thumbnail,
    unsigned int thumbnailBytes);
bool NativeDesktopRefreshPersistedSaveCatalog();
void NativeDesktopTickSaves();
