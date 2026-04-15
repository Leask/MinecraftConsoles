#include "NativeDesktopClientSaveControl.h"

#include <atomic>
#include <algorithm>
#include <cwchar>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    constexpr int kNativeDesktopSaveStateLoadCompleteSuccess = 12;
    constexpr int kNativeDesktopSaveStateLoadCompleteFail = 13;
    constexpr int kNativeDesktopSaveStateDeleteSuccess = 15;
    constexpr int kNativeDesktopSaveStateDeleteFail = 16;
    constexpr int kNativeDesktopSaveStateGetSaveThumbnail = 25;
    constexpr unsigned int kNativeDesktopSaveBlockBytes = 1024U;

    std::atomic<bool> g_savesDisabled{false};
    std::atomic<bool> g_saveExists{false};
    std::atomic<bool> g_saveBusy{false};
    std::atomic<int> g_saveUniqueNumber{0};
    std::mutex g_saveMutex;
    std::vector<unsigned char> g_saveData;
    std::vector<unsigned char> g_saveThumbnailData;
    std::wstring g_saveTitle;
    std::wstring g_saveTitleExtraFileSuffix;
    std::time_t g_saveModifiedTime = 0;

    void NativeDesktopTouchSaveLocked()
    {
        g_saveModifiedTime = std::time(nullptr);
        g_saveUniqueNumber.fetch_add(1, std::memory_order_acq_rel);
    }

    bool NativeDesktopSaveSlotExistsLocked()
    {
        return g_saveExists.load(std::memory_order_acquire) ||
            !g_saveData.empty();
    }

    std::wstring NativeDesktopSaveTitleLocked()
    {
        if (!g_saveTitle.empty())
        {
            return g_saveTitle;
        }
        return L"NativeDesktopWorld";
    }

    void NativeDesktopCopyWide(
        const std::wstring& value,
        wchar_t* destination,
        std::size_t destinationCount)
    {
        if (destination == nullptr || destinationCount == 0)
        {
            return;
        }

        std::wmemset(destination, 0, destinationCount);
        const std::size_t copyCount =
            std::min(destinationCount - 1, value.size());
        if (copyCount > 0)
        {
            std::wmemcpy(destination, value.c_str(), copyCount);
        }
    }

    void NativeDesktopCopyWideAsUtf8Fallback(
        const std::wstring& value,
        char* destination,
        std::size_t destinationCount)
    {
        if (destination == nullptr || destinationCount == 0)
        {
            return;
        }

        std::memset(destination, 0, destinationCount);
        const std::size_t copyCount =
            std::min(destinationCount - 1, value.size());
        for (std::size_t index = 0; index < copyCount; ++index)
        {
            const wchar_t ch = value[index];
            destination[index] = (ch >= 0 && ch <= 0x7f)
                ? static_cast<char>(ch)
                : '?';
        }
    }

    unsigned int NativeDesktopSaveBlocksUsedLocked()
    {
        if (g_saveData.empty())
        {
            return 0;
        }

        const unsigned int bytes =
            static_cast<unsigned int>(g_saveData.size());
        return (bytes + kNativeDesktopSaveBlockBytes - 1U) /
            kNativeDesktopSaveBlockBytes;
    }
}

bool NativeDesktopSavesAreDisabled()
{
    return g_savesDisabled.load(std::memory_order_acquire);
}

void NativeDesktopSetSavesDisabled(bool disabled)
{
    g_savesDisabled.store(disabled, std::memory_order_release);
}

bool NativeDesktopDoesSaveExist()
{
    return g_saveExists.load(std::memory_order_acquire);
}

void NativeDesktopSetSaveExists(bool exists)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    if (exists)
    {
        NativeDesktopTouchSaveLocked();
    }
    g_saveExists.store(exists, std::memory_order_release);
}

bool NativeDesktopSavesAreIdle()
{
    return !g_saveBusy.load(std::memory_order_acquire);
}

void NativeDesktopResetSaveData()
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveData.clear();
    g_saveThumbnailData.clear();
    g_saveTitle.clear();
    g_saveTitleExtraFileSuffix.clear();
    g_saveModifiedTime = 0;
    g_saveExists.store(false, std::memory_order_release);
    g_saveBusy.store(false, std::memory_order_release);
}

void NativeDesktopSetSaveTitle(const wchar_t* title)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveTitle = (title != nullptr) ? title : L"";
}

void NativeDesktopSetSaveTitleExtraFileSuffix(const wchar_t* suffix)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveTitleExtraFileSuffix = (suffix != nullptr) ? suffix : L"";
}

bool NativeDesktopGetSaveUniqueNumber(int* value)
{
    if (value != nullptr)
    {
        *value = g_saveUniqueNumber.load(std::memory_order_acquire);
    }
    return true;
}

unsigned int NativeDesktopGetSaveDataSize()
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    return static_cast<unsigned int>(g_saveData.size());
}

void NativeDesktopCopySaveData(void* data, unsigned int* bytes)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    if (bytes != nullptr)
    {
        *bytes = static_cast<unsigned int>(g_saveData.size());
    }
    if (data != nullptr && !g_saveData.empty())
    {
        std::memcpy(data, g_saveData.data(), g_saveData.size());
    }
}

void* NativeDesktopAllocateSaveData(unsigned int bytes)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveData.assign(bytes, 0);
    if (bytes > 0)
    {
        NativeDesktopTouchSaveLocked();
        g_saveExists.store(true, std::memory_order_release);
    }
    return g_saveData.data();
}

void NativeDesktopSetSaveDataSize(unsigned int bytes)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveData.resize(bytes);
    if (bytes > 0)
    {
        NativeDesktopTouchSaveLocked();
        g_saveExists.store(true, std::memory_order_release);
    }
}

int NativeDesktopGetSaveCount()
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    return NativeDesktopSaveSlotExistsLocked() ? 1 : 0;
}

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
    int* blocksUsed)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    if (index != 0 || !NativeDesktopSaveSlotExistsLocked())
    {
        return false;
    }

    const std::wstring title = NativeDesktopSaveTitleLocked();
    const std::wstring filename = L"native";
    NativeDesktopCopyWideAsUtf8Fallback(title, utf8Title, utf8TitleCount);
    NativeDesktopCopyWideAsUtf8Fallback(
        filename,
        utf8Filename,
        utf8FilenameCount);
    NativeDesktopCopyWide(title, utf16Title, utf16TitleCount);
    NativeDesktopCopyWide(filename, utf16Filename, utf16FilenameCount);

    if (modifiedTime != nullptr)
    {
        *modifiedTime = g_saveModifiedTime;
    }
    if (thumbnailData != nullptr)
    {
        *thumbnailData = g_saveThumbnailData.empty()
            ? nullptr
            : g_saveThumbnailData.data();
    }
    if (thumbnailBytes != nullptr)
    {
        *thumbnailBytes = static_cast<unsigned int>(
            g_saveThumbnailData.size());
    }
    if (blocksUsed != nullptr)
    {
        *blocksUsed = static_cast<int>(NativeDesktopSaveBlocksUsedLocked());
    }
    return true;
}

int NativeDesktopLoadSaveDataByIndex(
    int index,
    NativeDesktopLoadSaveDataCallback callback,
    void* param)
{
    bool loaded = false;
    {
        std::lock_guard<std::mutex> lock(g_saveMutex);
        g_saveBusy.store(true, std::memory_order_release);
        loaded = index == 0 && NativeDesktopSaveSlotExistsLocked();
    }

    if (callback != nullptr)
    {
        callback(param, !loaded, loaded);
    }

    g_saveBusy.store(false, std::memory_order_release);
    return loaded
        ? kNativeDesktopSaveStateLoadCompleteSuccess
        : kNativeDesktopSaveStateLoadCompleteFail;
}

int NativeDesktopLoadSaveDataThumbnailByIndex(
    int index,
    NativeDesktopLoadSaveDataThumbnailCallback callback,
    void* param,
    bool force)
{
    (void)force;
    unsigned char* thumbnailData = nullptr;
    unsigned int thumbnailBytes = 0;
    {
        std::lock_guard<std::mutex> lock(g_saveMutex);
        if (index == 0 && NativeDesktopSaveSlotExistsLocked() &&
            !g_saveThumbnailData.empty())
        {
            thumbnailData = g_saveThumbnailData.data();
            thumbnailBytes = static_cast<unsigned int>(
                g_saveThumbnailData.size());
        }
    }

    if (callback != nullptr)
    {
        callback(param, thumbnailData, thumbnailBytes);
    }
    return kNativeDesktopSaveStateGetSaveThumbnail;
}

int NativeDesktopDeleteSaveDataByIndex(
    int index,
    NativeDesktopDeleteSaveDataCallback callback,
    void* param)
{
    bool deleted = false;
    {
        std::lock_guard<std::mutex> lock(g_saveMutex);
        deleted = index == 0 && NativeDesktopSaveSlotExistsLocked();
        if (deleted)
        {
            g_saveData.clear();
            g_saveThumbnailData.clear();
            g_saveTitle.clear();
            g_saveTitleExtraFileSuffix.clear();
            g_saveModifiedTime = 0;
            g_saveExists.store(false, std::memory_order_release);
        }
    }

    if (callback != nullptr)
    {
        callback(param, deleted);
    }
    return deleted
        ? kNativeDesktopSaveStateDeleteSuccess
        : kNativeDesktopSaveStateDeleteFail;
}

void NativeDesktopSetSaveImages(
    const unsigned char* thumbnail,
    unsigned int thumbnailBytes)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    if (thumbnail != nullptr && thumbnailBytes > 0)
    {
        g_saveThumbnailData.assign(thumbnail, thumbnail + thumbnailBytes);
        NativeDesktopTouchSaveLocked();
    }
    else
    {
        g_saveThumbnailData.clear();
    }
}

void NativeDesktopTickSaves()
{
}
