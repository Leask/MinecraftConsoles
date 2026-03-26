#include "DedicatedServerShutdownPlan.h"

namespace ServerRuntime
{
    DedicatedServerShutdownPlan BuildDedicatedServerShutdownPlan(
        bool serverInstancePresent,
        bool serverStoppedSignalValid)
    {
        DedicatedServerShutdownPlan shutdownPlan = {};
        shutdownPlan.shouldSetSaveOnExit = serverInstancePresent;
        shutdownPlan.shouldLogShutdownSave = serverInstancePresent;
        shutdownPlan.shouldWaitForServerStop = serverStoppedSignalValid;
        return shutdownPlan;
    }
}
