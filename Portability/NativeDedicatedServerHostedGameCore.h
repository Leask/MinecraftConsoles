#pragma once

#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameCoreStartupResult
    {
        int exitCode = -1;
        NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot = {};
    };

    struct NativeDedicatedServerHostedGameCoreFrameResult
    {
        NativeDedicatedServerHostedGameWorkerFrameResult workerFrame = {};
        NativeDedicatedServerHostedGameSessionFrameInput
            sessionFrameInput = {};
        NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot = {};
        std::uint64_t frameTimestampMs = 0;
    };

    struct NativeDedicatedServerHostedGameCoreRunResult
    {
        int exitCode = -1;
        NativeDedicatedServerHostedGameCoreStartupResult startup = {};
        std::uint64_t loopIterations = 0;
        NativeDedicatedServerHostedGameCoreFrameResult lastFrame = {};
        NativeDedicatedServerHostedGameSessionStopResult finalState = {};
    };

    struct NativeDedicatedServerHostedGameCoreHooks
    {
        void (*onThreadReady)() = nullptr;
        void (*onThreadStopped)() = nullptr;
    };

    NativeDedicatedServerHostedGameCoreStartupResult
    StartNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData);

    NativeDedicatedServerHostedGameCoreFrameResult
    TickNativeDedicatedServerHostedGameCoreFrameWithResult(
        bool hostedThreadActive = true);

    NativeDedicatedServerHostedGameCoreRunResult
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks);
}
