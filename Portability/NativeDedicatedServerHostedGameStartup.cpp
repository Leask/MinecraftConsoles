#include "NativeDedicatedServerHostedGameStartup.h"

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThreadBridge.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    bool PrepareNativeDedicatedServerHostedGameStartup(
        bool persistentSession,
        HANDLE *outStartupReadyEvent)
    {
        if (outStartupReadyEvent != nullptr)
        {
            *outStartupReadyEvent = nullptr;
        }

        if (!persistentSession)
        {
            return true;
        }

        ResetDedicatedServerHostedGameRuntimeSnapshot();
        ResetNativeDedicatedServerHostedGameSessionState();
        HANDLE startupReadyEvent = nullptr;
        if (!PrepareNativeDedicatedServerHostedGameHostStartup(
                &startupReadyEvent))
        {
            return false;
        }

        if (outStartupReadyEvent != nullptr)
        {
            *outStartupReadyEvent = startupReadyEvent;
        }
        return true;
    }

    int CompleteNativeDedicatedServerHostedGameStartup(
        bool persistentSession,
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs)
    {
        if (persistentSession)
        {
            ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                startupResult,
                threadInvoked,
                nowMs);
            return startupResult;
        }

        return CompleteDedicatedServerHostedGameRuntimeStartup(
            startupResult,
            threadInvoked);
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

    namespace
    {
        NativeDedicatedServerHostedGameRuntimeStartResult
        StartPersistentNativeDedicatedServerHostedGameRuntime(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam,
            const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
        {
            NativeDedicatedServerHostedGameRuntimeStartResult result = {};
            if (!PrepareNativeDedicatedServerHostedGameStartup(
                    true,
                    nullptr))
            {
                ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
                return result;
            }

            PopulateNativeDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
            const NativeDedicatedServerHostedGameHostStartResult hostResult =
                StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
                    threadProc,
                    threadParam,
                    callbacks);
            result.threadInvoked = hostResult.threadInvoked;
            result.startupReady = hostResult.startupReady;
            result.startupResult = hostResult.startupResult;
            return result;
        }
    }

    NativeDedicatedServerHostedGameRuntimeStartResult
    StartNativeDedicatedServerHostedGameRuntimePath(
        bool persistentSession,
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
    {
        if (persistentSession)
        {
            return StartPersistentNativeDedicatedServerHostedGameRuntime(
                hostedGamePlan,
                threadProc,
                threadParam,
                callbacks);
        }

        NativeDedicatedServerHostedGameRuntimeStartResult result = {};
        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            return result;
        }

        result.threadInvoked = true;
        PumpNativeDedicatedServerHostedGameThreadUntilExit(
            threadHandle,
            callbacks);
        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            CloseHandle(threadHandle);
            return result;
        }

        CloseHandle(threadHandle);
        result.startupResult = static_cast<int>(threadExitCode);
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
        const NativeDedicatedServerHostedGameThreadCallbacks callbacks =
            BuildNativeDedicatedServerHostedGameThreadCallbacks();
        const NativeDedicatedServerHostedGameRuntimeStartResult result =
            StartNativeDedicatedServerHostedGameRuntimePath(
                persistentSession,
                hostedGamePlan,
                threadProc,
                threadParam,
                callbacks);
        if (persistentSession && result.startupReady)
        {
            ProjectNativeDedicatedServerHostedGameThreadSnapshot();
        }

        return CompleteNativeDedicatedServerHostedGameStartup(
            persistentSession,
            result.startupResult,
            result.threadInvoked,
            LceGetMonotonicMilliseconds());
    }
}
