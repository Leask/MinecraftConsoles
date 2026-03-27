#pragma once

#include "../Console/ServerCli.h"
#include "../ServerLogger.h"
#include "DedicatedServerLifecycle.h"
#include "DedicatedServerPlatformState.h"

namespace ServerRuntime
{
    struct DedicatedServerCliRunHooksContext
    {
        const DedicatedServerPlatformState *platformState = nullptr;
        ServerCli *serverCli = nullptr;
    };

    inline void LogDedicatedServerReadyHook(void *context)
    {
        const DedicatedServerCliRunHooksContext *hookContext =
            static_cast<const DedicatedServerCliRunHooksContext *>(context);
        if (hookContext == nullptr || hookContext->platformState == nullptr)
        {
            return;
        }

        LogStartupStep("server startup complete");
        LogInfof(
            "startup",
            "Dedicated server listening on %s:%d",
            hookContext->platformState->bindIp.c_str(),
            hookContext->platformState->multiplayerPort);
    }

    inline void StartDedicatedServerCliHook(void *context)
    {
        DedicatedServerCliRunHooksContext *hookContext =
            static_cast<DedicatedServerCliRunHooksContext *>(context);
        if (hookContext != nullptr && hookContext->serverCli != nullptr)
        {
            hookContext->serverCli->Start();
        }
    }

    inline void PollDedicatedServerCliHook(void *context)
    {
        DedicatedServerCliRunHooksContext *hookContext =
            static_cast<DedicatedServerCliRunHooksContext *>(context);
        if (hookContext != nullptr && hookContext->serverCli != nullptr)
        {
            hookContext->serverCli->Poll();
        }
    }

    inline void StopDedicatedServerCliHook(void *context)
    {
        DedicatedServerCliRunHooksContext *hookContext =
            static_cast<DedicatedServerCliRunHooksContext *>(context);
        if (hookContext != nullptr && hookContext->serverCli != nullptr)
        {
            hookContext->serverCli->Stop();
        }
    }

    inline DedicatedServerRunHooks BuildDedicatedServerCliRunHooks(
        DedicatedServerCliRunHooksContext *context)
    {
        DedicatedServerRunHooks hooks = {};
        hooks.afterStartupProc = &LogDedicatedServerReadyHook;
        hooks.afterStartupContext = context;
        hooks.beforeSessionProc = &StartDedicatedServerCliHook;
        hooks.beforeSessionContext = context;
        hooks.pollProc = &PollDedicatedServerCliHook;
        hooks.pollContext = context;
        hooks.afterSessionProc = &StopDedicatedServerCliHook;
        hooks.afterSessionContext = context;
        return hooks;
    }
}
