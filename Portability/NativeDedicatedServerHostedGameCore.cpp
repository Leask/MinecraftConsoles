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

        void TickNativeDedicatedServerHostedGameCoreFrame()
        {
            const NativeDedicatedServerHostedGameWorkerFrameResult
                workerFrame = TickNativeDedicatedServerHostedGameWorkerFrame();
            UpdateDedicatedServerAutosaveTracker(workerFrame.idle);
            const std::uint64_t autosaveCompletions = std::max(
                GetDedicatedServerAutosaveCompletionCount(),
                workerFrame.snapshot.processedAutosaveCommands);
            TickNativeDedicatedServerHostedGameSessionFrameAndProject(
                workerFrame.snapshot,
                autosaveCompletions,
                true,
                LceGetMonotonicMilliseconds());
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

    int RunNativeDedicatedServerHostedGameCore(
        NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
        const NativeDedicatedServerHostedGameCoreHooks &hooks)
    {
        const std::uint64_t startMs = LceGetMonotonicMilliseconds();
        if (initData == nullptr)
        {
            ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
                0,
                0,
                LceGetMonotonicMilliseconds());
            return -2;
        }

        const bool startupPayloadPresent = initData->saveData != nullptr;
        const bool startupPayloadValidated =
            ValidateNativeDedicatedServerHostedGamePayload(*initData);
        const std::uint64_t startupIterations =
            kNativeHostedStartupBaseIterations +
            (startupPayloadPresent ? 2ULL : 0ULL);
        for (std::uint64_t i = 0; i < startupIterations; ++i)
        {
            LceSleepMilliseconds(kNativeHostedStartupStepDelayMs);
        }

        const std::uint64_t durationMs =
            LceGetMonotonicMilliseconds() - startMs;
        StartNativeDedicatedServerHostedGameSession(
            *initData,
            startupPayloadValidated);
        ObserveNativeDedicatedServerHostedGameSessionStartupTelemetryAndProject(
            startupIterations,
            durationMs,
            LceGetMonotonicMilliseconds());
        if (!startupPayloadValidated)
        {
            return CompleteFailedNativeDedicatedServerHostedGameCoreStartup(
                hooks);
        }

        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadReady);
        while (!ShouldStopNativeDedicatedServerHostedGameCore())
        {
            TickNativeDedicatedServerHostedGameCoreFrame();
            LceSleepMilliseconds(10);
        }

        StopNativeDedicatedServerHostedGameSession();
        InvokeNativeDedicatedServerHostedGameCoreHook(hooks.onThreadStopped);
        return 0;
    }
}
