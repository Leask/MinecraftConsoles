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
- stream-mode stdin now has a portability layer, but interactive console and
  console-color paths still depend on Win32 console semantics
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
- first non-Windows Win32 compatibility shim for
  `DWORD/HANDLE/CRITICAL_SECTION/Tls*/Event/CreateThread/Wait*`

That is intentionally narrow. The existing build graph is still too tightly
coupled to Windows-only headers and libraries to move directly to a native
client target in one pass.
