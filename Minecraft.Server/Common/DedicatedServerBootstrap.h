#pragma once

#include <cstdint>
#include <string>

#include "../Access/Access.h"
#include "DedicatedServerOptions.h"
#include "DedicatedServerRuntime.h"

namespace ServerRuntime
{
    enum EDedicatedServerBootstrapStatus
    {
        eDedicatedServerBootstrap_Ready = 0,
        eDedicatedServerBootstrap_ShowHelp = 1,
        eDedicatedServerBootstrap_Failed = 2
    };

    struct DedicatedServerBootstrapContext
    {
        DedicatedServerConfig config = {};
        ServerPropertiesConfig serverProperties = {};
        DedicatedServerRuntimeState runtimeState = {};
        std::string storageRoot;
        std::uint64_t autosaveIntervalMs = 0;
        bool accessInitialized = false;
    };

    EDedicatedServerBootstrapStatus PrepareDedicatedServerBootstrapContext(
        int argc,
        char **argv,
        DedicatedServerBootstrapContext *context,
        std::string *outError);

    bool InitializeDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context,
        std::string *outError);

    void ShutdownDedicatedServerBootstrapEnvironment(
        DedicatedServerBootstrapContext *context);

    void LogDedicatedServerBootstrapSummary(
        const char *label,
        const DedicatedServerBootstrapContext &context);
}
