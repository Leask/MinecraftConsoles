#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include <atomic>
#include <string>

#include "lce_win32/lce_win32.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionCoreState();

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
            const NativeDedicatedServerHostedGameSessionSnapshot
                sessionSnapshot =
                    GetNativeDedicatedServerHostedGameSessionSnapshot();
            RecordDedicatedServerHostedGameRuntimeThreadState(
                g_nativeHostedThreadRunning.load(),
                sessionSnapshot.sessionTicks);
            RecordDedicatedServerHostedGameRuntimeCoreState(
                sessionSnapshot.saveGeneration,
                sessionSnapshot.stateChecksum);
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
                RecordDedicatedServerHostedGameRuntimeStartupTelemetry(
                    false,
                    false,
                    0,
                    0);
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
            RecordDedicatedServerHostedGameRuntimeStartupTelemetry(
                startupPayloadPresent,
                startupPayloadValidated,
                startupIterations,
                durationMs);
            StartNativeDedicatedServerHostedGameSession(
                *initData,
                startupPayloadValidated);
            RecordNativeDedicatedServerHostedThreadSnapshot();
            if (!startupPayloadValidated)
            {
                return -2;
            }

            g_nativeHostedThreadRunning.store(true);
            RecordNativeDedicatedServerHostedThreadSnapshot();
            if (g_nativeHostedStartupReadyEvent != nullptr &&
                g_nativeHostedStartupReadyEvent != INVALID_HANDLE_VALUE)
            {
                SetEvent(g_nativeHostedStartupReadyEvent);
            }

            while (!IsDedicatedServerShutdownRequested() &&
                !IsDedicatedServerAppShutdownRequested() &&
                !IsDedicatedServerGameplayHalted())
            {
                TickNativeDedicatedServerHostedGameSession();
                RecordNativeDedicatedServerHostedThreadSnapshot();
                LceSleepMilliseconds(10);
            }

            g_nativeHostedThreadRunning.store(false);
            StopNativeDedicatedServerHostedGameSession();
            RecordNativeDedicatedServerHostedThreadSnapshot();
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
            g_nativeHostedThreadRunning.store(false);
            RecordNativeDedicatedServerHostedThreadSnapshot();
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
        g_nativeHostedThreadRunning.store(false);
        RecordNativeDedicatedServerHostedThreadSnapshot();
        return true;
    }

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam)
    {
        int startupResult = 0;
        if (!BeginDedicatedServerHostedGameRuntimeStartup(
                hostedGamePlan,
                threadProc,
                &startupResult))
        {
            return startupResult;
        }

        const bool usePersistentNativeSession =
            threadProc == &RunNativeDedicatedServerHostedGameThread;
        if (usePersistentNativeSession)
        {
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
                return CompleteDedicatedServerHostedGameRuntimeStartup(
                    -1,
                    false);
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
            return CompleteDedicatedServerHostedGameRuntimeStartup(-1, false);
        }

        if (usePersistentNativeSession)
        {
            g_nativeHostedThreadHandle = threadHandle;
            while (!IsDedicatedServerShutdownRequested())
            {
                if (WaitForSingleObject(
                        g_nativeHostedStartupReadyEvent,
                        0) == WAIT_OBJECT_0)
                {
                    RecordNativeDedicatedServerHostedThreadSnapshot();
                    return CompleteDedicatedServerHostedGameRuntimeStartup(
                        0,
                        true);
                }

                const DWORD threadWait = WaitForSingleObject(threadHandle, 10);
                if (threadWait == WAIT_OBJECT_0)
                {
                    break;
                }

                TickDedicatedServerPlatformRuntime();
                HandleDedicatedServerPlatformActions();
            }
        }
        else
        {
            while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
                !IsDedicatedServerShutdownRequested())
            {
                TickDedicatedServerPlatformRuntime();
                HandleDedicatedServerPlatformActions();
            }
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!GetExitCodeThread(threadHandle, &threadExitCode))
        {
            CloseHandle(threadHandle);
            if (usePersistentNativeSession)
            {
                g_nativeHostedThreadHandle = nullptr;
                CloseNativeDedicatedServerHostedStartupReadyEvent();
            }
            return CompleteDedicatedServerHostedGameRuntimeStartup(-1, true);
        }

        CloseHandle(threadHandle);
        if (usePersistentNativeSession)
        {
            g_nativeHostedThreadHandle = nullptr;
            CloseNativeDedicatedServerHostedStartupReadyEvent();
        }
        startupResult = static_cast<int>(threadExitCode);
        return CompleteDedicatedServerHostedGameRuntimeStartup(
            startupResult,
            true);
    }

    DedicatedServerHostedGameThreadProc
        *GetDedicatedServerHostedGameRuntimeThreadProc()
    {
        return &RunNativeDedicatedServerHostedGameThread;
    }
}
