# Native-Only Port Plan

## Goal

`main` is now a macOS/Linux native-only branch. Windows compatibility
maintenance has stopped on this line of development. The archived compatibility
state lives on `last-windows-compatible`; new work on `main` must not restore
or preserve that build path.

The primary deliverable is:

```text
Minecraft.Server.NativeBootstrap
```

The native client remains an early macOS/Linux smoke target. It is useful for
future desktop work, but the server-native runtime is the active shipping
surface.

## Platform Scope

Supported targets:

- macOS native headless server
- Linux native headless server
- macOS native client smoke
- Linux native client smoke

Unsupported on `main`:

- archived desktop compatibility branches
- archived console toolchains
- Wine-backed runtime packaging
- renderer paths from removed platform builds
- platform-specific release automation outside macOS/Linux native validation

Cleanup rule:

- delete removed-platform scripts, presets, generated project files, and
  proprietary platform documents from `main`
- keep only the legacy ABI terms required by current macOS/Linux native shims
- replace those legacy ABI terms with native names only when the replacement is
  covered by smoke tests

Residual scan result:

- no Windows build preset, workflow, Docker/Wine runtime path, or supported
  Windows server/client entrypoint remains on `main`
- active native client networking now uses `NativeDesktopNetLayer` and
  `NativeDesktop*` names instead of the old `Win64*` native shim names
- remaining `_WINDOWS64`, `_WIN64`, Direct3D, `SAVE_FILE_PLATFORM_WIN64`, and
  Win32-like symbols live in dormant legacy gameplay branches, save-format
  compatibility enums, vendor code, or native ABI shims
- those remaining names are cleanup backlog only; new work must not expand them
  or treat them as Windows compatibility maintenance

## Technical Direction

### Native server first

The server runtime is the first complete native deliverable because it can
exercise platform, filesystem, time, socket, storage, lifecycle, and admin
control contracts without depending on a renderer.

The active server runtime rules are:

- `Minecraft.Server.NativeBootstrap` is the only native server executable.
- `NativeDedicatedServerHostedGameSession` owns session state.
- `NativeDedicatedServerHostedGameWorker` owns command side effects.
- `DedicatedServerHostedGameRuntimeState` is only an observable projection.
- Native save/load/reload flows use the native storage adapter and `.save`
  persistence contract.
- Corrupt saves fail during bootstrap/load, before a session is considered
  running.

### Native client second

The native client remains a smoke-tested shell. It should only grow through
macOS/Linux abstractions and must not reintroduce removed platform build paths.

Near-term client work should focus on:

- native window/input ownership
- renderer-independent lifecycle boundaries
- asset loading that does not assume removed platform directories
- smoke coverage before user-facing gameplay claims

## Phases

### Phase 1: Native-only build surface

Status: complete in the current cutover.

Deliverables:

- top-level CMake rejects unsupported hosts
- CMake presets list only macOS/Linux native targets
- removed archived platform toolchain presets
- removed Windows release workflows
- removed Wine Docker runtime scripts
- removed Windows64 source/resource directories from `main`
- removed stale Visual Studio/AppWizard project documentation from `main`

### Phase 2: Headless runtime hardening

Status: complete for the current contract.

Deliverables:

- bootstrap-only smoke
- loaded bootstrap smoke
- corrupt loaded bootstrap smoke
- loaded shell smoke
- save/reload smoke
- listener/self-connect smoke
- scripted shell smoke
- live accept smoke
- remote command shell smoke

### Phase 3: Runtime ownership cleanup

Status: in progress.

Direction:

- keep hosted runtime as coordinator only
- keep session core as canonical state owner
- keep worker queue as side-effect owner
- keep shell/status responses sourced from session/runtime snapshots
- remove remaining projection helpers when session-owned frame APIs can replace
  them cleanly

### Phase 4: Native client convergence

Status: active.

Direction:

- keep native client smoke green
- remove legacy platform assumptions from client startup
- build renderer and input work only through macOS/Linux abstractions
- do not restore archived platform compatibility to make client progress faster

Automation:

- `tools/native_mainline_harness.sh` is the required broad-change gate
- the mainline harness checks native-only source contracts, server runtime
  lifecycle, local client smoke, and Linux client smoke
- use `RUN_REMOTE_CLIENT=0` only when the Linux host is unavailable, not to
  bypass a known failure

### Phase 5: Native desktop runtime completion

Status: next.

Direction:

- make the client runtime summary the canonical startup health contract
  (startup milestones, final phase, exit code, runtime health, observed
  gameplay world/player readiness, screen/menu state observation, and QNet
  gameplay lineage are now smoke-checked)
- replace remaining active `_WINDOWS64` UI/input branches with
  `_NATIVE_DESKTOP` branches
  (native keyboard scene, direct text edit, KBM mouse UI dispatch, and the
  create-world/sign/anvil/seed text flows now use `_NATIVE_DESKTOP`;
  saved-server add/edit/delete/join-index flows now use `_NATIVE_DESKTOP`;
  load/join save-list sorting, worldname sidecars, thumbnail index, and
  save rename/delete option flow now use `_NATIVE_DESKTOP`;
  slider KBM behaviour, desktop bitmap-font cache ranges, and debug menu
  direct-edit text inputs now use `_NATIVE_DESKTOP`;
  native desktop KBM input now stores per-frame key, mouse, wheel, char,
  movement, and look state for `Screen`/UI/gameplay consumers;
  client smoke now injects a deterministic native desktop input replay script
  and validates the runtime input lineage in the summary;
  native desktop summary now captures high-water gameplay, world/player, UI,
  and QNet observations so smoke fails if startup only reaches a shallow shell;
  native Iggy stubs now expose deterministic control visibility/geometry so
  mouse hit-testing and crafting H-slot dispatch can run on macOS/Linux)
- isolate Direct3D-shaped renderer structs behind native renderer-neutral names
  (`NativeRendererRect` now fronts the clear-rect path used by the shared UI
  controller, and `NativeRendererViewport` now fronts the gamma post-process
  viewport path used by `GameRenderer`)
- keep native UI mouse geometry sourced from native desktop runtime state
  (`NativeDesktopGetClientAreaSize` now replaces direct `HWND`/`GetClientRect`
  reads in active mouse hit-testing and container pointer scale paths)
- make native desktop skin startup explicit and smoke-checked
  (`_NATIVE_DESKTOP` now enters the desktop SWF skin-loading branch, and the
  client summary must report a positive `startup.uiSkinLibraries` count)
- keep screen clipboard operations on a native desktop substitute
  (`Screen::getClipboard/setClipboard` now round-trip through the native
  desktop runtime stub, and startup smoke requires `startup.clipboard=1`)
- keep native desktop XUID bootstrap on native path/file/time substitutes
  (`NativeDesktop_Xuid` now uses C++17/POSIX path, I/O, clock, process and
  thread signals, and startup smoke only reports `startup.xuid=1` for a valid
  persisted XUID)
- keep native client input repeat/callback timing on a monotonic native clock
  (`Screen` and `UIController` now use `NativeDesktopGetMonotonicMilliseconds`
  instead of reaching for `GetTickCount` semantics on the native path)
- keep native desktop save-list UTF-8/wide conversion on native desktop
  ABI helpers
  (`UIScene_LoadOrJoinMenu` and `UIScene_InGameSaveManagementMenu` now route
  native save filename/title/rename buffers through native desktop UTF
  conversion helpers instead of locale-based `mbstowcs` or `_WINDOWS64`
  thumbnail-only branches; `UIScene_LoadMenu` and the saved-server add/edit
  paths now use the same helpers for displayed save titles and `servers.db`
  UTF-8 round-trips)
- keep native desktop asset/font loading on native UTF-8 file reads
  (`ArchiveFile` and `UITTFFont` no longer use locale path conversion or
  direct Win32-shaped file I/O in the active native client loader path)
- keep active native client input/UI/gameplay guards native-only
  (`Minecraft`, `Screen`, UI controller, crafting/load menu, audio, and
  platform-network active paths no longer share `_WINDOWS64` and
  `_NATIVE_DESKTOP` preprocessor guards)
- keep native client startup/data helpers on native abstractions
  (`Mouse::getY` now reads client geometry through
  `NativeDesktopGetClientAreaSize`, and `LevelGenerationOptions` reads native
  desktop game-rule/base-save bytes through `NativeDesktopReadFileBytes`
  instead of direct Win32-shaped file handles or `WPACK:`/`StorageManager`
  mount callbacks; `GameNetworkManager` uses the same native byte-read
  helper for tutorial base-save bootstrap data instead of console
  `GAME:`/`UPDATE:` paths or Win32 file handles; `PendingConnection` now
  fills the native pre-login map identity directly instead of asking
  `StorageManager` for a console save filename; active gameplay save-disabled
  checks now read `NativeDesktopClientSaveControl` instead of reaching into
  `StorageManager`, and runtime/network wait loops tick native save-control
  directly instead of pumping `StorageManager`; native local-game bootstrap
  also resets save data and sets the active save title through the same
  save-control owner)
- keep native world stream primitives on native file handles
  (`FileInputStream` and `FileOutputStream` now use C stdio on macOS/Linux
  instead of exposing Win32 `HANDLE` or direct `CreateFile`/`ReadFile`/
  `WriteFile` paths)
- keep native world zone storage on native file channels
  (`ZoneFile`, `ZoneIo`, `NbtSlotFile`, and `ZonedChunkStorage` now use C
  stdio for zone/entity chunk storage instead of direct Win32 file handles)
- keep the native world file adapter on portable filesystem primitives
  (`File` now routes delete, mkdir, exists, rename, list, length, and
  last-modified queries through `std::filesystem` instead of Win32-shaped
  platform/storage glue)
- keep native world-generation override assets on portable file reads
  (`CustomLevelSource` and `BiomeOverrideLayer` now load `GameRules/*.bin`
  override data through C stdio instead of `CreateFile`/`GetFileSize`/
  `ReadFile` console paths)
- keep native DLC metadata loading on portable file reads
  (`DLCManager` now reads DLC data and pack-id metadata through normalized
  C stdio paths instead of `StorageManager.GetMountedPath` plus
  Win32-shaped `CreateFile`/`ReadFile` calls)
- keep native DLC texture-pack payload loading on portable file reads
  (`DLCTexturePack` now reads texture-pack UI payloads, game-rule files, and
  base-save bytes through normalized C stdio paths instead of Win32-shaped file
  handles)
- keep native DLC texture-pack loading off console mount drives
  (`DLCTexturePack` now enters its native load callback directly, and native
  title-update texture pack roots resolve to `Common/res/TitleUpdate/DLC`
  instead of `TPACK:`/`StorageManager` mount glue)
- keep every convergence step covered by `Minecraft.NativeDesktop.Check`
- keep `Minecraft.Server.NativeBootstrap` green while client work proceeds

## Validation

Primary local validation:

```bash
cmake --preset macos-native
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

Linux validation:

```bash
cmake --preset linux-native
cmake --build --preset linux-native-debug --target Minecraft.Portability.Check
```

Harness validation:

```bash
tools/headless_server_native_harness.sh
```

Mainline validation:

```bash
tools/native_mainline_harness.sh
```

CI validation:

```text
.github/workflows/native-smoke.yml
```

## Non-Goals

- Do not restore removed desktop compatibility builds.
- Do not restore removed Wine/Docker runtime packaging.
- Do not add platform-specific release workflows outside macOS/Linux native
  validation.
- Do not add a second native server executable.
- Do not bypass the worker/session runtime ownership model for save or stop
  behavior.
