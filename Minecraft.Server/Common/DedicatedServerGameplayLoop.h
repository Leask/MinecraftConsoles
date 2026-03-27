#pragma once

#include "DedicatedServerRuntime.h"

namespace ServerRuntime
{
    typedef void DedicatedServerGameplayLoopPollProc(void* pollContext);

    struct DedicatedServerGameplayLoopIterationResult
    {
        bool shouldExit = false;
        bool autosaveCompleted = false;
        bool autosaveRequested = false;
        bool advancedDeadline = false;
    };

    DedicatedServerGameplayLoopIterationResult
    TickDedicatedServerGameplayLoop(
        DedicatedServerAutosaveState *autosaveState,
        const ServerPropertiesConfig &serverProperties,
        int actionPad,
        DedicatedServerGameplayLoopPollProc *pollProc,
        void *pollContext);
}
