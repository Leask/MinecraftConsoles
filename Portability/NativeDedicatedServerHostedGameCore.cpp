#include "NativeDedicatedServerHostedGameCore.h"

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

#include "lce_time/lce_time.h"

#include <cstdint>

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameSessionSnapshot
    StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
        const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs = 0);

    NativeDedicatedServerHostedGameSessionFrameResult
    TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
        const NativeDedicatedServerHostedGameWorkerFrameResult &workerFrame,
        bool hostedThreadActive,
        std::uint64_t nowMs);

    NativeDedicatedServerHostedGameSessionStopResult
    CaptureNativeDedicatedServerHostedGameSessionState();

    NativeDedicatedServerHostedGameSessionStopResult
    StopNativeDedicatedServerHostedGameSessionAndCaptureFinalState(
        std::uint64_t stoppedMs = 0);

    namespace
    {
        constexpr std::uint64_t kNativeHostedStartupStepDelayMs = 5;
        constexpr std::uint64_t kNativeHostedStartupBaseIterations = 2;

        void InvokeNativeDedicatedServerHostedGameCoreReadyHook(
            void (*hook)(std::uint64_t nowMs),
            std::uint64_t nowMs)
        {
            if (hook != nullptr)
            {
                hook(nowMs);
            }
        }

        void InvokeNativeDedicatedServerHostedGameCoreStoppedHook(
            void (*hook)(
                std::uint64_t hostedThreadTicks,
                std::uint64_t nowMs),
            std::uint64_t hostedThreadTicks,
            std::uint64_t nowMs)
        {
            if (hook != nullptr)
            {
                hook(hostedThreadTicks, nowMs);
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
        std::uint64_t startupIterations = 0;
        if (initData != nullptr)
        {
            startupIterations =
                kNativeHostedStartupBaseIterations +
                (initData->saveData != nullptr ? 2ULL : 0ULL);
            for (std::uint64_t i = 0; i < startupIterations; ++i)
            {
                LceSleepMilliseconds(kNativeHostedStartupStepDelayMs);
            }
        }

        const std::uint64_t startupDurationMs =
            LceGetMonotonicMilliseconds() - startMs;
        result.sessionSnapshot =
            StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
                initData,
                startupIterations,
                startupDurationMs,
                LceGetMonotonicMilliseconds());
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
        if (result.startup.sessionSnapshot.startupResult != 0)
        {
            result.finalState =
                CaptureNativeDedicatedServerHostedGameSessionState();
            InvokeNativeDedicatedServerHostedGameCoreStoppedHook(
                hooks.onThreadStopped,
                result.finalState.sessionSnapshot.hostedThreadTicks,
                LceGetMonotonicMilliseconds());
            return result;
        }

        InvokeNativeDedicatedServerHostedGameCoreReadyHook(
            hooks.onThreadReady,
            LceGetMonotonicMilliseconds());
        while (true)
        {
            result.lastFrame =
                TickNativeDedicatedServerHostedGameCoreFrameWithResult();
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
        InvokeNativeDedicatedServerHostedGameCoreStoppedHook(
            hooks.onThreadStopped,
            result.finalState.sessionSnapshot.hostedThreadTicks,
            LceGetMonotonicMilliseconds());
        return result;
    }
}
