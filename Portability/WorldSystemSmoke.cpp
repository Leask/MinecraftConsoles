#include "WorldSystemSmoke.h"

#include "Minecraft.World/stdafx.h"
#include "Minecraft.World/System.h"

#include "lce_time/lce_time.h"

WorldSystemSmokeResult RunWorldSystemSmoke()
{
    WorldSystemSmokeResult result = {};

    const std::int64_t beforeNs = System::nanoTime();
    const std::int64_t currentMs = System::currentTimeMillis();
    const std::int64_t realMs = System::currentRealTimeMillis();

    LceSleepMilliseconds(5);

    const std::int64_t afterNs = System::nanoTime();

    result.monotonicAdvanced = afterNs > beforeNs;
    result.currentTimePositive = currentMs > 0;
    result.realTimePositive = realMs > 0;
    result.currentTimeMs = currentMs;
    result.realTimeMs = realMs;
    result.deltaNs = afterNs - beforeNs;

    return result;
}
