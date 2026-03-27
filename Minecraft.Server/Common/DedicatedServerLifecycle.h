#pragma once

#include "../ServerProperties.h"
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

    DedicatedServerShutdownExecutionResult ExecuteDedicatedServerShutdown();
}
