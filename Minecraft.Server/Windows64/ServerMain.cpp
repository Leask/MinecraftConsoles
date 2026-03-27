#include "stdafx.h"

#include "Common/App_Defines.h"
#include "Common/Network/GameNetworkManager.h"
#include "Input.h"
#include "Minecraft.h"
#include "MinecraftServer.h"
#include "..\Common\DedicatedServerBootstrap.h"
#include "..\Common\DedicatedServerGameplayLoop.h"
#include "..\Common\DedicatedServerHostedGameRuntime.h"
#include "..\Common\DedicatedServerLifecycle.h"
#include "..\Common\DedicatedServerOptions.h"
#include "..\Common\DedicatedServerPlatformState.h"
#include "..\Common\DedicatedServerPlatformRuntime.h"
#include "..\Common\DedicatedServerRunHooks.h"
#include "..\Common\DedicatedServerRuntime.h"
#include "..\Common\DedicatedServerSignalHandlers.h"
#include "..\Common\DedicatedServerSignalState.h"
#include "..\Common\DedicatedServerShutdownPlan.h"
#include "..\Common\DedicatedServerSessionConfig.h"
#include "..\Common\DedicatedServerWorldBootstrap.h"
#include "..\ServerLogger.h"
#include "..\ServerLogManager.h"
#include "..\ServerProperties.h"
#include "..\ServerShutdown.h"
#include "..\WorldManager.h"
#include "..\Console\ServerCli.h"
#include <lce_process/lce_process.h>
#include <lce_time/lce_time.h>
#include "Tesselator.h"
#include "Windows64/4JLibs/inc/4J_Render.h"
#include "Windows64/Windows64DedicatedServerRuntime.h"
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
#include "../../Minecraft.World/Random.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cwchar>

static const int kServerActionPad = 0;

/**
 * Calls dedicated bootstrap shutdown automatically once native/bootstrap
 * environment initialization succeeded.
 * 共有ブートストラップ初期化後のShutdownを自動化する
 */
class DedicatedServerBootstrapGuard
{
public:
	DedicatedServerBootstrapGuard()
		: m_context(nullptr)
	{
	}

	void Activate(ServerRuntime::DedicatedServerBootstrapContext *context)
	{
		m_context = context;
	}

	~DedicatedServerBootstrapGuard()
	{
		ServerRuntime::ShutdownDedicatedServerBootstrapEnvironment(m_context);
	}

private:
	ServerRuntime::DedicatedServerBootstrapContext *m_context;
};

class DedicatedServerLogManagerGuard
{
public:
	DedicatedServerLogManagerGuard()
		: m_active(false)
	{
	}

	void Activate()
	{
		m_active = true;
	}

	void Release()
	{
		m_active = false;
	}

	~DedicatedServerLogManagerGuard()
	{
		if (m_active)
		{
			ServerRuntime::ServerLogManager::Shutdown();
		}
	}

private:
	bool m_active;
};

static void PrintUsage()
{
	std::vector<std::string> usageLines;
	ServerRuntime::BuildDedicatedServerUsageLines(&usageLines);
	for (size_t i = 0; i < usageLines.size(); ++i)
	{
		ServerRuntime::LogInfo("usage", usageLines[i].c_str());
	}
}

using ServerRuntime::LogError;
using ServerRuntime::LogErrorf;
using ServerRuntime::LogInfof;
using ServerRuntime::LogDebugf;
using ServerRuntime::LogStartupStep;
using ServerRuntime::LogWarn;
using ServerRuntime::LogWorldIO;
using ServerRuntime::ServerPropertiesConfig;
using ServerRuntime::TryParseServerLogLevel;
using ServerRuntime::DedicatedServerConfig;
using ServerRuntime::BootstrapWorldForServer;
using ServerRuntime::eWorldBootstrap_CreatedNew;
using ServerRuntime::eWorldBootstrap_Failed;
using ServerRuntime::eWorldBootstrap_Loaded;
using ServerRuntime::WaitForWorldActionIdle;
using ServerRuntime::WorldBootstrapResult;

/**
 * Dedicated Server Entory Point
 *
 * 主な責務:
 * - プロセス/描画/ネットワークの初期化
 * - `WorldManager` によるワールドロードまたは新規作成
 * - メインループと定期オートセーブ実行
 * - 終了時の最終保存と各サブシステムの安全停止
 */
int main(int argc, char **argv)
{
	ServerRuntime::DedicatedServerBootstrapContext bootstrapContext = {};
	DedicatedServerBootstrapGuard bootstrapShutdownGuard;
	DedicatedServerLogManagerGuard logManagerGuard;
	ServerRuntime::ResetDedicatedServerShutdownRequest();

	if (!ServerRuntime::InstallDedicatedServerShutdownSignalHandlers())
	{
		LogWarn(
			"startup",
			"Failed to install dedicated server shutdown handlers.");
	}
	std::string bootstrapError;
	switch (ServerRuntime::PrepareDedicatedServerBootstrapContext(
		argc,
		argv,
		&bootstrapContext,
		&bootstrapError))
	{
	case ServerRuntime::eDedicatedServerBootstrap_Ready:
		break;
	case ServerRuntime::eDedicatedServerBootstrap_ShowHelp:
		PrintUsage();
		return 0;
	case ServerRuntime::eDedicatedServerBootstrap_Failed:
	default:
		if (!bootstrapError.empty())
		{
			LogError("startup", bootstrapError.c_str());
		}
		PrintUsage();
		return 1;
	}

	LogStartupStep("initializing process state");
	DedicatedServerConfig &config = bootstrapContext.config;
	ServerPropertiesConfig &serverProperties =
		bootstrapContext.serverProperties;
	const ServerRuntime::DedicatedServerRuntimeState &runtimeState =
		bootstrapContext.runtimeState;
	const ServerRuntime::DedicatedServerPlatformState platformState =
		ServerRuntime::BuildDedicatedServerPlatformState(
			runtimeState,
			MINECRAFT_NET_MAX_PLAYERS);
	LogStartupStep("initializing server log manager");
	ServerRuntime::ServerLogManager::Initialize();
	logManagerGuard.Activate();
	LogStartupStep("initializing dedicated access control");
	if (!ServerRuntime::InitializeDedicatedServerBootstrapEnvironment(
		&bootstrapContext,
		&bootstrapError))
	{
		LogError("startup", bootstrapError.c_str());
		return 2;
	}
	bootstrapShutdownGuard.Activate(&bootstrapContext);
	LogInfof("startup", "LAN advertise: %s", serverProperties.lanAdvertise ? "enabled" : "disabled");
	LogInfof("startup", "Whitelist: %s", serverProperties.whiteListEnabled ? "enabled" : "disabled");
	LogInfof("startup", "Spawn protection radius: %d", serverProperties.spawnProtectionRadius);
#ifdef _LARGE_WORLDS
	LogInfof(
		"startup",
		"World size preset: %d (xz=%d, hell-scale=%d)",
		config.worldSize,
		config.worldSizeChunks,
		config.worldHellScale);
#endif

	const ServerRuntime::DedicatedServerPlatformRuntimeStartResult
		windowsRuntimeStartResult =
			ServerRuntime::StartDedicatedServerPlatformRuntime(platformState);
	if (!windowsRuntimeStartResult.ok)
	{
		LogError("startup", windowsRuntimeStartResult.errorMessage.c_str());
		return windowsRuntimeStartResult.exitCode;
	}

	// Read world name and fixed save-id from server.properties
	// Delegate load-vs-create decision to WorldManager
	WorldBootstrapResult worldBootstrap = BootstrapWorldForServer(
		serverProperties,
		kServerActionPad,
		&ServerRuntime::TickDedicatedServerPlatformRuntime);
	NetworkGameInitData param;
	ServerRuntime::ServerCli serverCli;
	ServerRuntime::DedicatedServerCliRunHooksContext serverCliHooksContext =
		{
			&platformState,
			&serverCli
		};
	const ServerRuntime::DedicatedServerRunHooks runHooks =
		ServerRuntime::BuildDedicatedServerCliRunHooks(
			&serverCliHooksContext);
	LogStartupStep("starting hosted network game thread");
	const ServerRuntime::DedicatedServerRunExecutionResult
		runExecution =
			ServerRuntime::ExecuteDedicatedServerRun(
		config,
		&serverProperties,
		worldBootstrap,
		kServerActionPad,
		(new Random())->nextLong(),
		&CGameNetworkManager::RunNetworkGameThreadProc,
		&param,
		runHooks,
		LceGetMonotonicMilliseconds(),
		10);
	if (runExecution.abortedStartup)
	{
		if (runExecution.startup.hostedGameStartup.startupPlan.shouldAbortStartup)
		{
			LogErrorf(
				"startup",
				"Failed to start dedicated server (code %d).",
				runExecution.startup.hostedGameStartup.startupResult);
		}
		ServerRuntime::StopDedicatedServerPlatformRuntime();
		
		return runExecution.abortExitCode;
	}

	LogInfof("shutdown", "Dedicated server stopped");

	LogInfof("shutdown", "Cleaning up and exiting.");
	ServerRuntime::StopDedicatedServerPlatformRuntime();
	logManagerGuard.Release();
	ServerRuntime::ServerLogManager::Shutdown();
	

	return 0;
}
