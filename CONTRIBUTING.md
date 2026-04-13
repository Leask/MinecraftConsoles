# Contributing

`main` is a macOS/Linux native-only branch. Windows compatibility maintenance
has stopped on this line of development. Pull requests must not add Windows
presets, Windows release automation, Wine/Docker runtime paths, Direct3D-backed
build paths, or other compatibility work for removed platforms.

The archived compatibility line is `last-windows-compatible`. Use that branch
for historical Windows build work.

## Project Scope

Current supported targets:

- macOS native headless server
- Linux native headless server
- macOS native client smoke
- Linux native client smoke

Current priorities:

- keep `Minecraft.Server.NativeBootstrap` reliable on macOS and Linux
- keep native create/load/save/reload/shutdown covered by smoke tests
- keep stdin and plain-text TCP shell behavior stable
- continue isolating legacy platform assumptions behind native abstractions
- move the native desktop client forward only through macOS/Linux paths

Out of scope for `main`:

- Windows client or server builds
- Wine-backed runtime packaging
- console SDK/toolchain restoration
- Direct3D renderer restoration as a supported build path
- platform-specific release automation outside macOS/Linux native validation

## Pull Request Requirements

- Keep changes focused on one topic.
- Document behavior changes clearly.
- Include the relevant native smoke or harness results.
- Do not bypass the native worker/session ownership model for server saves,
  autosaves, stop, or halt behavior.
- Do not reintroduce removed platform directories, presets, or workflows.

Minimum server validation:

```bash
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

Full server-native validation when Linux is available:

```bash
tools/headless_server_native_harness.sh
```

Native client smoke validation:

```bash
cmake --build --preset macos-native-client-debug \
    --target Minecraft.NativeDesktop.Check
```

## AI And Automation Disclosure

AI-assisted or automated changes must be disclosed in the pull request. The
submitter is responsible for reviewing the result, explaining the design, and
providing passing native validation.
