#!/usr/bin/env bash
set -eu
if (set -o pipefail) 2>/dev/null; then
    set -o pipefail
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEVICE_ID="${PFACES_DEVICE_ID:-1}"

if ! command -v pfaces >/dev/null 2>&1; then
    echo "Error: 'pfaces' is not on PATH." >&2
    exit 1
fi

run_test_case() {
    local example_dir="$1"
    local cfg_file="$2"
    shift 2

    echo "=== Running OmegaThreads test: ${example_dir}/${cfg_file} (device ${DEVICE_ID}) ==="

    local full_dir="${SCRIPT_DIR}/${example_dir}"
    for out_file in "$@"; do
        rm -f "${full_dir}/${out_file}"
    done

    local log_file
    log_file="$(mktemp)"

    if ! (cd "$full_dir" && pfaces -CG -d "$DEVICE_ID" -k "omega@../../kernel-pack" -cfg "$cfg_file") >"$log_file" 2>&1; then
        cat "$log_file"
        rm -f "$log_file"
        echo "Error: pfaces failed for ${example_dir}/${cfg_file}." >&2
        exit 1
    fi

    cat "$log_file"

    if ! grep -q "A controller is found !" "$log_file"; then
        rm -f "$log_file"
        echo "Error: expected 'A controller is found !' in output for ${example_dir}/${cfg_file}." >&2
        exit 1
    fi
    rm -f "$log_file"

    for out_file in "$@"; do
        if [ ! -s "${full_dir}/${out_file}" ]; then
            echo "Error: expected non-empty output file '${example_dir}/${out_file}' was not generated." >&2
            exit 1
        fi
        echo "Verified output: ${example_dir}/${out_file} ($(wc -c < "${full_dir}/${out_file}") bytes)"
    done
}

run_test_case "examples/robot2d" "robot.cfg" "robot.mdf" "robot.symmodel"
run_test_case "examples/pickupdelivery" "pickupdelivery.cfg" "pickupdelivery.mdf"

echo "All OmegaThreads E2E tests passed."
