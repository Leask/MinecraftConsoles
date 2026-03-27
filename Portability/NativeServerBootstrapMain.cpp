#include <cstdint>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
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
    std::uint64_t shutdownAfterMs = 0;
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
        if (std::strcmp(argv[i], "--shutdown-after-ms") == 0)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(
                    stderr,
                    "startup error: missing value for --shutdown-after-ms\n");
                return 2;
            }

            errno = 0;
            char* end = nullptr;
            const unsigned long long parsedValue = std::strtoull(
                argv[i + 1],
                &end,
                10);
            if (argv[i + 1] == end || *end != '\0' || errno != 0)
            {
                std::fprintf(
                    stderr,
                    "startup error: invalid --shutdown-after-ms value\n");
                return 2;
            }

            shutdownAfterMs = (std::uint64_t)parsedValue;
            ++i;
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
            std::printf("\n");
            std::printf("Native bootstrap options:\n");
            std::printf("  --bootstrap-only           "
                "Initialize bootstrap and exit\n");
            std::printf("  --self-connect             "
                "Loop back a TCP client and exit\n");
            std::printf("  --shutdown-after-ms <ms>   "
                "Run shell mode for a bounded time\n");
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
            selfConnect,
            shutdownAfterMs
        };
    const int exitCode = ServerRuntime::RunDedicatedServerHeadlessRuntime(
        bootstrapContext,
        platformState,
        runtimeOptions);
    ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(
        &bootstrapContext);
    return exitCode;
}
