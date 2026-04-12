#include "stdafx.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include "../Minecraft.h"
#include "../MinecraftServer.h"
#include "../Tesselator.h"
#include "../User.h"
#include "../Common/UI/IUIScene_CreativeMenu.h"
#include "../Common/UI/UIStructs.h"
#include "../Common/Leaderboards/LeaderboardManager.h"
#include "../Common/Network/GameNetworkManager.h"
#include "Network/WinsockNetLayer.h"
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

char g_Win64Username[17] = "Player";
wchar_t g_Win64UsernameW[17] = L"Player";

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

    pSigninInfo->xuid = Win64Xuid::DeriveXuidForPad(
        Win64Xuid::ResolvePersistentXuid(),
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

    if (WinsockNetLayer::IsActive())
    {
        if (!WinsockNetLayer::IsHosting() && !m_isRemote)
        {
            SOCKET socket = WinsockNetLayer::GetLocalSocket(m_smallId);
            if (socket != INVALID_SOCKET)
            {
                WinsockNetLayer::SendOnSocket(socket, pvData, dwDataSize);
            }
        }
        else
        {
            WinsockNetLayer::SendToSmallId(player->m_smallId, pvData, dwDataSize);
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
        Win64Xuid::GetLegacyEmbeddedBaseXuid() + m_smallId);
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

void Win64_SetupRemoteQNetPlayer(
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
    player.m_resolvedXuid = Win64Xuid::DeriveXuidForPad(
        Win64Xuid::ResolvePersistentXuid(),
        static_cast<int>(dwUserIndex));
    if (dwUserIndex == 0)
    {
        wcscpy_s(player.m_gamertag, 32, g_Win64UsernameW);
    }
    else
    {
        swprintf_s(
            player.m_gamertag,
            32,
            L"%ls(%u)",
            g_Win64UsernameW,
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
    m_player[0].m_resolvedXuid = Win64Xuid::GetLegacyEmbeddedHostXuid();
    wcscpy_s(m_player[0].m_gamertag, 32, g_Win64UsernameW);
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
    };

    struct NativeDesktopRuntimeSummary
    {
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
    };

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
            ui.tick();
            ui.render();
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

    void NativeDesktopRunFrontendFrame()
    {
        g_NetworkManager.DoWork();
        ui.tick();
        ui.render();
        g_NetworkManager.DoWork();
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
        minecraft->user->name = g_Win64UsernameW;
        MinecraftServer::resetFlags();
        app.SetTutorialMode(false);
        app.SetCorruptSaveDeleted(false);
        app.ClearTerrainFeaturePosition();

        StorageManager.ResetSaveData();
        StorageManager.SetSaveTitle(L"NativeDesktopTestWorld");

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
            NativeDesktopRunFrontendFrame();

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
            g_NetworkManager.DoWork();
            if (minecraft != nullptr)
            {
                minecraft->run_middle();
            }
            ui.tick();
            ui.render();
            g_NetworkManager.DoWork();
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
            StorageManager.Tick();
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
            "bootstrapFrames=%d/%d "
            "gameplayThreadStarted=%d "
            "gameplayReady=%d "
            "gameplayReadyAfterMs=%d "
            "gameplayFrames=%d/%d "
            "shutdownRequested=%d "
            "leaveGameComplete=%d "
            "networkThreadStopped=%d "
            "shutdownComplete=%d "
            "loopComplete=%d\n",
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
            summary.loopComplete ? 1 : 0);
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
    std::fprintf(
        stderr,
        "NativeDesktop bootstrap: config bootstrapFrames=%d "
        "gameplayFrames=%d waitMs=%d\n",
        runtimeConfig.bootstrapFrames,
        runtimeConfig.gameplayFrames,
        runtimeConfig.gameplayWaitMs);
    NativeDesktopSetWorkingDirectory(argv[0]);
    app.loadMediaArchive();
    std::fprintf(stderr, "NativeDesktop bootstrap: media archive ready\n");
    app.loadStringTable();
    std::fprintf(stderr, "NativeDesktop bootstrap: string table ready\n");
    Win64Xuid::ResolvePersistentXuid();
    std::fprintf(stderr, "NativeDesktop bootstrap: xuid ready\n");
    NativeDesktopInitialiseProfile();
    std::fprintf(stderr, "NativeDesktop bootstrap: profile ready\n");
    g_NetworkManager.Initialise();
    std::fprintf(stderr, "NativeDesktop bootstrap: network manager ready\n");
    IQNet qnet;
    std::fprintf(stderr, "NativeDesktop bootstrap: qnet ready\n");
    qnet.AddLocalPlayerByUserIndex(0);
    std::fprintf(stderr, "NativeDesktop bootstrap: local player ready\n");
    if (!NativeDesktopInitialiseMinecraftRuntime())
    {
        return 1;
    }
    ui.init(g_iScreenWidth, g_iScreenHeight);
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
        NativeDesktopPrintRuntimeSummary(runtimeSummary);
        return 1;
    }
    NativeDesktopRunGameplayFrames(
        runtimeConfig.gameplayFrames,
        &runtimeSummary);
    if (!NativeDesktopShutdownLocalGame(gameplayThread, &runtimeSummary))
    {
        NativeDesktopPrintRuntimeSummary(runtimeSummary);
        return 1;
    }
    runtimeSummary.loopComplete = true;
    NativeDesktopPrintRuntimeSummary(runtimeSummary);
    std::fprintf(stderr, "NativeDesktop gameplay: complete\n");
    std::fprintf(stderr, "NativeDesktop bootstrap: loop complete\n");
    app.DebugPrintf("Minecraft NativeDesktop client bootstrap initialized.\n");
    return 0;
}
