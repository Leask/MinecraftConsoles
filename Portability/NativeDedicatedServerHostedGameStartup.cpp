#include "NativeDedicatedServerHostedGameStartup.h"

#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

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

    NativeDedicatedServerHostedGamePersistentStartResult
    StartPersistentNativeDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        const NativeDedicatedServerHostedGameThreadCallbacks &callbacks)
    {
        NativeDedicatedServerHostedGamePersistentStartResult result = {};
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
        HANDLE threadHandle = StartNativeDedicatedServerHostedGameThread(
            threadProc,
            threadParam);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
            return result;
        }

        SetNativeDedicatedServerHostedGameHostThreadHandle(threadHandle);
        result.threadInvoked = true;
        if (WaitForNativeDedicatedServerHostedGameThreadReady(
                GetNativeDedicatedServerHostedGameHostStartupReadyEvent(),
                threadHandle,
                callbacks))
        {
            result.ready = true;
            result.startupResult = 0;
            return result;
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedGameThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            threadExitCode = static_cast<DWORD>(-1);
        }

        CloseHandle(threadHandle);
        ReleaseNativeDedicatedServerHostedGameHostThreadHandle(false);
        ReleaseNativeDedicatedServerHostedGameHostStartupReadyEvent();
        result.startupResult = static_cast<int>(threadExitCode);
        return result;
    }
}
