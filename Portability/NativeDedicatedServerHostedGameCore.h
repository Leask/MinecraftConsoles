#pragma once

#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameCoreFrameResult
    {
        NativeDedicatedServerHostedGameWorkerFrameResult workerFrame = {};
        NativeDedicatedServerHostedGameSessionSnapshot
            sessionSnapshot = {};
        std::uint64_t frameTimestampMs = 0;
    };

    struct NativeDedicatedServerHostedGameCoreRunResult
    {
        NativeDedicatedServerHostedGameSessionSnapshot startup = {};
        NativeDedicatedServerHostedGameCoreFrameResult lastFrame = {};
        NativeDedicatedServerHostedGameSessionSnapshot finalState = {};
    };

    struct NativeDedicatedServerHostedGameCoreHooks
    {
        void (*onThreadReady)(std::uint64_t nowMs) = nullptr;
        void (*onThreadStopped)(
            std::uint64_t hostedThreadTicks,
            std::uint64_t nowMs) = nullptr;
    };

    NativeDedicatedServerHostedGameSessionSnapshot
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
