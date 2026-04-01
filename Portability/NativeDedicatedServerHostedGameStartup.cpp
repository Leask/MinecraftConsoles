#include "NativeDedicatedServerHostedGameStartup.h"

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"

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

        int CompletePersistentNativeDedicatedServerHostedGameStartup(
            const NativeDedicatedServerHostedGameRuntimeStartResult
                &startResult)
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

        NativeDedicatedServerHostedGameRuntimeStartResult
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
    }

    NativeDedicatedServerHostedGameRuntimeStartResult
    StartNativeDedicatedServerHostedGameRuntimePath(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        const bool persistentSession =
            threadProc == GetNativeDedicatedServerHostedGameThreadProc();
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

        NativeDedicatedServerHostedGameRuntimeStartResult result = {};
        const NativeDedicatedServerHostedGameThreadRunResult threadRunResult =
            RunNativeDedicatedServerHostedGameThreadAndReadExitCode(
                threadProc,
                threadParam,
                callbacks);
        result.threadInvoked = threadRunResult.threadInvoked;
        result.startupResult = threadRunResult.exitCode;
        return result;
    }
}
