#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
log_root="${HARNESS_LOG_DIR:-$repo_root/build/harness}"
summary_file="$log_root/summary.txt"
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

current_step=''
local_build_ok=0
local_bootstrap_ok=0
local_live_ok=0
local_signal_ok=0
remote_sync_ok=0
remote_build_ok=0
remote_bootstrap_ok=0
remote_live_ok=0
remote_signal_ok=0
source_contract_ok=0

write_summary_header() {
    local git_head
    git_head="$(git -C "$repo_root" rev-parse HEAD)"

    cat > "$summary_file" <<EOF
result=running
started_at=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
repo_root=$repo_root
git_head=$git_head
remote_host=$remote_host
remote_root=$remote_root
log_root=$log_root
EOF
}

append_summary_line() {
    printf '%s\n' "$1" >> "$summary_file"
}

mark_summary_failed() {
    local exit_code="$1"

    {
        printf 'result=failure\n'
        printf 'failed_step=%s\n' "${current_step:-unknown}"
        printf 'exit_code=%s\n' "$exit_code"
        printf 'finished_at=%s\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    } >> "$summary_file"
}

mark_summary_success() {
    {
        printf 'result=success\n'
        printf 'finished_at=%s\n' "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    } >> "$summary_file"
}

on_error() {
    local exit_code="$?"
    mark_summary_failed "$exit_code"
    exit "$exit_code"
}

run_and_log() {
    local name="$1"
    shift

    local log_file="$log_root/$name.log"
    current_step="$name"
    echo "==> $name"
    append_summary_line "step.$name.log=$log_file"
    "$@" > >(tee "$log_file") 2>&1
    append_summary_line "step.$name=ok"
}

wait_for_log_marker() {
    local log_file="$1"
    local marker="$2"
    local pid="$3"

    for _ in $(seq 1 100); do
        if grep -Fq "$marker" "$log_file"; then
            return 0
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            return 1
        fi
        sleep 0.05
    done

    return 1
}

assert_log_contains() {
    local log_file="$1"
    local marker="$2"

    if ! grep -Fq "$marker" "$log_file"; then
        echo "Expected log marker not found: $marker" >&2
        cat "$log_file" >&2
        return 1
    fi
}

assert_save_uint_at_least() {
    local save_file="$1"
    local key="$2"
    local minimum="$3"
    local value

    value="$(
        awk -F= -v key="$key" '$1 == key { print $2; found = 1; exit }
            END { if (!found) exit 2 }' "$save_file"
    )"
    if [[ ! "$value" =~ ^[0-9]+$ || "$value" -lt "$minimum" ]]; then
        echo "Expected $key >= $minimum, got ${value:-missing}" >&2
        cat "$save_file" >&2
        return 1
    fi
}

assert_save_contains() {
    local save_file="$1"
    local marker="$2"

    if ! grep -Fq "$marker" "$save_file"; then
        echo "Expected save marker not found: $marker" >&2
        cat "$save_file" >&2
        return 1
    fi
}

run_source_contract_checks() {
    local shell_source
    local log_file

    shell_source="$repo_root/Minecraft.Server/Common/DedicatedServerHeadlessShell.cpp"
    log_file="$log_root/source-contract.log"
    current_step="source-contract"

    echo "==> source-contract"
    append_summary_line "step.source-contract.log=$log_file"

    {
        if grep -Fq \
            "ProjectNativeDedicatedServerHostedGameWorkerToRuntimeSnapshot" \
            "$shell_source"; then
            echo \
                "DedicatedServerHeadlessShell must use session projection APIs" \
                >&2
            return 1
        fi

        echo "source contract checks passed"
    } > >(tee "$log_file") 2>&1

    append_summary_line "step.source-contract=ok"
    source_contract_ok=1
    append_summary_line "source.contract=ok"
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
    local_build_ok=1
    append_summary_line "local.build_test=ok"
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
    local_bootstrap_ok=1
    append_summary_line "local.bootstrap=ok"
}

run_local_live() {
    local storage_root="$log_root/local-live-storage"

    rm -rf "$storage_root"
    mkdir -p "$storage_root"

    run_and_log \
        local-live \
        "$local_bootstrap_bin" \
        --shutdown-after-ms \
        120 \
        --storage-root \
        "$storage_root" \
        -port \
        0 \
        -bind \
        127.0.0.1 \
        -name \
        HarnessLocalLive
    local_live_ok=1
    append_summary_line "local.live=ok"
}

run_local_signal() {
    local storage_root="$log_root/local-signal-storage"
    local log_file="$log_root/local-signal.log"
    local save_file="$storage_root/world.save"
    local pid

    current_step="local-signal"
    echo "==> local-signal"
    append_summary_line "step.local-signal.log=$log_file"

    rm -rf "$storage_root"
    mkdir -p "$storage_root"

    "$local_bootstrap_bin" \
        --storage-root \
        "$storage_root" \
        -port \
        0 \
        -bind \
        127.0.0.1 \
        -name \
        HarnessLocalSignal > "$log_file" 2>&1 &
    pid="$!"

    if ! wait_for_log_marker "$log_file" "initial save completed" "$pid"; then
        echo "local signal run did not reach initial save" >&2
        cat "$log_file" >&2
        kill -TERM "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
        return 1
    fi

    kill -TERM "$pid"
    wait "$pid"
    cat "$log_file"

    assert_log_contains "$log_file" "requesting save before shutdown"
    assert_log_contains "$log_file" "using saveOnExit for shutdown"
    assert_log_contains "$log_file" "native bootstrap shell stopped"
    assert_save_contains "$save_file" "session-completed=true"
    assert_save_contains "$save_file" "requested-app-shutdown=true"
    assert_save_contains "$save_file" "shutdown-halted=true"
    assert_save_uint_at_least "$save_file" "worker-halt-commands" 1

    append_summary_line "step.local-signal=ok"
    local_signal_ok=1
    append_summary_line "local.signal=ok"
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
    remote_sync_ok=1
    append_summary_line "remote.sync=ok"
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
    remote_build_ok=1
    append_summary_line "remote.build_test=ok"
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
    remote_bootstrap_ok=1
    append_summary_line "remote.bootstrap=ok"
}

run_remote_live() {
    local remote_cmd
    remote_cmd=$(
        cat <<EOF
set -euo pipefail
storage_root=\$(mktemp -d /tmp/minecraft-native-live.XXXXXX)
cleanup() {
    rm -rf "\$storage_root"
}
trap cleanup EXIT
export PATH="\$HOME/.local/bin:\$PATH"
cd "$remote_root"
"$remote_bootstrap_bin" \
    --shutdown-after-ms 120 \
    --storage-root "\$storage_root" \
    -port 0 \
    -bind 127.0.0.1 \
    -name HarnessLinuxLive
EOF
    )

    run_and_log \
        remote-live \
        ssh \
        "$remote_host" \
        "$remote_cmd"
    remote_live_ok=1
    append_summary_line "remote.live=ok"
}

run_remote_signal() {
    local remote_cmd
    remote_cmd=$(
        cat <<EOF
set -euo pipefail
export PATH="\$HOME/.local/bin:\$PATH"
storage_root=\$(mktemp -d /tmp/minecraft-native-signal.XXXXXX)
log_file="\$storage_root/signal.log"
save_file="\$storage_root/world.save"
cleanup() {
    rm -rf "\$storage_root"
}
wait_for_log_marker() {
    local marker="\$1"
    local pid="\$2"
    for _ in \$(seq 1 100); do
        if grep -Fq "\$marker" "\$log_file"; then
            return 0
        fi
        if ! kill -0 "\$pid" 2>/dev/null; then
            return 1
        fi
        sleep 0.05
    done
    return 1
}
assert_log_contains() {
    local marker="\$1"
    if ! grep -Fq "\$marker" "\$log_file"; then
        echo "Expected log marker not found: \$marker" >&2
        cat "\$log_file" >&2
        return 1
    fi
}
assert_save_contains() {
    local marker="\$1"
    if ! grep -Fq "\$marker" "\$save_file"; then
        echo "Expected save marker not found: \$marker" >&2
        cat "\$save_file" >&2
        return 1
    fi
}
assert_save_uint_at_least() {
    local key="\$1"
    local minimum="\$2"
    local value
    value="\$(awk -F= -v key="\$key" '\$1 == key { print \$2; found = 1; exit } END { if (!found) exit 2 }' "\$save_file")"
    if [[ ! "\$value" =~ ^[0-9]+$ || "\$value" -lt "\$minimum" ]]; then
        echo "Expected \$key >= \$minimum, got \${value:-missing}" >&2
        cat "\$save_file" >&2
        return 1
    fi
}
trap cleanup EXIT
cd "$remote_root"
"$remote_bootstrap_bin" \
    --storage-root "\$storage_root" \
    -port 0 \
    -bind 127.0.0.1 \
    -name HarnessLinuxSignal > "\$log_file" 2>&1 &
pid="\$!"
if ! wait_for_log_marker "initial save completed" "\$pid"; then
    echo "remote signal run did not reach initial save" >&2
    cat "\$log_file" >&2
    kill -TERM "\$pid" 2>/dev/null || true
    wait "\$pid" 2>/dev/null || true
    exit 1
fi
kill -TERM "\$pid"
wait "\$pid"
cat "\$log_file"
assert_log_contains "requesting save before shutdown"
assert_log_contains "using saveOnExit for shutdown"
assert_log_contains "native bootstrap shell stopped"
assert_save_contains "session-completed=true"
assert_save_contains "requested-app-shutdown=true"
assert_save_contains "shutdown-halted=true"
assert_save_uint_at_least "worker-halt-commands" 1
EOF
    )

    run_and_log \
        remote-signal \
        ssh \
        "$remote_host" \
        "$remote_cmd"
    remote_signal_ok=1
    append_summary_line "remote.signal=ok"
}

main() {
    mkdir -p "$log_root"
    trap on_error ERR
    write_summary_header

    echo "Harness root: $log_root"
    echo "Repo root: $repo_root"
    echo "Remote host: $remote_host"
    echo "Remote root: $remote_root"

    run_source_contract_checks
    run_local_build_and_test
    run_local_bootstrap
    run_local_live
    run_local_signal
    sync_remote_tree
    run_remote_build_and_test
    run_remote_bootstrap
    run_remote_live
    run_remote_signal

    if [[ "$local_build_ok" -eq 1 && "$local_bootstrap_ok" -eq 1 &&
        "$local_live_ok" -eq 1 && "$local_signal_ok" -eq 1 ]]; then
        append_summary_line "platform.local=ok"
    fi
    if [[ "$remote_sync_ok" -eq 1 && "$remote_build_ok" -eq 1 &&
        "$remote_bootstrap_ok" -eq 1 && "$remote_live_ok" -eq 1 &&
        "$remote_signal_ok" -eq 1 ]]; then
        append_summary_line "platform.remote=ok"
    fi
    if [[ "$local_build_ok" -eq 1 && "$local_bootstrap_ok" -eq 1 &&
        "$local_live_ok" -eq 1 && "$local_signal_ok" -eq 1 &&
        "$remote_sync_ok" -eq 1 &&
        "$remote_build_ok" -eq 1 && "$remote_bootstrap_ok" -eq 1 &&
        "$remote_live_ok" -eq 1 && "$remote_signal_ok" -eq 1 ]]; then
        append_summary_line "goal.headless_server_native_runtime=complete"
    fi

    mark_summary_success
    echo "Harness completed successfully."
}

main "$@"
