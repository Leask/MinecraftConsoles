#pragma once

#include <string>
#include <vector>

#include <lce_abi/lce_abi.h>

#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    typedef void (*DedicatedServerWorldLoadTickProc)();

    struct DedicatedServerWorldLoadCandidate
    {
        std::wstring title;
        std::wstring filename;
    };

    struct DedicatedServerWorldLoadPayload
    {
        std::wstring matchedTitle;
        std::wstring matchedFilename;
        std::string resolvedSaveId;
        std::vector<unsigned char> bytes;
    };

    typedef bool (*DedicatedServerClearWorldSavesProc)(void *context);
    typedef bool (*DedicatedServerBeginWorldSaveQueryProc)(
        int actionPad,
        void *context);
    typedef bool (*DedicatedServerPollWorldSaveQueryProc)(
        DWORD timeoutMs,
        DedicatedServerWorldLoadTickProc tickProc,
        void *context,
        std::vector<DedicatedServerWorldLoadCandidate> *outCandidates);
    typedef bool (*DedicatedServerBeginWorldSaveLoadProc)(
        int matchedIndex,
        void *context);
    typedef bool (*DedicatedServerPollWorldSaveLoadProc)(
        DWORD timeoutMs,
        DedicatedServerWorldLoadTickProc tickProc,
        void *context,
        bool *outIsCorrupt);
    typedef bool (*DedicatedServerReadWorldSaveBytesProc)(
        void *context,
        std::vector<unsigned char> *outBytes);

    struct DedicatedServerWorldLoadHooks
    {
        DedicatedServerClearWorldSavesProc clearSavesProc = nullptr;
        DedicatedServerBeginWorldSaveQueryProc beginQueryProc = nullptr;
        DedicatedServerPollWorldSaveQueryProc pollQueryProc = nullptr;
        DedicatedServerBeginWorldSaveLoadProc beginLoadProc = nullptr;
        DedicatedServerPollWorldSaveLoadProc pollLoadProc = nullptr;
        DedicatedServerReadWorldSaveBytesProc readBytesProc = nullptr;
        void *context = nullptr;
    };

    EDedicatedServerWorldLoadStatus LoadDedicatedServerWorldData(
        const DedicatedServerWorldTarget &worldTarget,
        int actionPad,
        DedicatedServerWorldLoadTickProc tickProc,
        const DedicatedServerWorldLoadHooks &hooks,
        DedicatedServerWorldLoadPayload *outPayload);
}
