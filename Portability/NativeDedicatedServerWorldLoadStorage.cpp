#include "Minecraft.Server/Common/DedicatedServerWorldLoadStorage.h"

namespace
{
    static void SetNativeDedicatedServerWorldStorageTitle(
        const std::wstring &,
        void *)
    {
    }

    static void SetNativeDedicatedServerWorldStorageSaveId(
        const std::string &,
        void *)
    {
    }

    static void ResetNativeDedicatedServerWorldStorageData(void *)
    {
    }

    static bool ClearNativeDedicatedServerWorldSaveQuery(void *)
    {
        return true;
    }

    static bool BeginNativeDedicatedServerWorldSaveQuery(int, void *)
    {
        return true;
    }

    static bool PollNativeDedicatedServerWorldSaveQuery(
        DWORD,
        ServerRuntime::DedicatedServerWorldLoadTickProc,
        void *,
        std::vector<ServerRuntime::DedicatedServerWorldLoadCandidate>
            *outCandidates)
    {
        if (outCandidates == nullptr)
        {
            return false;
        }

        outCandidates->clear();
        return true;
    }

    static bool BeginNativeDedicatedServerWorldSaveLoad(int, void *)
    {
        return false;
    }

    static bool PollNativeDedicatedServerWorldSaveLoad(
        DWORD,
        ServerRuntime::DedicatedServerWorldLoadTickProc,
        void *,
        bool *outIsCorrupt)
    {
        if (outIsCorrupt != nullptr)
        {
            *outIsCorrupt = false;
        }
        return false;
    }

    static bool ReadNativeDedicatedServerWorldSaveBytes(
        void *,
        std::vector<unsigned char> *outBytes)
    {
        if (outBytes == nullptr)
        {
            return false;
        }

        outBytes->clear();
        return false;
    }
}

namespace ServerRuntime
{
    DedicatedServerWorldLoadStorageRuntime
    CreateDedicatedServerPlatformWorldLoadStorageRuntime()
    {
        DedicatedServerWorldLoadStorageRuntime runtime = {};
        runtime.storageHooks.setWorldTitleProc =
            &SetNativeDedicatedServerWorldStorageTitle;
        runtime.storageHooks.setWorldSaveIdProc =
            &SetNativeDedicatedServerWorldStorageSaveId;
        runtime.storageHooks.resetSaveDataProc =
            &ResetNativeDedicatedServerWorldStorageData;
        runtime.loadHooks.clearSavesProc =
            &ClearNativeDedicatedServerWorldSaveQuery;
        runtime.loadHooks.beginQueryProc =
            &BeginNativeDedicatedServerWorldSaveQuery;
        runtime.loadHooks.pollQueryProc =
            &PollNativeDedicatedServerWorldSaveQuery;
        runtime.loadHooks.beginLoadProc =
            &BeginNativeDedicatedServerWorldSaveLoad;
        runtime.loadHooks.pollLoadProc =
            &PollNativeDedicatedServerWorldSaveLoad;
        runtime.loadHooks.readBytesProc =
            &ReadNativeDedicatedServerWorldSaveBytes;
        return runtime;
    }
}
