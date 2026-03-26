#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

static constexpr int LCE_LAN_DISCOVERY_PORT = 25566;
static constexpr std::uint32_t LCE_LAN_BROADCAST_MAGIC = 0x4D434C4E;
static constexpr std::uint64_t LCE_LAN_SESSION_TIMEOUT_MS = 5000;

#pragma pack(push, 1)
struct LceLanBroadcast
{
    std::uint32_t magic;
    std::uint16_t netVersion;
    std::uint16_t gamePort;
    std::uint16_t hostName[32];
    std::uint8_t playerCount;
    std::uint8_t maxPlayers;
    std::uint32_t gameHostSettings;
    std::uint32_t texturePackParentId;
    std::uint8_t subTexturePackId;
    std::uint8_t isJoinable;
};
#pragma pack(pop)

struct LceLanSession
{
    char hostIP[64];
    int hostPort;
    wchar_t hostName[32];
    std::uint16_t netVersion;
    std::uint8_t playerCount;
    std::uint8_t maxPlayers;
    std::uint32_t gameHostSettings;
    std::uint32_t texturePackParentId;
    std::uint8_t subTexturePackId;
    bool isJoinable;
    std::uint64_t lastSeenMs;
};

bool LceLanBuildBroadcast(
    int gamePort,
    const wchar_t* hostName,
    std::uint32_t gameSettings,
    std::uint32_t texturePackParentId,
    std::uint8_t subTexturePackId,
    std::uint16_t netVersion,
    unsigned int playerCount,
    unsigned int maxPlayers,
    bool isJoinable,
    LceLanBroadcast* outBroadcast);

bool LceLanDecodeBroadcast(
    const void* data,
    std::size_t dataSize,
    const char* senderIp,
    std::uint64_t nowMs,
    LceLanSession* outSession);

void LceLanUpsertSession(
    const LceLanSession& session,
    std::vector<LceLanSession>* sessions,
    bool* addedSession);

void LceLanPruneExpiredSessions(
    std::uint64_t nowMs,
    std::uint64_t timeoutMs,
    std::vector<LceLanSession>* sessions,
    std::vector<LceLanSession>* expiredSessions);
