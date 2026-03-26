#pragma once

#include <cstdint>

struct WorldSystemSmokeResult
{
    bool monotonicAdvanced;
    bool currentTimePositive;
    bool realTimePositive;
    std::int64_t currentTimeMs;
    std::int64_t realTimeMs;
    std::int64_t deltaNs;
};

WorldSystemSmokeResult RunWorldSystemSmoke();
