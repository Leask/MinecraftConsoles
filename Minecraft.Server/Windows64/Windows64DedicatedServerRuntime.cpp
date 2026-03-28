#include "stdafx.h"

#include "..\Common\DedicatedServerPlatformRuntime.h"

#include "Common/App_Defines.h"
#include "Common/Network/GameNetworkManager.h"
#include "Input.h"
#include "Minecraft.h"
#include "MinecraftServer.h"
#include "..\ServerLogger.h"
#include "Tesselator.h"
#include "Windows64/4JLibs/inc/4J_Render.h"
#include "Windows64/GameConfig/Minecraft.spa.h"
#include "Windows64/KeyboardMouseInput.h"
#include "Windows64/Network/WinsockNetLayer.h"
#include "Windows64/Windows64_UIController.h"
#include "Common/UI/UI.h"

#include "../../Minecraft.World/AABB.h"
#include "../../Minecraft.World/Vec3.h"
#include "../../Minecraft.World/IntCache.h"
#include "../../Minecraft.World/ChunkSource.h"
#include "../../Minecraft.World/TilePos.h"
#include "../../Minecraft.World/compression.h"
#include "../../Minecraft.World/OldChunkStorage.h"
#include "../../Minecraft.World/net.minecraft.world.level.tile.h"

#include <cwchar>

extern ATOM MyRegisterClass(HINSTANCE hInstance);
extern BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
extern HRESULT InitDevice();
extern void CleanupDevice();
extern void DefineActions(void);

extern HWND g_hWnd;
extern int g_iScreenWidth;
extern int g_iScreenHeight;
extern char g_Win64Username[17];
extern wchar_t g_Win64UsernameW[17];
extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pImmediateContext;
extern IDXGISwapChain* g_pSwapChain;
extern ID3D11RenderTargetView* g_pRenderTargetView;
extern ID3D11DepthStencilView* g_pDepthStencilView;
extern DWORD dwProfileSettingsA[];

namespace
{
    static constexpr int kProfileValueCount = 5;
    static constexpr int kProfileSettingCount = 4;

    void ApplyWindowsDedicatedServerPlatformState(
        const ServerRuntime::DedicatedServerPlatformState &platformState)
    {
        g_iScreenWidth = 1280;
        g_iScreenHeight = 720;

        strncpy_s(
            g_Win64Username,
            sizeof(g_Win64Username),
            platformState.userNameUtf8.c_str(),
            _TRUNCATE);
        wmemset(
            g_Win64UsernameW,
            0,
            sizeof(g_Win64UsernameW) / sizeof(g_Win64UsernameW[0]));
        wcsncpy(
            g_Win64UsernameW,
            platformState.userNameWide.c_str(),
            (sizeof(g_Win64UsernameW) / sizeof(g_Win64UsernameW[0])) - 1);
        g_Win64MultiplayerHost = platformState.multiplayerHost;
        g_Win64MultiplayerJoin = platformState.multiplayerJoin;
        g_Win64MultiplayerPort = platformState.multiplayerPort;
        strncpy_s(
            g_Win64MultiplayerIP,
            sizeof(g_Win64MultiplayerIP),
            platformState.bindIp.c_str(),
            _TRUNCATE);
        g_Win64DedicatedServer = platformState.dedicatedServer;
        g_Win64DedicatedServerPort = platformState.dedicatedServerPort;
        strncpy_s(
            g_Win64DedicatedServerBindIP,
            sizeof(g_Win64DedicatedServerBindIP),
            platformState.bindIp.c_str(),
            _TRUNCATE);
        g_Win64DedicatedServerLanAdvertise = platformState.lanAdvertise;
    }

    int WaitForServerStoppedThreadProc(void *)
    {
        if (g_NetworkManager.ServerStoppedValid())
        {
            g_NetworkManager.ServerStoppedWait();
        }
        return 0;
    }

    bool IsDedicatedServerWorldActionIdleHook(int actionPad, void *)
    {
        return ServerRuntime::IsDedicatedServerWorldActionIdle(actionPad);
    }

    bool IsDedicatedServerGameplayHaltedHook(void *)
    {
        return ServerRuntime::IsDedicatedServerGameplayHalted();
    }

    void TickDedicatedServerPlatformRuntimeHook(void *)
    {
        ServerRuntime::TickDedicatedServerPlatformRuntime();
    }

    void HandleDedicatedServerPlatformActionsHook(void *)
    {
        ServerRuntime::HandleDedicatedServerPlatformActions();
    }
}

namespace ServerRuntime
{
    DedicatedServerPlatformRuntimeStartResult
    StartDedicatedServerPlatformRuntime(
        const DedicatedServerPlatformState &platformState)
    {
        DedicatedServerPlatformRuntimeStartResult result = {};
        result.runtimeName = "Windows64Legacy";
        result.headless = false;
        ApplyWindowsDedicatedServerPlatformState(platformState);

        LogStartupStep("registering hidden window class");
        HINSTANCE hInstance = GetModuleHandle(NULL);
        MyRegisterClass(hInstance);

        LogStartupStep("creating hidden window");
        if (!InitInstance(hInstance, SW_HIDE))
        {
            result.exitCode = 2;
            result.errorMessage = "Failed to create window instance.";
            return result;
        }
        ShowWindow(g_hWnd, SW_HIDE);

        LogStartupStep("initializing graphics device wrappers");
        if (FAILED(InitDevice()))
        {
            CleanupDevice();
            result.exitCode = 2;
            result.errorMessage = "Failed to initialize D3D device.";
            return result;
        }

        LogStartupStep("loading media/string tables");
        app.loadMediaArchive();
        RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);
        app.loadStringTable();
        ui.init(
            g_pd3dDevice,
            g_pImmediateContext,
            g_pRenderTargetView,
            g_pDepthStencilView,
            g_iScreenWidth,
            g_iScreenHeight);

        InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
        g_KBMInput.Init();
        DefineActions();
        InputManager.SetJoypadMapVal(0, 0);
        InputManager.SetKeyRepeatRate(0.3f, 0.2f);

        ProfileManager.Initialise(
            TITLEID_MINECRAFT,
            app.m_dwOfferID,
            PROFILE_VERSION_10,
            kProfileValueCount,
            kProfileSettingCount,
            dwProfileSettingsA,
            app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
            &app.uiGameDefinedDataChangedBitmask);
        ProfileManager.SetDefaultOptionsCallback(
            &CConsoleMinecraftApp::DefaultOptionsCallback,
            (LPVOID)&app);
        ProfileManager.SetDebugFullOverride(true);

        LogStartupStep("initializing network manager");
        g_NetworkManager.Initialise();

        for (size_t i = 0; i < platformState.players.size(); ++i)
        {
            const DedicatedServerPlayerState &playerState =
                platformState.players[i];
            IQNet::m_player[i].m_smallId =
                static_cast<BYTE>(playerState.smallId);
            IQNet::m_player[i].m_isRemote = playerState.isRemote;
            IQNet::m_player[i].m_isHostPlayer = playerState.isHostPlayer;
            wmemset(IQNet::m_player[i].m_gamertag, 0, 32);
            wcsncpy(
                IQNet::m_player[i].m_gamertag,
                playerState.gamertag.c_str(),
                31);
        }
        WinsockNetLayer::Initialize();

        Tesselator::CreateNewThreadStorage(1024 * 1024);
        AABB::CreateNewThreadStorage();
        Vec3::CreateNewThreadStorage();
        IntCache::CreateNewThreadStorage();
        Compression::CreateNewThreadStorage();
        OldChunkStorage::CreateNewThreadStorage();
        Level::enableLightingCache();
        Tile::CreateNewThreadStorage();

        LogStartupStep("creating Minecraft singleton");
        Minecraft::main();
        if (Minecraft::GetInstance() == NULL)
        {
            CleanupDevice();
            result.exitCode = 3;
            result.errorMessage = "Minecraft initialization failed.";
            return result;
        }

        result.ok = true;
        return result;
    }

    DedicatedServerAppSessionApplyResult ApplyDedicatedServerAppSessionPlan(
        const DedicatedServerAppSessionPlan &appSessionPlan)
    {
        DedicatedServerAppSessionApplyResult result = {};
        MinecraftServer::resetFlags();
        if (appSessionPlan.shouldInitGameSettings)
        {
            app.InitGameSettings();
            result.initializedGameSettings = true;
        }

        app.SetTutorialMode(appSessionPlan.tutorialMode);
        app.SetCorruptSaveDeleted(appSessionPlan.corruptSaveDeleted);
        app.SetGameHostOption(eGameHostOption_All, appSessionPlan.hostSettings);
#ifdef _LARGE_WORLDS
        if (appSessionPlan.shouldApplyWorldSize)
        {
            app.SetGameNewWorldSize(appSessionPlan.worldSizeChunks, true);
            app.SetGameNewHellScale(appSessionPlan.worldHellScale);
            result.appliedWorldSize = true;
        }
#endif
        StorageManager.SetSaveDisabled(appSessionPlan.saveDisabled);
        result.applied = true;
        result.saveDisabled = appSessionPlan.saveDisabled;
        return result;
    }

    void TickDedicatedServerPlatformRuntime()
    {
        g_NetworkManager.DoWork();
        ProfileManager.Tick();
        StorageManager.Tick();
    }

    void HandleDedicatedServerPlatformActions()
    {
        app.HandleXuiActions();
    }

    bool IsDedicatedServerWorldActionIdle(int actionPad)
    {
        return app.GetXuiServerAction(actionPad) == eXuiServerAction_Idle;
    }

    void RequestDedicatedServerWorldAutosave(int actionPad)
    {
        app.SetXuiServerAction(actionPad, eXuiServerAction_AutoSaveGame);
    }

    bool WaitForDedicatedServerWorldActionIdle(
        int actionPad,
        DWORD timeoutMs)
    {
        DedicatedServerWorldActionWaitHooks hooks = {};
        hooks.isIdleProc = &IsDedicatedServerWorldActionIdleHook;
        hooks.haltProc = &IsDedicatedServerGameplayHaltedHook;
        hooks.tickProc = &TickDedicatedServerPlatformRuntimeHook;
        hooks.handleActionsProc = &HandleDedicatedServerPlatformActionsHook;
        return WaitForDedicatedServerWorldActionIdleWithHooks(
            actionPad,
            timeoutMs,
            hooks);
    }

    bool HasDedicatedServerGameplayInstance()
    {
        return MinecraftServer::getInstance() != NULL;
    }

    bool IsDedicatedServerAppShutdownRequested()
    {
        return app.m_bShutdown;
    }

    void SetDedicatedServerAppShutdownRequested(bool shutdownRequested)
    {
        app.m_bShutdown = shutdownRequested;
    }

    bool IsDedicatedServerGameplayHalted()
    {
        return MinecraftServer::serverHalted();
    }

    bool IsDedicatedServerStopSignalValid()
    {
        return g_NetworkManager.ServerStoppedValid();
    }

    void EnableDedicatedServerSaveOnExit()
    {
        MinecraftServer *server = MinecraftServer::getInstance();
        if (server != NULL)
        {
            server->setSaveOnExit(true);
        }
    }

    std::uint64_t GetDedicatedServerAutosaveRequestCount()
    {
        return 0;
    }

    std::uint64_t GetDedicatedServerAutosaveCompletionCount()
    {
        return 0;
    }

    void HaltDedicatedServerGameplay()
    {
        MinecraftServer::HaltServer();
    }

    void WaitForDedicatedServerStopSignal()
    {
        C4JThread waitThread(
            &WaitForServerStoppedThreadProc,
            NULL,
            "WaitServerStopped");
        waitThread.Run();
        waitThread.WaitForCompletion(INFINITE);
    }

    void StopDedicatedServerPlatformRuntime()
    {
        WinsockNetLayer::Shutdown();
        LogDebugf("shutdown", "Network layer shutdown complete.");
        g_NetworkManager.Terminate();
        LogDebugf("shutdown", "Network manager terminated.");
        CleanupDevice();
    }
}
