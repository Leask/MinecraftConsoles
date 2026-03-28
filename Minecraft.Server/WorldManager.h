#pragma once

#include "Common/DedicatedServerWorldTypes.h"
#include "ServerProperties.h"

namespace ServerRuntime
{
	/** Tick callback used while waiting on async storage/network work */
	typedef void (*WorldManagerTickProc)();

	/**
	 * **Bootstrap Target World For Server Startup**
	 *
	 * Resolves whether the target world should be loaded from an existing save or created as new
	 * - Applies `level-name` and `level-id` from `server.properties`
	 * - Loads when a matching save exists
	 * - Creates a new world context only when no save matches
	 * サーバー起動時のロードか新規作成かを確定する
	 *
	 * @param config Normalized `server.properties` values
	 * @param actionPad padId used by async storage APIs
	 * @param tickProc Tick callback run while waiting for async completion
	 * @return Bootstrap result including whether save data was loaded
	 */
	WorldBootstrapResult BootstrapWorldForServer(
		const ServerPropertiesConfig &config,
		int actionPad,
		WorldManagerTickProc tickProc);

}
