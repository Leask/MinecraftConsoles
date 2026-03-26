#include "WorldCompressionSmoke.h"

#include <vector>

#include "Minecraft.World/stdafx.h"
#include "Minecraft.World/compression.h"

namespace
{
    std::vector<unsigned char> BuildCompressionSmokeInput()
    {
        std::vector<unsigned char> bytes;
        bytes.reserve(4096);

        for (int block = 0; block < 32; ++block)
        {
            for (int repeat = 0; repeat < 48; ++repeat)
            {
                bytes.push_back(static_cast<unsigned char>(block));
            }

            bytes.push_back(255);
            bytes.push_back(255);
            bytes.push_back(255);
            bytes.push_back(255);

            for (int step = 0; step < 64; ++step)
            {
                bytes.push_back(
                    static_cast<unsigned char>((block * 17 + step) & 0xFF));
            }
        }

        return bytes;
    }
}

WorldCompressionSmokeResult RunWorldCompressionSmoke()
{
    WorldCompressionSmokeResult result = {};

    const std::vector<unsigned char> input = BuildCompressionSmokeInput();
    Compression compression;

    std::vector<unsigned char> compressed(input.size() * 4U, 0);
    std::vector<unsigned char> rleCompressed(input.size() * 2U, 0);
    std::vector<unsigned char> lzxRleCompressed(input.size() * 4U, 0);
    std::vector<unsigned char> decompressed(input.size(), 0);
    std::vector<unsigned char> rleDecompressed(input.size(), 0);
    std::vector<unsigned char> lzxRleDecompressed(input.size(), 0);

    unsigned int compressedSize =
        static_cast<unsigned int>(compressed.size());
    unsigned int decompressedSize =
        static_cast<unsigned int>(decompressed.size());
    const HRESULT compressHr = compression.Compress(
        compressed.data(),
        &compressedSize,
        const_cast<unsigned char *>(input.data()),
        static_cast<unsigned int>(input.size()));
    const HRESULT decompressHr = compression.Decompress(
        decompressed.data(),
        &decompressedSize,
        compressed.data(),
        compressedSize);

    unsigned int rleSize = static_cast<unsigned int>(rleCompressed.size());
    unsigned int rleDecompressedSize =
        static_cast<unsigned int>(rleDecompressed.size());
    const HRESULT rleCompressHr = compression.CompressRLE(
        rleCompressed.data(),
        &rleSize,
        const_cast<unsigned char *>(input.data()),
        static_cast<unsigned int>(input.size()));
    const HRESULT rleDecompressHr = compression.DecompressRLE(
        rleDecompressed.data(),
        &rleDecompressedSize,
        rleCompressed.data(),
        rleSize);

    unsigned int lzxRleSize =
        static_cast<unsigned int>(lzxRleCompressed.size());
    unsigned int lzxRleDecompressedSize =
        static_cast<unsigned int>(lzxRleDecompressed.size());
    const HRESULT lzxRleCompressHr = compression.CompressLZXRLE(
        lzxRleCompressed.data(),
        &lzxRleSize,
        const_cast<unsigned char *>(input.data()),
        static_cast<unsigned int>(input.size()));
    const HRESULT lzxRleDecompressHr = compression.DecompressLZXRLE(
        lzxRleDecompressed.data(),
        &lzxRleDecompressedSize,
        lzxRleCompressed.data(),
        lzxRleSize);

    result.zlibRoundTripOk =
        compressHr == S_OK &&
        decompressHr == S_OK &&
        decompressedSize == input.size() &&
        std::equal(input.begin(), input.end(), decompressed.begin());
    result.rleRoundTripOk =
        rleCompressHr == S_OK &&
        rleDecompressHr == S_OK &&
        rleDecompressedSize == input.size() &&
        std::equal(input.begin(), input.end(), rleDecompressed.begin());
    result.lzxRleRoundTripOk =
        lzxRleCompressHr == S_OK &&
        lzxRleDecompressHr == S_OK &&
        lzxRleDecompressedSize == input.size() &&
        std::equal(input.begin(), input.end(), lzxRleDecompressed.begin());
    result.inputSize = static_cast<unsigned int>(input.size());
    result.compressedSize = compressedSize;
    result.rleSize = rleSize;
    result.lzxRleSize = lzxRleSize;

    return result;
}
