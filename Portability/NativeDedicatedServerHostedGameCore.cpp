#include "NativeDedicatedServerHostedGameCore.h"

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

#include "lce_time/lce_time.h"

#include <cstdint>

namespace ServerRuntime
{
    NativeDedicatedServerHostedGameWorkerFrameResult
    TickNativeDedicatedServerHostedGameWorkerFrame();

    NativeDedicatedServerHostedGameSessionSnapshot
    StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
        const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs = 0);

    void
    TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
        const NativeDedicatedServerHostedGameWorkerFrameResult &workerFrame,
        bool hostedThreadActive,
        std::uint64_t nowMs);

    NativeDedicatedServerHostedGameSessionSnapshot
    CaptureNativeDedicatedServerHostedGameSessionState();

    NativeDedicatedServerHostedGameSessionSnapshot
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

    NativeDedicatedServerHostedGameWorkerFrameResult
    TickNativeDedicatedServerHostedGameCoreFrameWithResult(
        bool hostedThreadActive)
    {
        const NativeDedicatedServerHostedGameWorkerFrameResult workerFrame =
            TickNativeDedicatedServerHostedGameWorkerFrame();
        const std::uint64_t nowMs = LceGetMonotonicMilliseconds();
        TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
            workerFrame,
            hostedThreadActive,
            nowMs);
        return workerFrame;
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    StartNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData)
    {
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
        return StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
            initData,
            startupIterations,
            startupDurationMs,
            LceGetMonotonicMilliseconds());
    }

    NativeDedicatedServerHostedGameSessionSnapshot
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks)
    {
        const NativeDedicatedServerHostedGameSessionSnapshot startupSnapshot =
            StartNativeDedicatedServerHostedGameCoreWithResult(initData);
        if (startupSnapshot.startupResult != 0)
        {
            const NativeDedicatedServerHostedGameSessionSnapshot finalState =
                CaptureNativeDedicatedServerHostedGameSessionState();
            InvokeNativeDedicatedServerHostedGameCoreStoppedHook(
                hooks.onThreadStopped,
                finalState.hostedThreadTicks,
                LceGetMonotonicMilliseconds());
            return finalState;
        }

        InvokeNativeDedicatedServerHostedGameCoreReadyHook(
            hooks.onThreadReady,
            LceGetMonotonicMilliseconds());
        while (true)
        {
            const NativeDedicatedServerHostedGameWorkerFrameResult lastFrame =
                TickNativeDedicatedServerHostedGameCoreFrameWithResult(true);
            if (lastFrame.shouldStopRunning)
            {
                break;
            }
            if (lastFrame.nextSleepDurationMs > 0U)
            {
                LceSleepMilliseconds(
                    lastFrame.nextSleepDurationMs);
            }
        }

        const NativeDedicatedServerHostedGameSessionSnapshot finalState =
            StopNativeDedicatedServerHostedGameSessionAndCaptureFinalState();
        InvokeNativeDedicatedServerHostedGameCoreStoppedHook(
            hooks.onThreadStopped,
            finalState.hostedThreadTicks,
            LceGetMonotonicMilliseconds());
        return finalState;
    }
}
