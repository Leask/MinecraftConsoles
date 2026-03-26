#pragma once

namespace ServerRuntime
{
    struct DedicatedServerShutdownPlan
    {
        bool shouldSetSaveOnExit = false;
        bool shouldLogShutdownSave = false;
        bool shouldWaitForServerStop = false;
    };

    DedicatedServerShutdownPlan BuildDedicatedServerShutdownPlan(
        bool serverInstancePresent,
        bool serverStoppedSignalValid);
}
