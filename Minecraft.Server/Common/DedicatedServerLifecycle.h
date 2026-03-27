#pragma once

#include <cstdint>

#include "../ServerProperties.h"
#include "DedicatedServerGameplayLoop.h"
#include "DedicatedServerShutdownPlan.h"
#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    struct DedicatedServerInitialSaveExecutionResult
    {
        bool requested = false;
        bool completed = false;
        bool timedOut = false;
    };

    DedicatedServerInitialSaveExecutionResult
    ExecuteDedicatedServerInitialSave(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        int actionPad);

    struct DedicatedServerWorldLoadExecutionResult
    {
        bool updatedSaveId = false;
        bool savedResolvedSaveId = false;
        bool abortedStartup = false;
        int abortExitCode = 0;
    };

    DedicatedServerWorldLoadExecutionResult ExecuteDedicatedServerWorldLoadPlan(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        const DedicatedServerWorldLoadPlan &worldLoadPlan,
        ServerPropertiesConfig *serverProperties);

    struct DedicatedServerShutdownExecutionResult
    {
        DedicatedServerShutdownPlan plan = {};
        bool haltedGameplay = false;
    };

    struct DedicatedServerSessionExecutionResult
    {
        DedicatedServerInitialSaveExecutionResult initialSave = {};
        DedicatedServerAutosaveState autosaveState = {};
        DedicatedServerGameplayLoopRunResult gameplayLoop = {};
        DedicatedServerShutdownExecutionResult shutdown = {};
    };

    DedicatedServerSessionExecutionResult ExecuteDedicatedServerSession(
        const DedicatedServerWorldBootstrapPlan &worldBootstrapPlan,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext,
        std::uint64_t initialTickMs,
        unsigned int sleepMs);

    DedicatedServerShutdownExecutionResult ExecuteDedicatedServerShutdown();
}
