#include "Minecraft.Server/Common/DedicatedServerHostedGameRuntime.h"
#include "Minecraft.Server/Common/NativeDedicatedServerHostedGameRuntimeStub.h"
#include "NativeDedicatedServerHostedGameHost.h"
#include "NativeDedicatedServerHostedGameSession.h"

#include "lce_time/lce_time.h"

namespace ServerRuntime
{
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
