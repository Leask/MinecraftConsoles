# Build Instructions

This branch only supports macOS and Linux native builds. Windows compatibility
maintenance has stopped on `main`; Windows build instructions are no longer
kept here. The archived compatibility line is `last-windows-compatible`.

## Requirements

- CMake 3.24 or newer
- Ninja with multi-config support
- A C++17 compiler
- POSIX shell utilities

Recommended:

- macOS: Apple Clang from current Xcode command line tools
- Linux: GCC or Clang plus standard development headers

## macOS Native Server

Configure:

```bash
cmake --preset macos-native
```

Build and run the native smoke suite:

```bash
cmake --build --preset macos-native-debug --target Minecraft.Portability.Check
```

Run the native server bootstrap manually:

```bash
./build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --bootstrap-only
```

Run a short shell-mode session:

```bash
./build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --shutdown-after-ms 250 \
    -port 0 \
    -bind 127.0.0.1 \
    -name NativeShell
```

## Linux Native Server

Configure:

```bash
cmake --preset linux-native
```

Build and run the native smoke suite:

```bash
cmake --build --preset linux-native-debug --target Minecraft.Portability.Check
```

Run the native server bootstrap manually:

```bash
./build/linux-native/Portability/Debug/Minecraft.Server.NativeBootstrap \
    --bootstrap-only
```

## Native Client Smoke

The native client is an early macOS/Linux smoke target. It is not the primary
shipping artifact yet.

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

## Unattended Harness

Use the harness for server-native runtime iterations:

```bash
tools/headless_server_native_harness.sh
```

The harness writes logs under `build/harness/` and a summary file at:

```text
build/harness/summary.txt
```

Useful overrides:

| Variable | Purpose |
|----------|---------|
| `REMOTE_HOST` | Linux host used by the harness, default `elm` |
| `REMOTE_ROOT` | Remote checkout path |
| `LOCAL_BUILD_PRESET` | Local build preset |
| `REMOTE_CONFIGURE_PRESET` | Remote configure preset |
| `REMOTE_BUILD_DIR` | Remote build directory |

## Build Presets

| Preset | Purpose |
|--------|---------|
| `macos-native` | macOS native headless runtime configure |
| `linux-native` | Linux native headless runtime configure |
| `macos-native-client` | macOS native client smoke configure |
| `linux-native-client` | Linux native client smoke configure |

## Build Targets

| Target | Purpose |
|--------|---------|
| `Minecraft.Portability.Check` | Native server/runtime smoke suite |
| `Minecraft.Server.NativeBootstrap` | Native headless server executable |
| `Minecraft.Portability.SmokeTest` | Shared portability smoke executable |
| `Minecraft.NativeDesktop.Check` | Experimental native client smoke suite |

## Notes

- `Minecraft.Server.NativeBootstrap` is the only native server executable.
- Server storage defaults to `NativeDesktop/GameHDD` unless overridden with
  `--storage-root`.
- `server.properties` is read from the executable working directory.
- CI uses `.github/workflows/native-smoke.yml` for macOS and Linux validation.
