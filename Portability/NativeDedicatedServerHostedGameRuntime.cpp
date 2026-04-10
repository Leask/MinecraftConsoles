#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionState();

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData);

    int StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked);

    int RunNativeDedicatedServerHostedGameTransientThread(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked);

    void ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs = 0);

    namespace
    {
        int StartPersistentNativeDedicatedServerHostedGameRuntime(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam)
        {
            bool threadInvoked = false;

            ResetNativeDedicatedServerHostedGameSessionState();
            PopulateDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
            const int startupResult =
                StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
                threadProc,
                threadParam,
                &threadInvoked);

            ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                startupResult,
                threadInvoked,
                LceGetMonotonicMilliseconds());
            return startupResult;
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
        if (!persistentSession)
        {
            NativeDedicatedServerHostedGameRuntimeStubInitData
                sessionInitData = {};
            ResetNativeDedicatedServerHostedGameSessionState();
            PopulateDedicatedServerNetworkGameInitData(
                &sessionInitData,
                hostedGamePlan.networkInitPlan);
            PopulateDedicatedServerHostedGameRuntimeStubInitData(
                &sessionInitData,
                hostedGamePlan);
            (void)StartNativeDedicatedServerHostedGameSession(
                sessionInitData);
            if (threadProc == nullptr)
            {
                startupResult = -1;
                ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
                    startupResult,
                    false,
                    LceGetMonotonicMilliseconds());
                return startupResult;
            }
        }

        if (persistentSession)
        {
            return StartPersistentNativeDedicatedServerHostedGameRuntime(
                hostedGamePlan,
                threadProc,
                threadParam);
        }

        bool threadInvoked = false;
        const int exitCode = RunNativeDedicatedServerHostedGameTransientThread(
            threadProc,
            threadParam,
            &threadInvoked);
        ObserveNativeDedicatedServerHostedGameSessionStartupResultAndProject(
            exitCode,
            threadInvoked,
            LceGetMonotonicMilliseconds());
        return exitCode;
    }
}
