#include "stdafx.h"

#include "WorldManager.h"

#include "ServerLogger.h"
#include "Common\\DedicatedServerWorldBootstrap.h"
#include "Common\\DedicatedServerWorldLoadPipeline.h"
#include "Common\\DedicatedServerWorldLoadStorage.h"
#include "Common\\ServerStoragePaths.h"
#include <lce_filesystem/lce_filesystem.h>

namespace ServerRuntime
{
class DedicatedServerWorldLoadStorageRuntimeGuard
{
public:
	explicit DedicatedServerWorldLoadStorageRuntimeGuard(
		DedicatedServerWorldLoadStorageRuntime *runtime)
		: m_runtime(runtime)
	{
	}

	~DedicatedServerWorldLoadStorageRuntimeGuard()
	{
		DestroyDedicatedServerWorldLoadStorageRuntime(m_runtime);
	}

private:
	DedicatedServerWorldLoadStorageRuntime *m_runtime;
};

static void LogSaveFilename(const char *prefix, const std::string &saveFilename)
{
	LogInfof("world-io", "%s: %s", (prefix != NULL) ? prefix : "save-filename", saveFilename.c_str());
}

/**
 * Verifies a directory exists and creates it when missing
 * - Treats an existing non-directory path as failure
 * - Returns whether the directory had to be created
 * ディレクトリ存在保証の補助処理
 */
static bool EnsureDirectoryExists(const std::string &directoryPath, bool *outCreated)
{
	if (outCreated != NULL)
	{
		*outCreated = false;
	}
	if (directoryPath.empty())
	{
		return false;
	}

	if (DirectoryExists(directoryPath.c_str()))
	{
		return true;
	}

	if (FileExists(directoryPath.c_str()))
	{
		LogErrorf(
			"world-io",
			"path exists but is not a directory: %s",
			directoryPath.c_str());
		return false;
	}

	if (CreateDirectoryIfMissing(directoryPath.c_str(), outCreated))
	{
		return true;
	}

	LogErrorf(
		"world-io",
		"failed to create directory %s",
		directoryPath.c_str());
	return false;
}

/**
 * Prepares the save root used by the current platform storage layout
 * - Creates the platform root first because the directory helper is not recursive
 * - Creates `{platform}/GameHDD` when missing before world bootstrap starts
 * プラットフォーム保存先ディレクトリの存在保証
 */
static bool EnsureGameHddRootExists()
{
	const std::string platformDirectory =
		ServerRuntime::GetServerStoragePlatformDirectory();
	const std::string gameHddRoot =
		ServerRuntime::GetServerGameHddRootPath();

	bool windows64Created = false;
	if (!EnsureDirectoryExists(platformDirectory, &windows64Created))
	{
		return false;
	}

	bool gameHddCreated = false;
	if (!EnsureDirectoryExists(gameHddRoot, &gameHddCreated))
	{
		return false;
	}

	if (windows64Created || gameHddCreated)
	{
		LogInfof(
			"world-io",
			"created missing %s storage directories",
			gameHddRoot.c_str());
	}

	return true;
}

/**
 * **Prepare World Save Data For Startup**
 *
 * Searches for a save matching the target world and extracts startup payload when found
 * Match priority:
 * 1. Exact match by `level-id` (`UTF8SaveFilename`)
 * 2. Fallback match by `level-name` against title or filename
 * ワールド一致セーブの探索処理
 *
 * @return
 * - `eWorldSaveLoad_Loaded`: Existing save loaded successfully
 * - `eWorldSaveLoad_NotFound`: No matching save found
 * - `eWorldSaveLoad_Failed`: API failure, corruption, or invalid data
 */
static EDedicatedServerWorldLoadStatus PrepareWorldSaveData(
	const std::wstring &targetWorldName,
	const std::string &targetSaveFilename,
	int actionPad,
	WorldManagerTickProc tickProc,
	const DedicatedServerWorldLoadStorageRuntime &storageRuntime,
	LoadSaveDataThreadParam **outSaveData,
	std::string *outResolvedSaveFilename)
{
	if (outSaveData == NULL)
	{
		return eDedicatedServerWorldLoad_Failed;
	}
	*outSaveData = NULL;
	if (outResolvedSaveFilename != NULL)
	{
		outResolvedSaveFilename->clear();
	}

	LogWorldIO("enumerating saves for configured world");
	const DedicatedServerWorldTarget worldTarget = {
		targetWorldName,
		targetSaveFilename
	};

	DedicatedServerWorldLoadPayload loadPayload = {};
	const EDedicatedServerWorldLoadStatus loadStatus =
		LoadDedicatedServerWorldData(
			worldTarget,
			actionPad,
			tickProc,
			storageRuntime.loadHooks,
			&loadPayload);
	if (loadStatus != eDedicatedServerWorldLoad_Loaded)
	{
		if (loadStatus == eDedicatedServerWorldLoad_NotFound)
		{
			LogWorldIO("no save matched configured world name");
		}
		else
		{
			LogWorldIO("failed to load matched world save");
		}
		return loadStatus;
	}

	LogWorldName("matched save title", loadPayload.matchedTitle);
	if (!loadPayload.matchedFilename.empty())
	{
		LogWorldName("matched save filename", loadPayload.matchedFilename);
	}

	if (outResolvedSaveFilename != NULL)
	{
		*outResolvedSaveFilename = loadPayload.resolvedSaveId;
	}

	ApplyDedicatedServerWorldStoragePlan(
		BuildLoadedDedicatedServerWorldStoragePlan(
			worldTarget,
			loadPayload.resolvedSaveId),
		storageRuntime.storageHooks);

	*outSaveData = new LoadSaveDataThreadParam(
		loadPayload.bytes.data(),
		static_cast<unsigned int>(loadPayload.bytes.size()),
		loadPayload.matchedTitle);
	LogWorldIO("prepared save data payload for server startup");
	return loadStatus;
}

/**
 * **Bootstrap World State For Server Startup**
 *
 * Determines final world startup state
 * - Returns loaded save data when an existing save is found
 * - Prepares a new world context when not found
 * - Returns `Failed` when startup should be aborted
 * サーバー起動時のワールド確定処理
 */
WorldBootstrapResult BootstrapWorldForServer(
	const ServerPropertiesConfig &config,
	int actionPad,
	WorldManagerTickProc tickProc)
{
	WorldBootstrapResult result;
	if (!EnsureGameHddRootExists())
	{
		LogErrorf(
			"world-io",
			"failed to prepare %s storage root",
			GetServerGameHddRootPath().c_str());
		return result;
	}

	const DedicatedServerWorldTarget worldTarget =
		ResolveDedicatedServerWorldTarget(config);
	const std::wstring &targetWorldName = worldTarget.worldName;
	const std::string &targetSaveFilename = worldTarget.saveId;
	DedicatedServerWorldLoadStorageRuntime storageRuntime =
		CreateDedicatedServerPlatformWorldLoadStorageRuntime();
	DedicatedServerWorldLoadStorageRuntimeGuard storageRuntimeGuard(
		&storageRuntime);
	const DedicatedServerWorldStorageHooks &storageHooks =
		storageRuntime.storageHooks;

	LogWorldName("configured level-name", targetWorldName);
	if (!targetSaveFilename.empty())
	{
		LogSaveFilename("configured level-id", targetSaveFilename);
	}

	ApplyDedicatedServerWorldStoragePlan(
		BuildConfiguredDedicatedServerWorldStoragePlan(worldTarget),
		storageHooks);

	LoadSaveDataThreadParam *loadedSaveData = NULL;
	std::string loadedSaveFilename;
	const EDedicatedServerWorldLoadStatus worldLoadResult =
		PrepareWorldSaveData(
			targetWorldName,
			targetSaveFilename,
			actionPad,
			tickProc,
			storageRuntime,
			&loadedSaveData,
			&loadedSaveFilename);
	result = BuildDedicatedServerWorldBootstrapResult(
		worldLoadResult,
		loadedSaveData,
		worldTarget,
		loadedSaveFilename);
	if (result.status == eWorldBootstrap_Loaded)
	{
		LogStartupStep("loading configured world from save data");
	}
	else if (result.status == eWorldBootstrap_CreatedNew)
	{
		// Create a new context only when no matching save exists
		// Fix saveId here so the next startup writes to the same location
		LogStartupStep("configured world not found; creating new world");
		LogWorldIO("creating new world save context");
		ApplyDedicatedServerWorldStoragePlan(
			BuildCreatedDedicatedServerWorldStoragePlan(worldTarget),
			storageHooks);
	}

	return result;
}
}
