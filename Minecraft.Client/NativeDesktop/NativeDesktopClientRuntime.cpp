#include "stdafx.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "../Minecraft.h"
#include "../MinecraftServer.h"
#include "../Screen.h"
#include "../Tesselator.h"
#include "../TexturePackRepository.h"
#include "../User.h"
#include "../Common/UI/IUIScene_CreativeMenu.h"
#include "../Common/UI/UIStructs.h"
#include "../Common/Leaderboards/LeaderboardManager.h"
#include "../Common/Network/GameNetworkManager.h"
#include "Network/NativeDesktopNetLayer.h"
#include "NativeDesktopClientSaveControl.h"
#include "NativeDesktop_Xuid.h"
#include "SocialManager.h"
#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/IntCache.h"
#include "../../Minecraft.World/LevelSettings.h"
#include "../../Minecraft.World/Level.h"
#include "../../Minecraft.World/TilePos.h"
#include "../../Minecraft.World/OldChunkStorage.h"
#include "../../Minecraft.World/Tile.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/compression.h"

BOOL g_bWidescreen = TRUE;
int g_iScreenWidth = 1280;
int g_iScreenHeight = 720;
int g_rScreenWidth = 1280;
int g_rScreenHeight = 720;
float g_iAspectRatio = static_cast<float>(g_iScreenWidth) / g_iScreenHeight;
HWND g_hWnd = nullptr;

const int IUIScene_CreativeMenu::TabSpec::MAX_SIZE;

char g_NativeDesktopUsername[17] = "Player";
wchar_t g_NativeDesktopUsernameW[17] = L"Player";

void MemSect(int sect)
{
    (void)sect;
}

bool IsEqualXUID(PlayerUID a, PlayerUID b)
{
    return a == b;
}

DWORD XGetLanguage()
{
    return 1;
}

DWORD XGetLocale()
{
    return 0;
}

DWORD XEnableGuestSignin(BOOL fEnable)
{
    (void)fEnable;
    return ERROR_SUCCESS;
}

DWORD XUserGetSigninInfo(
    DWORD dwUserIndex,
    DWORD dwFlags,
    PXUSER_SIGNIN_INFO pSigninInfo)
{
    (void)dwFlags;
    if (pSigninInfo == nullptr || dwUserIndex >= XUSER_MAX_COUNT)
    {
        return E_FAIL;
    }

    pSigninInfo->xuid = NativeDesktopXuid::DeriveXuidForPad(
        NativeDesktopXuid::ResolvePersistentXuid(),
        static_cast<int>(dwUserIndex));
    pSigninInfo->dwGuestNumber = 0;
    return ERROR_SUCCESS;
}

void XMemCpy(void* a, const void* b, size_t s)
{
    std::memcpy(a, b, s);
}

void XMemSet(void* a, int t, size_t s)
{
    std::memset(a, t, s);
}

void XMemSet128(void* a, int t, size_t s)
{
    std::memset(a, t, s);
}

void* XPhysicalAlloc(SIZE_T a, ULONG_PTR b, ULONG_PTR c, DWORD d)
{
    (void)b;
    (void)c;
    (void)d;
    return std::malloc(a);
}

void XPhysicalFree(void* a)
{
    std::free(a);
}

D3DXVECTOR3::D3DXVECTOR3()
    : x(0.0f), y(0.0f), z(0.0f), pad(0.0f)
{
}

D3DXVECTOR3::D3DXVECTOR3(float xValue, float yValue, float zValue)
    : x(xValue), y(yValue), z(zValue), pad(0.0f)
{
}

D3DXVECTOR3& D3DXVECTOR3::operator += (CONST D3DXVECTOR3& add)
{
    x += add.x;
    y += add.y;
    z += add.z;
    return *this;
}

BYTE IQNetPlayer::GetSmallId()
{
    return m_smallId;
}

void IQNetPlayer::SendData(
    IQNetPlayer* player,
    const void* pvData,
    DWORD dwDataSize,
    DWORD dwFlags)
{
    (void)dwFlags;
    if (player == nullptr || pvData == nullptr || dwDataSize == 0)
    {
        return;
    }

    if (NativeDesktopNetLayer::IsActive())
    {
        if (!NativeDesktopNetLayer::IsHosting() && !m_isRemote)
        {
            SOCKET socket = NativeDesktopNetLayer::GetLocalSocket(m_smallId);
            if (socket != INVALID_SOCKET)
            {
                NativeDesktopNetLayer::SendOnSocket(socket, pvData, dwDataSize);
            }
        }
        else
        {
            NativeDesktopNetLayer::SendToSmallId(player->m_smallId, pvData, dwDataSize);
        }
    }
}

bool IQNetPlayer::IsSameSystem(IQNetPlayer* player)
{
    return this == player || (!m_isRemote && !player->m_isRemote);
}

DWORD IQNetPlayer::GetSendQueueSize(IQNetPlayer* player, DWORD dwFlags)
{
    (void)player;
    (void)dwFlags;
    return 0;
}

DWORD IQNetPlayer::GetCurrentRtt()
{
    return 0;
}

bool IQNetPlayer::IsHost()
{
    return m_isHostPlayer;
}

bool IQNetPlayer::IsGuest()
{
    return false;
}

bool IQNetPlayer::IsLocal()
{
    return !m_isRemote;
}

PlayerUID IQNetPlayer::GetXuid()
{
    if (m_resolvedXuid != INVALID_XUID)
    {
        return m_resolvedXuid;
    }
    return static_cast<PlayerUID>(
        NativeDesktopXuid::GetLegacyEmbeddedBaseXuid() + m_smallId);
}

LPCWSTR IQNetPlayer::GetGamertag()
{
    return m_gamertag;
}

int IQNetPlayer::GetSessionIndex()
{
    return m_smallId;
}

bool IQNetPlayer::IsTalking()
{
    return false;
}

bool IQNetPlayer::IsMutedByLocalUser(DWORD dwUserIndex)
{
    (void)dwUserIndex;
    return false;
}

bool IQNetPlayer::HasVoice()
{
    return false;
}

bool IQNetPlayer::HasCamera()
{
    return false;
}

int IQNetPlayer::GetUserIndex()
{
    return static_cast<int>(this - &IQNet::m_player[0]);
}

void IQNetPlayer::SetCustomDataValue(ULONG_PTR ulpCustomDataValue)
{
    m_customData = ulpCustomDataValue;
}

ULONG_PTR IQNetPlayer::GetCustomDataValue()
{
    return m_customData;
}

IQNetPlayer IQNet::m_player[MINECRAFT_NET_MAX_PLAYERS];
DWORD IQNet::s_playerCount = 1;
bool IQNet::s_isHosting = true;

QNET_STATE _iQNetStubState = QNET_STATE_IDLE;

void NativeDesktop_SetupRemoteQNetPlayer(
    IQNetPlayer* player,
    BYTE smallId,
    bool isHost,
    bool isLocal)
{
    if (player == nullptr)
    {
        return;
    }

    player->m_smallId = smallId;
    player->m_isRemote = !isLocal;
    player->m_isHostPlayer = isHost;
    player->m_resolvedXuid = INVALID_XUID;
    swprintf_s(player->m_gamertag, 32, L"Player%d", smallId);
    if (smallId >= IQNet::s_playerCount)
    {
        IQNet::s_playerCount = smallId + 1;
    }
}

HRESULT IQNet::AddLocalPlayerByUserIndex(DWORD dwUserIndex)
{
    if (dwUserIndex >= MINECRAFT_NET_MAX_PLAYERS)
    {
        return E_FAIL;
    }

    IQNetPlayer& player = m_player[dwUserIndex];
    player.m_smallId = static_cast<BYTE>(dwUserIndex);
    player.m_isRemote = false;
    player.m_isHostPlayer = dwUserIndex == 0;
    player.m_resolvedXuid = NativeDesktopXuid::DeriveXuidForPad(
        NativeDesktopXuid::ResolvePersistentXuid(),
        static_cast<int>(dwUserIndex));
    if (dwUserIndex == 0)
    {
        wcscpy_s(player.m_gamertag, 32, g_NativeDesktopUsernameW);
    }
    else
    {
        swprintf_s(
            player.m_gamertag,
            32,
            L"%ls(%u)",
            g_NativeDesktopUsernameW,
            static_cast<unsigned>(dwUserIndex + 1));
    }

    if (dwUserIndex >= s_playerCount)
    {
        s_playerCount = dwUserIndex + 1;
    }
    return S_OK;
}

IQNetPlayer* IQNet::GetHostPlayer()
{
    return &m_player[0];
}

IQNetPlayer* IQNet::GetLocalPlayerByUserIndex(DWORD dwUserIndex)
{
    if (dwUserIndex >= s_playerCount || dwUserIndex >= MINECRAFT_NET_MAX_PLAYERS)
    {
        return nullptr;
    }
    return &m_player[dwUserIndex];
}

IQNetPlayer* IQNet::GetPlayerByIndex(DWORD dwPlayerIndex)
{
    return GetLocalPlayerByUserIndex(dwPlayerIndex);
}

IQNetPlayer* IQNet::GetPlayerBySmallId(BYTE smallId)
{
    for (DWORD i = 0; i < s_playerCount; ++i)
    {
        if (m_player[i].m_smallId == smallId)
        {
            return &m_player[i];
        }
    }
    return nullptr;
}

IQNetPlayer* IQNet::GetPlayerByXuid(PlayerUID xuid)
{
    for (DWORD i = 0; i < s_playerCount; ++i)
    {
        if (m_player[i].GetXuid() == xuid)
        {
            return &m_player[i];
        }
    }
    return nullptr;
}

DWORD IQNet::GetPlayerCount()
{
    return s_playerCount;
}

QNET_STATE IQNet::GetState()
{
    return _iQNetStubState;
}

bool IQNet::IsHost()
{
    return s_isHosting;
}

HRESULT IQNet::JoinGameFromInviteInfo(
    DWORD dwUserIndex,
    DWORD dwUserMask,
    const INVITE_INFO* pInviteInfo)
{
    (void)dwUserIndex;
    (void)dwUserMask;
    (void)pInviteInfo;
    return S_OK;
}

void IQNet::HostGame()
{
    s_isHosting = true;
    _iQNetStubState = QNET_STATE_SESSION_STARTING;
    m_player[0].m_smallId = 0;
    m_player[0].m_isRemote = false;
    m_player[0].m_isHostPlayer = true;
    m_player[0].m_resolvedXuid = NativeDesktopXuid::GetLegacyEmbeddedHostXuid();
    wcscpy_s(m_player[0].m_gamertag, 32, g_NativeDesktopUsernameW);
    s_playerCount = 1;
}

void IQNet::ClientJoinGame()
{
    s_isHosting = false;
    _iQNetStubState = QNET_STATE_SESSION_STARTING;
}

void IQNet::EndGame()
{
    _iQNetStubState = QNET_STATE_IDLE;
    s_isHosting = true;
    s_playerCount = 1;
}

void XSetThreadProcessor(HANDLE a, int b)
{
    (void)a;
    (void)b;
}

DWORD XShowPartyUI(DWORD dwUserIndex)
{
    (void)dwUserIndex;
    return ERROR_SUCCESS;
}

DWORD XShowFriendsUI(DWORD dwUserIndex)
{
    (void)dwUserIndex;
    return ERROR_SUCCESS;
}

HRESULT XPartyGetUserList(XPARTY_USER_LIST* pUserList)
{
    if (pUserList != nullptr)
    {
        pUserList->dwUserCount = 0;
    }
    return XPARTY_E_NOT_IN_PARTY;
}

DWORD XContentGetThumbnail(
    DWORD dwUserIndex,
    const XCONTENT_DATA* pContentData,
    PBYTE pbThumbnail,
    PDWORD pcbThumbnail,
    PXOVERLAPPED* pOverlapped)
{
    (void)dwUserIndex;
    (void)pContentData;
    (void)pbThumbnail;
    (void)pOverlapped;
    if (pcbThumbnail != nullptr)
    {
        *pcbThumbnail = 0;
    }
    return ERROR_SUCCESS;
}

void XShowAchievementsUI(int i)
{
    (void)i;
}

DWORD XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE mode)
{
    (void)mode;
    return ERROR_SUCCESS;
}

DWORD XUserAreUsersFriends(
    DWORD dwUserIndex,
    PPlayerUID pXuids,
    DWORD dwXuidCount,
    PBOOL pfResult,
    void* pOverlapped)
{
    (void)dwUserIndex;
    (void)pXuids;
    (void)dwXuidCount;
    (void)pOverlapped;
    if (pfResult != nullptr)
    {
        *pfResult = FALSE;
    }
    return ERROR_SUCCESS;
}

HRESULT XMemDecompress(
    XMEMDECOMPRESSION_CONTEXT context,
    VOID* pDestination,
    SIZE_T* pDestSize,
    CONST VOID* pSource,
    SIZE_T srcSize)
{
    (void)context;
    if (pDestination == nullptr || pDestSize == nullptr || pSource == nullptr)
    {
        return E_FAIL;
    }
    if (*pDestSize < srcSize)
    {
        return E_FAIL;
    }
    std::memcpy(pDestination, pSource, srcSize);
    *pDestSize = srcSize;
    return S_OK;
}

HRESULT XMemCompress(
    XMEMCOMPRESSION_CONTEXT context,
    VOID* pDestination,
    SIZE_T* pDestSize,
    CONST VOID* pSource,
    SIZE_T srcSize)
{
    (void)context;
    return XMemDecompress(nullptr, pDestination, pDestSize, pSource, srcSize);
}

HRESULT XMemCreateCompressionContext(
    XMEMCODEC_TYPE codecType,
    CONST VOID* pCodecParams,
    DWORD flags,
    XMEMCOMPRESSION_CONTEXT* pContext)
{
    (void)codecType;
    (void)pCodecParams;
    (void)flags;
    if (pContext != nullptr)
    {
        *pContext = nullptr;
    }
    return S_OK;
}

HRESULT XMemCreateDecompressionContext(
    XMEMCODEC_TYPE codecType,
    CONST VOID* pCodecParams,
    DWORD flags,
    XMEMDECOMPRESSION_CONTEXT* pContext)
{
    (void)codecType;
    (void)pCodecParams;
    (void)flags;
    if (pContext != nullptr)
    {
        *pContext = nullptr;
    }
    return S_OK;
}

void XMemDestroyCompressionContext(XMEMCOMPRESSION_CONTEXT context)
{
    (void)context;
}

void XMemDestroyDecompressionContext(XMEMDECOMPRESSION_CONTEXT context)
{
    (void)context;
}

LPCWSTR CXuiStringTable::Lookup(LPCWSTR szId)
{
    return szId;
}

LPCWSTR CXuiStringTable::Lookup(UINT nIndex)
{
    (void)nIndex;
    return L"";
}

void CXuiStringTable::Clear()
{
}

HRESULT CXuiStringTable::Load(LPCWSTR szId)
{
    (void)szId;
    return S_OK;
}

CSocialManager* CSocialManager::m_pInstance = nullptr;

CSocialManager::CSocialManager()
{
}

CSocialManager* CSocialManager::Instance()
{
    if (m_pInstance == nullptr)
    {
        m_pInstance = new CSocialManager();
    }
    return m_pInstance;
}

void CSocialManager::Initialise()
{
}

void CSocialManager::Tick()
{
}

bool CSocialManager::RefreshPostingCapability()
{
    return false;
}

bool CSocialManager::IsTitleAllowedToPostAnything()
{
    return false;
}

bool CSocialManager::IsTitleAllowedToPostImages()
{
    return false;
}

bool CSocialManager::IsTitleAllowedToPostLinks()
{
    return false;
}

bool CSocialManager::AreAllUsersAllowedToPostImages()
{
    return false;
}

bool CSocialManager::PostLinkToSocialNetwork(
    ESocialNetwork eSocialNetwork,
    DWORD dwUserIndex,
    bool bUsingKinect)
{
    (void)eSocialNetwork;
    (void)dwUserIndex;
    (void)bUsingKinect;
    return false;
}

bool CSocialManager::PostImageToSocialNetwork(
    ESocialNetwork eSocialNetwork,
    DWORD dwUserIndex,
    bool bUsingKinect)
{
    (void)eSocialNetwork;
    (void)dwUserIndex;
    (void)bUsingKinect;
    return false;
}

void CSocialManager::SetSocialPostText(
    LPCWSTR title,
    LPCWSTR caption,
    LPCWSTR desc)
{
    (void)title;
    (void)caption;
    (void)desc;
}

namespace
{
    constexpr int kDefaultNativeDesktopBootstrapFrames = 3;
    constexpr int kDefaultNativeDesktopGameplayFrames = 10;
    constexpr int kDefaultNativeDesktopGameplayWaitMs = 60000;
    constexpr int kNativeDesktopShutdownWaitMs = 30000;
    constexpr UINT kNativeDesktopProfileValues = 5;
    constexpr UINT kNativeDesktopProfileSettings = 4;

    DWORD g_NativeDesktopProfileSettingsA[kNativeDesktopProfileValues] = {};

    struct NativeDesktopRuntimeConfig
    {
        int bootstrapFrames = kDefaultNativeDesktopBootstrapFrames;
        int gameplayFrames = kDefaultNativeDesktopGameplayFrames;
        int gameplayWaitMs = kDefaultNativeDesktopGameplayWaitMs;
        std::string inputScript;
    };

    struct NativeDesktopRuntimeSummary
    {
        bool mediaArchiveReady = false;
        bool stringTableReady = false;
        bool clipboardReady = false;
        bool xuidReady = false;
        bool profileReady = false;
        bool networkManagerReady = false;
        bool qnetReady = false;
        bool localPlayerReady = false;
        bool minecraftRuntimeReady = false;
        bool uiReady = false;
        int uiSkinLibrariesCreated = 0;
        int texturePackCountMax = 0;
        bool bundledDLCReady = false;
        bool startupComplete = false;
        int bootstrapFramesRequested = 0;
        int bootstrapFramesCompleted = 0;
        int gameplayFramesRequested = 0;
        int gameplayFramesCompleted = 0;
        int gameplayReadyAfterMs = -1;
        bool gameplayThreadStarted = false;
        bool gameplayReady = false;
        bool shutdownRequested = false;
        bool leaveGameComplete = false;
        bool networkThreadStopped = false;
        bool shutdownComplete = false;
        bool loopComplete = false;
        bool runtimeHealthy = false;
        bool saveLoadedAtStartup = false;
        bool saveRequested = false;
        bool saveCompleted = false;
        bool savePersisted = false;
        unsigned long long savePersistedBytes = 0;
        int saveCatalogAtStartup = 0;
        unsigned int saveCatalogBytesAtStartup = 0;
        int saveCatalogAfterSave = 0;
        unsigned int saveCatalogBytesAfterSave = 0;
        bool gameplayGameStarted = false;
        bool gameplayLevelReady = false;
        bool gameplayPlayerReady = false;
        bool gameplayScreenCleared = false;
        bool gameplayMenuStateObserved = false;
        bool gameplayMenuClosed = false;
        bool qnetHostObserved = false;
        bool qnetGameplayObserved = false;
        int qnetPlayerCountMax = 0;
        int qnetLastState = -1;
        int exitCode = 1;
        const char* phase = "starting";
        const char* failure = "none";
        bool inputScriptLoaded = false;
        int inputScriptEvents = 0;
        int inputInvalidEvents = 0;
        int inputAppliedEvents = 0;
        const char* inputLastAppliedPhase = "none";
        int inputLastAppliedFrame = -1;
        bool inputHasAny = false;
        bool inputKbmActive = false;
        int inputMouseX = 0;
        int inputMouseY = 0;
    };

    enum class NativeDesktopInputFramePhase
    {
        Bootstrap,
        Frontend,
        Gameplay
    };

    const char* NativeDesktopInputFramePhaseName(
        NativeDesktopInputFramePhase phase)
    {
        switch (phase)
        {
        case NativeDesktopInputFramePhase::Bootstrap:
            return "bootstrap";
        case NativeDesktopInputFramePhase::Frontend:
            return "frontend";
        case NativeDesktopInputFramePhase::Gameplay:
            return "gameplay";
        }
        return "unknown";
    }

    enum class NativeDesktopInputEventType
    {
        KeyDown,
        KeyUp,
        MouseDown,
        MouseUp,
        MouseMove,
        MouseDelta,
        MouseWheel,
        Character,
        Focus
    };

    struct NativeDesktopInputEvent
    {
        NativeDesktopInputFramePhase phase =
            NativeDesktopInputFramePhase::Bootstrap;
        NativeDesktopInputEventType type = NativeDesktopInputEventType::KeyDown;
        int frame = 0;
        int firstValue = 0;
        int secondValue = 0;
        wchar_t character = 0;
        bool applied = false;
    };

    std::string NativeDesktopTrim(std::string value)
    {
        auto isNotSpace = [](unsigned char c)
        {
            return std::isspace(c) == 0;
        };

        value.erase(
            value.begin(),
            std::find_if(value.begin(), value.end(), isNotSpace));
        value.erase(
            std::find_if(value.rbegin(), value.rend(), isNotSpace).base(),
            value.end());
        return value;
    }

    std::string NativeDesktopLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return value;
    }

    std::vector<std::string> NativeDesktopSplit(
        const std::string& value,
        char separator)
    {
        std::vector<std::string> parts;
        std::size_t start = 0;
        while (start <= value.size())
        {
            const std::size_t end = value.find(separator, start);
            if (end == std::string::npos)
            {
                parts.push_back(value.substr(start));
                break;
            }
            parts.push_back(value.substr(start, end - start));
            start = end + 1;
        }
        return parts;
    }

    bool NativeDesktopParseInt(const std::string& text, int* outValue)
    {
        if (outValue == nullptr || text.empty())
        {
            return false;
        }

        errno = 0;
        char* end = nullptr;
        long parsed = std::strtol(text.c_str(), &end, 10);
        if (errno != 0 || end == text.c_str() || *end != '\0')
        {
            return false;
        }

        *outValue = static_cast<int>(parsed);
        return true;
    }

    bool NativeDesktopParsePair(
        const std::string& text,
        int* firstValue,
        int* secondValue)
    {
        std::vector<std::string> values = NativeDesktopSplit(text, ',');
        if (values.size() != 2)
        {
            return false;
        }

        return NativeDesktopParseInt(
            NativeDesktopTrim(values[0]),
            firstValue) &&
            NativeDesktopParseInt(
                NativeDesktopTrim(values[1]),
                secondValue);
    }

    bool NativeDesktopParseInputPhase(
        const std::string& text,
        NativeDesktopInputFramePhase* phase)
    {
        if (phase == nullptr)
        {
            return false;
        }

        const std::string lower = NativeDesktopLower(NativeDesktopTrim(text));
        if (lower == "bootstrap" || lower == "b")
        {
            *phase = NativeDesktopInputFramePhase::Bootstrap;
            return true;
        }
        if (lower == "frontend" || lower == "f")
        {
            *phase = NativeDesktopInputFramePhase::Frontend;
            return true;
        }
        if (lower == "gameplay" || lower == "g")
        {
            *phase = NativeDesktopInputFramePhase::Gameplay;
            return true;
        }
        return false;
    }

    bool NativeDesktopParseKeyName(const std::string& text, int* key)
    {
        if (key == nullptr)
        {
            return false;
        }

        const std::string trimmed = NativeDesktopTrim(text);
        if (trimmed.size() == 1)
        {
            unsigned char c = static_cast<unsigned char>(trimmed[0]);
            *key = static_cast<int>(std::toupper(c));
            return true;
        }

        const std::string lower = NativeDesktopLower(trimmed);
        if (lower == "backspace") { *key = VK_BACK; return true; }
        if (lower == "tab") { *key = VK_TAB; return true; }
        if (lower == "enter" || lower == "return") { *key = VK_RETURN; return true; }
        if (lower == "shift") { *key = VK_SHIFT; return true; }
        if (lower == "lshift") { *key = VK_LSHIFT; return true; }
        if (lower == "rshift") { *key = VK_RSHIFT; return true; }
        if (lower == "control" || lower == "ctrl") { *key = VK_CONTROL; return true; }
        if (lower == "lcontrol" || lower == "lctrl") { *key = VK_LCONTROL; return true; }
        if (lower == "rcontrol" || lower == "rctrl") { *key = VK_RCONTROL; return true; }
        if (lower == "alt") { *key = VK_MENU; return true; }
        if (lower == "lalt") { *key = VK_LMENU; return true; }
        if (lower == "ralt") { *key = VK_RMENU; return true; }
        if (lower == "escape" || lower == "esc") { *key = VK_ESCAPE; return true; }
        if (lower == "left") { *key = VK_LEFT; return true; }
        if (lower == "right") { *key = VK_RIGHT; return true; }
        if (lower == "up") { *key = VK_UP; return true; }
        if (lower == "down") { *key = VK_DOWN; return true; }
        if (lower == "delete" || lower == "del") { *key = VK_DELETE; return true; }
        if (lower == "home") { *key = VK_HOME; return true; }
        if (lower == "end") { *key = VK_END; return true; }
        if (lower == "space") { *key = VK_SPACE; return true; }
        if (lower == "pageup") { *key = VK_PRIOR; return true; }
        if (lower == "pagedown") { *key = VK_NEXT; return true; }
        if (lower == "f1") { *key = VK_F1; return true; }
        if (lower == "f2") { *key = VK_F2; return true; }
        if (lower == "f3") { *key = VK_F3; return true; }
        if (lower == "f4") { *key = VK_F4; return true; }
        if (lower == "f5") { *key = VK_F5; return true; }
        if (lower == "f6") { *key = VK_F6; return true; }
        if (lower == "f8") { *key = VK_F8; return true; }
        if (lower == "f9") { *key = VK_F9; return true; }
        if (lower == "f11") { *key = VK_F11; return true; }
        return false;
    }

    bool NativeDesktopParseMouseButton(const std::string& text, int* button)
    {
        if (button == nullptr)
        {
            return false;
        }

        const std::string lower = NativeDesktopLower(NativeDesktopTrim(text));
        if (lower == "left" || lower == "0")
        {
            *button = KeyboardMouseInput::MOUSE_LEFT;
            return true;
        }
        if (lower == "right" || lower == "1")
        {
            *button = KeyboardMouseInput::MOUSE_RIGHT;
            return true;
        }
        if (lower == "middle" || lower == "2")
        {
            *button = KeyboardMouseInput::MOUSE_MIDDLE;
            return true;
        }
        return false;
    }

    bool NativeDesktopParseBool(const std::string& text, bool* value)
    {
        if (value == nullptr)
        {
            return false;
        }

        const std::string lower = NativeDesktopLower(NativeDesktopTrim(text));
        if (lower == "1" || lower == "true" || lower == "focused")
        {
            *value = true;
            return true;
        }
        if (lower == "0" || lower == "false" || lower == "blurred")
        {
            *value = false;
            return true;
        }
        return false;
    }

    bool NativeDesktopParseInputEvent(
        const std::string& token,
        NativeDesktopInputEvent* event)
    {
        if (event == nullptr)
        {
            return false;
        }

        std::vector<std::string> parts = NativeDesktopSplit(token, ':');
        if (parts.size() != 4 ||
            !NativeDesktopParseInputPhase(parts[0], &event->phase) ||
            !NativeDesktopParseInt(NativeDesktopTrim(parts[1]), &event->frame) ||
            event->frame < 0)
        {
            return false;
        }

        const std::string type = NativeDesktopLower(NativeDesktopTrim(parts[2]));
        const std::string value = NativeDesktopTrim(parts[3]);
        if (type == "key-down" || type == "keydown" || type == "key_down")
        {
            event->type = NativeDesktopInputEventType::KeyDown;
            return NativeDesktopParseKeyName(value, &event->firstValue);
        }
        if (type == "key-up" || type == "keyup" || type == "key_up")
        {
            event->type = NativeDesktopInputEventType::KeyUp;
            return NativeDesktopParseKeyName(value, &event->firstValue);
        }
        if (type == "mouse-down" ||
            type == "mousedown" ||
            type == "mouse_down")
        {
            event->type = NativeDesktopInputEventType::MouseDown;
            return NativeDesktopParseMouseButton(value, &event->firstValue);
        }
        if (type == "mouse-up" || type == "mouseup" || type == "mouse_up")
        {
            event->type = NativeDesktopInputEventType::MouseUp;
            return NativeDesktopParseMouseButton(value, &event->firstValue);
        }
        if (type == "mouse-move" ||
            type == "mousemove" ||
            type == "mouse_move")
        {
            event->type = NativeDesktopInputEventType::MouseMove;
            return NativeDesktopParsePair(
                value,
                &event->firstValue,
                &event->secondValue);
        }
        if (type == "mouse-delta" ||
            type == "mousedelta" ||
            type == "mouse_delta")
        {
            event->type = NativeDesktopInputEventType::MouseDelta;
            return NativeDesktopParsePair(
                value,
                &event->firstValue,
                &event->secondValue);
        }
        if (type == "wheel" || type == "mouse-wheel" || type == "mouse_wheel")
        {
            event->type = NativeDesktopInputEventType::MouseWheel;
            return NativeDesktopParseInt(value, &event->firstValue);
        }
        if (type == "char" || type == "character")
        {
            if (value.empty())
            {
                return false;
            }
            event->type = NativeDesktopInputEventType::Character;
            event->character = static_cast<unsigned char>(value[0]);
            return true;
        }
        if (type == "focus")
        {
            bool focused = false;
            if (!NativeDesktopParseBool(value, &focused))
            {
                return false;
            }
            event->type = NativeDesktopInputEventType::Focus;
            event->firstValue = focused ? 1 : 0;
            return true;
        }

        return false;
    }

    class NativeDesktopInputReplay
    {
    public:
        void Load(const std::string& script)
        {
            m_events.clear();
            m_invalidEvents = 0;
            m_appliedEvents = 0;
            m_lastAppliedPhase = "none";
            m_lastAppliedFrame = -1;

            std::string normalized = script;
            std::replace(normalized.begin(), normalized.end(), ';', '|');
            for (const std::string& rawToken :
                NativeDesktopSplit(normalized, '|'))
            {
                const std::string token = NativeDesktopTrim(rawToken);
                if (token.empty())
                {
                    continue;
                }

                NativeDesktopInputEvent event;
                if (!NativeDesktopParseInputEvent(token, &event))
                {
                    ++m_invalidEvents;
                    std::fprintf(
                        stderr,
                        "NativeDesktop input: ignored script token: %s\n",
                        token.c_str());
                    continue;
                }
                m_events.push_back(event);
            }

            m_scriptLoaded = !m_events.empty();
            std::fprintf(
                stderr,
                "NativeDesktop input: scriptLoaded=%d events=%zu invalid=%d\n",
                m_scriptLoaded ? 1 : 0,
                m_events.size(),
                m_invalidEvents);
        }

        void Apply(
            NativeDesktopInputFramePhase phase,
            int frame,
            NativeDesktopRuntimeSummary* summary)
        {
            int appliedThisFrame = 0;
            for (NativeDesktopInputEvent& event : m_events)
            {
                if (event.applied ||
                    event.phase != phase ||
                    event.frame != frame)
                {
                    continue;
                }

                ApplyEvent(event);
                event.applied = true;
                ++m_appliedEvents;
                ++appliedThisFrame;
                m_lastAppliedPhase = NativeDesktopInputFramePhaseName(phase);
                m_lastAppliedFrame = frame;
            }

            if (appliedThisFrame > 0)
            {
                std::fprintf(
                    stderr,
                    "NativeDesktop input: applied phase=%s frame=%d "
                    "count=%d total=%d\n",
                    NativeDesktopInputFramePhaseName(phase),
                    frame,
                    appliedThisFrame,
                    m_appliedEvents);
            }
            UpdateSummary(summary);
        }

        void UpdateSummary(NativeDesktopRuntimeSummary* summary) const
        {
            if (summary == nullptr)
            {
                return;
            }

            summary->inputScriptLoaded = m_scriptLoaded;
            summary->inputScriptEvents =
                static_cast<int>(m_events.size());
            summary->inputInvalidEvents = m_invalidEvents;
            summary->inputAppliedEvents = m_appliedEvents;
            summary->inputLastAppliedPhase = m_lastAppliedPhase;
            summary->inputLastAppliedFrame = m_lastAppliedFrame;
            summary->inputHasAny = g_KBMInput.HasAnyInput();
            summary->inputKbmActive = g_KBMInput.IsKBMActive();
            summary->inputMouseX = g_KBMInput.GetMouseX();
            summary->inputMouseY = g_KBMInput.GetMouseY();
        }

    private:
        static void ApplyEvent(const NativeDesktopInputEvent& event)
        {
            switch (event.type)
            {
            case NativeDesktopInputEventType::KeyDown:
                g_KBMInput.OnKeyDown(event.firstValue);
                break;
            case NativeDesktopInputEventType::KeyUp:
                g_KBMInput.OnKeyUp(event.firstValue);
                break;
            case NativeDesktopInputEventType::MouseDown:
                g_KBMInput.OnMouseButtonDown(event.firstValue);
                break;
            case NativeDesktopInputEventType::MouseUp:
                g_KBMInput.OnMouseButtonUp(event.firstValue);
                break;
            case NativeDesktopInputEventType::MouseMove:
                g_KBMInput.OnMouseMove(event.firstValue, event.secondValue);
                break;
            case NativeDesktopInputEventType::MouseDelta:
                g_KBMInput.OnRawMouseDelta(event.firstValue, event.secondValue);
                break;
            case NativeDesktopInputEventType::MouseWheel:
                g_KBMInput.OnMouseWheel(event.firstValue);
                break;
            case NativeDesktopInputEventType::Character:
                g_KBMInput.OnChar(event.character);
                break;
            case NativeDesktopInputEventType::Focus:
                g_KBMInput.SetWindowFocused(event.firstValue != 0);
                break;
            }
        }

        std::vector<NativeDesktopInputEvent> m_events;
        bool m_scriptLoaded = false;
        int m_invalidEvents = 0;
        int m_appliedEvents = 0;
        const char* m_lastAppliedPhase = "none";
        int m_lastAppliedFrame = -1;
    };

    NativeDesktopInputReplay g_NativeDesktopInputReplay;

    int NativeDesktopReadIntSetting(
        const char* name,
        int defaultValue,
        int minimumValue,
        int maximumValue)
    {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0')
        {
            return defaultValue;
        }

        errno = 0;
        char* end = nullptr;
        long parsed = std::strtol(value, &end, 10);
        if (errno != 0 || end == value || *end != '\0')
        {
            return defaultValue;
        }

        if (parsed < minimumValue)
        {
            return minimumValue;
        }
        if (parsed > maximumValue)
        {
            return maximumValue;
        }

        return static_cast<int>(parsed);
    }

    void NativeDesktopUpdateRuntimeObservation(
        NativeDesktopRuntimeSummary* summary)
    {
        if (summary == nullptr)
        {
            return;
        }

        Minecraft* minecraft = Minecraft::GetInstance();
        const bool gameStarted = app.GetGameStarted();
        const bool levelReady =
            minecraft != nullptr && minecraft->level != nullptr;
        const bool playerReady =
            minecraft != nullptr && minecraft->player != nullptr;
        const bool screenCleared =
            gameStarted &&
            minecraft != nullptr &&
            minecraft->getScreen() == nullptr;
        int primaryPad = ProfileManager.GetPrimaryPad();
        if (primaryPad < 0 || primaryPad >= XUSER_MAX_COUNT)
        {
            primaryPad = 0;
        }
        const bool menuStateObserved =
            summary->uiReady &&
            gameStarted;
        const bool menuClosed =
            menuStateObserved && !ui.GetMenuDisplayed(primaryPad);
        const QNET_STATE qnetState = ::_iQNetStubState;
        int texturePackCount = 0;
        if (minecraft != nullptr && minecraft->skins != nullptr)
        {
            texturePackCount =
                static_cast<int>(minecraft->skins->getTexturePackCount());
        }

        summary->gameplayGameStarted =
            summary->gameplayGameStarted || gameStarted;
        summary->gameplayLevelReady =
            summary->gameplayLevelReady || levelReady;
        summary->gameplayPlayerReady =
            summary->gameplayPlayerReady || playerReady;
        summary->gameplayScreenCleared =
            summary->gameplayScreenCleared || screenCleared;
        summary->gameplayMenuStateObserved =
            summary->gameplayMenuStateObserved || menuStateObserved;
        summary->gameplayMenuClosed =
            summary->gameplayMenuClosed || menuClosed;
        summary->qnetHostObserved =
            summary->qnetHostObserved || IQNet::s_isHosting;
        summary->qnetGameplayObserved =
            summary->qnetGameplayObserved ||
            qnetState == QNET_STATE_GAME_PLAY;
        summary->qnetPlayerCountMax = std::max(
            summary->qnetPlayerCountMax,
            static_cast<int>(IQNet::s_playerCount));
        summary->qnetLastState = static_cast<int>(qnetState);
        summary->texturePackCountMax = std::max(
            summary->texturePackCountMax,
            texturePackCount);
        summary->bundledDLCReady =
            summary->bundledDLCReady || texturePackCount > 1;
    }

    NativeDesktopRuntimeConfig NativeDesktopLoadRuntimeConfig()
    {
        NativeDesktopRuntimeConfig config;
        config.bootstrapFrames = NativeDesktopReadIntSetting(
            "MINECRAFT_NATIVE_DESKTOP_BOOTSTRAP_FRAMES",
            kDefaultNativeDesktopBootstrapFrames,
            1,
            120);
        config.gameplayFrames = NativeDesktopReadIntSetting(
            "MINECRAFT_NATIVE_DESKTOP_GAMEPLAY_FRAMES",
            kDefaultNativeDesktopGameplayFrames,
            1,
            10000);
        config.gameplayWaitMs = NativeDesktopReadIntSetting(
            "MINECRAFT_NATIVE_DESKTOP_WAIT_MS",
            kDefaultNativeDesktopGameplayWaitMs,
            1000,
            300000);
        const char* inputScript =
            std::getenv("MINECRAFT_NATIVE_DESKTOP_INPUT_SCRIPT");
        if (inputScript != nullptr)
        {
            config.inputScript = inputScript;
        }
        return config;
    }

    void NativeDesktopSetWorkingDirectory(const char* executablePath)
    {
        if (executablePath == nullptr || executablePath[0] == '\0')
        {
            return;
        }

        std::error_code error;
        std::filesystem::path exePath =
            std::filesystem::absolute(executablePath, error);
        if (error)
        {
            return;
        }

        std::filesystem::path exeDir = exePath.parent_path();
        if (!exeDir.empty())
        {
            std::filesystem::current_path(exeDir, error);
        }
    }

    std::filesystem::path NativeDesktopSaveRootPath()
    {
        const char* configuredRoot =
            std::getenv("MINECRAFT_NATIVE_DESKTOP_SAVE_ROOT");
        if (configuredRoot != nullptr && configuredRoot[0] != '\0')
        {
            return std::filesystem::path(configuredRoot);
        }

        std::error_code error;
        std::filesystem::path root = std::filesystem::current_path(error);
        if (error)
        {
            root = std::filesystem::path(".");
        }
        root /= "NativeDesktopSaves";
        return root;
    }

    std::filesystem::path NativeDesktopSaveSlotPath()
    {
        return NativeDesktopSaveRootPath() / "native.savebin";
    }

    bool NativeDesktopPersistedSaveStats(unsigned long long* bytes)
    {
        std::error_code error;
        const std::filesystem::path savePath = NativeDesktopSaveSlotPath();
        if (!std::filesystem::exists(savePath, error) || error)
        {
            if (bytes != nullptr)
            {
                *bytes = 0;
            }
            return false;
        }

        const std::uintmax_t saveBytes =
            std::filesystem::file_size(savePath, error);
        if (error || saveBytes == 0)
        {
            if (bytes != nullptr)
            {
                *bytes = 0;
            }
            return false;
        }

        if (bytes != nullptr)
        {
            *bytes = static_cast<unsigned long long>(saveBytes);
        }
        return true;
    }

    void NativeDesktopUpdateSaveCatalogSummary(
        NativeDesktopRuntimeSummary* summary,
        bool startup)
    {
        if (summary == nullptr)
        {
            return;
        }

        NativeDesktopRefreshPersistedSaveCatalog();
        const int saveCount = NativeDesktopGetSaveCount();
        const unsigned int saveBytes = NativeDesktopGetSaveDataSize();
        if (startup)
        {
            summary->saveCatalogAtStartup = saveCount;
            summary->saveCatalogBytesAtStartup = saveBytes;
            return;
        }

        summary->saveCatalogAfterSave = saveCount;
        summary->saveCatalogBytesAfterSave = saveBytes;
    }

    class NativeDesktopLeaderboardManager : public LeaderboardManager
    {
    public:
        void Tick() override {}
        bool OpenSession() override { return true; }
        void CloseSession() override {}
        void DeleteSession() override {}
        bool WriteStats(unsigned int viewCount, ViewIn views) override
        {
            (void)viewCount;
            (void)views;
            return true;
        }
        void FlushStats() override {}
        void CancelOperation() override {}
        bool isIdle() override { return true; }
    };

    void NativeDesktopRunBootstrapFrames(
        int frameCount,
        NativeDesktopRuntimeSummary* summary)
    {
        if (summary != nullptr)
        {
            summary->bootstrapFramesRequested = frameCount;
        }

        for (int frame = 0; frame < frameCount; ++frame)
        {
            std::fprintf(
                stderr,
                "NativeDesktop bootstrap: frame %d begin\n",
                frame);
            g_NativeDesktopInputReplay.Apply(
                NativeDesktopInputFramePhase::Bootstrap,
                frame,
                summary);
            ui.tick();
            ui.render();
            g_KBMInput.EndFrame();
            std::fprintf(
                stderr,
                "NativeDesktop bootstrap: frame %d end\n",
                frame);
            if (summary != nullptr)
            {
                summary->bootstrapFramesCompleted += 1;
            }
        }
    }

    void NativeDesktopRunFrontendFrame(
        int frame,
        NativeDesktopRuntimeSummary* summary)
    {
        g_NativeDesktopInputReplay.Apply(
            NativeDesktopInputFramePhase::Frontend,
            frame,
            summary);
        g_NetworkManager.DoWork();
        ui.tick();
        ui.render();
        g_NetworkManager.DoWork();
        g_KBMInput.EndFrame();
        NativeDesktopUpdateRuntimeObservation(summary);
    }

    void NativeDesktopConfigureLocalWorld(NetworkGameInitData* param)
    {
        app.SetGameHostOption(eGameHostOption_Difficulty, 0);
        app.SetGameHostOption(eGameHostOption_FriendsOfFriends, 0);
        app.SetGameHostOption(eGameHostOption_Gamertags, 1);
        app.SetGameHostOption(eGameHostOption_BedrockFog, 1);
        app.SetGameHostOption(
            eGameHostOption_GameType,
            GameType::CREATIVE->getId());
        app.SetGameHostOption(eGameHostOption_LevelType, 0);
        app.SetGameHostOption(eGameHostOption_Structures, 1);
        app.SetGameHostOption(eGameHostOption_BonusChest, 0);
        app.SetGameHostOption(eGameHostOption_PvP, 1);
        app.SetGameHostOption(eGameHostOption_TrustPlayers, 1);
        app.SetGameHostOption(eGameHostOption_FireSpreads, 1);
        app.SetGameHostOption(eGameHostOption_TNT, 1);
        app.SetGameHostOption(eGameHostOption_HostCanFly, 1);
        app.SetGameHostOption(eGameHostOption_HostCanChangeHunger, 1);
        app.SetGameHostOption(eGameHostOption_HostCanBeInvisible, 1);

        param->seed = 0;
        param->saveData = nullptr;
        param->levelName = L"NativeDesktopTestWorld";
        param->settings = app.GetGameHostOption(eGameHostOption_All);
    }

    C4JThread* NativeDesktopStartLocalGame(
        NativeDesktopRuntimeSummary* summary)
    {
        Minecraft* minecraft = Minecraft::GetInstance();
        if (minecraft == nullptr || minecraft->user == nullptr)
        {
            std::fprintf(
                stderr,
                "NativeDesktop gameplay: Minecraft instance unavailable\n");
            return nullptr;
        }

        std::fprintf(stderr, "NativeDesktop gameplay: configure local world\n");
        app.setLevelGenerationOptions(nullptr);
        app.ReleaseSaveThumbnail();
        ProfileManager.SetLockedProfile(0);
        minecraft->user->name = g_NativeDesktopUsernameW;
        MinecraftServer::resetFlags();
        app.SetTutorialMode(false);
        app.SetCorruptSaveDeleted(false);
        app.ClearTerrainFeaturePosition();

        NativeDesktopResetSaveData();
        NativeDesktopSetSaveTitle(L"NativeDesktopTestWorld");

        NetworkGameInitData* param = new NetworkGameInitData();
        NativeDesktopConfigureLocalWorld(param);

        g_NetworkManager.HostGame(0, false, true, 255, 0);
        g_NetworkManager.FakeLocalPlayerJoined();

        LoadingInputParams* loadingParams = new LoadingInputParams();
        loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
        loadingParams->lpParam = static_cast<LPVOID>(param);

        app.SetAutosaveTimerTime();

        C4JThread* thread = new C4JThread(
            loadingParams->func,
            loadingParams->lpParam,
            "NativeDesktopRunNetworkGame",
            256 * 1024);
        thread->Run();
        std::fprintf(stderr, "NativeDesktop gameplay: network thread started\n");
        if (summary != nullptr)
        {
            summary->gameplayThreadStarted = true;
        }
        return thread;
    }

    bool NativeDesktopWaitForGameplay(
        C4JThread* networkThread,
        int gameplayWaitMs,
        NativeDesktopRuntimeSummary* summary)
    {
        int elapsedMs = 0;
        while (elapsedMs < gameplayWaitMs)
        {
            NativeDesktopRunFrontendFrame(elapsedMs / 50, summary);

            Minecraft* minecraft = Minecraft::GetInstance();
            if (app.GetGameStarted() &&
                minecraft != nullptr &&
                minecraft->level != nullptr &&
                minecraft->player != nullptr)
            {
                std::fprintf(
                    stderr,
                    "NativeDesktop gameplay: ready after %d ms\n",
                    elapsedMs);
                if (summary != nullptr)
                {
                    summary->gameplayReady = true;
                    summary->gameplayReadyAfterMs = elapsedMs;
                }
                return true;
            }

            if (networkThread != nullptr &&
                !networkThread->isRunning() &&
                !app.GetGameStarted())
            {
                std::fprintf(
                    stderr,
                    "NativeDesktop gameplay: network thread exited early\n");
                return false;
            }

            C4JThread::Sleep(50);
            elapsedMs += 50;
        }

        std::fprintf(stderr, "NativeDesktop gameplay: wait timed out\n");
        return false;
    }

    void NativeDesktopRunGameplayFrames(
        int frameCount,
        NativeDesktopRuntimeSummary* summary)
    {
        if (summary != nullptr)
        {
            summary->gameplayFramesRequested = frameCount;
        }

        Minecraft* minecraft = Minecraft::GetInstance();
        for (int frame = 0; frame < frameCount; ++frame)
        {
            std::fprintf(
                stderr,
                "NativeDesktop gameplay: frame %d begin\n",
                frame);
            g_NativeDesktopInputReplay.Apply(
                NativeDesktopInputFramePhase::Gameplay,
                frame,
                summary);
            g_NetworkManager.DoWork();
            if (minecraft != nullptr)
            {
                minecraft->run_middle();
            }
            ui.tick();
            ui.render();
            g_NetworkManager.DoWork();
            g_KBMInput.EndFrame();
            NativeDesktopUpdateRuntimeObservation(summary);
            std::fprintf(
                stderr,
                "NativeDesktop gameplay: frame %d end\n",
                frame);
            if (summary != nullptr)
            {
                summary->gameplayFramesCompleted += 1;
            }
        }
    }

    bool NativeDesktopRequestGameplaySave(
        int waitMs,
        NativeDesktopRuntimeSummary* summary)
    {
        if (summary != nullptr)
        {
            summary->saveRequested = true;
        }

        int primaryPad = ProfileManager.GetPrimaryPad();
        if (primaryPad < 0 || primaryPad >= XUSER_MAX_COUNT)
        {
            primaryPad = 0;
        }

        std::fprintf(stderr, "NativeDesktop save: request begin\n");
        app.SetXuiServerAction(primaryPad, eXuiServerAction_SaveGame);

        int elapsedMs = 0;
        const int clampedWaitMs = std::max(waitMs, 1000);
        while (elapsedMs < clampedWaitMs && !MinecraftServer::serverHalted())
        {
            NativeDesktopTickSaves();
            NativeDesktopUpdateRuntimeObservation(summary);
            if (app.GetXuiServerAction(primaryPad) == eXuiServerAction_Idle)
            {
                if (summary != nullptr)
                {
                    summary->saveCompleted = true;
                }
                break;
            }

            C4JThread::Sleep(10);
            elapsedMs += 10;
        }

        unsigned long long persistedBytes = 0;
        const bool persisted =
            NativeDesktopPersistedSaveStats(&persistedBytes);
        if (summary != nullptr)
        {
            summary->savePersisted = persisted;
            summary->savePersistedBytes = persistedBytes;
        }
        if (persisted)
        {
            NativeDesktopSetSaveExists(true);
        }
        NativeDesktopUpdateSaveCatalogSummary(summary, false);

        std::fprintf(
            stderr,
            "NativeDesktop save: request %s persisted=%d bytes=%llu\n",
            summary != nullptr && summary->saveCompleted
                ? "complete"
                : "timeout",
            persisted ? 1 : 0,
            persistedBytes);
        return summary != nullptr &&
            summary->saveCompleted &&
            persisted &&
            persistedBytes > 0;
    }

    bool NativeDesktopWaitForGameplayThread(
        C4JThread* gameplayThread,
        int waitMs)
    {
        if (gameplayThread == nullptr)
        {
            return true;
        }

        int elapsedMs = 0;
        while (elapsedMs < waitMs)
        {
            DWORD result = gameplayThread->WaitForCompletion(20);
            if (result == WAIT_OBJECT_0)
            {
                std::fprintf(
                    stderr,
                    "NativeDesktop gameplay: network thread stopped "
                    "after %d ms\n",
                    elapsedMs);
                return true;
            }
            if (result == WAIT_FAILED)
            {
                std::fprintf(
                    stderr,
                    "NativeDesktop gameplay: network thread wait failed\n");
                return false;
            }

            ProfileManager.Tick();
            NativeDesktopTickSaves();
            InputManager.Tick();
            RenderManager.Tick();
            g_NetworkManager.DoWork();
            C4JThread::Sleep(30);
            elapsedMs += 50;
        }

        std::fprintf(
            stderr,
            "NativeDesktop gameplay: network thread stop timed out\n");
        return false;
    }

    bool NativeDesktopShutdownLocalGame(
        C4JThread* gameplayThread,
        NativeDesktopRuntimeSummary* summary)
    {
        std::fprintf(stderr, "NativeDesktop gameplay: shutdown begin\n");
        if (summary != nullptr)
        {
            summary->shutdownRequested = true;
        }
        MinecraftServer::HaltServer();

        if (!g_NetworkManager.LeaveGame(false))
        {
            std::fprintf(stderr, "NativeDesktop gameplay: leave game failed\n");
            return false;
        }
        std::fprintf(stderr, "NativeDesktop gameplay: leave game complete\n");
        if (summary != nullptr)
        {
            summary->leaveGameComplete = true;
        }

        bool networkStopped = NativeDesktopWaitForGameplayThread(
            gameplayThread,
            kNativeDesktopShutdownWaitMs);
        if (summary != nullptr)
        {
            summary->networkThreadStopped = networkStopped;
        }

        if (networkStopped && gameplayThread != nullptr)
        {
            delete gameplayThread;
        }

        if (!networkStopped)
        {
            std::fprintf(stderr, "NativeDesktop gameplay: shutdown timeout\n");
            return false;
        }

        g_NetworkManager.LeaveGame(false);
        std::fprintf(stderr, "NativeDesktop gameplay: shutdown complete\n");
        if (summary != nullptr)
        {
            summary->shutdownComplete = true;
        }
        return true;
    }

    void NativeDesktopPrintRuntimeSummary(
        const NativeDesktopRuntimeSummary& summary)
    {
        std::fprintf(
            stderr,
            "NativeDesktop runtime summary: "
            "phase=%s "
            "failure=%s "
            "exitCode=%d "
            "startup.mediaArchive=%d "
            "startup.stringTable=%d "
            "startup.clipboard=%d "
            "startup.xuid=%d "
            "startup.profile=%d "
            "startup.networkManager=%d "
            "startup.qnet=%d "
            "startup.localPlayer=%d "
            "startup.minecraftRuntime=%d "
            "startup.ui=%d "
            "startup.uiSkinLibraries=%d "
            "startup.texturePacks=%d "
            "startup.bundledDLC=%d "
            "startupComplete=%d "
            "bootstrapFrames=%d/%d "
            "gameplayThreadStarted=%d "
            "gameplayReady=%d "
            "gameplayReadyAfterMs=%d "
            "gameplayFrames=%d/%d "
            "shutdownRequested=%d "
            "leaveGameComplete=%d "
            "networkThreadStopped=%d "
            "shutdownComplete=%d "
            "loopComplete=%d "
            "runtimeHealthy=%d "
            "save.loadedAtStartup=%d "
            "save.requested=%d "
            "save.completed=%d "
            "save.persisted=%d "
            "save.persistedBytes=%llu "
            "save.catalogAtStartup=%d "
            "save.catalogBytesAtStartup=%u "
            "save.catalogAfterSave=%d "
            "save.catalogBytesAfterSave=%u "
            "runtime.gameStarted=%d "
            "runtime.levelReady=%d "
            "runtime.playerReady=%d "
            "runtime.screenCleared=%d "
            "runtime.menuObserved=%d "
            "runtime.menuClosed=%d "
            "qnet.hostObserved=%d "
            "qnet.gameplayObserved=%d "
            "qnet.playersMax=%d "
            "qnet.lastState=%d "
            "input.scriptLoaded=%d "
            "input.scriptEvents=%d "
            "input.invalidEvents=%d "
            "input.appliedEvents=%d "
            "input.lastAppliedPhase=%s "
            "input.lastAppliedFrame=%d "
            "input.any=%d "
            "input.kbmActive=%d "
            "input.mouse=%d,%d\n",
            summary.phase,
            summary.failure,
            summary.exitCode,
            summary.mediaArchiveReady ? 1 : 0,
            summary.stringTableReady ? 1 : 0,
            summary.clipboardReady ? 1 : 0,
            summary.xuidReady ? 1 : 0,
            summary.profileReady ? 1 : 0,
            summary.networkManagerReady ? 1 : 0,
            summary.qnetReady ? 1 : 0,
            summary.localPlayerReady ? 1 : 0,
            summary.minecraftRuntimeReady ? 1 : 0,
            summary.uiReady ? 1 : 0,
            summary.uiSkinLibrariesCreated,
            summary.texturePackCountMax,
            summary.bundledDLCReady ? 1 : 0,
            summary.startupComplete ? 1 : 0,
            summary.bootstrapFramesCompleted,
            summary.bootstrapFramesRequested,
            summary.gameplayThreadStarted ? 1 : 0,
            summary.gameplayReady ? 1 : 0,
            summary.gameplayReadyAfterMs,
            summary.gameplayFramesCompleted,
            summary.gameplayFramesRequested,
            summary.shutdownRequested ? 1 : 0,
            summary.leaveGameComplete ? 1 : 0,
            summary.networkThreadStopped ? 1 : 0,
            summary.shutdownComplete ? 1 : 0,
            summary.loopComplete ? 1 : 0,
            summary.runtimeHealthy ? 1 : 0,
            summary.saveLoadedAtStartup ? 1 : 0,
            summary.saveRequested ? 1 : 0,
            summary.saveCompleted ? 1 : 0,
            summary.savePersisted ? 1 : 0,
            summary.savePersistedBytes,
            summary.saveCatalogAtStartup,
            summary.saveCatalogBytesAtStartup,
            summary.saveCatalogAfterSave,
            summary.saveCatalogBytesAfterSave,
            summary.gameplayGameStarted ? 1 : 0,
            summary.gameplayLevelReady ? 1 : 0,
            summary.gameplayPlayerReady ? 1 : 0,
            summary.gameplayScreenCleared ? 1 : 0,
            summary.gameplayMenuStateObserved ? 1 : 0,
            summary.gameplayMenuClosed ? 1 : 0,
            summary.qnetHostObserved ? 1 : 0,
            summary.qnetGameplayObserved ? 1 : 0,
            summary.qnetPlayerCountMax,
            summary.qnetLastState,
            summary.inputScriptLoaded ? 1 : 0,
            summary.inputScriptEvents,
            summary.inputInvalidEvents,
            summary.inputAppliedEvents,
            summary.inputLastAppliedPhase,
            summary.inputLastAppliedFrame,
            summary.inputHasAny ? 1 : 0,
            summary.inputKbmActive ? 1 : 0,
            summary.inputMouseX,
            summary.inputMouseY);
    }

    int NativeDesktopFinishRuntime(
        NativeDesktopRuntimeSummary* summary,
        int exitCode,
        const char* phase,
        const char* failure)
    {
        if (summary != nullptr)
        {
            summary->exitCode = exitCode;
            summary->phase = phase;
            summary->failure = failure;
            summary->uiSkinLibrariesCreated =
                NativeDesktopGetIggyLibraryCreateCount();
            NativeDesktopUpdateRuntimeObservation(summary);
            g_NativeDesktopInputReplay.UpdateSummary(summary);
            summary->runtimeHealthy =
                exitCode == 0 &&
                summary->clipboardReady &&
                summary->startupComplete &&
                summary->gameplayReady &&
                summary->saveCompleted &&
                summary->savePersisted &&
                summary->shutdownComplete &&
                summary->loopComplete;
            NativeDesktopPrintRuntimeSummary(*summary);
        }
        return exitCode;
    }

    void NativeDesktopInitialiseProfile()
    {
        ProfileManager.Initialise(
            TITLEID_MINECRAFT,
            app.m_dwOfferID,
            PROFILE_VERSION_10,
            kNativeDesktopProfileValues,
            kNativeDesktopProfileSettings,
            g_NativeDesktopProfileSettingsA,
            app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
            &app.uiGameDefinedDataChangedBitmask);
        ProfileManager.SetDefaultOptionsCallback(
            &CConsoleMinecraftApp::DefaultOptionsCallback,
            static_cast<LPVOID>(&app));
        app.InitGameSettings();
    }

    void NativeDesktopInitialiseMainThreadRuntimeStorage()
    {
        Tesselator::CreateNewThreadStorage(1024 * 1024);
        AABB::CreateNewThreadStorage();
        Vec3::CreateNewThreadStorage();
        IntCache::CreateNewThreadStorage();
        Compression::CreateNewThreadStorage();
        OldChunkStorage::CreateNewThreadStorage();
        Level::enableLightingCache();
        Tile::CreateNewThreadStorage();
    }

    bool NativeDesktopInitialiseMinecraftRuntime()
    {
        NativeDesktopInitialiseMainThreadRuntimeStorage();
        std::fprintf(stderr, "NativeDesktop bootstrap: thread storage ready\n");
        std::fprintf(stderr, "NativeDesktop bootstrap: Minecraft::main begin\n");
        Minecraft::main();
        std::fprintf(stderr, "NativeDesktop bootstrap: Minecraft::main returned\n");
        Minecraft* minecraft = Minecraft::GetInstance();
        std::fprintf(stderr, "NativeDesktop bootstrap: Minecraft::GetInstance returned\n");
        if (minecraft == nullptr)
        {
            std::fprintf(
                stderr,
                "NativeDesktop bootstrap: Minecraft::main failed\n");
            return false;
        }

        std::fprintf(stderr, "NativeDesktop bootstrap: game settings begin\n");
        app.InitGameSettings();
        std::fprintf(stderr, "NativeDesktop bootstrap: tips begin\n");
        app.InitialiseTips();
        std::fprintf(stderr, "NativeDesktop bootstrap: Minecraft::main ready\n");
        return true;
    }
}

LeaderboardManager* LeaderboardManager::m_instance =
    new NativeDesktopLeaderboardManager();

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    std::fprintf(stderr, "NativeDesktop bootstrap: start\n");
    NativeDesktopRuntimeConfig runtimeConfig = NativeDesktopLoadRuntimeConfig();
    NativeDesktopRuntimeSummary runtimeSummary;
    g_NativeDesktopInputReplay.Load(runtimeConfig.inputScript);
    g_NativeDesktopInputReplay.UpdateSummary(&runtimeSummary);
    std::fprintf(
        stderr,
        "NativeDesktop bootstrap: config bootstrapFrames=%d "
        "gameplayFrames=%d waitMs=%d\n",
        runtimeConfig.bootstrapFrames,
        runtimeConfig.gameplayFrames,
        runtimeConfig.gameplayWaitMs);
    NativeDesktopSetWorkingDirectory(argv[0]);
    runtimeSummary.saveLoadedAtStartup =
        NativeDesktopPersistedSaveStats(&runtimeSummary.savePersistedBytes);
    NativeDesktopUpdateSaveCatalogSummary(&runtimeSummary, true);
    app.loadMediaArchive();
    runtimeSummary.mediaArchiveReady = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: media archive ready\n");
    app.loadStringTable();
    runtimeSummary.stringTableReady = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: string table ready\n");
    const std::wstring clipboardProbe = L"NativeDesktopClipboardProbe";
    Screen::setClipboard(clipboardProbe);
    runtimeSummary.clipboardReady =
        Screen::getClipboard() == clipboardProbe;
    std::fprintf(
        stderr,
        "NativeDesktop bootstrap: clipboard %s\n",
        runtimeSummary.clipboardReady ? "ready" : "unavailable");
    PlayerUID resolvedXuid = NativeDesktopXuid::ResolvePersistentXuid();
    runtimeSummary.xuidReady =
        NativeDesktopXuid::IsPersistedUidValid(resolvedXuid);
    std::fprintf(
        stderr,
        "NativeDesktop bootstrap: xuid %s\n",
        runtimeSummary.xuidReady ? "ready" : "unavailable");
    NativeDesktopInitialiseProfile();
    runtimeSummary.profileReady = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: profile ready\n");
    g_NetworkManager.Initialise();
    runtimeSummary.networkManagerReady = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: network manager ready\n");
    IQNet qnet;
    runtimeSummary.qnetReady = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: qnet ready\n");
    qnet.AddLocalPlayerByUserIndex(0);
    runtimeSummary.localPlayerReady = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: local player ready\n");
    if (!NativeDesktopInitialiseMinecraftRuntime())
    {
        return NativeDesktopFinishRuntime(
            &runtimeSummary,
            1,
            "startup",
            "minecraftRuntime");
    }
    runtimeSummary.minecraftRuntimeReady = true;
    app.StartInstallDLCProcess(0);
    NativeDesktopUpdateRuntimeObservation(&runtimeSummary);
    std::fprintf(stderr, "NativeDesktop bootstrap: bundled DLC checked\n");
    ui.init(g_iScreenWidth, g_iScreenHeight);
    runtimeSummary.uiReady = true;
    runtimeSummary.uiSkinLibrariesCreated =
        NativeDesktopGetIggyLibraryCreateCount();
    runtimeSummary.startupComplete = true;
    std::fprintf(stderr, "NativeDesktop bootstrap: ui ready\n");
    NativeDesktopRunBootstrapFrames(
        runtimeConfig.bootstrapFrames,
        &runtimeSummary);
    C4JThread* gameplayThread = NativeDesktopStartLocalGame(&runtimeSummary);
    if (gameplayThread == nullptr ||
        !NativeDesktopWaitForGameplay(
            gameplayThread,
            runtimeConfig.gameplayWaitMs,
            &runtimeSummary))
    {
        (void)NativeDesktopShutdownLocalGame(gameplayThread, &runtimeSummary);
        return NativeDesktopFinishRuntime(
            &runtimeSummary,
            1,
            "gameplay",
            "gameplayReady");
    }
    NativeDesktopRunGameplayFrames(
        runtimeConfig.gameplayFrames,
        &runtimeSummary);
    if (!NativeDesktopRequestGameplaySave(
            runtimeConfig.gameplayWaitMs,
            &runtimeSummary))
    {
        (void)NativeDesktopShutdownLocalGame(gameplayThread, &runtimeSummary);
        return NativeDesktopFinishRuntime(
            &runtimeSummary,
            1,
            "save",
            "savePersisted");
    }
    if (!NativeDesktopShutdownLocalGame(gameplayThread, &runtimeSummary))
    {
        return NativeDesktopFinishRuntime(
            &runtimeSummary,
            1,
            "shutdown",
            "shutdown");
    }
    runtimeSummary.loopComplete = true;
    NativeDesktopFinishRuntime(
        &runtimeSummary,
        0,
        "complete",
        "none");
    std::fprintf(stderr, "NativeDesktop gameplay: complete\n");
    std::fprintf(stderr, "NativeDesktop bootstrap: loop complete\n");
    app.DebugPrintf("Minecraft NativeDesktop client bootstrap initialized.\n");
    return 0;
}
