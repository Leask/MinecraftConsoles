#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ServerRuntime
{
    struct LoadSaveDataThreadParam
    {
        void *data = nullptr;
        std::int64_t fileSize = 0;
        std::wstring saveName;

        LoadSaveDataThreadParam() = default;

        LoadSaveDataThreadParam(
            void *dataValue,
            std::int64_t fileSizeValue,
            const std::wstring &saveNameValue)
            : data(dataValue)
            , fileSize(fileSizeValue)
            , saveName(saveNameValue)
        {
        }
    };

    enum EWorldBootstrapStatus
    {
        eWorldBootstrap_Loaded,
        eWorldBootstrap_CreatedNew,
        eWorldBootstrap_Failed
    };

    struct WorldBootstrapResult
    {
        EWorldBootstrapStatus status = eWorldBootstrap_Failed;
        LoadSaveDataThreadParam *saveData = nullptr;
        std::string resolvedSaveId;
    };

    inline LoadSaveDataThreadParam *CreateOwnedLoadSaveDataThreadParam(
        const std::vector<unsigned char> &bytes,
        const std::wstring &saveName)
    {
        if (bytes.empty())
        {
            return nullptr;
        }

        unsigned char *ownedBytes = new unsigned char[bytes.size()];
        std::copy(bytes.begin(), bytes.end(), ownedBytes);
        return new LoadSaveDataThreadParam(
            ownedBytes,
            static_cast<std::int64_t>(bytes.size()),
            saveName);
    }

    inline void DestroyLoadSaveDataThreadParam(
        LoadSaveDataThreadParam *param)
    {
        if (param == nullptr)
        {
            return;
        }

        delete[] static_cast<unsigned char *>(param->data);
        delete param;
    }
}

using LoadSaveDataThreadParam = ServerRuntime::LoadSaveDataThreadParam;
