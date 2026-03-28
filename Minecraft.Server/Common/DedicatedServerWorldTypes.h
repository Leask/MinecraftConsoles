#pragma once

#include <cstdint>
#include <string>

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
}

using LoadSaveDataThreadParam = ServerRuntime::LoadSaveDataThreadParam;
