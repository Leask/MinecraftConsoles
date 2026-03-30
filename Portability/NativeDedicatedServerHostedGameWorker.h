#pragma once

namespace ServerRuntime
{
    void ResetNativeDedicatedServerHostedGameWorkerState();

    void ClearNativeDedicatedServerHostedGameWorkerState();

    void RequestNativeDedicatedServerHostedGameWorkerAutosave(
        unsigned int workTicks);

    void TickNativeDedicatedServerHostedGameWorker();

    bool IsNativeDedicatedServerHostedGameWorkerIdle();
}
