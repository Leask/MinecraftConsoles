#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
log_root="${HARNESS_LOG_DIR:-$repo_root/build/harness}"
remote_host="${REMOTE_HOST:-elm}"
remote_root="${REMOTE_ROOT:-/home/leask/MinecraftConsoles}"

local_build_preset="${LOCAL_BUILD_PRESET:-macos-native-debug}"
local_test_target="${LOCAL_TEST_TARGET:-Minecraft.Portability.Check}"
local_bootstrap_bin="${LOCAL_BOOTSTRAP_BIN:-$repo_root/build/macos-native/Portability/Debug/Minecraft.Server.NativeBootstrap}"

remote_configure_preset="${REMOTE_CONFIGURE_PRESET:-linux-native}"
remote_build_dir="${REMOTE_BUILD_DIR:-build/linux-elm}"
remote_test_target="${REMOTE_TEST_TARGET:-Minecraft.Portability.Check}"
remote_bootstrap_bin="${REMOTE_BOOTSTRAP_BIN:-$remote_root/$remote_build_dir/Portability/Minecraft.Server.NativeBootstrap}"

sync_excludes=(
    "--exclude" ".git"
    "--exclude" "build"
)

run_and_log() {
    local name="$1"
    shift

    local log_file="$log_root/$name.log"
    echo "==> $name"
    "$@" > >(tee "$log_file") 2>&1
}

run_local_build_and_test() {
    run_and_log \
        local-build-test \
        cmake \
        --build \
        --preset \
        "$local_build_preset" \
        --target \
        "$local_test_target"
}

run_local_bootstrap() {
    local storage_root="$log_root/local-bootstrap-storage"

    rm -rf "$storage_root"
    mkdir -p "$storage_root"

    run_and_log \
        local-bootstrap \
        "$local_bootstrap_bin" \
        --bootstrap-only \
        --storage-root \
        "$storage_root" \
        -port \
        0 \
        -bind \
        127.0.0.1 \
        -name \
        HarnessLocal
}

sync_remote_tree() {
    run_and_log \
        remote-sync \
        rsync \
        -az \
        --delete \
        "${sync_excludes[@]}" \
        "$repo_root/" \
        "$remote_host:$remote_root/"
}

run_remote_build_and_test() {
    local remote_cmd
    remote_cmd=$(
        cat <<EOF
set -euo pipefail
export PATH="\$HOME/.local/bin:\$PATH"
cd "$remote_root"
if command -v ninja >/dev/null 2>&1; then
    cmake --preset "$remote_configure_preset"
else
    cmake \
        -S . \
        -B "$remote_build_dir" \
        -G "Unix Makefiles" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DMINECRAFT_BUILD_PORTABILITY=ON \
        -DMINECRAFT_BUILD_WORLD=OFF \
        -DMINECRAFT_BUILD_CLIENT=OFF \
        -DMINECRAFT_BUILD_SERVER=OFF \
        -DPLATFORM_DEFINES=_NATIVE_DESKTOP \
        -DPLATFORM_NAME=NativeDesktop
fi
cmake --build "$remote_build_dir" --target "$remote_test_target" -j4
EOF
    )

    run_and_log \
        remote-build-test \
        ssh \
        "$remote_host" \
        "$remote_cmd"
}

run_remote_bootstrap() {
    local remote_cmd
    remote_cmd=$(
        cat <<EOF
set -euo pipefail
storage_root=\$(mktemp -d /tmp/minecraft-native-harness.XXXXXX)
cleanup() {
    rm -rf "\$storage_root"
}
trap cleanup EXIT
export PATH="\$HOME/.local/bin:\$PATH"
cd "$remote_root"
"$remote_bootstrap_bin" \
    --bootstrap-only \
    --storage-root "\$storage_root" \
    -port 0 \
    -bind 127.0.0.1 \
    -name HarnessLinux
EOF
    )

    run_and_log \
        remote-bootstrap \
        ssh \
        "$remote_host" \
        "$remote_cmd"
}

main() {
    mkdir -p "$log_root"

    echo "Harness root: $log_root"
    echo "Repo root: $repo_root"
    echo "Remote host: $remote_host"
    echo "Remote root: $remote_root"

    run_local_build_and_test
    run_local_bootstrap
    sync_remote_tree
    run_remote_build_and_test
    run_remote_bootstrap

    echo "Harness completed successfully."
}

main "$@"
