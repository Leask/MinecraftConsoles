#include "lce_lan.h"

#include <algorithm>
#include <cstring>
#include <cwchar>

namespace
{
    void CopyWireNameFromWide(
        std::uint16_t* destination,
        std::size_t count,
        const wchar_t* source)
    {
        if (destination == nullptr || count == 0)
        {
            return;
        }

        std::memset(destination, 0, count * sizeof(std::uint16_t));
        if (source == nullptr)
        {
            return;
        }

        for (std::size_t i = 0; i + 1 < count && source[i] != 0; ++i)
        {
            const wchar_t ch = source[i];
            destination[i] = static_cast<std::uint16_t>(
                ch >= 0 && ch <= 0xffff ? ch : '?');
        }
    }

    void CopyWideFromWireName(
        wchar_t* destination,
        std::size_t count,
        const std::uint16_t* source)
    {
        if (destination == nullptr || count == 0)
        {
            return;
        }

        std::wmemset(destination, 0, count);
        if (source == nullptr)
        {
            return;
        }

        for (std::size_t i = 0; i + 1 < count && source[i] != 0; ++i)
        {
            destination[i] = static_cast<wchar_t>(source[i]);
        }
    }

    void CopyNarrowString(char* destination, std::size_t count, const char* source)
    {
        if (destination == nullptr || count == 0)
        {
            return;
        }

        std::memset(destination, 0, count);
        if (source == nullptr)
        {
            return;
        }

        std::strncpy(destination, source, count - 1);
    }

    std::uint8_t ClampToByte(unsigned int value)
    {
        const unsigned int maxByte = 0xffu;
        return static_cast<std::uint8_t>(value > maxByte ? maxByte : value);
    }
}

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
    LceLanBroadcast* outBroadcast)
{
    if (outBroadcast == nullptr || gamePort <= 0 || gamePort > 65535)
    {
        return false;
    }

    std::memset(outBroadcast, 0, sizeof(*outBroadcast));
    outBroadcast->magic = LCE_LAN_BROADCAST_MAGIC;
    outBroadcast->netVersion = netVersion;
    outBroadcast->gamePort = static_cast<std::uint16_t>(gamePort);
    CopyWireNameFromWide(outBroadcast->hostName, 32, hostName);
    outBroadcast->playerCount = ClampToByte(playerCount);
    outBroadcast->maxPlayers = ClampToByte(maxPlayers);
    outBroadcast->gameHostSettings = gameSettings;
    outBroadcast->texturePackParentId = texturePackParentId;
    outBroadcast->subTexturePackId = subTexturePackId;
    outBroadcast->isJoinable = isJoinable ? 1 : 0;
    return true;
}

bool LceLanDecodeBroadcast(
    const void* data,
    std::size_t dataSize,
    const char* senderIp,
    std::uint64_t nowMs,
    LceLanSession* outSession)
{
    if (data == nullptr ||
        dataSize < sizeof(LceLanBroadcast) ||
        senderIp == nullptr ||
        senderIp[0] == 0 ||
        outSession == nullptr)
    {
        return false;
    }

    const LceLanBroadcast* broadcast =
        static_cast<const LceLanBroadcast*>(data);
    if (broadcast->magic != LCE_LAN_BROADCAST_MAGIC)
    {
        return false;
    }

    std::memset(outSession, 0, sizeof(*outSession));
    CopyNarrowString(outSession->hostIP, sizeof(outSession->hostIP), senderIp);
    outSession->hostPort = static_cast<int>(broadcast->gamePort);
    CopyWideFromWireName(outSession->hostName, 32, broadcast->hostName);
    outSession->netVersion = broadcast->netVersion;
    outSession->playerCount = broadcast->playerCount;
    outSession->maxPlayers = broadcast->maxPlayers;
    outSession->gameHostSettings = broadcast->gameHostSettings;
    outSession->texturePackParentId = broadcast->texturePackParentId;
    outSession->subTexturePackId = broadcast->subTexturePackId;
    outSession->isJoinable = broadcast->isJoinable != 0;
    outSession->lastSeenMs = nowMs;
    return true;
}

void LceLanUpsertSession(
    const LceLanSession& session,
    std::vector<LceLanSession>* sessions,
    bool* addedSession)
{
    if (addedSession != nullptr)
    {
        *addedSession = false;
    }

    if (sessions == nullptr)
    {
        return;
    }

    for (std::size_t i = 0; i < sessions->size(); ++i)
    {
        if (std::strcmp((*sessions)[i].hostIP, session.hostIP) == 0 &&
            (*sessions)[i].hostPort == session.hostPort)
        {
            (*sessions)[i] = session;
            return;
        }
    }

    sessions->push_back(session);
    if (addedSession != nullptr)
    {
        *addedSession = true;
    }
}

void LceLanPruneExpiredSessions(
    std::uint64_t nowMs,
    std::uint64_t timeoutMs,
    std::vector<LceLanSession>* sessions,
    std::vector<LceLanSession>* expiredSessions)
{
    if (sessions == nullptr)
    {
        return;
    }

    if (expiredSessions != nullptr)
    {
        expiredSessions->clear();
    }

    for (std::size_t i = sessions->size(); i > 0; --i)
    {
        const LceLanSession& session = (*sessions)[i - 1];
        if (nowMs < session.lastSeenMs ||
            nowMs - session.lastSeenMs <= timeoutMs)
        {
            continue;
        }

        if (expiredSessions != nullptr)
        {
            expiredSessions->push_back(session);
        }
        sessions->erase(sessions->begin() + static_cast<std::ptrdiff_t>(i - 1));
    }

    if (expiredSessions != nullptr)
    {
        std::reverse(expiredSessions->begin(), expiredSessions->end());
    }
}
