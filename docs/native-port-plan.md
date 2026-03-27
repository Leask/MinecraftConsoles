# Native Port Plan

## Goal

Build a public, multi-platform native edition of this codebase that can run
on macOS and Linux without Wine, while preserving the current desktop fixes
already present on the `main` branch. A native Windows build remains desirable,
but only if it can share the same renderer and platform layer without forcing
the project back into Direct3D-only assumptions.

## Baseline And Donors

### Primary baseline

Use the current `main` branch as the code baseline.

Reason:

- It already contains desktop-focused fixes that matter for a public build:
  keyboard and mouse support, fullscreen, desktop timing work, dedicated server
  packaging, and modern CMake structure.
- It is the branch we can keep shipping from while native work is in progress.

### Primary donor

Use `Reasearch/minecraftcpp-master` as the primary donor for native host
design ideas.

Reason:

- It contains explicit native entry points and platform abstractions such as
  `AppPlatform_iOS`, `main_dedicated.cpp`, and `AppPlatform_linux` in
  `main_rpi.h`.
- It demonstrates the shape of a non-Windows bootstrap path using Unix APIs,
  EGL, SDL, and dedicated-server startup flow.
- It is not drop-in compatible with LCE, but it is the clearest reference for
  how to structure native bootstrap code.

### Secondary donors

- `Reasearch/MinecraftConsoles-backups-oct_10_2014`
  Best donor for LCE-specific platform shims such as `OrbisStubs.*`. This is
  the reference for replacing Win32 assumptions with a compatibility layer.
- `Reasearch/MinecraftConsoles-141217`
  Best donor for `ConsoleSaveFileSplit.*` and the `SPLIT_SAVES` path already
  referenced in the active codebase.
- `Reasearch/MinecraftPS3Edition`
  Useful as a second non-Windows LCE implementation to confirm PS3/Orbis-side
  patterns, but not the main starting point.

## Technical Direction

### Platform layer

Create a new native desktop platform path rather than trying to make the
existing Windows64 path impersonate Unix.

Initial native platform responsibilities:

- filesystem
- time
- threading and synchronization
- socket wrappers
- process and signal handling
- path normalization

These are the lowest-risk modules to extract first and they unblock both
macOS/Linux server work and later client work.

### Windowing, input, and OpenGL

The target desktop stack is:

- SDL2 for window creation, event pump, controller input, and clipboard/text
  input
- OpenGL core profile for the first cross-platform renderer target
- `miniaudio` retained where possible instead of replacing audio up front

Why SDL2:

- mature and stable on macOS, Linux, and Windows
- solves window/input/controller concerns in one dependency
- does not prevent a later Metal or Vulkan renderer if needed

Why OpenGL first:

- it is the only realistic short-path option for a shared desktop renderer
  across Linux and macOS
- the tree already contains Iggy OpenGL shared fragments, which lowers the risk
  compared with a fresh Metal-only path

Constraints:

- macOS should target a core profile path that remains practical on current
  Apple systems
- Linux should target OpenGL first, not Vulkan-first
- Windows OpenGL is optional until macOS/Linux parity exists

### Server strategy

The first true native deliverable should be a Linux/macOS dedicated server.

Reason:

- current server code is still entangled with client and renderer assumptions
- removing those assumptions creates the same platform abstractions the client
  will later depend on
- it gives a shipping target before the renderer rewrite is complete

## Integration Rules

- Preserve all current `main` desktop fixes unless they directly block native
  work.
- Import research code by function, not by wholesale directory copy.
- Every imported donor file must be attributed in commit history and wrapped
  behind the active project layout and build system.
- Prefer replacing hard platform dependencies with narrow interfaces instead of
  adding more conditional compilation to gameplay code.

## Phased Plan

### Phase 0: Build scaffolding

- remove top-level CMake Windows-only hard fail
- add native configure presets for macOS and Linux
- add portable utility targets that can be compiled on non-Windows hosts
- document donor map and integration sequence

### Phase 1: Shared native core

- port filesystem helpers
- port clocks and sleep
- introduce mutex/thread/TLS wrappers
- introduce BSD socket wrappers
- make these utilities build and test on macOS and Linux

### Phase 2: Native dedicated server

- split dedicated server from D3D/UI startup code
- isolate server asset requirements
- make server boot, generate worlds, and save on macOS/Linux
- restore multiplayer and LAN behavior using native sockets

Current server blockers already visible in the active tree:

- `Minecraft.Server/Windows64/ServerMain.cpp` still pulls in `4J_Render`,
  `Windows64_UIController`, `UI`, `Input`, and D3D device globals
- world save bootstrap still assumes `Windows64\\GameHDD` layout
- server CLI input now has a native smoke path, but the full interactive
  console stack is still only partially detached from legacy dedicated-server
  startup
- networking still enters through `WinsockNetLayer`
- `Minecraft.World/stdafx.h` still routes non-Windows builds into legacy
  console platform include trees instead of a dedicated `NativeDesktop`
  branch

### Phase 3: Native client bootstrap

- add SDL2 desktop app bootstrap
- create a `NativeDesktop` platform target
- establish input, window resize, and main loop
- keep gameplay disabled where renderer gaps remain

### Phase 4: Renderer migration

- replace Direct3D-specific renderer assumptions with an API-neutral layer
- map enough `4J_Render` behavior to support world rendering on OpenGL
- map Iggy custom draw and UI presentation to the new path

### Phase 5: Functional parity

- restore audio, storage, DLC-safe fallbacks, and controller features
- add Windows OpenGL build only if it shares the same path cleanly
- bring back packaging and release automation

## Current Iteration

This iteration starts with:

- native presets
- native portability library target
- first cross-platform filesystem implementation
- first cross-platform time and sleep implementation
- first cross-platform stdin and socket utility implementation
- first extracted LAN discovery helper
  (`include/lce_net/lce_lan.cpp`) covering broadcast encoding, session upsert,
  timeout pruning, and a fixed 16-bit wire layout for future native host/join
  work
- first cross-platform UDP socket runtime in `lce_net`
  covering UDP open/bind/send/receive, socket options, and bound-port
  discovery for native LAN discovery plumbing
- first cross-platform TCP socket runtime in `lce_net`
  covering listen/connect/accept/send/receive helpers for native host/join
  groundwork
- first cross-platform IPv4 resolution helper in `lce_net`
  so native and Windows desktop paths can share `localhost` / hostname
  resolution without directly depending on WinSock address setup
- first extracted dedicated server platform-state helper
  (`Minecraft.Server/Common/DedicatedServerPlatformState.cpp`) so default
  host identity, bind/runtime flags, and `IQNet::m_player[]` bootstrap data
  stop living inline inside `ServerMain.cpp`
- native bootstrap self-connect coverage in `CTest`, so listener bind/accept
  stays under automated regression while `ServerMain.cpp` keeps shrinking
- first extracted dedicated hosted-game startup plan in
  `Minecraft.Server/Common/DedicatedServerWorldBootstrap.cpp`, so seed
  resolution plus `HostGame` defaults stop living inline in `ServerMain.cpp`
- first extracted dedicated app-session setup plan in
  `Minecraft.Server/Common/DedicatedServerSessionConfig.cpp`, so tutorial,
  corrupt-save, host-settings, world-size, and save-disable defaults stop
  living inline in `ServerMain.cpp`
- autosave loop decisions now live in
  `Minecraft.Server/Common/DedicatedServerRuntime.cpp`, so completion,
  request, and deadline-pushback rules stay under native smoke coverage
- `NetworkGameInitData` field mapping now lives behind
  `PopulateDedicatedServerNetworkGameInitData(...)`, so `ServerMain.cpp`
  no longer hand-populates the game-thread init payload directly
- first non-Windows Win32 compatibility shim for
  `DWORD/HANDLE/CRITICAL_SECTION/Tls*/Event/CreateThread/Wait*`
- first server-common utility source (`Minecraft.Server/Common/StringUtils.cpp`)
  compiling in the native smoke target without `stdafx.h`
- first server file I/O utility source (`Minecraft.Server/Common/FileUtils.cpp`)
  compiling and running in the native smoke target without Win32 file APIs
- first access-control manager source (`Minecraft.Server/Access/BanManager.cpp`)
  compiling and running in the native smoke target with native JSON/file paths
- first server logging source (`Minecraft.Server/ServerLogger.cpp`)
  compiling and running in the native smoke target with non-Windows time/color paths
- first server CLI input source (`Minecraft.Server/Console/ServerCliInput.cpp`)
  compiling and running in the native smoke target through a narrow
  `IServerCliInputSink` interface and a native `linenoise` fallback
- first world utility source (`Minecraft.World/ThreadName.cpp`)
  compiling and running in the native smoke target with a native
  pthread-based thread-name path
- second world utility source (`Minecraft.World/PerformanceTimer.cpp`)
  compiling and running in the native smoke target through `lce_time`
  instead of Win32 performance-counter APIs
- first extracted Windows network-path helper reused by the active tree
  (`WinsockNetLayer`) and the native smoke target, including a fix for the
  LAN broadcast `maxPlayers` byte overflow when desktop builds report 256
- `WinsockNetLayer` LAN advertise/discovery now routes through the shared
  `lce_net` UDP helpers instead of directly open-coding UDP socket setup and
  `sendto`/`recvfrom`
- `WinsockNetLayer` packet framing and accepted-socket setup now partially
  route through shared `lce_net` TCP helpers (`accept`, `send`, `recv`)
- `WinsockNetLayer` host/join/split-screen connection setup now partially
  route through shared `lce_net` helpers (`resolve`, `bind`, `listen`,
  `connect`, `TCP_NODELAY`) instead of directly open-coding WinSock setup
- dedicated server CLI/default/property config parsing now lives in a
  standalone `Minecraft.Server/Common/DedicatedServerOptions.cpp` module
  that compiles and runs in the native smoke target, including argument
  parsing, property overlay, help text, and `int64` seed handling without
  MSVC-only integer helpers
- `Minecraft.World/stdafx.h` now has an explicit `_NATIVE_DESKTOP` path
  instead of falling through to Orbis headers, and the native smoke target
  now compiles `Minecraft.World/system.cpp` through a narrow
  `NativeDesktopStubs.h` shim plus `lce_win32` compatibility types
- native smoke now also runs `System::nanoTime()` and
  `System::currentTimeMillis()` through a dedicated world wrapper instead of
  treating `system.cpp` as compile-only coverage
- native smoke now compiles and round-trips `Minecraft.World/FileHeader.cpp`
  so save-header metadata layout, duplicate-entry reuse, offsets, endian
  selection, and version fields are checked on macOS/Linux paths
- native smoke now compiles and round-trips `Minecraft.World/compression.cpp`
  with bundled zlib, covering `Compress`, `CompressRLE`, `CompressLZXRLE`,
  and the matching native decompression paths
- native smoke is now wired into `CTest`, `CMakePresets.json`, and a
  `Minecraft.Portability.Check` target so portability regressions can be
  exercised by automation instead of only ad hoc manual runs
- a dedicated `.github/workflows/native-smoke.yml` workflow now runs the
  same native smoke presets on macOS and Linux in CI
- dedicated server storage layout no longer hard-codes `Windows64/GameHDD`
  inside `WorldManager`; it now resolves through
  `Minecraft.Server/Common/ServerStoragePaths.cpp`, with native smoke
  asserting the `NativeDesktop/GameHDD` path for macOS/Linux work
- process working-directory bootstrap is now moving behind
  `include/lce_process/lce_process.cpp`, so dedicated server startup can
  resolve config and save files from the executable directory without
  directly open-coding `GetModuleFileName` / `SetCurrentDirectory`
- dedicated server runtime state and autosave scheduling are now moving out
  of `ServerMain.cpp` into
  `Minecraft.Server/Common/DedicatedServerRuntime.cpp`, with native smoke
  verifying host name/bind IP flags and autosave deadline calculation
- `Minecraft.Server.NativeBootstrap` now builds and runs on native presets as
  a first real dedicated-server entry point for macOS/Linux; it currently
  handles executable-directory bootstrap, `server.properties` loading, access
  file initialization, storage-root creation, and a bootstrap-only startup
  path verified by CTest
- native smoke now also covers `ServerProperties.cpp` and
  `WhitelistManager.cpp`, including default file creation, normalization, save
  persistence, and whitelist JSON workflow
- dedicated server startup preflight is now being consolidated in
  `Minecraft.Server/Common/DedicatedServerBootstrap.cpp`, which centralizes
  executable-directory setup, `server.properties` loading, CLI override
  parsing, runtime-state derivation, storage-root creation, and access-control
  initialization so the native bootstrap path and `Windows64/ServerMain.cpp`
  can converge on the same server-only startup flow
- native smoke now covers that shared bootstrap module end-to-end, including
  prepare/init/shutdown transitions and `Access.cpp` state publication on the
  `_NATIVE_DESKTOP` path
- native dedicated bootstrap now also routes TCP listener setup through
  `Minecraft.Server/Common/DedicatedServerSocketBootstrap.cpp`, and native
  smoke validates the bind/listen/loopback-accept path so the non-Windows
  server shell no longer only logs a target port but actually owns one
- dedicated server host/game session parameters are now being moved behind
  `Minecraft.Server/Common/DedicatedServerSessionConfig.cpp`, so
  `ServerMain.cpp` no longer open-codes the entire host-option bitfield and
  network bootstrap parameter build; native smoke now validates the encoded
  host settings, world-size/seed flow, and clamps the legacy `256` player
  default to `255` before it crosses the `unsigned char` network boundary
- world-bootstrap result handling and autosave scheduling are now also being
  pulled behind shared server modules
  (`Minecraft.Server/Common/DedicatedServerWorldBootstrap.cpp` and
  `Minecraft.Server/Common/DedicatedServerRuntime.cpp`), so `ServerMain.cpp`
  is shrinking toward a thin coordinator while native smoke verifies
  save-id persistence decisions, initial-save gating, and autosave deadline
  transitions without depending on D3D/UI startup
- shutdown save/wait decisions are now also moving behind
  `Minecraft.Server/Common/DedicatedServerShutdownPlan.cpp`, so the tail of
  `ServerMain.cpp` is starting to converge on shared lifecycle rules instead
  of open-coded `saveOnExit` / `ServerStoppedValid` branches

That is intentionally narrow. The existing build graph is still too tightly
coupled to Windows-only headers and libraries to move directly to a native
client target in one pass.
