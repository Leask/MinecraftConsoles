#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Minecraft.Server/Common/DedicatedServerBootstrap.h"
#include "Minecraft.Server/Common/DedicatedServerHeadlessRuntime.h"
#include "Minecraft.Server/Common/DedicatedServerOptions.h"
#include "Minecraft.Server/Common/DedicatedServerPlatformState.h"
#include "Minecraft.Server/Common/DedicatedServerSignalHandlers.h"
#include "Minecraft.Server/Common/DedicatedServerSignalState.h"
#include "Minecraft.Server/ServerLogger.h"

int main(int argc, char** argv)
{
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    bool bootstrapOnly = false;
    bool selfConnect = false;
    int serverArgc = 1;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--bootstrap-only") == 0)
        {
            bootstrapOnly = true;
            continue;
        }
        if (std::strcmp(argv[i], "--self-connect") == 0)
        {
            selfConnect = true;
            continue;
        }

        argv[serverArgc++] = argv[i];
    }

    if (!ServerRuntime::InstallDedicatedServerShutdownSignalHandlers())
    {
        std::fprintf(
            stderr,
            "startup warning: failed to install shutdown signal handlers\n");
    }

    ServerRuntime::DedicatedServerBootstrapContext bootstrapContext = {};
    std::string parseError;
    switch (ServerRuntime::PrepareDedicatedServerBootstrapContext(
            serverArgc,
            argv,
            &bootstrapContext,
            &parseError))
    {
    case ServerRuntime::eDedicatedServerBootstrap_Ready:
        break;
    case ServerRuntime::eDedicatedServerBootstrap_ShowHelp:
        {
            std::vector<std::string> usageLines;
            ServerRuntime::BuildDedicatedServerUsageLines(&usageLines);
            for (const std::string& line : usageLines)
            {
                std::printf("%s\n", line.c_str());
            }
            return 0;
        }
    case ServerRuntime::eDedicatedServerBootstrap_Failed:
    default:
        if (!parseError.empty())
        {
            std::fprintf(stderr, "startup error: %s\n", parseError.c_str());
        }
        return 2;
    }

    std::string bootstrapError;
    if (!ServerRuntime::InitializeDedicatedServerBootstrapEnvironment(
            &bootstrapContext,
            &bootstrapError))
    {
        ServerRuntime::LogError("startup", bootstrapError.c_str());
        return 3;
    }

    ServerRuntime::LogDedicatedServerBootstrapSummary(
        "native bootstrap",
        bootstrapContext);
    const ServerRuntime::DedicatedServerPlatformState platformState =
        ServerRuntime::BuildDedicatedServerPlatformState(
            bootstrapContext.runtimeState,
            1);
    const ServerRuntime::DedicatedServerHeadlessRuntimeOptions
        runtimeOptions = {
            bootstrapOnly,
            selfConnect
        };
    const int exitCode = ServerRuntime::RunDedicatedServerHeadlessRuntime(
        bootstrapContext,
        platformState,
        runtimeOptions);
    ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
        &bootstrapContext);
    return exitCode;
}
