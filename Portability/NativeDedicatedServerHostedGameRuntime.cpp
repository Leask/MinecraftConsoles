#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThread.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        struct NativeDedicatedServerHostedGameRuntimePathResult
        {
            bool threadInvoked = false;
            bool sessionSnapshotAvailable = false;
            int startupResult = -1;
            NativeDedicatedServerHostedGameSessionSnapshot
                sessionSnapshot = {};
        };

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

        NativeDedicatedServerHostedGameRuntimePathResult
        StartPersistentNativeDedicatedServerHostedGameRuntime(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam,
            const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
        {
            NativeDedicatedServerHostedGameRuntimePathResult result = {};
            ResetNativeDedicatedServerHostedGameSessionState();
            PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
            result.startupResult =
                StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
                    threadProc,
                    threadParam,
                    callbacks,
                    &result.threadInvoked,
                    &result.sessionSnapshot,
                    &result.sessionSnapshotAvailable);
            return result;
        }

        NativeDedicatedServerHostedGameRuntimePathResult
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

            NativeDedicatedServerHostedGameRuntimePathResult result = {};
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

        const NativeDedicatedServerHostedGameRuntimePathResult result =
            StartNativeDedicatedServerHostedGameRuntimePath(
                hostedGamePlan,
                threadProc,
                threadParam);

        if (persistentSession)
        {
            return FinalizeNativeDedicatedServerHostedGameSessionStartupAndProject(
                result.startupResult,
                result.threadInvoked,
                result.sessionSnapshotAvailable
                    ? &result.sessionSnapshot
                    : nullptr,
                LceGetMonotonicMilliseconds());
        }

        return CompleteDedicatedServerHostedGameRuntimeStartup(
            result.startupResult,
            result.threadInvoked);
    }
}
