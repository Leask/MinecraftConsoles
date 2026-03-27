#pragma once

#include <cstdint>

#include "DedicatedServerBootstrap.h"
#include "DedicatedServerPlatformState.h"

namespace ServerRuntime
{
    struct DedicatedServerHeadlessRuntimeOptions
    {
        bool bootstrapOnly = false;
        bool selfConnect = false;
        std::uint64_t shutdownAfterMs = 0;
    };

    int RunDedicatedServerHeadlessRuntime(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        const DedicatedServerHeadlessRuntimeOptions &options);
}
