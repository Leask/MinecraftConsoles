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
        int localUsersMask = 0;
        bool onlineGame = false;
        bool privateGame = false;
        unsigned int publicSlots = 0;
        unsigned int privateSlots = 0;
        bool fakeLocalPlayerJoined = false;
        bool dedicatedNoLocalHostPlayer = false;
        unsigned int xzSize = 0;
        unsigned char hellScale = 0;
    };
}
