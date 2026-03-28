#include "NativeDedicatedServerLoadedSaveState.h"

namespace
{
    ServerRuntime::NativeDedicatedServerLoadedSaveMetadata
        g_nativeDedicatedServerLoadedSaveMetadata = {};
}

namespace ServerRuntime
{
    void ResetNativeDedicatedServerLoadedSaveMetadata()
    {
        g_nativeDedicatedServerLoadedSaveMetadata =
            NativeDedicatedServerLoadedSaveMetadata{};
    }

    void RecordNativeDedicatedServerLoadedSaveMetadata(
        const std::string &savePath,
        const NativeDedicatedServerSaveStub &saveStub)
    {
        g_nativeDedicatedServerLoadedSaveMetadata.available = true;
        g_nativeDedicatedServerLoadedSaveMetadata.savePath = savePath;
        g_nativeDedicatedServerLoadedSaveMetadata.saveStub = saveStub;
    }

    NativeDedicatedServerLoadedSaveMetadata
    GetNativeDedicatedServerLoadedSaveMetadata()
    {
        return g_nativeDedicatedServerLoadedSaveMetadata;
    }
}
