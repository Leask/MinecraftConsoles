#pragma once

#include "DedicatedServerBootstrap.h"
#include "DedicatedServerPlatformState.h"

namespace ServerRuntime
{
    struct DedicatedServerHeadlessRuntimeOptions
    {
        bool bootstrapOnly = false;
        bool selfConnect = false;
    };

    int RunDedicatedServerHeadlessRuntime(
        const DedicatedServerBootstrapContext &context,
        const DedicatedServerPlatformState &platformState,
        const DedicatedServerHeadlessRuntimeOptions &options);
}
