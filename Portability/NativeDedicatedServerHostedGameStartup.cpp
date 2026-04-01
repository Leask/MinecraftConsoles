#include "NativeDedicatedServerHostedGameStartup.h"

#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"

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

        bool PrepareNativeDedicatedServerHostedGameStartup()
        {
            ResetNativeDedicatedServerHostedGameSessionState();
            return true;
        }

        int CompleteNativeDedicatedServerHostedGameStartup(
            bool persistentSession,
            const NativeDedicatedServerHostedGameRuntimeStartResult
                &startResult,
            std::uint64_t nowMs)
        {
            if (persistentSession)
            {
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

            return CompleteDedicatedServerHostedGameRuntimeStartup(
                startResult.startupResult,
                startResult.threadInvoked);
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
            NativeDedicatedServerHostedGameRuntimeStartResult result = {};
            if (!PrepareNativeDedicatedServerHostedGameStartup())
            {
                return result;
            }

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

    int StartNativeDedicatedServerHostedGameRuntimeAndComplete(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        const bool persistentSession =
            threadProc == GetNativeDedicatedServerHostedGameThreadProc();
        if (!persistentSession &&
            !BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        ActivateDedicatedServerHostedGamePlan(hostedGamePlan);
        const NativeDedicatedServerHostedGameRuntimeStartResult result =
            StartNativeDedicatedServerHostedGameRuntimePath(
                hostedGamePlan,
                threadProc,
                threadParam);

        return CompleteNativeDedicatedServerHostedGameStartup(
            persistentSession,
            result,
            LceGetMonotonicMilliseconds());
    }
}
