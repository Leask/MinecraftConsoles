#pragma once

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

    struct DedicatedServerShutdownExecutionResult
    {
        DedicatedServerShutdownPlan plan = {};
        bool haltedGameplay = false;
    };

    DedicatedServerShutdownExecutionResult ExecuteDedicatedServerShutdown();
}
