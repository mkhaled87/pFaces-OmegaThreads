#!/usr/bin/env bash
set -eu
if (set -o pipefail) 2>/dev/null; then
    set -o pipefail
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEVICE_ID="${PFACES_DEVICE_ID:-1}"

# build kernel driver if not present
[ ! -f "$SCRIPT_DIR/kernel-pack/omega.driver" ] && (cd "$SCRIPT_DIR" && sh ./build.sh)

if ! command -v pfaces >/dev/null 2>&1; then
    echo "Error: 'pfaces' is not on PATH." >&2
    exit 1
fi

PYTHON_BIN="${PYTHON_BIN:-python3}"
if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    if command -v python >/dev/null 2>&1; then
        PYTHON_BIN="python"
    else
        echo "Error: Neither 'python3' nor 'python' found on PATH." >&2
        exit 1
    fi
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

run_closedloop_verification() {
    local example_dir="$1"
    local script_name="${2:-verify_closedloop.py}"
    local verify_script="${SCRIPT_DIR}/${example_dir}/${script_name}"

    if [ ! -f "$verify_script" ]; then
        echo "Error: verification script '${verify_script}' does not exist." >&2
        exit 1
    fi

    echo "=== Running closed-loop E2E verification: ${example_dir}/${script_name} ==="
    if ! (cd "${SCRIPT_DIR}/${example_dir}" && "$PYTHON_BIN" "$script_name"); then
        echo "Error: closed-loop verification failed for ${example_dir}." >&2
        exit 1
    fi

    local expected_plot="${SCRIPT_DIR}/${example_dir}/closedloop_trajectory.png"
    if [ ! -s "$expected_plot" ]; then
        echo "Error: expected visual plot '${expected_plot}' was not generated or is empty." >&2
        exit 1
    fi
    echo "Verified visual verification plot: ${example_dir}/closedloop_trajectory.png ($(wc -c < "$expected_plot") bytes)"
}

# 1. 2D Robot with LTL Reach-and-Stay specification (FG target & G !obstacles)
run_test_case "examples/robot2d" "robot.cfg" "robot.mdf" "robot.symmodel"
run_closedloop_verification "examples/robot2d"

# 2. Pickup-Delivery Drone with Multi-Goal Sequential LTL & Battery Constraints
run_test_case "examples/pickupdelivery" "pickupdelivery.cfg" "pickupdelivery.mdf"
run_closedloop_verification "examples/pickupdelivery"

# 3. 2D Robot with Direct Deterministic Parity Automaton (.pa) Specification
run_test_case "examples/robot2d_dpa" "robot.cfg" "robot.mdf"
run_closedloop_verification "examples/robot2d_dpa"

# 4. Amazon DeepRacer with 4D Bicycle Dynamics & Recurrent Patrol LTL
run_test_case "examples/deepracer" "deepracer_xsmall.cfg" "deepracer_xsmall.mdf"
run_closedloop_verification "examples/deepracer"

# 5. 3D Autonomous Vehicle with Growth-Bound ODE Abstraction & Multi-Target LTL
run_test_case "examples/vehicle3d" "vehicle.cfg" "vehicle.mdf" "vehicle.symmodel"
run_closedloop_verification "examples/vehicle3d"

echo "All OmegaThreads E2E tests passed."
