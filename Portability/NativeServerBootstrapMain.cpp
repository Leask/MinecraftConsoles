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
#include "Minecraft.Server/Common/ServerStoragePaths.h"
#include "Minecraft.Server/ServerLogger.h"

namespace
{
    bool SetNativeServerStorageRootOverride(const char *storageRoot)
    {
        if (storageRoot == nullptr)
        {
            return false;
        }

#if defined(_WINDOWS64) || defined(_WIN32)
        return _putenv_s(
            "MINECRAFT_SERVER_STORAGE_ROOT",
            storageRoot) == 0;
#else
        return setenv(
            "MINECRAFT_SERVER_STORAGE_ROOT",
            storageRoot,
            1) == 0;
#endif
    }
}

int main(int argc, char** argv)
{
    ServerRuntime::ResetDedicatedServerShutdownRequest();
    bool bootstrapOnly = false;
    bool selfConnect = false;
    bool shellSelfConnect = false;
    std::string storageRootOverride;
    std::uint64_t shutdownAfterMs = 0;
    std::uint64_t requiredAcceptedConnections = 0;
    std::uint64_t requiredRemoteCommands = 0;
    std::vector<std::string> scriptedCommands;
    std::vector<std::string> shellSelfCommands;
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
        if (std::strcmp(argv[i], "--shell-self-connect") == 0)
        {
            shellSelfConnect = true;
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
        if (std::strcmp(argv[i], "--require-accepted-connections") == 0)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(
                    stderr,
                    "startup error: missing value for "
                    "--require-accepted-connections\n");
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
                    "startup error: invalid "
                    "--require-accepted-connections value\n");
                return 2;
            }

            requiredAcceptedConnections = (std::uint64_t)parsedValue;
            ++i;
            continue;
        }
        if (std::strcmp(argv[i], "--require-remote-commands") == 0)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(
                    stderr,
                    "startup error: missing value for "
                    "--require-remote-commands\n");
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
                    "startup error: invalid "
                    "--require-remote-commands value\n");
                return 2;
            }

            requiredRemoteCommands = (std::uint64_t)parsedValue;
            ++i;
            continue;
        }
        if (std::strcmp(argv[i], "--storage-root") == 0)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(
                    stderr,
                    "startup error: missing value for --storage-root\n");
                return 2;
            }

            storageRootOverride = argv[i + 1];
            ++i;
            continue;
        }
        if (std::strcmp(argv[i], "--command") == 0)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(
                    stderr,
                    "startup error: missing value for --command\n");
                return 2;
            }

            scriptedCommands.push_back(argv[i + 1]);
            ++i;
            continue;
        }
        if (std::strcmp(argv[i], "--shell-self-command") == 0)
        {
            if (i + 1 >= argc)
            {
                std::fprintf(
                    stderr,
                    "startup error: missing value for "
                    "--shell-self-command\n");
                return 2;
            }

            shellSelfCommands.push_back(argv[i + 1]);
            ++i;
            continue;
        }

        argv[serverArgc++] = argv[i];
    }

    if (!shellSelfCommands.empty())
    {
        shellSelfConnect = true;
    }

    if (!ServerRuntime::InstallDedicatedServerShutdownSignalHandlers())
    {
        std::fprintf(
            stderr,
            "startup warning: failed to install shutdown signal handlers\n");
    }

    ServerRuntime::DedicatedServerBootstrapContext bootstrapContext = {};
    ServerRuntime::DedicatedServerBootstrapEnvironmentGuard
        bootstrapShutdownGuard;
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
            ServerRuntime::PrintDedicatedServerUsage(stdout);
            std::printf("\n");
            std::printf("Native bootstrap options:\n");
            std::printf("  --bootstrap-only           "
                "Initialize bootstrap and exit\n");
            std::printf("  --self-connect             "
                "Loop back a TCP client and exit\n");
            std::printf("  --shell-self-connect       "
                "Loop back one TCP client during shell mode\n");
            std::printf("  --shutdown-after-ms <ms>   "
                "Run shell mode for a bounded time\n");
            std::printf("  --require-accepted-connections <n> "
                "Fail if shell accepts fewer than n clients\n");
            std::printf("  --require-remote-commands <n> "
                "Fail if shell runs fewer than n remote commands\n");
            std::printf("  --storage-root <path>      "
                "Override native storage root\n");
            std::printf("  --command <cmd>            "
                "Run one native shell command before stdin\n");
            std::printf("  --shell-self-command <cmd> "
                "Send one loopback remote shell command\n");
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

    if (!storageRootOverride.empty() &&
        !SetNativeServerStorageRootOverride(storageRootOverride.c_str()))
    {
        std::fprintf(
            stderr,
            "startup error: failed to set storage root override\n");
        return 2;
    }

    if (!storageRootOverride.empty())
    {
        bootstrapContext.storageRoot =
            ServerRuntime::GetServerGameHddRootPath();
    }

    std::string bootstrapError;
    if (!ServerRuntime::InitializeDedicatedServerBootstrapEnvironment(
            &bootstrapContext,
            &bootstrapError))
    {
        ServerRuntime::LogError("startup", bootstrapError.c_str());
        return 3;
    }
    bootstrapShutdownGuard.Activate(&bootstrapContext);

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
            shellSelfConnect,
            shutdownAfterMs,
            requiredAcceptedConnections,
            requiredRemoteCommands,
            scriptedCommands,
            shellSelfCommands
        };
    const int exitCode = ServerRuntime::RunDedicatedServerHeadlessRuntime(
        bootstrapContext,
        platformState,
        runtimeOptions);
    return exitCode;
}
