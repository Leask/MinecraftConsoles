#pragma once

#include <string>

#include "NativeDedicatedServerSaveStub.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerLoadedSaveMetadata
    {
        bool available = false;
        std::string savePath;
        NativeDedicatedServerSaveStub saveStub = {};
    };

    void ResetNativeDedicatedServerLoadedSaveMetadata();

    void RecordNativeDedicatedServerLoadedSaveMetadata(
        const std::string &savePath,
        const NativeDedicatedServerSaveStub &saveStub);

    NativeDedicatedServerLoadedSaveMetadata
    GetNativeDedicatedServerLoadedSaveMetadata();
}
