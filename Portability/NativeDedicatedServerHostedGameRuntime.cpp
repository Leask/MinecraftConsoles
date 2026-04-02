#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThread.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        struct NativeDedicatedServerHostedGameTransientThreadResult
        {
            bool threadInvoked = false;
            int exitCode = -1;
        };

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

        NativeDedicatedServerHostedGameTransientThreadResult
        RunTransientNativeDedicatedServerHostedGameThread(
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam)
        {
            NativeDedicatedServerHostedGameTransientThreadResult result = {};
            HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
                threadProc,
                threadParam);
            if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
            {
                return result;
            }

            result.threadInvoked = true;
            while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
                !ServerRuntime::IsDedicatedServerShutdownRequested())
            {
                TickDedicatedServerPlatformRuntime();
                HandleDedicatedServerPlatformActions();
            }

            WaitForSingleObject(threadHandle, INFINITE);
            DWORD threadExitCode = static_cast<DWORD>(-1);
            if (GetExitCodeThread(threadHandle, &threadExitCode))
            {
                result.exitCode = static_cast<int>(threadExitCode);
            }

            CloseHandle(threadHandle);
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

        if (persistentSession)
        {
            return StartPersistentNativeDedicatedServerHostedGameRuntime(
                hostedGamePlan,
                threadProc,
                threadParam);
        }

        const NativeDedicatedServerHostedGameTransientThreadResult result =
            RunTransientNativeDedicatedServerHostedGameThread(
                threadProc,
                threadParam);
        return CompleteDedicatedServerHostedGameRuntimeStartup(
            result.exitCode,
            result.threadInvoked);
    }
}
