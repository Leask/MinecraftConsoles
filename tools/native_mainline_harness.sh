#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
log_root="${MAINLINE_HARNESS_LOG_DIR:-$repo_root/build/mainline-harness}"
summary_file="$log_root/summary.txt"

server_harness="${SERVER_HARNESS:-$repo_root/tools/headless_server_native_harness.sh}"
server_log_root="${SERVER_HARNESS_LOG_DIR:-$log_root/server}"

local_client_configure_preset="${LOCAL_CLIENT_CONFIGURE_PRESET:-macos-native-client}"
local_client_build_preset="${LOCAL_CLIENT_BUILD_PRESET:-macos-native-client-debug}"
local_client_target="${LOCAL_CLIENT_TARGET:-Minecraft.NativeDesktop.Check}"

remote_host="${REMOTE_HOST:-elm}"
remote_root="${REMOTE_ROOT:-/home/leask/MinecraftConsoles}"
remote_client_configure_preset="${REMOTE_CLIENT_CONFIGURE_PRESET:-linux-native-client}"
remote_client_build_preset="${REMOTE_CLIENT_BUILD_PRESET:-linux-native-client-debug}"
remote_client_target="${REMOTE_CLIENT_TARGET:-Minecraft.NativeDesktop.Check}"

run_server_harness="${RUN_SERVER_HARNESS:-1}"
run_local_client="${RUN_LOCAL_CLIENT:-1}"
run_remote_client="${RUN_REMOTE_CLIENT:-1}"
clean_client_builds="${CLEAN_CLIENT_BUILDS:-1}"

current_step=''

write_summary_header() {
    local git_head
    git_head="$(git -C "$repo_root" rev-parse HEAD)"

    mkdir -p "$log_root"
    cat > "$summary_file" <<EOF
result=running
started_at=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
repo_root=$repo_root
git_head=$git_head
log_root=$log_root
remote_host=$remote_host
remote_root=$remote_root
clean_client_builds=$clean_client_builds
EOF
}

append_summary_line() {
    printf '%s\n' "$1" >> "$summary_file"
}

mark_summary_failed() {
    local exit_code="$1"

    {
        printf 'result=failure\n'
        printf 'failed_step=%s\n' "${current_step:-unknown}"
        printf 'exit_code=%s\n' "$exit_code"
        printf 'finished_at=%s\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    } >> "$summary_file"
}

mark_summary_success() {
    {
        printf 'goal.native_mainline=complete\n'
        printf 'result=success\n'
        printf 'finished_at=%s\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    } >> "$summary_file"
}

on_error() {
    local exit_code="$?"
    mark_summary_failed "$exit_code"
    exit "$exit_code"
}

run_and_log() {
    local name="$1"
    shift

    local log_file="$log_root/$name.log"
    current_step="$name"
    echo "==> $name"
    append_summary_line "step.$name.log=$log_file"
    "$@" > >(tee "$log_file") 2>&1
    append_summary_line "step.$name=ok"
}

assert_not_exists() {
    local path="$1"

    if [[ -e "$repo_root/$path" ]]; then
        echo "Removed-platform path still exists: $path" >&2
        return 1
    fi
}

assert_grep_absent() {
    local pattern="$1"
    shift

    if rg -n -S "$pattern" "$@" > "$log_root/residual-match.txt"; then
        echo "Unexpected residual pattern found: $pattern" >&2
        cat "$log_root/residual-match.txt" >&2
        return 1
    fi
}

assert_grep_present() {
    local pattern="$1"
    shift

    if ! rg -n -S "$pattern" "$@" >/dev/null; then
        echo "Expected pattern not found: $pattern" >&2
        return 1
    fi
}

run_native_only_contract() {
    run_and_log native-only-contract bash -c '
        set -euo pipefail
        cd "$1"
        log_root="$2"

        assert_not_exists() {
            local path="$1"
            if [[ -e "$path" ]]; then
                echo "Removed-platform path still exists: $path" >&2
                return 1
            fi
        }

        assert_not_exists "Minecraft.Client/Windows64"
        assert_not_exists "Minecraft.Server/Windows64"
        assert_not_exists "docker"
        assert_not_exists "docker-compose.dedicated-server.yml"
        assert_not_exists "docker-compose.dedicated-server.mount.yml"
        assert_not_exists "start-dedicated-server.sh"
        assert_not_exists "docker-build-dedicated-server.sh"
        assert_not_exists "Minecraft.Client/ReadMe.txt"
        assert_not_exists "Minecraft.World/ReadMe.txt"

        if rg -n -S "ReadMe\\.txt|VS_STARTUP_PROJECT|WinsockNetLayer|\\bWin64\\b|\\bWIN64_" \
            CMakeLists.txt CMakePresets.json cmake Minecraft.Client/cmake \
            Minecraft.World/cmake Minecraft.Client/NativeDesktop \
            Minecraft.Client/Common/Network > "$log_root/native-only-residual.txt"; then
            echo "Native-only residuals found in active build/native paths" >&2
            cat "$log_root/native-only-residual.txt" >&2
            exit 1
        fi

        if rg -n -S "windows64|durango|orbis|ps3|psvita|xbox|wine|docker" \
            CMakePresets.json .github/workflows > "$log_root/native-only-ci-residual.txt"; then
            echo "Removed-platform CI/build target residuals found" >&2
            cat "$log_root/native-only-ci-residual.txt" >&2
            exit 1
        fi

        if rg -n -S "wcstombs\\(narrow(New)?(IP|Port|Name)" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_JoinMenu.cpp \
            > "$log_root/native-saved-server-locale-conversion.txt"; then
            echo "Native saved-server paths still use locale conversion" >&2
            cat "$log_root/native-saved-server-locale-conversion.txt" >&2
            exit 1
        fi

        if rg -n -S "mbstowcs\\(wDefaultIP|mbstowcs\\(wSaveName, params->saveDetails->UTF8SaveName, 127" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_JoinMenu.cpp \
            > "$log_root/native-load-menu-locale-conversion.txt"; then
            echo "Native load/join paths still use locale conversion" >&2
            cat "$log_root/native-load-menu-locale-conversion.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopWideToUtf8Buffer" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_JoinMenu.cpp >/dev/null
        rg -n -S "NativeDesktopUtf8ToWideBuffer" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_JoinMenu.cpp >/dev/null

        if rg -n -S "CreateFile\\(|GetFileSize\\(|ReadFile\\(|SetFilePointer\\(|wcstombs" \
            Minecraft.Client/ArchiveFile.cpp \
            Minecraft.Client/Common/UI/UITTFFont.cpp \
            > "$log_root/native-client-asset-loader-win32-io.txt"; then
            echo "Native client asset/font loaders still use Win32-shaped I/O" >&2
            cat "$log_root/native-client-asset-loader-win32-io.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopReadFileBytes" \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStubs.h \
            Minecraft.Client/Common/UI/UITTFFont.cpp >/dev/null
        rg -n -S "NativeDesktopReadFileBytesAt" \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStubs.h \
            Minecraft.Client/ArchiveFile.cpp >/dev/null

        if rg -n -S "_WINDOWS64.*_NATIVE_DESKTOP|_NATIVE_DESKTOP.*_WINDOWS64" \
            Minecraft.Client/Minecraft.cpp \
            Minecraft.Client/Minecraft.h \
            Minecraft.Client/Screen.cpp \
            Minecraft.Client/Common/Audio/SoundEngine.h \
            Minecraft.Client/Common/Network/PlatformNetworkManagerStub.cpp \
            Minecraft.Client/Common/UI/UIController.cpp \
            Minecraft.Client/Common/UI/UIController.h \
            Minecraft.Client/Common/UI/UIScene_CraftingMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_CraftingMenu.h \
            Minecraft.Client/Common/UI/UIScene_Intro.cpp \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            > "$log_root/native-client-shared-windows-native-guards.txt"; then
            echo "Active native client code still shares Windows/native guards" >&2
            cat "$log_root/native-client-shared-windows-native-guards.txt" >&2
            exit 1
        fi

        if rg -n -S "HWND|GetClientRect" \
            Minecraft.Client/stubs.cpp \
            > "$log_root/native-client-stub-window-geometry.txt"; then
            echo "Native client stubs still read Win32-shaped window geometry" >&2
            cat "$log_root/native-client-stub-window-geometry.txt" >&2
            exit 1
        fi

        if rg -n -S "CreateFile\\(|GetFileSize\\(|ReadFile\\(|CloseHandle\\(|StorageManager\\.|MountInstalledDLC|UnmountInstalledDLC|WPACK|_DURANGO|_WINDOWS64" \
            Minecraft.Client/Common/GameRules/LevelGenerationOptions.cpp \
            Minecraft.Client/Common/GameRules/LevelGenerationOptions.h \
            > "$log_root/native-client-lgo-win32-io.txt"; then
            echo "Native level-generation option loading still uses Win32-shaped I/O" >&2
            cat "$log_root/native-client-lgo-win32-io.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopGetClientAreaSize" \
            Minecraft.Client/stubs.cpp >/dev/null
        rg -n -S "NativeDesktopReadFileBytes" \
            Minecraft.Client/Common/GameRules/LevelGenerationOptions.cpp >/dev/null

        if rg -n -S "HANDLE fileHandle|GetFileSize\\(|ReadFile\\(|CloseHandle\\(|GAME:\\\\|UPDATE:\\\\|_TU_BUILD" \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp \
            > "$log_root/native-client-network-tutorial-win32-io.txt"; then
            echo "Native network tutorial save loading still uses Win32-shaped I/O" >&2
            cat "$log_root/native-client-network-tutorial-win32-io.txt" >&2
            exit 1
        fi

        rg -n -S "ReadNativeDesktopNetworkFileBytes" \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp >/dev/null

        if rg -n -S "StorageManager\\.GetSaveUniqueFilename" \
            Minecraft.Client/PendingConnection.cpp \
            > "$log_root/native-client-pending-connection-storage.txt"; then
            echo "Native pending connection pre-login still uses StorageManager map identity" >&2
            cat "$log_root/native-client-pending-connection-storage.txt" >&2
            exit 1
        fi

        rg -n -S "FillNativeUniqueMapName" \
            Minecraft.Client/PendingConnection.cpp >/dev/null

        if rg -n -S "StorageManager\\.GetSaveDisabled\\(\\)" \
            Minecraft.Client/ServerLevel.cpp \
            Minecraft.Client/MinecraftServer.cpp \
            Minecraft.Client/Minecraft.cpp \
            > "$log_root/native-client-gameplay-save-disabled-storage.txt"; then
            echo "Native gameplay save-disabled gates still use StorageManager" >&2
            cat "$log_root/native-client-gameplay-save-disabled-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopSavesAreDisabled" \
            Minecraft.Client/ServerLevel.cpp \
            Minecraft.Client/MinecraftServer.cpp \
            Minecraft.Client/Minecraft.cpp >/dev/null

        if rg -n -S "StorageManager\\.Tick\\(\\)" \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientRuntime.cpp \
            > "$log_root/native-client-save-tick-storage.txt"; then
            echo "Native runtime/network save pumping still uses StorageManager" >&2
            cat "$log_root/native-client-save-tick-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopTickSaves" \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientRuntime.cpp >/dev/null

        if rg -n -S "StorageManager\\.(ResetSaveData|SetSaveTitle)\\(" \
            Minecraft.Client/NativeDesktop/NativeDesktopClientRuntime.cpp \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            > "$log_root/native-client-save-identity-storage.txt"; then
            echo "Native local-game save identity still uses StorageManager" >&2
            cat "$log_root/native-client-save-identity-storage.txt" >&2
            exit 1
        fi

        if rg -n -S "StorageManager\\.(SetSaveDisabled|ResetSaveData|SetSaveTitle)\\(" \
            Minecraft.Client/Common/UI/UIScene_CreateWorldMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_MainMenu.cpp \
            > "$log_root/native-client-startup-save-control-storage.txt"; then
            echo "Native create/trial startup save-control paths still use StorageManager" >&2
            cat "$log_root/native-client-startup-save-control-storage.txt" >&2
            exit 1
        fi

        if rg -n -S "StorageManager\\.GetSaveUniqueNumber\\(" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            > "$log_root/native-client-save-unique-storage.txt"; then
            echo "Native load-menu save telemetry still uses StorageManager" >&2
            cat "$log_root/native-client-save-unique-storage.txt" >&2
            exit 1
        fi

        if rg -n -S "StorageManager\\.(ReturnSavesInfo|LoadSaveData|LoadSaveDataThumbnail|DeleteSaveData)\\(" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            > "$log_root/native-client-load-menu-save-storage.txt"; then
            echo "Native load-menu save lifecycle still uses StorageManager" >&2
            cat "$log_root/native-client-load-menu-save-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopResetSaveData|NativeDesktopSetSaveTitle|NativeDesktopGetSaveUniqueNumber" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientRuntime.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientSaveControl.cpp \
            >/dev/null

        rg -n -S "NativeDesktop(ResetSaveData|SetSaveTitle|SetSavesDisabled)" \
            Minecraft.Client/Common/UI/UIScene_CreateWorldMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_MainMenu.cpp \
            >/dev/null

        if rg -n -S "NativeDesktop(GetSaveInfo|LoadSaveDataByIndex|LoadSaveDataThumbnailByIndex|DeleteSaveDataByIndex)" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            > "$log_root/native-client-ui-save-catalog-bypass.txt"; then
            echo "Native UI save lifecycle paths bypass NativeDesktopClientSaveCatalog" >&2
            cat "$log_root/native-client-ui-save-catalog-bypass.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopClientSaveCatalog|g_NativeDesktopLoad(Menu|OrJoin)SaveCatalog" \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientSaveCatalog.h \
            >/dev/null

        rg -n -S "NativeDesktopLoadSaveDataByIndex|NativeDesktopLoadSaveDataThumbnailByIndex|NativeDesktopDeleteSaveDataByIndex|NativeDesktopGetSaveInfo" \
            Minecraft.Client/NativeDesktop/NativeDesktopClientSaveCatalog.h \
            Minecraft.Client/NativeDesktop/NativeDesktopClientSaveControl.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStubs.h \
            >/dev/null

        if rg -n -S "StorageManager\\.(GetSavesInfo|ReturnSavesInfo|ClearSavesInfo|LoadSaveDataThumbnail|GetSaveDisabled|EnoughSpaceForAMinSaveGame|ResetSaveData|SetSaveTitle|GetSaveDeviceSelected)\\(" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            > "$log_root/native-client-load-or-join-save-storage.txt"; then
            echo "Native load-or-join save-list paths still use StorageManager" >&2
            cat "$log_root/native-client-load-or-join-save-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopLoadOrJoinReturnSavesInfo|NativeDesktopLoadOrJoinGetSavesInfo|NativeDesktopLoadOrJoinLoadThumbnail|NativeDesktopSavesAreDisabled" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            >/dev/null

        if rg -n -S "StorageManager\\.(ClearSavesInfo|SetSaveDisabled|ContinueIncompleteOperation|GetSaveDisabled|EnoughSpaceForAMinSaveGame|ReturnSavesInfo|LoadSaveDataThumbnail|GetSavesInfo|DeleteSaveData)\\(" \
            Minecraft.Client/Common/UI/UIScene_InGameSaveManagementMenu.cpp \
            > "$log_root/native-client-ingame-save-storage.txt"; then
            echo "Native in-game save management paths still use StorageManager" >&2
            cat "$log_root/native-client-ingame-save-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopInGame(ReturnSavesInfo|ClearSavesInfo|GetSavesInfo|LoadSaveThumbnail|DeleteSaveData|SavesAreDisabled|SetSavesDisabled|ContinueIncompleteOperation|EnoughSpaceForMinSave)" \
            Minecraft.Client/Common/UI/UIScene_InGameSaveManagementMenu.cpp \
            >/dev/null

        if rg -n -S "StorageManager\\.(AllocateSaveData|GetDefaultSaveImage|GetDefaultSaveThumbnail|SetSaveImages|SaveSaveData|GetSaveUniqueFileDir|LoadSaveData|GetSaveSize|GetSaveData|SetSaveUniqueFilename|GetSaveState|Tick|CopySaveData|RenameSaveData|DeleteSaveData|SetSaveTransferInProgress|SaveTransferClearState|SaveTransferGetDetails|SaveTransferGetData|CancelSaveTransfer)\\(" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            > "$log_root/native-client-load-or-join-transfer-storage.txt"; then
            echo "Native load-or-join transfer save paths still use StorageManager" >&2
            cat "$log_root/native-client-load-or-join-transfer-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopLoadOrJoin(AllocateSaveData|SetSaveImages|SaveSaveData|LoadSaveData|CopySaveData|RenameSaveData|DeleteSaveDataByIndex|SetSaveTransferInProgress|SaveTransferClearState|SaveTransferGetDetails|SaveTransferGetData|CancelSaveTransfer)" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            >/dev/null

        if rg -n -S "StorageManager\\.(GetSaveDisabled|SetSaveDisabled|DoesSaveExist|GetSaveState)\\(" \
            Minecraft.Client/ClientConnection.cpp \
            Minecraft.Client/Common/Consoles_App.cpp \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp \
            Minecraft.Client/Common/UI/IUIScene_PauseMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_DeathMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_PauseMenu.cpp \
            Minecraft.Client/MinecraftServer.cpp \
            > "$log_root/native-client-save-control-storage.txt"; then
            echo "Native save-control paths still use StorageManager" >&2
            cat "$log_root/native-client-save-control-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopDoesSaveExist|NativeDesktopSavesAreIdle|NativeDesktopDeathMenuSavesAreDisabled|NativeDesktopPauseMenu(DoesSaveExist|SavesAreDisabled|SetSavesDisabled)" \
            Minecraft.Client/ClientConnection.cpp \
            Minecraft.Client/Common/Consoles_App.cpp \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp \
            Minecraft.Client/Common/UI/IUIScene_PauseMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_DeathMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_PauseMenu.cpp \
            Minecraft.Client/MinecraftServer.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientSaveControl.cpp \
            >/dev/null

        if rg -n -S "StorageManager\\.SetSaveDevice\\(" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_MainMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_PauseMenu.cpp \
            > "$log_root/native-client-device-picker-storage.txt"; then
            echo "Native device-picker UI paths still use StorageManager" >&2
            cat "$log_root/native-client-device-picker-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopSelectSaveDevice" \
            Minecraft.Client/Common/UI/UIScene_LoadOrJoinMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_MainMenu.cpp \
            Minecraft.Client/Common/UI/UIScene_PauseMenu.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStorageControl.h \
            >/dev/null

        if rg -n -S "StorageManager\\.(GetGameDefinedProfileData|GetDashboardProfileSettings|WriteToProfile|ForceQueuedProfileWrites|ReadFromProfile|SetSaveDeviceSelected|GetSaveDeviceSelected|SetSaveDevice)\\(" \
            Minecraft.Client/Common/Consoles_App.cpp \
            Minecraft.Client/Common/Tutorial/Tutorial.cpp \
            Minecraft.Client/Common/UI/UIScene_LoadMenu.cpp \
            Minecraft.Client/StatsCounter.cpp \
            > "$log_root/native-client-app-profile-storage.txt"; then
            echo "Native app profile/save-device paths still use StorageManager" >&2
            cat "$log_root/native-client-app-profile-storage.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopGetGameDefinedProfileData|NativeDesktopWriteProfile|NativeDesktopForceQueuedProfileWrites|NativeDesktopSetSaveDeviceSelected|NativeDesktopGetSaveDeviceSelected" \
            Minecraft.Client/Common/Consoles_App.cpp \
            Minecraft.Client/Common/Tutorial/Tutorial.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStorageControl.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStubs.h \
            Minecraft.Client/StatsCounter.cpp \
            >/dev/null

        if rg -n -S "StorageManager\\." \
            Minecraft.Client/ClientConnection.cpp \
            Minecraft.Client/Common/Network/GameNetworkManager.cpp \
            Minecraft.Client/NativeDesktop/Legacy/ShutdownManager.cpp \
            > "$log_root/native-runtime-network-storage-manager.txt"; then
            echo "Native runtime/network paths still mention StorageManager" >&2
            cat "$log_root/native-runtime-network-storage-manager.txt" >&2
            exit 1
        fi

        rg -n -S "NativeDesktopRequestStorageExit|NativeDesktopSetSystemUIDisplaying" \
            Minecraft.Client/NativeDesktop/Legacy/ShutdownManager.cpp \
            Minecraft.Client/NativeDesktop/NativeDesktopClientStorageControl.cpp \
            >/dev/null

        if rg -n -S "CreateFile\\(|GetFileSize\\(|ReadFile\\(|CloseHandle\\(|StorageManager\\.GetMountedPath|_WINDOWS64|_DURANGO" \
            Minecraft.Client/Common/DLC/DLCManager.cpp \
            > "$log_root/native-client-dlc-manager-win32-io.txt"; then
            echo "Native DLC manager still uses Win32-shaped file I/O" >&2
            cat "$log_root/native-client-dlc-manager-win32-io.txt" >&2
            exit 1
        fi

        rg -n -S "ReadNativeDLCFileBytes" \
            Minecraft.Client/Common/DLC/DLCManager.cpp >/dev/null
        rg -n -S "ReadDLCUTF16String|DLCParamRecordBytes" \
            Minecraft.Client/Common/DLC/DLCManager.cpp >/dev/null
        rg -n -S "ReadDLCUTF16String|DLCParamRecordBytes" \
            Minecraft.Client/Common/DLC/DLCAudioFile.cpp >/dev/null

        if rg -n -S "sizeof\\(WCHAR\\)|\\(WCHAR \\*\\)pParams->wchData|static_cast<WCHAR \\*>" \
            Minecraft.Client/Common/DLC/DLCManager.cpp \
            Minecraft.Client/Common/DLC/DLCAudioFile.cpp \
            > "$log_root/native-client-dlc-wchar-disk-format.txt"; then
            echo "Native DLC parser still uses native WCHAR for disk format" >&2
            cat "$log_root/native-client-dlc-wchar-disk-format.txt" >&2
            exit 1
        fi
        rg -n -S "__PS3__\\) \\|\\| defined\\(_NATIVE_DESKTOP\\)" \
            Minecraft.Client/Common/DLC/DLCSkinFile.cpp >/dev/null

        if rg -n -S "CreateFile\\(|GetFileSize\\(|ReadFile\\(|CloseHandle\\(|StorageManager\\.|MountInstalledDLC|UnmountInstalledDLC|_DURANGO|_WINDOWS64|_UNICODE" \
            Minecraft.Client/DLCTexturePack.cpp \
            > "$log_root/native-client-dlc-texturepack-win32-io.txt"; then
            echo "Native DLC texture pack loading still uses Win32-shaped file I/O" >&2
            cat "$log_root/native-client-dlc-texturepack-win32-io.txt" >&2
            exit 1
        fi

        rg -n -S "ReadNativeTexturePackFileBytes" \
            Minecraft.Client/DLCTexturePack.cpp >/dev/null
        rg -n -F "Common\\\\res\\\\TitleUpdate\\\\DLC" \
            Minecraft.Client/Common/Consoles_App.cpp >/dev/null
        rg -n -S "NativeDesktopLoadBundledTitleUpdateDLC" \
            Minecraft.Client/Common/Consoles_App.cpp >/dev/null
        rg -n -S "_NATIVE_DESKTOP|IDS_NO_DLCOFFERS" \
            Minecraft.Client/Common/UI/UIScene_DLCMainMenu.cpp >/dev/null
        rg -n -S "_NATIVE_DESKTOP|IDS_NO_DLCOFFERS" \
            Minecraft.Client/Common/UI/UIScene_DLCOffersMenu.cpp >/dev/null
        rg -n -S "startup\\.texturePacks|startup\\.bundledDLC|texturePackCountMax" \
            Minecraft.Client/NativeDesktop/NativeDesktopClientRuntime.cpp \
            >/dev/null
        rg -n -S "save\\.loadedAtStartup|save\\.persisted|NativeDesktopRequestGameplaySave" \
            Minecraft.Client/NativeDesktop/NativeDesktopClientRuntime.cpp \
            >/dev/null
        rg -n -S "startup[.]texturePacks|startup[.]bundledDLC" \
            Minecraft.Client/NativeDesktop/RunNativeDesktopClientSmoke.cmake \
            >/dev/null
        rg -n -S "SAVE_RESTART|save[.]loadedAtStartup|NativeDesktop save: loaded" \
            Minecraft.Client/NativeDesktop/RunNativeDesktopClientSmoke.cmake \
            >/dev/null
        rg -n -S "minecraft_native_desktop_client_save_restart_smoke" \
            Minecraft.Client/CMakeLists.txt >/dev/null
        rg -n -S "MINECRAFT_NATIVE_DESKTOP_SAVE_ROOT|NativeFlushSaveData|NativeLoadSaveData" \
            Minecraft.World/NativeDesktop/NativeDesktopLegacyClientStubs.cpp \
            >/dev/null
        rg -n -S "offline local DLC only|no storefront|no online purchase/update|no platform license/install flow" \
            docs/native-port-plan.md >/dev/null

        if rg -n -S "HANDLE|CreateFile\\(|ReadFile\\(|WriteFile\\(|SetFilePointer\\(|CloseHandle\\(" \
            Minecraft.World/FileInputStream.h \
            Minecraft.World/FileInputStream.cpp \
            Minecraft.World/FileOutputStream.h \
            Minecraft.World/FileOutputStream.cpp \
            > "$log_root/native-world-file-stream-win32-io.txt"; then
            echo "Native world file streams still use Win32-shaped I/O" >&2
            cat "$log_root/native-world-file-stream-win32-io.txt" >&2
            exit 1
        fi

        if rg -n -S "HANDLE|CreateFile\\(|ReadFile\\(|WriteFile\\(|SetFilePointer\\(|CloseHandle\\(" \
            Minecraft.World/ZoneFile.h \
            Minecraft.World/ZoneFile.cpp \
            Minecraft.World/ZoneIo.h \
            Minecraft.World/ZoneIo.cpp \
            Minecraft.World/NbtSlotFile.h \
            Minecraft.World/NbtSlotFile.cpp \
            Minecraft.World/ZonedChunkStorage.cpp \
            > "$log_root/native-world-zone-storage-win32-io.txt"; then
            echo "Native world zone storage still uses Win32-shaped I/O" >&2
            cat "$log_root/native-world-zone-storage-win32-io.txt" >&2
            exit 1
        fi

        if rg -n -S "CreateDirectory\\(|GetFileAttributes|DeleteFile\\(|MoveFile\\(|FindFirstFile\\(|FindNextFile\\(|WIN32_FILE_ATTRIBUTE_DATA|StorageManager" \
            Minecraft.World/File.cpp \
            > "$log_root/native-world-file-adapter-win32-glue.txt"; then
            echo "Native world file adapter still uses Win32/platform storage glue" >&2
            cat "$log_root/native-world-file-adapter-win32-glue.txt" >&2
            exit 1
        fi

        rg -n -S "std::filesystem" Minecraft.World/File.cpp >/dev/null

        if rg -n -S "HANDLE|CreateFile\\(|GetFileSize\\(|ReadFile\\(|CloseHandle\\(|_WINDOWS64|_DURANGO|GAME:\\\\" \
            Minecraft.World/CustomLevelSource.cpp \
            Minecraft.World/BiomeOverrideLayer.cpp \
            > "$log_root/native-world-generation-override-win32-io.txt"; then
            echo "Native world generation overrides still use Win32/console file I/O" >&2
            cat "$log_root/native-world-generation-override-win32-io.txt" >&2
            exit 1
        fi

        rg -n -S "ReadOverrideBytes" \
            Minecraft.World/CustomLevelSource.cpp \
            Minecraft.World/BiomeOverrideLayer.cpp >/dev/null

        rg -n -S "Windows compatibility maintenance has stopped" \
            README.md COMPILE.md CONTRIBUTING.md docs/native-port-plan.md >/dev/null
        git diff --check
        echo "native-only contract checks passed"
    ' bash "$repo_root" "$log_root"
    append_summary_line "native_only.contract=ok"
}

run_server_runtime_harness() {
    if [[ "$run_server_harness" != "1" ]]; then
        append_summary_line "server.harness=skipped"
        return 0
    fi

    run_and_log server-runtime-harness env \
        HARNESS_LOG_DIR="$server_log_root" \
        REMOTE_HOST="$remote_host" \
        REMOTE_ROOT="$remote_root" \
        "$server_harness"

    if ! grep -Fq "result=success" "$server_log_root/summary.txt"; then
        echo "server harness summary did not report success" >&2
        cat "$server_log_root/summary.txt" >&2
        return 1
    fi

    append_summary_line "server.harness=ok"
    append_summary_line "server.summary=$server_log_root/summary.txt"
}

run_local_client_smoke() {
    if [[ "$run_local_client" != "1" ]]; then
        append_summary_line "local.client=skipped"
        return 0
    fi

    run_and_log local-client-smoke bash -c '
        set -euo pipefail
        cd "$1"
        if [[ "$5" == "1" ]]; then
            rm -rf "build/$2"
        fi
        cmake --preset "$2"
        cmake --build --preset "$3" --target "$4"
    ' bash \
        "$repo_root" \
        "$local_client_configure_preset" \
        "$local_client_build_preset" \
        "$local_client_target" \
        "$clean_client_builds"
    append_summary_line "local.client=ok"
}

sync_remote_tree_for_client() {
    run_and_log remote-client-sync rsync -az --delete \
        --exclude .git \
        --exclude build \
        "$repo_root/" \
        "$remote_host:$remote_root/"
    append_summary_line "remote.client_sync=ok"
}

run_remote_client_smoke() {
    if [[ "$run_remote_client" != "1" ]]; then
        append_summary_line "remote.client=skipped"
        return 0
    fi

    sync_remote_tree_for_client

    local remote_cmd
    remote_cmd=$(
        cat <<EOF
set -euo pipefail
export PATH="\$HOME/.local/bin:\$PATH"
cd "$remote_root"
if [[ "$clean_client_builds" == "1" ]]; then
    rm -rf "build/$remote_client_configure_preset"
fi
cmake --preset "$remote_client_configure_preset"
cmake --build --preset "$remote_client_build_preset" --target "$remote_client_target"
EOF
    )

    run_and_log remote-client-smoke ssh "$remote_host" "$remote_cmd"
    append_summary_line "remote.client=ok"
}

main() {
    trap on_error ERR
    write_summary_header

    echo "Mainline harness root: $log_root"
    echo "Repo root: $repo_root"
    echo "Remote host: $remote_host"
    echo "Remote root: $remote_root"

    run_native_only_contract
    run_server_runtime_harness
    run_local_client_smoke
    run_remote_client_smoke

    mark_summary_success
    echo "Native mainline harness completed successfully."
}

main "$@"
