#pragma once

#include <string>

#include "DedicatedServerBootstrap.h"
#include "lce_net/lce_net.h"

namespace ServerRuntime
{
    struct DedicatedServerSocketBootstrapState
    {
        LceSocketHandle listener = LCE_INVALID_SOCKET;
        int boundPort = -1;
    };

    bool StartDedicatedServerSocketBootstrap(
        const DedicatedServerBootstrapContext &context,
        DedicatedServerSocketBootstrapState *state,
        std::string *outError);

    void StopDedicatedServerSocketBootstrap(
        DedicatedServerSocketBootstrapState *state);
}
