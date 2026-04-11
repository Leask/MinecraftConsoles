# Headless Server-Native Harness Plan

## Goal

Run `server-native` in unattended engineering mode with a single harness entry
that:

- builds and validates the macOS native path
- syncs the tree to Linux `elm`
- builds and validates the Linux native path
- smoke-tests `Minecraft.Server.NativeBootstrap` on both platforms
- produces stage artifacts and logs that can be inspected after each iteration

This plan does not try to native-port the Windows gameplay kernel. It only
completes and hardens the headless native server runtime and its engineering
loop.

## Operating Rules

- The only shipped native server entrypoint remains
  `Minecraft.Server.NativeBootstrap`.
- Every iteration must validate on macOS and Linux before commit.
- The harness is the default execution path for unattended iterations.
- New work should prefer shrinking runtime ownership and public API surface
  rather than adding parallel wrappers.
- If a native requirement would pull `Minecraft::main`, `g_NetworkManager`,
  `MinecraftServer`, or Windows `StorageManager` back into the path, use a
  native substitute instead.

## Phase 1: Harness Foundation

### Scope

- add a single shell entrypoint under `tools/`
- standardize local macOS build/test/bootstrap validation
- standardize remote Linux sync/build/test/bootstrap validation
- write logs under `build/harness/`

### Exit Criteria

- one command runs macOS and Linux validation end-to-end
- the command fails fast on the first broken stage
- local and remote bootstrap checks both use isolated storage roots
- smoke output is captured into stage logs
- a summary artifact is written under `build/harness/summary.txt`

## Phase 2: Runtime Ownership Convergence

### Scope

- keep shrinking `runtime/host/thread/core/session/worker` glue
- ensure `session core` remains the only runtime state owner
- ensure `worker queue` remains the only side-effect owner
- keep `shared runtime snapshot` as projection only

### Exit Criteria

- no new direct shared-snapshot writes outside session projection
- no new side effects outside worker queue
- harness remains green after every contract change

## Phase 3: Headless Contract Hardening

### Scope

- harden create/load/save/reload/save-on-exit
- harden remote shell, live accept, and scripted shell
- harden corrupt-save rejection at bootstrap time
- harden previous-session summary replay and slot continuity

### Exit Criteria

- current 11 smoke cases stay green on both platforms
- bootstrap-only and shell boot both work with isolated storage roots
- corrupt saves fail before session start
- save/reload continuity remains stable after rename and restart

## Phase 4: Unattended Iteration Loop

### Scope

- drive every runtime refactor through the harness
- keep stage commits small and attributable
- record each completed stage in commit history and this document

### Exit Criteria

- a maintainer can run one harness command and get the current answer
- every stage commit is backed by harness logs
- the headless runtime remains runnable throughout refactors

## Current Stage

Stage 1 is complete.

Immediate deliverables:

1. add `tools/headless_server_native_harness.sh`
2. run the harness successfully on macOS and Linux `elm`
3. commit the harness as the default unattended validation path

## Active Stage

Stage 2 is complete.

Immediate deliverables:

1. keep routing every runtime refactor through the harness
2. reduce remaining `host/thread/core` glue only when it improves ownership
3. keep stage commits small, attributable, and dual-platform green

## Active Sprint: Startup Convergence

### Goal

Make native hosted startup ownership explicit and low-friction so that:

- `session` owns startup state and startup telemetry
- `core` only drives startup pacing and consumes a single startup result
- `runtime` only coordinates persistent vs transient startup paths
- every startup refactor still goes through the unattended harness

### Immediate Iteration Targets

1. move `null initData` startup handling into the session-side startup helper
2. remove startup telemetry writes from `core` when `session` can own them
3. keep shrinking startup-only public helper surface after each green harness run

## Completion Snapshot

The `headless server-native runtime` goal is complete.

Completed evidence:

- `Minecraft.Server.NativeBootstrap` is the only shipped native entrypoint
- the unattended harness validates macOS local build/smoke/bootstrap
- the same harness validates Linux `elm` sync/build/smoke/bootstrap
- all 11 native smoke cases stay green on both platforms
- create/load/save/reload/save-on-exit/remote-shell/live-accept remain covered
- the remaining native public surface is reduced to core owner APIs rather than
  projection glue
- `build/harness/summary.txt` now records per-platform success and a final
  `goal.headless_server_native_runtime=complete` marker

Stage status:

- Phase 1: complete
- Phase 2: complete
- Phase 3: complete
- Phase 4: complete

## Next Stage Queue

After the harness is in place, the next unattended iterations should target:

1. remaining `host/core` startup result glue
2. remaining `core/session/worker` result ownership cleanup
3. only then, deeper runtime-core contract work

## Phase 5: Native Core Convergence

### Scope

- move hosted thread lifecycle ownership into `core`
- keep shrinking `runtime/bridge` responsibilities down to wiring only
- make `core` the canonical owner of ready/stopped lifecycle callbacks
- continue validating every iteration through the unattended harness

### Exit Criteria

- `thread bridge` no longer patches lifecycle state after core returns
- `core` emits enough lifecycle context for runtime/session to project state
- macOS and Linux harness runs stay green after each core contract change

## Current Stage

Phase 5 is in progress.

Immediate deliverables:

1. route `ready/stopped` lifecycle callbacks directly through `core`
2. keep `bridge` as a thin adapter over thread entry only
3. keep dual-platform harness validation mandatory for each checkpoint

Checkpoint status:

- `core` owns hosted thread ready/stopped session projection and host-ready
  signaling
- `thread bridge` only adapts the native hosted thread proc to the core entry
- smoke coverage asserts direct core ready/stopped observers see session state
