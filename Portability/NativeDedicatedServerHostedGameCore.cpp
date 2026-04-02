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

        int CompleteFailedNativeDedicatedServerHostedGameCoreStartup(
            const NativeDedicatedServerHostedGameCoreHooks &hooks)
        {
            InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadStopped);
            return -2;
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
        result.autosaveCompletions =
            result.workerFrame.autosaveCompletions;
        result.sessionFrameInput = sessionFrameResult.input;
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

        result.payloadPresent = initData->saveData != nullptr;
        result.startupIterations =
            kNativeHostedStartupBaseIterations +
            (result.payloadPresent ? 2ULL : 0ULL);
        for (std::uint64_t i = 0; i < result.startupIterations; ++i)
        {
            LceSleepMilliseconds(kNativeHostedStartupStepDelayMs);
        }

        result.startupDurationMs =
            LceGetMonotonicMilliseconds() - startMs;
        const NativeDedicatedServerHostedGameSessionStartupResult
            sessionStartupResult =
                StartNativeDedicatedServerHostedGameSessionAndProjectStartupWithResult(
                    *initData,
                    result.startupIterations,
                    result.startupDurationMs,
                    LceGetMonotonicMilliseconds());
        result.payloadValidated =
            sessionStartupResult.payloadValidated;
        result.sessionSnapshot =
            sessionStartupResult.sessionSnapshot;
        result.exitCode = result.payloadValidated ? 0 : -2;
        return result;
    }

    NativeDedicatedServerHostedGameCoreRunResult
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks)
    {
        NativeDedicatedServerHostedGameCoreRunResult result = {};
        const NativeDedicatedServerHostedGameCoreStartupResult startup =
            StartNativeDedicatedServerHostedGameCoreWithResult(initData);
        result.startupPayloadValidated = startup.payloadValidated;
        result.startupIterations = startup.startupIterations;
        result.startupDurationMs = startup.startupDurationMs;
        if (startup.exitCode != 0)
        {
            const NativeDedicatedServerHostedGameSessionStopResult
                failedStartupState =
                    CaptureNativeDedicatedServerHostedGameSessionState();
            result.exitCode =
                CompleteFailedNativeDedicatedServerHostedGameCoreStartup(
                    hooks);
            result.finalWorkerSnapshot =
                failedStartupState.workerSnapshot;
            result.autosaveCompletions =
                failedStartupState.autosaveCompletions;
            result.finalSessionSnapshot =
                failedStartupState.sessionSnapshot;
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

        const NativeDedicatedServerHostedGameSessionStopResult stopResult =
            StopNativeDedicatedServerHostedGameSessionAndCaptureFinalState();
        result.finalWorkerSnapshot = stopResult.workerSnapshot;
        result.autosaveCompletions = stopResult.autosaveCompletions;
        result.finalSessionSnapshot = stopResult.sessionSnapshot;
        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadStopped);
        result.exitCode = 0;
        return result;
    }

    int RunNativeDedicatedServerHostedGameCore(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks)
    {
        return RunNativeDedicatedServerHostedGameCoreWithResult(
            initData,
            hooks).exitCode;
    }
}
