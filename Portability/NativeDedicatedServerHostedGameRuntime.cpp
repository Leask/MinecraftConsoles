#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        NativeDedicatedServerHostedGameThreadCallbacks
        BuildNativeDedicatedServerHostedGameThreadCallbacks()
        {
            NativeDedicatedServerHostedGameThreadCallbacks callbacks = {};
            callbacks.tickPlatformRuntime =
                &TickDedicatedServerPlatformRuntime;
            callbacks.handlePlatformActions =
                &HandleDedicatedServerPlatformActions;
            return callbacks;
        }

        void PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
            NativeDedicatedServerHostedGameRuntimeStubInitData *initData,
            const DedicatedServerHostedGamePlan &hostedGamePlan)
        {
            if (initData == nullptr)
            {
                return;
            }

            initData->localUsersMask = hostedGamePlan.localUsersMask;
            initData->onlineGame = hostedGamePlan.onlineGame;
            initData->privateGame = hostedGamePlan.privateGame;
            initData->publicSlots = hostedGamePlan.publicSlots;
            initData->privateSlots = hostedGamePlan.privateSlots;
            initData->fakeLocalPlayerJoined =
                hostedGamePlan.fakeLocalPlayerJoined;
        }

        NativeDedicatedServerHostedGameHostStartResult
        StartPersistentNativeDedicatedServerHostedGameRuntime(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam,
            const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
        {
            ResetNativeDedicatedServerHostedGameSessionState();
            PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
            return StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
                threadProc,
                threadParam,
                callbacks);
        }

        NativeDedicatedServerHostedGameHostStartResult
        StartNativeDedicatedServerHostedGameRuntimePath(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam)
        {
            const bool persistentSession =
                threadProc == GetDedicatedServerHostedGameRuntimeThreadProc();
            const NativeDedicatedServerHostedGameThreadCallbacks callbacks =
                BuildNativeDedicatedServerHostedGameThreadCallbacks();
            if (persistentSession)
            {
                return StartPersistentNativeDedicatedServerHostedGameRuntime(
                    hostedGamePlan,
                    threadProc,
                    threadParam,
                    callbacks);
            }

            NativeDedicatedServerHostedGameHostStartResult result = {};
            const NativeDedicatedServerHostedGameThreadRunResult
                threadRunResult =
                    RunNativeDedicatedServerHostedGameThreadAndReadExitCode(
                        threadProc,
                        threadParam,
                        callbacks);
            result.threadInvoked = threadRunResult.threadInvoked;
            result.startupResult = threadRunResult.exitCode;
            return result;
        }

        int CompletePersistentNativeDedicatedServerHostedGameRuntimeStartup(
            const NativeDedicatedServerHostedGameHostStartResult &startResult)
        {
            const std::uint64_t nowMs = LceGetMonotonicMilliseconds();
            if (startResult.sessionSnapshotAvailable)
            {
                ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                    startResult.sessionSnapshot,
                    startResult.startupResult,
                    startResult.threadInvoked,
                    nowMs);
            }
            else
            {
                ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                    startResult.startupResult,
                    startResult.threadInvoked,
                    nowMs);
            }
            return startResult.startupResult;
        }
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        const bool persistentSession =
            threadProc == GetDedicatedServerHostedGameRuntimeThreadProc();
        if (!persistentSession &&
            !BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        const NativeDedicatedServerHostedGameHostStartResult result =
            StartNativeDedicatedServerHostedGameRuntimePath(
                hostedGamePlan,
                threadProc,
                threadParam);

        if (persistentSession)
        {
            return CompletePersistentNativeDedicatedServerHostedGameRuntimeStartup(
                result);
        }

        return CompleteDedicatedServerHostedGameRuntimeStartup(
            result.startupResult,
            result.threadInvoked);
    }
}
