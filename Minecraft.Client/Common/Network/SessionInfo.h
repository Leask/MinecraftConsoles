#pragma once

// NativeDesktop LAN session metadata. This is the only session payload kept on
// main after removing console/Windows platform managers.
typedef struct _GameSessionData
{
    unsigned short netVersion;
    unsigned int m_uiGameHostSettings;
    unsigned int texturePackParentId;
    unsigned char subTexturePackId;

    bool isReadyToJoin;
    bool isJoinable;

    char hostIP[64];
    int hostPort;
    wchar_t hostName[XUSER_NAME_SIZE];
    unsigned char playerCount;
    unsigned char maxPlayers;

    _GameSessionData()
    {
        netVersion = 0;
        m_uiGameHostSettings = 0;
        texturePackParentId = 0;
        subTexturePackId = 0;
        isReadyToJoin = false;
        isJoinable = true;
        memset(hostIP, 0, sizeof(hostIP));
        hostPort = 0;
        memset(hostName, 0, sizeof(hostName));
        playerCount = 0;
        maxPlayers = MINECRAFT_NET_MAX_PLAYERS;
    }
} GameSessionData;

class FriendSessionInfo
{
public:
    SessionID sessionId;
    wchar_t* displayLabel;
    unsigned char displayLabelLength;
    unsigned char displayLabelViewableStartIndex;
    GameSessionData data;
    bool hasPartyMember;

    FriendSessionInfo()
    {
        displayLabel = nullptr;
        displayLabelLength = 0;
        displayLabelViewableStartIndex = 0;
        hasPartyMember = false;
    }

    FriendSessionInfo(const FriendSessionInfo& other)
    {
        sessionId = other.sessionId;
        displayLabelLength = other.displayLabelLength;
        displayLabelViewableStartIndex = other.displayLabelViewableStartIndex;
        data = other.data;
        hasPartyMember = other.hasPartyMember;
        if (other.displayLabel != nullptr)
        {
            displayLabel = new wchar_t[displayLabelLength + 1];
            wcscpy_s(displayLabel, displayLabelLength + 1, other.displayLabel);
        }
        else
        {
            displayLabel = nullptr;
        }
    }

    FriendSessionInfo& operator=(const FriendSessionInfo&) = delete;

    ~FriendSessionInfo()
    {
        if (displayLabel != nullptr)
            delete[] displayLabel;
    }
};
