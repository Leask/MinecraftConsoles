#include "DedicatedServerBootstrap.h"

#include "ServerStoragePaths.h"
#include "StringUtils.h"
#include "../ServerLogger.h"
#include "../ServerProperties.h"
#include "lce_filesystem/lce_filesystem.h"
#include "lce_process/lce_process.h"

namespace
{
    bool EnsureDirectoryChain(const std::string &path)
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

namespace ServerRuntime
{
    EDedicatedServerBootstrapStatus PrepareDedicatedServerBootstrapContext(
        int argc,
        char **argv,
        DedicatedServerBootstrapContext *context,
        std::string *outError)
    {
        if (context == nullptr)
        {
            if (outError != nullptr)
            {
                *outError = "missing dedicated server bootstrap context";
            }
            return eDedicatedServerBootstrap_Failed;
        }

        if (outError != nullptr)
        {
            outError->clear();
        }

        *context = DedicatedServerBootstrapContext{};
        context->config = CreateDefaultDedicatedServerConfig();

        if (!LceSetCurrentDirectoryToExecutable())
        {
            if (outError != nullptr)
            {
                *outError =
                    "failed to switch working directory to executable folder";
            }
            return eDedicatedServerBootstrap_Failed;
        }

        context->serverProperties = LoadServerPropertiesConfig();
        ApplyServerPropertiesToDedicatedConfig(
            context->serverProperties,
            &context->config);

        std::string parseError;
        if (!ParseDedicatedServerCommandLine(
                argc,
                argv,
                &context->config,
                &parseError))
        {
            if (outError != nullptr)
            {
                *outError = parseError;
            }
            return eDedicatedServerBootstrap_Failed;
        }

        if (context->config.showHelp)
        {
            return eDedicatedServerBootstrap_ShowHelp;
        }

        SetServerLogLevel(context->config.logLevel);
        context->runtimeState = BuildDedicatedServerRuntimeState(
            context->config,
            context->serverProperties);
        context->autosaveIntervalMs = GetDedicatedServerAutosaveIntervalMs(
            context->serverProperties);
        context->storageRoot = GetServerGameHddRootPath();
        return eDedicatedServerBootstrap_Ready;
    }

    bool InitializeDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context,
        std::string *outError)
    {
        if (context == nullptr)
        {
            if (outError != nullptr)
            {
                *outError = "missing dedicated server bootstrap context";
            }
            return false;
        }

        if (outError != nullptr)
        {
            outError->clear();
        }

        if (!EnsureDirectoryChain(context->storageRoot))
        {
            if (outError != nullptr)
            {
                *outError =
                    "failed to create storage root " + context->storageRoot;
            }
            return false;
        }

        if (!Access::Initialize(
                ".",
                context->serverProperties.whiteListEnabled))
        {
            if (outError != nullptr)
            {
                *outError =
                    "failed to initialize dedicated server access control";
            }
            return false;
        }

        context->accessInitialized = true;
        return true;
    }

    void ShutdownDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context)
    {
        if (context == nullptr || !context->accessInitialized)
        {
            return;
        }

        Access::Shutdown();
        context->accessInitialized = false;
    }

    void LogDedicatedServerBootstrapSummary(
        const char *label,
        const DedicatedServerBootstrapContext &context)
    {
        const char *resolvedLabel =
            (label != nullptr && label[0] != '\0') ? label : "startup";
        LogInfof(
            "startup",
            "%s listening on %s:%d name=%s autosave=%llums",
            resolvedLabel,
            context.runtimeState.bindIp.c_str(),
            context.runtimeState.multiplayerPort,
            context.runtimeState.hostNameUtf8.c_str(),
            static_cast<unsigned long long>(context.autosaveIntervalMs));
        LogInfof(
            "startup",
            "storage root=%s world=%s level-id=%s whitelist=%s lan=%s",
            context.storageRoot.c_str(),
            StringUtils::WideToUtf8(context.serverProperties.worldName).c_str(),
            context.serverProperties.worldSaveId.c_str(),
            context.serverProperties.whiteListEnabled ? "enabled" : "disabled",
            context.runtimeState.lanAdvertise ? "enabled" : "disabled");
    }
}
