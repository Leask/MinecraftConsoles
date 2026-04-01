#include "NativeDedicatedServerHostedGameCore.h"

#include "Minecraft.Server/Common/DedicatedServerAutosaveTracker.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameWorker.h"

#include "lce_time/lce_time.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace ServerRuntime
{
    namespace
    {
        constexpr std::uint64_t kNativeHostedStartupStepDelayMs = 5;
        constexpr std::uint64_t kNativeHostedStartupBaseIterations = 2;

        bool ValidateNativeDedicatedServerHostedGamePayload(
            const NativeDedicatedServerHostedGameRuntimeStubInitData &initData)
        {
            if (initData.saveData == nullptr)
            {
                return true;
            }

            if (initData.saveData->data == nullptr ||
                initData.saveData->fileSize <= 0)
            {
                return false;
            }

            const char *payloadBytes =
                static_cast<const char *>(initData.saveData->data);
            const std::string payloadText(
                payloadBytes,
                payloadBytes + initData.saveData->fileSize);
            NativeDedicatedServerSaveStub saveStub = {};
            return ParseNativeDedicatedServerSaveStubText(
                payloadText,
                &saveStub);
        }

        bool ShouldStopNativeDedicatedServerHostedGameCore()
        {
            const bool shutdownRequested =
                IsDedicatedServerShutdownRequested() ||
                IsDedicatedServerAppShutdownRequested() ||
                IsDedicatedServerGameplayHalted();
            return shutdownRequested &&
                IsNativeDedicatedServerHostedGameWorkerIdle();
        }

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
        UpdateDedicatedServerAutosaveTracker(result.workerFrame.idle);
        result.autosaveCompletions = std::max(
            GetDedicatedServerAutosaveCompletionCount(),
            result.workerFrame.snapshot.processedAutosaveCommands);
        result.sessionFrameInput.workerSnapshot =
            result.workerFrame.snapshot;
        result.sessionFrameInput.autosaveCompletions =
            result.autosaveCompletions;
        result.sessionFrameInput.hostedThreadActive = hostedThreadActive;
        result.frameTimestampMs = LceGetMonotonicMilliseconds();
        TickNativeDedicatedServerHostedGameSessionFrameAndProject(
            result.sessionFrameInput,
            result.frameTimestampMs);
        result.sessionSnapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
        return result;
    }

    NativeDedicatedServerHostedGameCoreRunResult
    RunNativeDedicatedServerHostedGameCoreWithResult(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks)
    {
        NativeDedicatedServerHostedGameCoreRunResult result = {};
        const std::uint64_t startMs = LceGetMonotonicMilliseconds();
        if (initData == nullptr)
        {
            ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
                0,
                0,
                LceGetMonotonicMilliseconds());
            result.finalWorkerSnapshot =
                GetNativeDedicatedServerHostedGameWorkerSnapshot();
            result.finalSessionSnapshot =
                GetNativeDedicatedServerHostedGameSessionSnapshot();
            result.exitCode = -2;
            return result;
        }

        const bool startupPayloadPresent = initData->saveData != nullptr;
        result.startupPayloadValidated =
            ValidateNativeDedicatedServerHostedGamePayload(*initData);
        result.startupIterations =
            kNativeHostedStartupBaseIterations +
            (startupPayloadPresent ? 2ULL : 0ULL);
        for (std::uint64_t i = 0; i < result.startupIterations; ++i)
        {
            LceSleepMilliseconds(kNativeHostedStartupStepDelayMs);
        }

        result.startupDurationMs =
            LceGetMonotonicMilliseconds() - startMs;
        StartNativeDedicatedServerHostedGameSession(
            *initData,
            result.startupPayloadValidated);
        ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
            result.startupIterations,
            result.startupDurationMs,
            LceGetMonotonicMilliseconds());
        if (!result.startupPayloadValidated)
        {
            result.exitCode =
                CompleteFailedNativeDedicatedServerHostedGameCoreStartup(
                    hooks);
            result.finalWorkerSnapshot =
                GetNativeDedicatedServerHostedGameWorkerSnapshot();
            result.finalSessionSnapshot =
                GetNativeDedicatedServerHostedGameSessionSnapshot();
            return result;
        }

        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadReady);
        while (!ShouldStopNativeDedicatedServerHostedGameCore())
        {
            result.lastFrame =
                TickNativeDedicatedServerHostedGameCoreFrameWithResult();
            ++result.loopIterations;
            LceSleepMilliseconds(10);
        }

        StopNativeDedicatedServerHostedGameSession();
        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadStopped);
        result.autosaveCompletions = GetDedicatedServerAutosaveCompletionCount();
        result.finalWorkerSnapshot =
            GetNativeDedicatedServerHostedGameWorkerSnapshot();
        result.finalSessionSnapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
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
