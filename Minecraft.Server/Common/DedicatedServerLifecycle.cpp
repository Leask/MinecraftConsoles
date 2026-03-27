#include "DedicatedServerLifecycle.h"

#include "../ServerLogger.h"
#include "DedicatedServerPlatformRuntime.h"
#include "DedicatedServerSignalState.h"

namespace ServerRuntime
{
    DedicatedServerInitialSaveExecutionResult
    ExecuteDedicatedServerInitialSave(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        int actionPad)
    {
        DedicatedServerInitialSaveExecutionResult result = {};
        const DedicatedServerInitialSavePlan plan =
            BuildDedicatedServerInitialSavePlan(
                worldBootstrapPlan,
                IsDedicatedServerShutdownRequested(),
                IsDedicatedServerAppShutdownRequested());
        result.requested = plan.shouldRequestInitialSave;
        if (!plan.shouldRequestInitialSave)
        {
            return result;
        }

        LogWorldIO("requesting initial save for newly created world");
        WaitForDedicatedServerWorldActionIdle(
            actionPad,
            plan.idleWaitBeforeRequestMs);
        RequestDedicatedServerWorldAutosave(actionPad);
        if (!WaitForDedicatedServerWorldActionIdle(
                actionPad,
                plan.requestTimeoutMs))
        {
            LogWorldIO("initial save timed out");
            LogWarn(
                "world-io",
                "Timed out waiting for initial save action to finish.");
            result.timedOut = true;
            return result;
        }

        LogWorldIO("initial save completed");
        result.completed = true;
        return result;
    }

    DedicatedServerShutdownExecutionResult ExecuteDedicatedServerShutdown()
    {
        DedicatedServerShutdownExecutionResult result = {};
        result.plan = BuildDedicatedServerShutdownPlan(
            HasDedicatedServerGameplayInstance(),
            IsDedicatedServerStopSignalValid());
        if (result.plan.shouldSetSaveOnExit)
        {
            EnableDedicatedServerSaveOnExit();
        }
        if (result.plan.shouldLogShutdownSave)
        {
            LogWorldIO("requesting save before shutdown");
            LogWorldIO("using saveOnExit for shutdown");
        }

        HaltDedicatedServerGameplay();
        result.haltedGameplay = true;
        if (result.plan.shouldWaitForServerStop)
        {
            WaitForDedicatedServerStopSignal();
        }
        return result;
    }
}
