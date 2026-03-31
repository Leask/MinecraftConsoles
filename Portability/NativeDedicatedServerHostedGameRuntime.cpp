#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerAutosaveTracker.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "NativeDedicatedServerHostedGameWorker.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include <atomic>
#include <string>

#include "lce_win32/lce_win32.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionCoreState();

    void ActivateDedicatedServerHostedGamePlan(
        const DedicatedServerHostedGamePlan &)
    {
    }

    namespace
    {
        constexpr std::uint64_t kNativeHostedStartupStepDelayMs = 5;
        constexpr std::uint64_t kNativeHostedStartupBaseIterations = 2;

        HANDLE g_nativeHostedStartupReadyEvent = nullptr;
        HANDLE g_nativeHostedThreadHandle = nullptr;
        std::atomic<bool> g_nativeHostedThreadRunning(false);

        struct NativeDedicatedServerHostedGameThreadContext
        {
            DedicatedServerHostedGameThreadProc *threadProc = nullptr;
            void *threadParam = nullptr;
        };

        DWORD WINAPI RunNativeDedicatedServerHostedThreadThunk(
            LPVOID threadParameter)
        {
            NativeDedicatedServerHostedGameThreadContext *context =
                static_cast<NativeDedicatedServerHostedGameThreadContext *>(
                    threadParameter);
            if (context == nullptr || context->threadProc == nullptr)
            {
                return static_cast<DWORD>(-1);
            }

            return static_cast<DWORD>(
                context->threadProc(context->threadParam));
        }

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

        void CloseNativeDedicatedServerHostedStartupReadyEvent()
        {
            if (g_nativeHostedStartupReadyEvent != nullptr &&
                g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
            {
                CloseHandle(g_nativeHostedStartupReadyEvent);
                g_nativeHostedStartupReadyEvent = nullptr;
            }
        }

        void RecordNativeDedicatedServerHostedThreadSnapshot()
        {
            ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot();
        }

        void SyncNativeDedicatedServerHostedThreadState(bool running)
        {
            g_nativeHostedThreadRunning.store(running);
            ObserveNativeDedicatedServerHostedGameSessionThreadState(
                running,
                GetNativeDedicatedServerHostedGameSessionThreadTicks());
            RecordNativeDedicatedServerHostedThreadSnapshot();
        }

        void SignalNativeDedicatedServerHostedThreadReady()
        {
            SyncNativeDedicatedServerHostedThreadState(true);
            if (g_nativeHostedStartupReadyEvent != nullptr &&
                g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
            {
                SetEvent(g_nativeHostedStartupReadyEvent);
            }
        }

        void FinalizeNativeDedicatedServerHostedThreadStop()
        {
            SyncNativeDedicatedServerHostedThreadState(false);
        }

        void TickNativeDedicatedServerHostedRuntimeFrame()
        {
            const NativeDedicatedServerHostedGameWorkerFrameResult
                workerFrame =
                    TickNativeDedicatedServerHostedGameWorkerFrame();
            UpdateDedicatedServerAutosaveTracker(workerFrame.idle);
            TickNativeDedicatedServerHostedGameSessionFrame(
                workerFrame.snapshot,
                GetDedicatedServerAutosaveCompletionCount(),
                true);
            ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot();
        }

        bool WaitForNativeDedicatedServerHostedThreadReady(
            HANDLE threadHandle)
        {
            while (!IsDedicatedServerShutdownRequested())
            {
                if (WaitForSingleObject(
                        g_nativeHostedStartupReadyEvent,
                        0) == WAIT_OBJECT_0)
                {
                    RecordNativeDedicatedServerHostedThreadSnapshot();
                    return true;
                }

                const DWORD threadWait = WaitForSingleObject(threadHandle, 10);
                if (threadWait == WAIT_OBJECT_0)
                {
                    return false;
                }

                TickDedicatedServerPlatformRuntime();
                HandleDedicatedServerPlatformActions();
            }

            return false;
        }

        void PumpNativeDedicatedServerHostedThreadUntilExit(HANDLE threadHandle)
        {
            while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
                !IsDedicatedServerShutdownRequested())
            {
                TickDedicatedServerPlatformRuntime();
                HandleDedicatedServerPlatformActions();
            }
        }

        bool TryReadNativeDedicatedServerHostedThreadExitCode(
            HANDLE threadHandle,
            DWORD *outExitCode)
        {
            if (outExitCode == nullptr)
            {
                return false;
            }

            *outExitCode = static_cast<DWORD>(-1);
            return GetExitCodeThread(threadHandle, outExitCode);
        }

        int RunNativeDedicatedServerHostedGameThread(void *threadParam)
        {
            const std::uint64_t startMs = LceGetMonotonicMilliseconds();
            g_nativeHostedThreadRunning.store(false);
            NativeDedicatedServerHostedGameRuntimeStubInitData *initData =
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam);
            if (initData == nullptr)
            {
                ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
                    0,
                    0);
                ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
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
            ObserveNativeDedicatedServerHostedGameSessionStartupTelemetry(
                startupIterations,
                durationMs);
            ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
                LceGetMonotonicMilliseconds());
            FinalizeNativeDedicatedServerHostedThreadStop();
            if (!startupPayloadValidated)
            {
                return -2;
            }

            SignalNativeDedicatedServerHostedThreadReady();

            while (true)
            {
                const bool shutdownRequested =
                    IsDedicatedServerShutdownRequested() ||
                    IsDedicatedServerAppShutdownRequested() ||
                    IsDedicatedServerGameplayHalted();
                if (shutdownRequested &&
                    IsNativeDedicatedServerHostedGameWorkerIdle())
                {
                    break;
                }

                TickNativeDedicatedServerHostedRuntimeFrame();
                LceSleepMilliseconds(10);
            }

            StopNativeDedicatedServerHostedGameSession();
            FinalizeNativeDedicatedServerHostedThreadStop();
            return 0;
        }
    }

    void ResetNativeDedicatedServerHostedGameSessionState()
    {
        if (g_nativeHostedThreadHandle != nullptr &&
            g_nativeHostedThreadHandle != INVALID_HANDLE_VALUE)
        {
            if (WaitForSingleObject(g_nativeHostedThreadHandle, 0) ==
                WAIT_OBJECT_0)
            {
                CloseHandle(g_nativeHostedThreadHandle);
                g_nativeHostedThreadHandle = nullptr;
            }
        }

        CloseNativeDedicatedServerHostedStartupReadyEvent();
        g_nativeHostedThreadRunning.store(false);
        ResetNativeDedicatedServerHostedGameWorkerState();
        ResetNativeDedicatedServerHostedGameSessionCoreState();
        RecordNativeDedicatedServerHostedThreadSnapshot();
    }

    bool WaitForNativeDedicatedServerHostedGameSessionStop(
        DWORD timeoutMs,
        DWORD *outExitCode)
    {
        if (outExitCode != nullptr)
        {
            *outExitCode = 0;
        }

        if (g_nativeHostedThreadHandle == nullptr ||
            g_nativeHostedThreadHandle == INVALID_HANDLE_VALUE)
        {
            CloseNativeDedicatedServerHostedStartupReadyEvent();
            FinalizeNativeDedicatedServerHostedThreadStop();
            return true;
        }

        const DWORD waitResult = WaitForSingleObject(
            g_nativeHostedThreadHandle,
            timeoutMs);
        if (waitResult != WAIT_OBJECT_0)
        {
            return false;
        }

        DWORD threadExitCode = 0;
        if (!GetExitCodeThread(g_nativeHostedThreadHandle, &threadExitCode))
        {
            threadExitCode = static_cast<DWORD>(-1);
        }

        if (outExitCode != nullptr)
        {
            *outExitCode = threadExitCode;
        }

        CloseHandle(g_nativeHostedThreadHandle);
        g_nativeHostedThreadHandle = nullptr;
        CloseNativeDedicatedServerHostedStartupReadyEvent();
        FinalizeNativeDedicatedServerHostedThreadStop();
        return true;
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        const bool usePersistentNativeSession =
            threadProc == &RunNativeDedicatedServerHostedGameThread;
        if (!usePersistentNativeSession &&
            !BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        auto completeNativeHostedStartup =
            [usePersistentNativeSession](int result, bool threadInvoked)
            {
                if (usePersistentNativeSession)
                {
                    ObserveNativeDedicatedServerHostedGameSessionStartupResult(
                        result,
                        threadInvoked);
                    ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
                        LceGetMonotonicMilliseconds());
                    return result;
                }

                return CompleteDedicatedServerHostedGameRuntimeStartup(
                    result,
                    threadInvoked);
            };

        if (usePersistentNativeSession)
        {
            ResetDedicatedServerHostedGameRuntimeSnapshot();
            ResetNativeDedicatedServerHostedGameSessionState();
            g_nativeHostedStartupReadyEvent = CreateEvent(
                nullptr,
                TRUE,
                FALSE,
                nullptr);
            if (g_nativeHostedStartupReadyEvent == nullptr ||
                g_nativeHostedStartupReadyEvent == INVALID_HANDLE_VALUE)
            {
                CloseNativeDedicatedServerHostedStartupReadyEvent();
                return completeNativeHostedStartup(
                    -1,
                    false);
            }
        }

        ActivateDedicatedServerHostedGamePlan(hostedGamePlan);

        if (usePersistentNativeSession)
        {
            NativeDedicatedServerHostedGameRuntimeStubInitData *initData =
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam);
            if (initData != nullptr)
            {
                initData->localUsersMask = hostedGamePlan.localUsersMask;
                initData->onlineGame = hostedGamePlan.onlineGame;
                initData->privateGame = hostedGamePlan.privateGame;
                initData->publicSlots = hostedGamePlan.publicSlots;
                initData->privateSlots = hostedGamePlan.privateSlots;
                initData->fakeLocalPlayerJoined =
                    hostedGamePlan.fakeLocalPlayerJoined;
            }
        }

        NativeDedicatedServerHostedGameThreadContext threadContext = {};
        threadContext.threadProc = threadProc;
        threadContext.threadParam = threadParam;
        HANDLE threadHandle = CreateThread(
            nullptr,
            0,
            &RunNativeDedicatedServerHostedThreadThunk,
            &threadContext,
            0,
            nullptr);
        if (threadHandle == nullptr || threadHandle == INVALID_HANDLE_VALUE)
        {
            if (usePersistentNativeSession)
            {
                CloseNativeDedicatedServerHostedStartupReadyEvent();
            }
            return completeNativeHostedStartup(-1, false);
        }

        if (usePersistentNativeSession)
        {
            g_nativeHostedThreadHandle = threadHandle;
            if (WaitForNativeDedicatedServerHostedThreadReady(threadHandle))
            {
                ProjectNativeDedicatedServerHostedGameSessionToRuntimeSnapshot(
                    LceGetMonotonicMilliseconds());
                return completeNativeHostedStartup(
                    0,
                    true);
            }
        }
        else
        {
            PumpNativeDedicatedServerHostedThreadUntilExit(threadHandle);
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!TryReadNativeDedicatedServerHostedThreadExitCode(
                threadHandle,
                &threadExitCode))
        {
            CloseHandle(threadHandle);
            if (usePersistentNativeSession)
            {
                g_nativeHostedThreadHandle = nullptr;
                CloseNativeDedicatedServerHostedStartupReadyEvent();
            }
            return completeNativeHostedStartup(-1, true);
        }

        CloseHandle(threadHandle);
        if (usePersistentNativeSession)
        {
            g_nativeHostedThreadHandle = nullptr;
            CloseNativeDedicatedServerHostedStartupReadyEvent();
        }
        startupResult = static_cast<int>(threadExitCode);
        return completeNativeHostedStartup(
            startupResult,
            true);
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
