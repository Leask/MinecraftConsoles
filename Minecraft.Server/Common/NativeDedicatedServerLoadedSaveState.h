#pragma once

#include <string>

#include "NativeDedicatedServerSaveStub.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerLoadedSaveMetadata
    {
        bool available = false;
        bool hasSaveStub = false;
        std::string savePath;
        NativeDedicatedServerSaveStub saveStub = {};
    };

    void ResetNativeDedicatedServerLoadedSaveMetadata();

    void RecordNativeDedicatedServerLoadedSavePath(
        const std::string &savePath);

    void RecordNativeDedicatedServerLoadedSaveMetadata(
        const std::string &savePath,
        const NativeDedicatedServerSaveStub &saveStub);

    NativeDedicatedServerLoadedSaveMetadata
    GetNativeDedicatedServerLoadedSaveMetadata();
}
