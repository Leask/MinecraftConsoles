# Native-Only Port Plan

## Goal

`main` is now a macOS/Linux native-only branch. The archived
Windows-compatible state lives on `last-windows-compatible`; new work on
`main` must not restore that build path.

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

Status: future work.

Direction:

- keep native client smoke green
- remove legacy platform assumptions from client startup
- build renderer and input work only through macOS/Linux abstractions
- do not restore archived platform compatibility to make client progress faster

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
