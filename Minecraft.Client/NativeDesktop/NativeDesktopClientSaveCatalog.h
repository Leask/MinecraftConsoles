#pragma once

#include <cstddef>
#include <cstring>

#include "NativeDesktopClientStubs.h"

class NativeDesktopClientSaveCatalog
{
public:
    PSAVE_DETAILS ReturnSavesInfo()
    {
        Refresh();
        return &m_saveDetails;
    }

    C4JStorage::ESaveGameState GetSavesInfo(
        int (*callback)(LPVOID, SAVE_DETAILS*, const bool),
        LPVOID param)
    {
        PSAVE_DETAILS saveDetails = ReturnSavesInfo();
        if (callback != nullptr)
        {
            callback(param, saveDetails, true);
        }
        return C4JStorage::ESaveGame_Idle;
    }

    void ClearSavesInfo()
    {
        std::memset(&m_saveInfoA, 0, sizeof(m_saveInfoA));
        m_saveDetails = {};
    }

    C4JStorage::ESaveGameState LoadSaveData(
        int saveIndex,
        NativeDesktopLoadSaveDataCallback callback,
        void* param,
        bool ignoreCRC = false)
    {
        (void)ignoreCRC;
        return static_cast<C4JStorage::ESaveGameState>(
            NativeDesktopLoadSaveDataByIndex(saveIndex, callback, param));
    }

    C4JStorage::ESaveGameState LoadSaveDataThumbnail(
        int saveIndex,
        NativeDesktopLoadSaveDataThumbnailCallback callback,
        void* param,
        bool force = false)
    {
        return static_cast<C4JStorage::ESaveGameState>(
            NativeDesktopLoadSaveDataThumbnailByIndex(
                saveIndex,
                callback,
                param,
                force));
    }

    C4JStorage::ESaveGameState DeleteSaveData(
        int saveIndex,
        NativeDesktopDeleteSaveDataCallback callback,
        void* param)
    {
        return static_cast<C4JStorage::ESaveGameState>(
            NativeDesktopDeleteSaveDataByIndex(saveIndex, callback, param));
    }

    C4JStorage::ESaveGameState CopySaveData(
        int saveIndex,
        int (*callback)(LPVOID, const bool, C4JStorage::ESaveGameState),
        bool (*progress)(LPVOID, const int),
        LPVOID param)
    {
        (void)saveIndex;
        if (progress != nullptr)
        {
            progress(param, 100);
        }
        if (callback != nullptr)
        {
            callback(param, true, C4JStorage::ESaveGame_CopyCompleteSuccess);
        }
        return C4JStorage::ESaveGame_CopyCompleteSuccess;
    }

    unsigned int GetSaveSize() const
    {
        return NativeDesktopGetSaveDataSize();
    }

    void CopySaveData(void* data, unsigned int* bytes)
    {
        NativeDesktopCopySaveData(data, bytes);
    }

    void* AllocateSaveData(unsigned int bytes)
    {
        return NativeDesktopAllocateSaveData(bytes);
    }

    void GetDefaultSaveImage(PBYTE* image, DWORD* bytes)
    {
        SetNullBuffer(image, bytes);
    }

    void GetDefaultSaveThumbnail(PBYTE* thumbnail, DWORD* bytes)
    {
        SetNullBuffer(thumbnail, bytes);
    }

    void SetSaveImages(PBYTE thumbnail,
                       DWORD thumbnailBytes,
                       PBYTE image,
                       DWORD imageBytes,
                       PBYTE textData,
                       DWORD textDataBytes)
    {
        (void)image;
        (void)imageBytes;
        (void)textData;
        (void)textDataBytes;
        NativeDesktopSetSaveImages(thumbnail, thumbnailBytes);
    }

    C4JStorage::ESaveGameState SaveSaveData(
        int (*callback)(LPVOID, const bool),
        LPVOID param)
    {
        NativeDesktopSetSaveExists(true);
        if (callback != nullptr)
        {
            callback(param, true);
        }
        return C4JStorage::ESaveGame_SaveCompleteSuccess;
    }

    bool GetSaveUniqueFileDir(char* name)
    {
        if (name != nullptr)
        {
            strcpy_s(name, MAX_SAVEFILENAME_LENGTH, "native");
        }
        return true;
    }

    void SetSaveUniqueFilename(char* name)
    {
        (void)name;
    }

    C4JStorage::ESaveGameState GetSaveState() const
    {
        return C4JStorage::ESaveGame_Idle;
    }

    void Tick()
    {
        NativeDesktopTickSaves();
    }

    bool EnoughSpaceForMinSave() const
    {
        return true;
    }

    bool CopySaveInfo(
        int saveIndex,
        char* utf8Title,
        std::size_t utf8TitleCount,
        char* utf8Filename,
        std::size_t utf8FilenameCount)
    {
        return NativeDesktopGetSaveInfo(
            saveIndex,
            utf8Title,
            utf8TitleCount,
            utf8Filename,
            utf8FilenameCount,
            nullptr,
            0,
            nullptr,
            0,
            nullptr,
            nullptr,
            nullptr,
            nullptr);
    }

private:
    static void SetNullBuffer(PBYTE* data, DWORD* bytes)
    {
        if (data != nullptr)
        {
            *data = nullptr;
        }
        if (bytes != nullptr)
        {
            *bytes = 0;
        }
    }

    void Refresh()
    {
        ClearSavesInfo();

        if (NativeDesktopGetSaveCount() <= 0)
        {
            return;
        }

        unsigned char* thumbnailData = nullptr;
        unsigned int thumbnailBytes = 0;
        int blocksUsed = 0;
        NativeDesktopGetSaveInfo(
            0,
            m_saveInfoA[0].UTF8SaveTitle,
            sizeof(m_saveInfoA[0].UTF8SaveTitle),
            m_saveInfoA[0].UTF8SaveFilename,
            sizeof(m_saveInfoA[0].UTF8SaveFilename),
            m_saveInfoA[0].UTF16SaveTitle,
            sizeof(m_saveInfoA[0].UTF16SaveTitle) /
                sizeof(m_saveInfoA[0].UTF16SaveTitle[0]),
            m_saveInfoA[0].UTF16SaveFilename,
            sizeof(m_saveInfoA[0].UTF16SaveFilename) /
                sizeof(m_saveInfoA[0].UTF16SaveFilename[0]),
            &m_saveInfoA[0].modifiedTime,
            &thumbnailData,
            &thumbnailBytes,
            &blocksUsed);

        m_saveInfoA[0].thumbnailData = thumbnailData;
        m_saveInfoA[0].thumbnailSize = thumbnailBytes;
        m_saveInfoA[0].blocksUsed = blocksUsed;
        m_saveDetails.iSaveC = 1;
        m_saveDetails.SaveInfoA = m_saveInfoA;
        m_saveDetails.pCurrentSaveInfo = m_saveInfoA;
        m_saveDetails.totalBlocksUsed = blocksUsed;
    }

    SAVE_DETAILS m_saveDetails = {};
    SAVE_INFO m_saveInfoA[1] = {};
};
