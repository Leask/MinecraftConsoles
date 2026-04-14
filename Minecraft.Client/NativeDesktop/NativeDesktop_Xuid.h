#pragma once

#include <chrono>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <functional>
#include <thread>
#include <unistd.h>
#include <lce_abi/lce_abi.h>

namespace NativeDesktopXuid
{
	inline PlayerUID GetLegacyEmbeddedBaseXuid()
	{
		return (PlayerUID)0xe000d45248242f2eULL;
	}

	inline PlayerUID GetLegacyEmbeddedHostXuid()
	{
		// Legacy behavior used "embedded base + smallId"; host was always smallId 0.
		// We intentionally keep this value for host/self compatibility with pre-migration worlds.
		return GetLegacyEmbeddedBaseXuid();
	}

	inline bool IsLegacyEmbeddedRange(PlayerUID xuid)
	{
		// Old NativeDesktop XUIDs were not persistent and always lived in this narrow base+smallId range.
		// Treat them as legacy/non-persistent so uid.dat values never collide with old slot IDs.
		const PlayerUID base = GetLegacyEmbeddedBaseXuid();
		return xuid >= base && xuid < (base + MINECRAFT_NET_MAX_PLAYERS);
	}

	inline bool IsPersistedUidValid(PlayerUID xuid)
	{
		return xuid != INVALID_XUID && !IsLegacyEmbeddedRange(xuid);
	}


	// ./uid.dat
	inline bool BuildUidFilePath(char* outPath, size_t outPathSize)
	{
		if (outPath == NULL || outPathSize == 0)
			return false;

		outPath[0] = 0;

		std::error_code error;
		std::filesystem::path uidPath = std::filesystem::current_path(error);
		if (error)
		{
			uidPath = std::filesystem::path();
		}
		uidPath /= "uid.dat";

		const std::string pathText = uidPath.string();
		if (pathText.empty() || pathText.size() + 1 > outPathSize)
			return false;

		memcpy(outPath, pathText.c_str(), pathText.size() + 1);
		return true;
	}

	inline bool ReadUid(PlayerUID* outXuid)
	{
		if (outXuid == NULL)
			return false;

		char path[MAX_PATH] = {};
		if (!BuildUidFilePath(path, MAX_PATH))
			return false;

		FILE* f = fopen(path, "rb");
		if (f == NULL)
			return false;

		char buffer[128] = {};
		size_t readBytes = fread(buffer, 1, sizeof(buffer) - 1, f);
		fclose(f);

		if (readBytes == 0)
			return false;

		// Compatibility: earlier experiments may have written raw 8-byte uid.dat.
		if (readBytes == sizeof(uint64_t))
		{
			uint64_t raw = 0;
			memcpy(&raw, buffer, sizeof(raw));
			PlayerUID parsed = (PlayerUID)raw;
			if (IsPersistedUidValid(parsed))
			{
				*outXuid = parsed;
				return true;
			}
		}

		buffer[readBytes] = 0;
		char* begin = buffer;
		while (*begin == ' ' || *begin == '\t' || *begin == '\r' || *begin == '\n')
		{
			++begin;
		}

		errno = 0;
		char* end = NULL;
		uint64_t raw = strtoull(begin, &end, 0);
		if (begin == end || errno != 0)
			return false;

		while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
		{
			++end;
		}
		if (*end != 0)
			return false;

		PlayerUID parsed = (PlayerUID)raw;
		if (!IsPersistedUidValid(parsed))
			return false;

		*outXuid = parsed;
		return true;
	}

	inline bool WriteUid(PlayerUID xuid)
	{
		char path[MAX_PATH] = {};
		if (!BuildUidFilePath(path, MAX_PATH))
			return false;

		FILE* f = fopen(path, "wb");
		if (f == NULL)
			return false;

		int written = fprintf(f, "0x%016llX\n", (unsigned long long)xuid);
		fclose(f);
		return written > 0;
	}

	inline uint64_t Mix64(uint64_t x)
	{
		x += 0x9E3779B97F4A7C15ULL;
		x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
		x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
		return x ^ (x >> 31);
	}

	inline PlayerUID GeneratePersistentUid()
	{
		// Avoid platform RNG dependencies: mix several native runtime values.
		const auto wallNow = std::chrono::system_clock::now()
			.time_since_epoch()
			.count();
		const auto steadyNow = std::chrono::steady_clock::now()
			.time_since_epoch()
			.count();
		const size_t threadHash =
			std::hash<std::thread::id>()(std::this_thread::get_id());
		static int moduleAnchor = 0;
		uint64_t stackAnchor = 0;

		uint64_t seed = (uint64_t)wallNow;
		seed ^= (uint64_t)steadyNow;
		seed ^= ((uint64_t)getpid() << 32);
		seed ^= (uint64_t)threadHash;
		seed ^= (uint64_t)(size_t)&stackAnchor;
		seed ^= (uint64_t)(size_t)&moduleAnchor;

		uint64_t raw = Mix64(seed) ^ Mix64(seed + 0xA0761D6478BD642FULL);
		raw ^= 0x8F4B2D6C1A93E705ULL;
		raw |= 0x8000000000000000ULL;

		PlayerUID xuid = (PlayerUID)raw;
		if (!IsPersistedUidValid(xuid))
		{
			raw ^= 0x0100000000000001ULL;
			xuid = (PlayerUID)raw;
		}

		if (!IsPersistedUidValid(xuid))
		{
			// Last-resort deterministic fallback for pathological cases.
			xuid = (PlayerUID)0xD15EA5E000000001ULL;
		}

		return xuid;
	}

	inline PlayerUID DeriveXuidForPad(PlayerUID baseXuid, int iPad)
	{
		if (iPad == 0)
			return baseXuid;

		// Deterministic per-pad XUID: hash the base XUID with the pad number.
		// Produces a fully unique 64-bit value with no risk of overlap.
		// Suggested by rtm516 to avoid adjacent-integer collisions from the old "+ iPad" approach.
		uint64_t raw = Mix64((uint64_t)baseXuid + (uint64_t)iPad);
		raw |= 0x8000000000000000ULL; // keep high bit set like all our XUIDs

		PlayerUID xuid = (PlayerUID)raw;
		if (!IsPersistedUidValid(xuid))
		{
			raw ^= 0x0100000000000001ULL;
			xuid = (PlayerUID)raw;
		}
		if (!IsPersistedUidValid(xuid))
			xuid = (PlayerUID)(0xD15EA5E000000001ULL + iPad);

		return xuid;
	}

	inline PlayerUID ResolvePersistentXuid()
	{
		// Process-local cache: uid.dat is immutable during runtime and this path is hot.
		static bool s_cached = false;
		static PlayerUID s_xuid = INVALID_XUID;

		if (s_cached)
			return s_xuid;

		PlayerUID fileXuid = INVALID_XUID;
		if (ReadUid(&fileXuid))
		{
			s_xuid = fileXuid;
			s_cached = true;
			return s_xuid;
		}

		// First launch on this client: generate once and persist to uid.dat.
		s_xuid = GeneratePersistentUid();
		WriteUid(s_xuid);
		s_cached = true;
		return s_xuid;
	}
}
