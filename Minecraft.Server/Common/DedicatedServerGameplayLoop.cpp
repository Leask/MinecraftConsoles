#include "DedicatedServerGameplayLoop.h"

#include "../ServerLogger.h"
#include "DedicatedServerPlatformRuntime.h"
#include "DedicatedServerSignalState.h"
#include "lce_time/lce_time.h"

namespace ServerRuntime
{
    DedicatedServerGameplayLoopIterationResult
    TickDedicatedServerGameplayLoop(
        DedicatedServerAutosaveState *autosaveState,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext)
    {
        DedicatedServerGameplayLoopIterationResult result = {};
        if (autosaveState == nullptr)
        {
            result.shouldExit = true;
            return result;
        }

        TickDedicatedServerPlatformRuntime();
        HandleDedicatedServerPlatformActions();
        if (pollProc != nullptr)
        {
            pollProc(pollContext);
        }

        if (IsDedicatedServerShutdownRequested() ||
            IsDedicatedServerAppShutdownRequested())
        {
            result.shouldExit = true;
            return result;
        }

        const bool autosaveActionIdle =
            IsDedicatedServerWorldActionIdle(actionPad);
        const std::uint64_t now = LceGetMonotonicMilliseconds();
        const DedicatedServerAutosaveLoopPlan autosaveLoopPlan =
            BuildDedicatedServerAutosaveLoopPlan(
                *autosaveState,
                autosaveActionIdle,
                now);

        if (autosaveLoopPlan.shouldMarkCompleted)
        {
            LogWorldIO("autosave completed");
            MarkDedicatedServerAutosaveCompleted(autosaveState);
            result.autosaveCompleted = true;
        }

        if (IsDedicatedServerGameplayHalted())
        {
            result.shouldExit = true;
            return result;
        }

        if (autosaveLoopPlan.shouldRequestAutosave)
        {
            LogWorldIO("requesting autosave");
            RequestDedicatedServerWorldAutosave(actionPad);
            MarkDedicatedServerAutosaveRequested(
                autosaveState,
                now,
                serverProperties);
            result.autosaveRequested = true;
        }
        else if (autosaveLoopPlan.shouldAdvanceDeadline)
        {
            autosaveState->nextTickMs =
                ComputeNextDedicatedServerAutosaveDeadlineMs(
                    now,
                    serverProperties);
            result.advancedDeadline = true;
        }

        return result;
    }
}
