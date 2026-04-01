#pragma once

#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameCoreFrameResult
    {
        NativeDedicatedServerHostedGameWorkerFrameResult workerFrame = {};
        NativeDedicatedServerHostedGameSessionFrameInput
            sessionFrameInput = {};
        NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot = {};
        std::uint64_t autosaveCompletions = 0;
        std::uint64_t frameTimestampMs = 0;
    };

    struct NativeDedicatedServerHostedGameCoreRunResult
    {
        int exitCode = -1;
        bool startupPayloadValidated = false;
        std::uint64_t startupIterations = 0;
        std::uint64_t startupDurationMs = 0;
        std::uint64_t loopIterations = 0;
        std::uint64_t autosaveCompletions = 0;
        NativeDedicatedServerHostedGameCoreFrameResult lastFrame = {};
        NativeDedicatedServerHostedGameWorkerSnapshot
            finalWorkerSnapshot = {};
        NativeDedicatedServerHostedGameSessionSnapshot
            finalSessionSnapshot = {};
    };

    struct NativeDedicatedServerHostedGameCoreHooks
    {
        void (*onThreadReady)() = nullptr;
        void (*onThreadStopped)() = nullptr;
    };

    NativeDedicatedServerHostedGameCoreFrameResult
    TickNativeDedicatedServerHostedGameCoreFrameWithResult(
        bool hostedThreadActive = true);

    NativeDedicatedServerHostedGameCoreRunResult
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks);

    int RunNativeDedicatedServerHostedGameCore(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks);
}
