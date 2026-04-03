#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"
#include "NativeDedicatedServerHostedGameThread.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    namespace
    {
        int StartPersistentNativeDedicatedServerHostedGameRuntime(
            const DedicatedServerHostedGamePlan &hostedGamePlan,
            DedicatedServerHostedGameThreadProc *threadProc,
            void *threadParam)
        {
            int startupResult = -1;
            bool threadInvoked = false;
            NativeDedicatedServerHostedGameSessionSnapshot sessionSnapshot = {};

            ResetNativeDedicatedServerHostedGameSessionState();
            PopulateDedicatedServerHostedGameRuntimeStubInitData(
                static_cast<
                    NativeDedicatedServerHostedGameRuntimeStubInitData *>(
                        threadParam),
                hostedGamePlan);
            StartNativeDedicatedServerHostedGameHostThreadAndWaitReady(
                threadProc,
                threadParam,
                &startupResult,
                &threadInvoked,
                &sessionSnapshot);
            return FinalizeNativeDedicatedServerHostedGameSessionStartupAndProject(
                startupResult,
                threadInvoked,
                startupResult == 0
                    ? &sessionSnapshot
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

        bool threadInvoked = false;
        const int exitCode = RunNativeDedicatedServerHostedGameTransientThread(
            threadProc,
            threadParam,
            &threadInvoked);
        return CompleteDedicatedServerHostedGameRuntimeStartup(
            exitCode,
            threadInvoked);
    }
}
