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
