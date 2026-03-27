#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "DedicatedServerBootstrap.h"
#include "DedicatedServerPlatformState.h"

namespace ServerRuntime
{
    struct DedicatedServerHeadlessRuntimeOptions
    {
        bool bootstrapOnly = false;
        bool selfConnect = false;
        bool shellSelfConnect = false;
        std::uint64_t shutdownAfterMs = 0;
        std::vector<std::string> scriptedCommands;
    };

    int RunDedicatedServerHeadlessRuntime(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        const DedicatedServerHeadlessRuntimeOptions &options);
}
