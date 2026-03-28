#pragma once

#include "DedicatedServerWorldBootstrap.h"
#include "DedicatedServerWorldLoadPipeline.h"

namespace ServerRuntime
{
    typedef void (*DestroyDedicatedServerWorldLoadStorageContextProc)(
        void *context);

    struct DedicatedServerWorldLoadStorageRuntime
    {
        DedicatedServerWorldStorageHooks storageHooks = {};
        DedicatedServerWorldLoadHooks loadHooks = {};
        void *platformContext = nullptr;
        DestroyDedicatedServerWorldLoadStorageContextProc destroyContextProc =
            nullptr;
    };

    DedicatedServerWorldLoadStorageRuntime
    CreateDedicatedServerPlatformWorldLoadStorageRuntime();

    inline void DestroyDedicatedServerWorldLoadStorageRuntime(
        DedicatedServerWorldLoadStorageRuntime *runtime)
    {
        if (runtime == nullptr)
        {
            return;
        }

        if (runtime->destroyContextProc != nullptr &&
            runtime->platformContext != nullptr)
        {
            runtime->destroyContextProc(runtime->platformContext);
        }

        *runtime = DedicatedServerWorldLoadStorageRuntime{};
    }
}
