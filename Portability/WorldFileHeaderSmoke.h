#pragma once

struct WorldFileHeaderSmokeResult
{
    bool duplicateReused;
    bool roundTripOk;
    bool prefixesOk;
    bool offsetsOk;
    int fileCount;
    unsigned int totalSize;
};

WorldFileHeaderSmokeResult RunWorldFileHeaderSmoke();
