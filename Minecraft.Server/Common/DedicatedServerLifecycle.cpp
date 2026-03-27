#include "DedicatedServerLifecycle.h"

#include "../ServerLogger.h"
#include "DedicatedServerPlatformRuntime.h"
#include "DedicatedServerSignalState.h"
#include "StringUtils.h"

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

    DedicatedServerWorldLoadExecutionResult ExecuteDedicatedServerWorldLoadPlan(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        const DedicatedServerWorldLoadPlan &worldLoadPlan,
        ServerPropertiesConfig *serverProperties)
    {
        DedicatedServerWorldLoadExecutionResult result = {};
        if (worldLoadPlan.shouldPersistResolvedSaveId &&
            serverProperties != nullptr)
        {
            LogWorldIO("updating level-id to loaded save filename");
            serverProperties->worldSaveId = worldLoadPlan.resolvedSaveId;
            result.updatedSaveId = true;
            result.savedResolvedSaveId =
                SaveServerPropertiesConfig(*serverProperties);
            if (!result.savedResolvedSaveId)
            {
                LogWorldIO("failed to persist updated level-id");
            }
        }

        if (worldLoadPlan.shouldAbortStartup)
        {
            LogErrorf(
                "world-io",
                "Failed to load configured world \"%s\".",
                StringUtils::WideToUtf8(
                    worldBootstrapPlan.targetWorldName).c_str());
            result.abortedStartup = true;
            result.abortExitCode = worldLoadPlan.abortExitCode;
        }

        return result;
    }

    DedicatedServerSessionExecutionResult ExecuteDedicatedServerSession(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext,
        std::uint64_t initialTickMs,
        unsigned int sleepMs)
    {
        DedicatedServerSessionExecutionResult result = {};
        result.initialSave = ExecuteDedicatedServerInitialSave(
            worldBootstrapPlan,
            actionPad);
        result.autosaveState = CreateDedicatedServerAutosaveState(
            initialTickMs,
            serverProperties);
        result.gameplayLoop = RunDedicatedServerGameplayLoopUntilExit(
            &result.autosaveState,
            serverProperties,
            actionPad,
            pollProc,
            pollContext,
            sleepMs);
        result.shutdown = ExecuteDedicatedServerShutdown();
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
