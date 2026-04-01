#include "NativeDedicatedServerHostedGameCore.h"

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
        result.autosaveCompletions =
            result.workerFrame.autosaveCompletions;
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
        result.payloadValidated =
            ValidateNativeDedicatedServerHostedGamePayload(*initData);
        result.startupIterations =
            kNativeHostedStartupBaseIterations +
            (result.payloadPresent ? 2ULL : 0ULL);
        for (std::uint64_t i = 0; i < result.startupIterations; ++i)
        {
            LceSleepMilliseconds(kNativeHostedStartupStepDelayMs);
        }

        result.startupDurationMs =
            LceGetMonotonicMilliseconds() - startMs;
        StartNativeDedicatedServerHostedGameSession(
            *initData,
            result.payloadValidated);
        ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
            result.startupIterations,
            result.startupDurationMs,
            LceGetMonotonicMilliseconds());
        result.sessionSnapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
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
            result.exitCode =
                CompleteFailedNativeDedicatedServerHostedGameCoreStartup(
                    hooks);
            result.finalWorkerSnapshot =
                GetNativeDedicatedServerHostedGameWorkerSnapshot();
            result.finalSessionSnapshot = startup.sessionSnapshot;
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
        result.finalWorkerSnapshot =
            GetNativeDedicatedServerHostedGameWorkerSnapshot();
        result.autosaveCompletions =
            GetNativeDedicatedServerHostedGameWorkerAutosaveCompletions();
        result.finalSessionSnapshot =
            GetNativeDedicatedServerHostedGameSessionSnapshot();
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
