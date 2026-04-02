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

        int StartPersistentNativeDedicatedServerHostedGameRuntime(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam)
        {
            ResetNativeDedicatedServerHostedGameSessionState();
            PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
            const NativeDedicatedServerHostedGameHostStartResult result =
                StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
                    threadProc,
                    threadParam);
            return FinalizeNativeDedicatedServerHostedGameSessionStartupAndProject(
                result.startupResult,
                result.threadInvoked,
                result.sessionSnapshotAvailable
                    ? &result.sessionSnapshot
                    : nullptr,
                LceGetMonotonicMilliseconds());
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

        if (persistentSession)
        {
            return StartPersistentNativeDedicatedServerHostedGameRuntime(
                hostedGamePlan,
                threadProc,
                threadParam);
        }

        const NativeDedicatedServerHostedGameThreadRunResult result =
            RunNativeDedicatedServerHostedGameThreadAndReadExitCode(
                threadProc,
                threadParam);
        return CompleteDedicatedServerHostedGameRuntimeStartup(
            result.exitCode,
            result.threadInvoked);
    }
}
