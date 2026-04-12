![Legacy Edition Banner](.github/banner.png)

# MinecraftConsoles

[![Native Smoke](https://github.com/Leask/MinecraftConsoles/actions/workflows/native-smoke.yml/badge.svg?branch=main)](https://github.com/Leask/MinecraftConsoles/actions/workflows/native-smoke.yml)
[![Nightly Client](https://github.com/Leask/MinecraftConsoles/actions/workflows/nightly.yml/badge.svg?branch=main)](https://github.com/Leask/MinecraftConsoles/actions/workflows/nightly.yml)
[![Nightly Server](https://github.com/Leask/MinecraftConsoles/actions/workflows/nightly-server.yml/badge.svg?branch=main)](https://github.com/Leask/MinecraftConsoles/actions/workflows/nightly-server.yml)
[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/jrum7HhegA)

MinecraftConsoles is a maintained Minecraft Legacy Console Edition codebase
based on LCE v1.6.0560.0 / TU19. This repository is being reshaped into a
portable desktop and server foundation while keeping the existing Windows
gameplay build shippable.

The current engineering focus is narrow and deliberate:

- keep the Windows64 client and dedicated server usable
- extract server/runtime code away from Windows-only startup assumptions
- ship a native macOS/Linux headless server path through
  `Minecraft.Server.NativeBootstrap`
- validate portability work with repeatable smoke tests and CI
- absorb upstream fixes selectively, by behavior, instead of blind merges

This is not yet a full native macOS/Linux game client. Native client work is
currently limited to build and smoke coverage while the server/runtime layer is
being made stable first.

## Status

| Target | Entry point | Status | Notes |
|--------|-------------|--------|-------|
| Windows64 client | `Minecraft.Client.exe` | Playable desktop target | DirectX 11 path, keyboard/mouse, fullscreen, LAN, server list, split-screen improvements |
| Windows64 dedicated server | `Minecraft.Server.exe` | Release target | Runs on Windows directly and in the Wine-based Docker image |
| macOS/Linux native headless server | `Minecraft.Server.NativeBootstrap` | Active native deliverable | Smoke-tested create/load/save/reload/shutdown/remote-shell lifecycle |
| macOS/Linux native client | native client presets | Early smoke only | Not a user-facing playable client yet |
| Console targets | legacy source tree | Not actively validated | Toolchain files remain, but current work does not verify console builds |

## Downloads

Nightly releases are generated from `main`.

- [Nightly Client](https://github.com/Leask/MinecraftConsoles/releases/tag/nightly)
- [Nightly Dedicated Server](https://github.com/Leask/MinecraftConsoles/releases/tag/nightly-dedicated-server)

For the Windows client, download `LCEWindows64.zip`, extract it, and run
`Minecraft.Client.exe` from the extracted directory. You can create
`username.txt` next to the executable to set the default username.

For the Windows dedicated server, download `LCEServerWindows64.zip`, extract it,
and run `Minecraft.Server.exe` from the extracted directory.

## Quick Validation

### macOS native headless path

```bash
cmake --preset macos-native
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

Run the native bootstrap directly:

```bash
./build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --bootstrap-only
```

### Linux native headless path

```bash
cmake --preset linux-native
cmake --build --preset linux-native-debug --target Minecraft.Portability.Check
```

Run the native bootstrap directly:

```bash
./build/linux-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --bootstrap-only
```

### Unattended headless harness

The project includes an engineering harness for the native headless server:

```bash
tools/headless_server_native_harness.sh
```

By default it validates the local macOS native path, syncs to `elm`, validates
the Linux native path, runs bootstrap/live/signal checks, and writes logs under
`build/harness/`. Override `REMOTE_HOST`, `REMOTE_ROOT`, and the preset
environment variables when using a different Linux machine.

### Windows64 build

Use Visual Studio 2022 or newer, or build from a Developer PowerShell:

```powershell
cmake --preset windows64
cmake --build --preset windows64-release --target Minecraft.Client
cmake --build --preset windows64-release --target Minecraft.Server
```

See [COMPILE.md](COMPILE.md) for full Visual Studio and CMake details.

## Native Headless Server

`Minecraft.Server.NativeBootstrap` is the single native server entry point for
macOS and Linux. It is designed as a server-native runtime, not as a wrapper
around the Windows gameplay startup chain.

The native path currently provides:

- bootstrap-only startup validation
- isolated storage roots with `--storage-root`
- create/load/save/reload coverage through the native storage adapter
- `.save` text stubs as the native persistence contract
- autosave and save-on-exit coverage
- stdin and plain-text TCP shell control
- remote shell lineage and connection accounting
- corruption rejection during bootstrap/load instead of after session start

Example:

```bash
./build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --storage-root ./native-server-data \
    -name NativeServer \
    -port 25565 \
    -loglevel info
```

Native shell commands are intentionally small:

| Command | Behavior |
|---------|----------|
| `help` | Print supported commands |
| `status` | Report world identity, runtime phase, autosave, worker, save path, session summary, and command lineage |
| `save` | Enqueue a save through the native worker queue |
| `stop` | Enqueue controlled shutdown through the native worker queue |

Useful bootstrap test options:

| Option | Purpose |
|--------|---------|
| `--bootstrap-only` | Initialize storage/bootstrap state and exit |
| `--self-connect` | Loop back one TCP client and exit |
| `--shell-self-connect` | Loop back one TCP client while the shell is running |
| `--shutdown-after-ms <ms>` | Run shell mode for a bounded time |
| `--require-accepted-connections <n>` | Fail unless at least `n` clients were accepted |
| `--require-remote-commands <n>` | Fail unless at least `n` remote commands ran |
| `--storage-root <path>` | Override the native storage root |
| `--command <cmd>` | Run one native shell command before stdin |
| `--shell-self-command <cmd>` | Send one loopback remote shell command |

## Dedicated Server Configuration

Both dedicated server paths read `server.properties` from the executable working
directory. Missing or invalid values are generated or normalized on startup.
CLI arguments override the file.

Common keys:

| Key | Values / Range | Default | Notes |
|-----|----------------|---------|-------|
| `server-port` | `1-65535` | `25565` | TCP listen port |
| `server-ip` | address string | `0.0.0.0` | Bind address |
| `server-name` | string, max 16 chars | `DedicatedServer` | Host display name |
| `max-players` | `1-256` | `16` | Public player slots |
| `level-name` | string | `world` | Display world name |
| `level-id` | safe ID string | `world` | Save slot ID, normalized automatically |
| `level-seed` | int64 or empty | empty | Empty means random seed |
| `world-size` | `classic`, `small`, `medium`, `large` | `classic` | World size preset |
| `log-level` | `debug`, `info`, `warn`, `error` | `info` | Server log verbosity |
| `autosave-interval` | `5-3600` | `60` | Seconds between autosaves |
| `white-list` | `true` / `false` | `false` | Enable access list checks |
| `lan-advertise` | `true` / `false` | `false` | LAN session advertisement |

Common CLI overrides:

| Argument | Description |
|----------|-------------|
| `-port <0-65535>` | Override `server-port`; `0` requests an ephemeral port where supported |
| `-ip <addr>` | Override `server-ip` |
| `-bind <addr>` | Alias of `-ip` |
| `-name <name>` | Override `server-name` |
| `-maxplayers <1-256>` | Override public slots |
| `-seed <int64>` | Override `level-seed` |
| `-loglevel <level>` | Override `log-level` |
| `-help`, `--help`, `-h` | Print usage |

## Docker Dedicated Server

The Docker setup runs the Windows dedicated server under Wine. This remains
separate from the native headless server.

Start the GHCR-backed image:

```bash
./start-dedicated-server.sh
```

Skip pulling and only start the existing local image:

```bash
./start-dedicated-server.sh --no-pull
```

Equivalent compose command:

```bash
docker compose -f docker-compose.dedicated-server.ghcr.yml up -d
```

Build a local image from a local `Minecraft.Server` runtime:

```bash
docker compose -f docker-compose.dedicated-server.yml up -d --build
```

Common environment variables:

| Variable | Default | Purpose |
|----------|---------|---------|
| `SERVER_BIND_IP` | `0.0.0.0` | Server bind address inside the container |
| `SERVER_PORT` | `25565` | TCP/UDP port exposed by compose |
| `SERVER_CLI_INPUT_MODE` | `stream` | Dedicated server stdin mode |
| `XVFB_DISPLAY` | `:99` | Virtual display for Wine |
| `XVFB_SCREEN` | `720x1280x16` | Minimum virtual screen for Wine |
| `TZ` | `Etc/UTC` | Container timezone |

Persistent files are mounted under `./server-data`.

## Windows Client Notes

### Launch arguments

| Argument | Description |
|----------|-------------|
| `-name <username>` | Override the in-game username |
| `-fullscreen` | Launch in fullscreen mode |

Example:

```powershell
Minecraft.Client.exe -name Steve -fullscreen
```

### Keyboard and mouse controls

| Action | Binding |
|--------|---------|
| Movement | `W` `A` `S` `D` |
| Jump / fly up | `Space` |
| Sneak / fly down | hold `Shift` |
| Sprint | hold `Ctrl` or double-tap `W` |
| Inventory | `E` |
| Chat | `T` |
| Drop item | `Q` |
| Crafting | `C`; use `Q` and `E` to move tabs |
| Toggle view | `F5` |
| Fullscreen | `F11` |
| Pause menu | `Esc` |
| Attack / destroy | left click |
| Use / place | right click |
| Select hotbar item | mouse wheel or `1` to `9` |
| Tutorial accept / decline | `Enter` / `B` |
| Player list and host options | `Tab` |
| Toggle HUD | `F1` |
| Toggle debug info | `F3` |
| Open debug overlay | `F4` |
| Toggle debug console | `F6` |

## LAN Multiplayer

LAN multiplayer is available in the Windows gameplay build.

- Hosting a multiplayer world advertises it on the local network.
- Other players on the same LAN can discover the session from Join Game.
- Game connections use TCP port `25565` by default.
- LAN discovery uses UDP port `25566`.
- Server list entries can be managed in-game.
- `uid.dat` keeps player identity stable across username changes.
- Split-screen players can join multiplayer sessions.

## Repository Map

| Path | Purpose |
|------|---------|
| `Minecraft.Client/` | Windows client and shared client-side code |
| `Minecraft.Server/` | Dedicated server, shared headless runtime, server properties, Docker runtime support |
| `Minecraft.World/` | Shared world/gameplay code |
| `Portability/` | Native bootstrap, native server runtime, native smoke tests |
| `include/` | Extracted portable helper libraries |
| `tools/headless_server_native_harness.sh` | macOS/Linux headless runtime validation harness |
| `docs/native-port-plan.md` | Native port direction and phased plan |
| `docs/headless-server-native-harness-plan.md` | Headless server-native harness and completion notes |
| `COMPILE.md` | Detailed build instructions |

## Development Policy

The project keeps Windows gameplay working while moving portable runtime code
out of Windows-only layers. Native server work should prefer narrow adapters and
shared contracts over more conditional compilation in gameplay code.

Upstream changes are reviewed one commit at a time. Fixes that still match this
repository's direction are reimplemented in the current layout; Windows-only
or policy-only changes are not merged blindly.

Before sending changes, run the narrowest relevant check:

```bash
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

For native server/runtime work, prefer the full harness when Linux validation is
available:

```bash
tools/headless_server_native_harness.sh
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution expectations.
