#include "stdafx.h"
#include "net.minecraft.world.level.biome.h"
#include "net.minecraft.world.level.newbiome.layer.h"
#include "net.minecraft.world.level.h"
#include "BiomeOverrideLayer.h"

#include <cstddef>
#include <cstdio>

namespace
{
	enum class OverrideReadResult
	{
		Ok,
		Missing,
		TooLarge,
		Error,
	};

	OverrideReadResult ReadOverrideBytes(
		const char* path,
		byteArray output)
	{
		FILE* file = std::fopen(path, "rb");
		if( file == nullptr )
		{
			return OverrideReadResult::Missing;
		}

		OverrideReadResult result = OverrideReadResult::Error;
		if( std::fseek(file, 0, SEEK_END) == 0 )
		{
			const long fileSize = std::ftell(file);
			if( fileSize >= 0 &&
				static_cast<unsigned long>(fileSize) > output.length )
			{
				result = OverrideReadResult::TooLarge;
			}
			else if( fileSize >= 0 && std::fseek(file, 0, SEEK_SET) == 0 )
			{
				const std::size_t bytesToRead =
					static_cast<std::size_t>(fileSize);
				const std::size_t bytesRead =
					bytesToRead == 0
						? 0
						: std::fread(output.data, 1, bytesToRead, file);
				result =
					bytesRead == bytesToRead
						? OverrideReadResult::Ok
						: OverrideReadResult::Error;
			}
		}

		std::fclose(file);
		return result;
	}
}

BiomeOverrideLayer::BiomeOverrideLayer(int seedMixup) : Layer(seedMixup)
{
	m_biomeOverride = byteArray( width * height );

	const OverrideReadResult result =
		ReadOverrideBytes("GameRules/biomemap.bin", m_biomeOverride);
	if( result == OverrideReadResult::Missing )
	{
		app.DebugPrintf("Biome override not found, using plains as default\n");

		memset(m_biomeOverride.data,Biome::plains->id,m_biomeOverride.length);
	}
	else if( result == OverrideReadResult::TooLarge )
	{
		app.DebugPrintf("Biomemap binary is too large!!\n");
		__debugbreak();
	}
	else if( result == OverrideReadResult::Error )
	{
		app.FatalLoadError();
	}
}

intArray BiomeOverrideLayer::getArea(int xo, int yo, int w, int h)
{
	intArray result = IntCache::allocate(w * h);

	int xOrigin = xo + width/2;
	int yOrigin = yo + height/2;
	if(xOrigin < 0 ) xOrigin = 0;
	if(xOrigin >= width) xOrigin = width - 1;
	if(yOrigin < 0 ) yOrigin = 0;
	if(yOrigin >= height) yOrigin = height - 1;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int curX = xOrigin + x;
			int curY = yOrigin + y;
			if(curX >= width) curX = width - 1;
			if(curY >= height) curY = height - 1;
			int index = curX + curY * width;

			unsigned char headerValue = m_biomeOverride[index];
			result[x + y * w] = headerValue;
		}
	}
	return result;
}
