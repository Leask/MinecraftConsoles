#pragma once

#include "DedicatedServerWorldBootstrap.h"

namespace ServerRuntime
{
    typedef int DedicatedServerHostedGameThreadProc(void* threadParam);

    int StartDedicatedServerHostedGameRuntime(
        const DedicatedServerHostedGamePlan &hostedGamePlan,
        DedicatedServerHostedGameThreadProc *threadProc,
        void *threadParam);
}
