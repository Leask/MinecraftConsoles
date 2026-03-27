#include <csignal>

#include "Minecraft.Server/Common/DedicatedServerSignalHandlers.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"

namespace
{
    void NativeDedicatedServerSignalHandler(int)
    {
        ServerRuntime::RequestDedicatedServerShutdown();
    }
}

namespace ServerRuntime
{
    bool InstallDedicatedServerShutdownSignalHandlers()
    {
        return std::signal(SIGINT, NativeDedicatedServerSignalHandler) != SIG_ERR &&
            std::signal(SIGTERM, NativeDedicatedServerSignalHandler) != SIG_ERR;
    }
}
