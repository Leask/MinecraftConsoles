![Legacy Edition Banner](.github/banner.png)

# MinecraftConsoles

[![Native Smoke](https://github.com/MCLCE/MinecraftConsoles/actions/workflows/native-smoke.yml/badge.svg?branch=main)](https://github.com/MCLCE/MinecraftConsoles/actions/workflows/native-smoke.yml)
[![Discord](https://img.shields.io/badge/Discord-Join%20Server-5865F2?logo=discord&logoColor=white)](https://discord.gg/jrum7HhegA)

MinecraftConsoles is now a macOS/Linux native-only Minecraft Legacy Console
Edition codebase. Starting from this branch, `main` no longer carries Windows
build support, release automation, Wine/Docker runtime support, or Windows
compatibility goals.

Windows compatibility maintenance has stopped on `main`. Any remaining
Windows-like symbol names are legacy ABI terminology used by the macOS/Linux
native shims, not a supported Windows runtime path.

The last branch that intentionally preserves the previous Windows-compatible
state is:

```text
last-windows-compatible
```

Use that branch if you need the archived Windows client/server build. All new
work on `main` targets macOS and Linux only.

## Current Goal

The active deliverable is the native headless server runtime:

```text
Minecraft.Server.NativeBootstrap
```

The project direction is:

- make native macOS/Linux server startup reliable
- keep create/load/save/reload/shutdown under native smoke coverage
- keep stdin and plain-text TCP shell control as the admin surface
- continue shrinking legacy platform glue behind native runtime contracts
- move toward a native desktop client only through macOS/Linux build paths

The old Windows client, Windows dedicated server, Direct3D path, Wine Docker
server, and Windows release workflows are intentionally out of scope for `main`.
Do not treat code cleanup that preserves those removed paths as a supported
maintenance goal.

## Platform Status

| Target | Entry Point | Status |
|--------|-------------|--------|
| macOS native headless server | `Minecraft.Server.NativeBootstrap` | Primary supported path |
| Linux native headless server | `Minecraft.Server.NativeBootstrap` | Primary supported path |
| macOS native client | `Minecraft.Client` | Early smoke target |
| Linux native client | `Minecraft.Client` | Early smoke target |

No other platform is a supported target on `main`.

## Quick Start

### macOS Headless Runtime

```bash
cmake --preset macos-native
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

Run the native server bootstrap:

```bash
./build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --bootstrap-only
```

### Linux Headless Runtime

```bash
cmake --preset linux-native
cmake --build --preset linux-native-debug --target Minecraft.Portability.Check
```

Run the native server bootstrap:

```bash
./build/linux-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --bootstrap-only
```

## Native Headless Server

`Minecraft.Server.NativeBootstrap` is the only native server executable. It is
not a wrapper around the old desktop gameplay startup chain.

Supported runtime coverage includes:

- bootstrap-only startup validation
- native storage-root overrides
- create/load/save/reload lifecycle smoke tests
- save-on-exit persistence
- corrupt save rejection during bootstrap/load
- autosave scheduling
- stdin shell commands
- plain-text TCP remote shell commands
- connection and command lineage in runtime status

Example:

```bash
./build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --storage-root ./native-server-data \
    -name NativeServer \
    -port 25565 \
    -loglevel info
```

### Shell Commands

| Command | Behavior |
|---------|----------|
| `help` | Print supported commands |
| `status` | Report world, runtime, autosave, worker, save, and session state |
| `save` | Enqueue a save through the native worker queue |
| `stop` | Enqueue controlled shutdown through the native worker queue |

### Bootstrap Test Options

| Option | Purpose |
|--------|---------|
| `--bootstrap-only` | Initialize bootstrap state and exit |
| `--self-connect` | Loop back one TCP client and exit |
| `--shell-self-connect` | Loop back one TCP client while shell mode runs |
| `--shutdown-after-ms <ms>` | Run shell mode for a bounded time |
| `--require-accepted-connections <n>` | Fail unless at least `n` clients were accepted |
| `--require-remote-commands <n>` | Fail unless at least `n` remote commands ran |
| `--storage-root <path>` | Override native storage root |
| `--command <cmd>` | Run one native shell command before stdin |
| `--shell-self-command <cmd>` | Send one loopback remote shell command |

## Server Configuration

The native server reads `server.properties` from the executable working
directory. Missing or invalid values are generated or normalized on startup.
Command-line arguments override the file.

Common keys:

| Key | Values / Range | Default | Notes |
|-----|----------------|---------|-------|
| `server-port` | `1-65535` | `25565` | TCP listen port |
| `server-ip` | address string | `0.0.0.0` | Bind address |
| `server-name` | string, max 16 chars | `DedicatedServer` | Host display name |
| `max-players` | `1-256` | `16` | Public player slots |
| `level-name` | string | `world` | Display world name |
| `level-id` | safe ID string | `world` | Save slot ID |
| `level-seed` | int64 or empty | empty | Empty means random seed |
| `world-size` | `classic`, `small`, `medium`, `large` | `classic` | World size preset |
| `log-level` | `debug`, `info`, `warn`, `error` | `info` | Log verbosity |
| `autosave-interval` | `5-3600` | `60` | Seconds between autosaves |
| `white-list` | `true` / `false` | `false` | Enable access checks |
| `lan-advertise` | `true` / `false` | `false` | LAN advertisement flag |

Common CLI overrides:

| Argument | Description |
|----------|-------------|
| `-port <0-65535>` | Override `server-port`; `0` requests an ephemeral port |
| `-ip <addr>` | Override `server-ip` |
| `-bind <addr>` | Alias of `-ip` |
| `-name <name>` | Override `server-name` |
| `-maxplayers <1-256>` | Override public slots |
| `-seed <int64>` | Override `level-seed` |
| `-loglevel <level>` | Override `log-level` |
| `-help`, `--help`, `-h` | Print usage |

## Unattended Harness

The native server engineering harness is:

```bash
tools/headless_server_native_harness.sh
```

By default it:

- builds and validates the local macOS native path
- syncs to Linux host `elm`
- builds and validates the Linux native path
- runs bootstrap, live shell, and signal checks
- writes logs under `build/harness/`

Override `REMOTE_HOST`, `REMOTE_ROOT`, and preset variables when using a
different Linux machine.

## Native Client Smoke

The native desktop client remains an early smoke target, not the main shipping
artifact yet.

macOS:

```bash
cmake --preset macos-native-client
cmake --build --preset macos-native-client-debug \
    --target Minecraft.NativeDesktop.Check
```

Linux:

```bash
cmake --preset linux-native-client
cmake --build --preset linux-native-client-debug \
    --target Minecraft.NativeDesktop.Check
```

## Repository Map

| Path | Purpose |
|------|---------|
| `Portability/` | Native bootstrap, native server runtime, native smoke tests |
| `Minecraft.Server/Common/` | Shared native server lifecycle, shell, storage, runtime contracts |
| `Minecraft.World/` | Shared world/gameplay code used by native smoke paths |
| `Minecraft.Client/NativeDesktop/` | Experimental macOS/Linux native client shell |
| `include/` | Native portability helpers and legacy ABI shims used by the native build |
| `tools/headless_server_native_harness.sh` | macOS/Linux unattended validation harness |
| `docs/native-port-plan.md` | Native-only direction and phased plan |
| `docs/headless-server-native-harness-plan.md` | Headless runtime harness notes |
| `COMPILE.md` | Build instructions |

## Development Rules

- `main` is macOS/Linux native-only.
- Do not add Windows presets, Windows release workflows, Wine runtime scripts,
  or Direct3D-backed build paths back to `main`.
- Do not maintain Windows compatibility in new code. If legacy ABI names remain,
  keep them isolated behind native macOS/Linux abstractions.
- Keep `Minecraft.Server.NativeBootstrap` as the only native server entry point.
- Route save, autosave, stop, and halt through the native worker/session flow.
- Validate native runtime work with `Minecraft.Portability.Check`.
- Use the full harness before stage commits when Linux validation is available.

Minimum local check:

```bash
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

Full harness:

```bash
tools/headless_server_native_harness.sh
```
