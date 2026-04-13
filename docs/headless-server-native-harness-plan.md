# Headless Server-Native Harness Plan

## Goal

`main` is now a macOS/Linux native branch. The unattended harness exists to keep
`Minecraft.Server.NativeBootstrap` runnable while native runtime ownership keeps
shrinking.

The harness validates one product surface:

- macOS native configure, build, smoke, and bootstrap checks
- Linux native sync, configure, build, smoke, and bootstrap checks
- isolated storage roots for every bootstrap case
- stage logs under `build/harness/`
- a summary artifact under `build/harness/summary.txt`

No Windows runtime, Wine packaging, or desktop compatibility path is part of
this plan. Windows compatibility maintenance has stopped on `main`; the
archived compatibility line is `last-windows-compatible`.

## Operating Rules

- The only native server entrypoint is `Minecraft.Server.NativeBootstrap`.
- Every unattended iteration must pass the native harness before commit.
- Runtime state is owned by `NativeDedicatedServerHostedGameSession`.
- Command side effects are owned by `NativeDedicatedServerHostedGameWorker`.
- Shared runtime snapshots are projection output, not independent state.
- If a new feature needs a removed platform runtime to work, implement a native
  macOS/Linux substitute instead.

## Phase 1: Harness Foundation

Scope:

- add a single shell entrypoint under `tools/`
- standardize local macOS build/test/bootstrap validation
- standardize remote Linux sync/build/test/bootstrap validation
- write logs under `build/harness/`

Exit criteria:

- one command runs native validation end-to-end
- the command fails fast on the first broken stage
- bootstrap checks use isolated storage roots
- smoke output is captured into stage logs
- `summary.txt` records per-platform success

Status: complete.

## Phase 2: Runtime Ownership Convergence

Scope:

- keep shrinking `runtime/host/thread/core/session/worker` glue
- keep `session core` as the only runtime state owner
- keep `worker queue` as the only side-effect owner
- keep `shared runtime snapshot` as projection only

Exit criteria:

- no direct shared-snapshot writes outside session projection
- no command side effects outside worker queue
- harness remains green after every contract change

Status: complete.

## Phase 3: Headless Contract Hardening

Scope:

- harden create/load/save/reload/save-on-exit
- harden remote shell, live accept, and scripted shell
- reject corrupt saves during bootstrap
- replay previous-session summaries and slot continuity

Exit criteria:

- all native smoke cases stay green on both platforms
- bootstrap-only and shell boot both work with isolated storage roots
- corrupt saves fail before session start
- save/reload continuity stays stable after rename and restart

Status: complete.

## Phase 4: Native-Only Mainline

Scope:

- remove active Windows build presets and release workflows from `main`
- remove Wine/Docker server runtime packaging from `main`
- move native-generated headers out of removed platform directories
- make CMake fail early on unsupported host platforms
- keep docs focused on macOS/Linux native development only
- remove stale generated project documents tied to removed platforms

Exit criteria:

- `main` documents macOS/Linux as the only supported platforms
- native builds no longer include removed platform entrypoints
- `last-windows-compatible` remains the only archived compatibility branch
- macOS native smoke is green after the cutover
- Linux native smoke is green or blocked only by remote toolchain availability

Status: complete for the current native-only cutover.

## Current Focus

This stage is about cutting over `main`, not re-porting the full gameplay client.
The active server-native path must stay buildable and testable first. Legacy
gameplay names that remain as ABI shims should be isolated behind native
headers and removed in later client convergence work.
