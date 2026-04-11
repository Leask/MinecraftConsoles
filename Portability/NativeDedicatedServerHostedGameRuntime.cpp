#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameSessionState();

    bool StartNativeDedicatedServerHostedGameSession(
        const NativeDedicatedServerHostedGameRuntimeStubInitData &initData);

    int RunNativeDedicatedServerHostedGameTransientThread(
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam,
        bool *outThreadInvoked);

    int StartNativeDedicatedServerHostedGameCoreRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);

    void CompleteNativeDedicatedServerHostedGameStartupResultAndProject(
        int startupResult,
        bool threadInvoked,
        std::uint64_t nowMs = 0);

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
                CompleteNativeDedicatedServerHostedGameStartupResultAndProject(
                    startupResult,
                    false,
                    LceGetMonotonicMilliseconds());
                return startupResult;
            }
        }

        if (persistentSession)
        {
            return StartNativeDedicatedServerHostedGameCoreRuntime(
                hostedGamePlan,
                threadProc,
                threadParam);
        }

        bool threadInvoked = false;
        const int exitCode = RunNativeDedicatedServerHostedGameTransientThread(
            threadProc,
            threadParam,
            &threadInvoked);
        CompleteNativeDedicatedServerHostedGameStartupResultAndProject(
            exitCode,
            threadInvoked,
            LceGetMonotonicMilliseconds());
        return exitCode;
    }
}
