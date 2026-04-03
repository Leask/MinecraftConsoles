#include "NativeDedicatedServerHostedGameCore.h"

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

#include "lce_time/lce_time.h"

#include <cstdint>

namespace ServerRuntime
{
    namespace
    {
        constexpr std::uint64_t kNativeHostedStartupStepDelayMs = 5;
        constexpr std::uint64_t kNativeHostedStartupBaseIterations = 2;

        void InvokeNativeDedicatedServerHostedGameCoreHook(
            void (*hook)())
        {
            if (hook != nullptr)
            {
                hook();
            }
        }
    }

    NativeDedicatedServerHostedGameCoreFrameResult
    TickNativeDedicatedServerHostedGameCoreFrameWithResult(
        bool hostedThreadActive)
    {
        NativeDedicatedServerHostedGameCoreFrameResult result = {};
        result.workerFrame = TickNativeDedicatedServerHostedGameWorkerFrame();
        result.frameTimestampMs = LceGetMonotonicMilliseconds();
        const NativeDedicatedServerHostedGameSessionFrameResult
            sessionFrameResult =
                TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
                    result.workerFrame,
                    hostedThreadActive,
                    result.frameTimestampMs);
        result.sessionSnapshot = sessionFrameResult.snapshot;
        return result;
    }

    NativeDedicatedServerHostedGameCoreStartupResult
    StartNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData)
    {
        NativeDedicatedServerHostedGameCoreStartupResult result = {};
        const std::uint64_t startMs = LceGetMonotonicMilliseconds();
        if (initData == nullptr)
        {
            ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
                0,
                0,
                LceGetMonotonicMilliseconds());
            result.exitCode = -2;
            result.sessionSnapshot =
                GetNativeDedicatedServerHostedGameSessionSnapshot();
            return result;
        }

        const std::uint64_t startupIterations =
            kNativeHostedStartupBaseIterations +
            (initData->saveData != nullptr ? 2ULL : 0ULL);
        for (std::uint64_t i = 0; i < startupIterations; ++i)
        {
            LceSleepMilliseconds(kNativeHostedStartupStepDelayMs);
        }

        const std::uint64_t startupDurationMs =
            LceGetMonotonicMilliseconds() - startMs;
        const NativeDedicatedServerHostedGameSessionStartupResult
            sessionStartupResult =
                StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
                    *initData,
                    startupIterations,
                    startupDurationMs,
                    LceGetMonotonicMilliseconds());
        result.sessionSnapshot =
            sessionStartupResult.sessionSnapshot;
        result.exitCode =
            result.sessionSnapshot.payloadValidated ? 0 : -2;
        return result;
    }

    NativeDedicatedServerHostedGameCoreRunResult
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks)
    {
        NativeDedicatedServerHostedGameCoreRunResult result = {};
        result.startup =
            StartNativeDedicatedServerHostedGameCoreWithResult(initData);
        if (result.startup.exitCode != 0)
        {
            result.finalState =
                CaptureNativeDedicatedServerHostedGameSessionState();
            InvokeNativeDedicatedServerHostedGameCoreHook(
                hooks.onThreadStopped);
            result.exitCode = result.startup.exitCode;
            return result;
        }

        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadReady);
        while (true)
        {
            result.lastFrame =
                TickNativeDedicatedServerHostedGameCoreFrameWithResult();
            ++result.loopIterations;
            if (result.lastFrame.workerFrame.shouldStopRunning)
            {
                break;
            }
            if (result.lastFrame.workerFrame.nextSleepDurationMs > 0U)
            {
                LceSleepMilliseconds(
                    result.lastFrame.workerFrame.nextSleepDurationMs);
            }
        }

        result.finalState =
            StopNativeDedicatedServerHostedGameSessionAndCaptureFinalState();
        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadStopped);
        result.exitCode = 0;
        return result;
    }
}
