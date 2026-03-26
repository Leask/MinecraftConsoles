#include <atomic>
#include <cstdint>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Minecraft.Server/Access/BanManager.h"
#include "Minecraft.Server/Access/WhitelistManager.h"
#include "Minecraft.Server/Common/DedicatedServerOptions.h"
#include "Minecraft.Server/Common/DedicatedServerRuntime.h"
#include "Minecraft.Server/Common/StringUtils.h"
#include "Minecraft.Server/Common/ServerStoragePaths.h"
#include "Minecraft.Server/ServerLogger.h"
#include "Minecraft.Server/ServerProperties.h"
#include "lce_filesystem/lce_filesystem.h"
#include "lce_process/lce_process.h"
#include "lce_stdin/lce_stdin.h"
#include "lce_time/lce_time.h"

namespace
{
    std::atomic<bool> g_shutdownRequested(false);

    void RequestShutdown()
    {
        g_shutdownRequested.store(true);
    }

    void NativeSignalHandler(int)
    {
        RequestShutdown();
    }

    bool EnsureDirectoryChain(const std::string& path)
    {
        if (path.empty())
        {
            return false;
        }

        std::string current;
        for (char ch : path)
        {
            if (ch == '/' || ch == '\\')
            {
                if (!current.empty() &&
                    !CreateDirectoryIfMissing(current.c_str(), nullptr))
                {
                    return false;
                }

                continue;
            }

            current.push_back(ch);
        }

        return !current.empty() &&
            CreateDirectoryIfMissing(current.c_str(), nullptr);
    }
}

int main(int argc, char** argv)
{
    bool bootstrapOnly = false;
    int serverArgc = 1;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--bootstrap-only") == 0)
        {
            bootstrapOnly = true;
            continue;
        }

        argv[serverArgc++] = argv[i];
    }

    if (!LceSetCurrentDirectoryToExecutable())
    {
        std::fprintf(stderr, "startup error: failed to switch to executable directory\n");
        return 1;
    }

    std::signal(SIGINT, NativeSignalHandler);
    std::signal(SIGTERM, NativeSignalHandler);

    ServerRuntime::DedicatedServerConfig config =
        ServerRuntime::CreateDefaultDedicatedServerConfig();
    ServerRuntime::ServerPropertiesConfig serverProperties =
        ServerRuntime::LoadServerPropertiesConfig();
    ServerRuntime::ApplyServerPropertiesToDedicatedConfig(
        serverProperties,
        &config);

    std::string parseError;
    if (!ServerRuntime::ParseDedicatedServerCommandLine(
            serverArgc,
            argv,
            &config,
            &parseError))
    {
        if (!parseError.empty())
        {
            std::fprintf(stderr, "startup error: %s\n", parseError.c_str());
        }
        return 2;
    }

    if (config.showHelp)
    {
        std::vector<std::string> usageLines;
        ServerRuntime::BuildDedicatedServerUsageLines(&usageLines);
        for (const std::string& line : usageLines)
        {
            std::printf("%s\n", line.c_str());
        }
        return 0;
    }

    ServerRuntime::SetServerLogLevel(config.logLevel);

    const std::string storageRoot =
        ServerRuntime::GetServerGameHddRootPath();
    if (!EnsureDirectoryChain(storageRoot))
    {
        ServerRuntime::LogErrorf(
            "startup",
            "failed to create storage root %s",
            storageRoot.c_str());
        return 3;
    }

    ServerRuntime::Access::BanManager banManager(".");
    ServerRuntime::Access::WhitelistManager whitelistManager(".");
    if (!banManager.EnsureBanFilesExist() ||
        !banManager.Reload() ||
        !whitelistManager.EnsureWhitelistFileExists() ||
        !whitelistManager.Reload())
    {
        ServerRuntime::LogError(
            "startup",
            "failed to initialize native access storage");
        return 4;
    }

    const ServerRuntime::DedicatedServerRuntimeState runtimeState =
        ServerRuntime::BuildDedicatedServerRuntimeState(
            config,
            serverProperties);
    const std::uint64_t autosaveMs =
        ServerRuntime::GetDedicatedServerAutosaveIntervalMs(
            serverProperties);

    ServerRuntime::LogInfof(
        "startup",
        "native bootstrap listening on %s:%d name=%s autosave=%llums",
        runtimeState.bindIp.c_str(),
        runtimeState.multiplayerPort,
        runtimeState.hostNameUtf8.c_str(),
        static_cast<unsigned long long>(autosaveMs));
    ServerRuntime::LogInfof(
        "startup",
        "storage root=%s world=%s level-id=%s whitelist=%s lan=%s",
        storageRoot.c_str(),
        ServerRuntime::StringUtils::WideToUtf8(serverProperties.worldName).c_str(),
        serverProperties.worldSaveId.c_str(),
        serverProperties.whiteListEnabled ? "enabled" : "disabled",
        runtimeState.lanAdvertise ? "enabled" : "disabled");

    if (bootstrapOnly)
    {
        ServerRuntime::LogInfo(
            "startup",
            "native bootstrap completed in bootstrap-only mode");
        return 0;
    }

    ServerRuntime::LogInfo(
        "startup",
        "native bootstrap shell running; type stop or send SIGINT to exit");
    while (!g_shutdownRequested.load())
    {
        if (LceStdinIsAvailable() && LceWaitForStdinReadable(50) == 1)
        {
            char lineBuffer[256] = {};
            if (std::fgets(lineBuffer, sizeof(lineBuffer), stdin) != nullptr)
            {
                const std::string line = lineBuffer;
                if (line == "stop\n" || line == "stop\r\n" || line == "stop")
                {
                    RequestShutdown();
                    break;
                }
            }
        }

        LceSleepMilliseconds(50);
    }

    ServerRuntime::LogInfo("shutdown", "native bootstrap shell stopped");
    return 0;
}
