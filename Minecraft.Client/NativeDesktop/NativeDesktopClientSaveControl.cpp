#include "NativeDesktopClientSaveControl.h"

#include <atomic>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace
{
    std::atomic<bool> g_savesDisabled{false};
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

void NativeDesktopResetSaveData()
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveData.clear();
    g_saveTitle.clear();
    g_saveTitleExtraFileSuffix.clear();
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
    return g_saveData.data();
}

void NativeDesktopSetSaveDataSize(unsigned int bytes)
{
    std::lock_guard<std::mutex> lock(g_saveMutex);
    g_saveData.resize(bytes);
}

void NativeDesktopTickSaves()
{
}
