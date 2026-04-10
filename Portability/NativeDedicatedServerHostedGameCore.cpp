#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

#include "lce_time/lce_time.h"

#include <cstdint>

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameCoreHooks
    {
        void (*onThreadReady)(std::uint64_t nowMs) = nullptr;
        void (*onThreadStopped)(
            std::uint64_t hostedThreadTicks,
            std::uint64_t nowMs) = nullptr;
    };

    NativeDedicatedServerHostedGameWorkerSnapshot
    TickNativeDedicatedServerHostedGameWorkerFrame(
        std::uint64_t *outNextSleepDurationMs,
        bool *outShouldStopRunning);

    NativeDedicatedServerHostedGameSessionSnapshot
    StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
        const NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        std::uint64_t startupThreadIterations,
        std::uint64_t startupThreadDurationMs,
        std::uint64_t nowMs = 0);

    void TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
        const NativeDedicatedServerHostedGameWorkerSnapshot &workerSnapshot,
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

    NativeDedicatedServerHostedGameWorkerSnapshot
    TickNativeDedicatedServerHostedGameCoreFrameWithResult(
        std::uint64_t *outNextSleepDurationMs,
        bool *outShouldStopRunning,
        bool hostedThreadActive)
    {
        const NativeDedicatedServerHostedGameWorkerSnapshot workerSnapshot =
            TickNativeDedicatedServerHostedGameWorkerFrame(
                outNextSleepDurationMs,
                outShouldStopRunning);
        const std::uint64_t nowMs = LceGetMonotonicMilliseconds();
        TickNativeDedicatedServerHostedGameSessionWorkerFrameAndProject(
            workerSnapshot,
            hostedThreadActive,
            nowMs);
        return workerSnapshot;
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
            std::uint64_t nextSleepDurationMs = 0;
            bool shouldStopRunning = false;
            TickNativeDedicatedServerHostedGameCoreFrameWithResult(
                &nextSleepDurationMs,
                &shouldStopRunning,
                true);
            if (shouldStopRunning)
            {
                break;
            }
            if (nextSleepDurationMs > 0U)
            {
                LceSleepMilliseconds(nextSleepDurationMs);
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
