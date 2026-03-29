#pragma once

#include <cstdint>

#include <lce_win32/lce_win32.h>

#include "DedicatedServerWorldTypes.h"

namespace ServerRuntime
{
    struct NativeDedicatedServerHostedGameRuntimeStubInitData
    {
        std::int64_t seed = 0;
        LoadSaveDataThreadParam *saveData = nullptr;
        DWORD settings = 0;
        bool dedicatedNoLocalHostPlayer = false;
        unsigned int xzSize = 0;
        unsigned char hellScale = 0;
    };
}
