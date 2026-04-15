#include "NativeDesktopClientSaveControl.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    std::atomic<bool> g_savesDisabled{false};
    std::atomic<bool> g_saveExists{false};
    std::atomic<bool> g_saveBusy{false};
    std::mutex g_saveMutex;
    std::vector<unsigned char> g_saveData;
    std::wstring g_saveTitle;
    std::wstring g_saveTitleExtraFileSuffix;
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
    g_saveTitle.clear();
    g_saveTitleExtraFileSuffix.clear();
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
        g_saveExists.store(true, std::memory_order_release);
    }
}

void NativeDesktopTickSaves()
{
}
