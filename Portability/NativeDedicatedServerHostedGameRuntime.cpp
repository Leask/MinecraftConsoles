#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntimeState.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "Minecraft.Server/Common/NativeDedicatedServerSaveStub.h"

#include <string>

#include "lce_win32/lce_win32.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        constexpr std::uint64_t kNativeHostedStartupStepDelayMs = 5;
        constexpr std::uint64_t kNativeHostedStartupBaseIterations = 2;

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

        int RunNativeDedicatedServerHostedGameThread(void *threadParam)
        {
            const std::uint64_t startMs = LceGetMonotonicMilliseconds();
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
            if (!startupPayloadValidated)
            {
                return -2;
            }

            return 0;
        }
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
            return CompleteDedicatedServerHostedGameRuntimeStartup(-1, false);
        }

        while (WaitForSingleObject(threadHandle, 10) == WAIT_TIMEOUT &&
            !IsDedicatedServerShutdownRequested())
        {
            TickDedicatedServerPlatformRuntime();
            HandleDedicatedServerPlatformActions();
        }

        WaitForSingleObject(threadHandle, INFINITE);
        DWORD threadExitCode = static_cast<DWORD>(-1);
        if (!GetExitCodeThread(threadHandle, &threadExitCode))
        {
            CloseHandle(threadHandle);
            return CompleteDedicatedServerHostedGameRuntimeStartup(-1, true);
        }

        CloseHandle(threadHandle);
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
