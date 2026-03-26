#pragma once

struct WorldCompressionSmokeResult
{
    bool zlibRoundTripOk;
    bool rleRoundTripOk;
    bool lzxRleRoundTripOk;
    unsigned int inputSize;
    unsigned int compressedSize;
    unsigned int rleSize;
    unsigned int lzxRleSize;
};

WorldCompressionSmokeResult RunWorldCompressionSmoke();
