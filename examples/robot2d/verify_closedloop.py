#!/usr/bin/env python3
"""
Automated Headless Closed-Loop Verification for 2D Robot Example (pFaces-OmegaThreads)

Specification: FG(target) & G(!(obstacles))
Validates that synthesized controller satisfies LTL specifications in closed loop:
  - Reaches target within maximum number of steps
  - Avoids all obstacles along the entire trajectory
  - Remains in target set (FG target satisfaction)
  - Operates deterministically across initial state(s)
  - Generates visual verification plot: closedloop_trajectory.png
"""

import argparse
import os
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as patches

# Add interface/python to module search path
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
INTERFACE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, "../../interface/python"))
if INTERFACE_DIR not in sys.path:
    sys.path.insert(0, INTERFACE_DIR)

from ConfigReader import ConfigReader
from OmegaInterface import Controller, Quantizer, str2hyperrects, SymbolicModel


def parse_float_list(str_val):
    return [float(item.strip()) for item in str_val.split(",") if item.strip()]


def is_in_hyperrects(point, hr_list):
    """Check if point is inside any hyperrectangle in hr_list."""
    for hr in hr_list:
        if hr.is_element(point):
            return True
    return False


def system_post(x, u):
    """Discrete system dynamics matching robot.cl."""
    action = int(u[0])
    next_x = list(x)
    if action == 0:      # move left
        next_x[0] = x[0] - 1.0
    elif action == 1:    # move up
        next_x[1] = x[1] + 1.0
    elif action == 2:    # move right
        next_x[0] = x[0] + 1.0
    elif action == 3:    # move down
        next_x[1] = x[1] - 1.0
    return next_x


def simulate_closed_loop(x_0, cfg_reader, controller, sym_model=None, max_steps=50, post_target_steps=6, verbose=True):
    """
    Run closed-loop simulation from initial state x_0.
    Returns dict of trace metrics, raises AssertionError on failure.
    """
    # Parse quantizers
    x_lb = parse_float_list(cfg_reader.get_value_string("system.states.first_symbol"))
    x_ub = parse_float_list(cfg_reader.get_value_string("system.states.last_symbol"))
    x_eta = parse_float_list(cfg_reader.get_value_string("system.states.quantizers"))

    u_lb = parse_float_list(cfg_reader.get_value_string("system.controls.first_symbol"))
    u_ub = parse_float_list(cfg_reader.get_value_string("system.controls.last_symbol"))
    u_eta = parse_float_list(cfg_reader.get_value_string("system.controls.quantizers"))

    qnt_x = Quantizer(x_lb, x_eta, x_ub)
    qnt_u = Quantizer(u_lb, u_eta, u_ub)

    # Subsets
    target_str = cfg_reader.get_value_string("system.states.subsets.mapping_target")
    obstacles_str = cfg_reader.get_value_string("system.states.subsets.mapping_obstacles")
    target_HRs = str2hyperrects(target_str)
    obstacle_HRs = str2hyperrects(obstacles_str)

    # Reset controller machine state to initial (0)
    controller.machine_state = 0

    state = list(x_0)
    trace = [list(state)]
    target_reached_step = None
    steps_in_target = 0

    if verbose:
        print(f"\n--- Simulating from initial state x_0 = {state} ---")

    # Verify initial state safety
    assert not is_in_hyperrects(state, obstacle_HRs), f"Initial state {state} is in obstacles!"

    for step in range(max_steps):
        # 1. Quantize current state to symbolic index
        x_sym = qnt_x.conc_to_flat(state)
        q_curr = controller.machine_state

        # 2. Query controller
        try:
            actions = controller.get_control_actions(x_sym)
        except Exception as e:
            raise AssertionError(f"Controller failure at step {step}, q={q_curr}, x={state}, sym={x_sym}: {e}")

        assert len(actions) > 0, f"No control actions returned for state {state} (sym {x_sym})"
        u_action_sym = actions[0]
        u = qnt_u.flat_to_conc(u_action_sym)

        # 3. Apply dynamics
        next_state = system_post(state, u)

        # 4. Optional symbolic model consistency check
        if sym_model is not None:
            post_hr = sym_model.get_HR(x_sym, u_action_sym)
            if post_hr is not None:
                assert post_hr.is_element(next_state), (
                    f"Dynamics output {next_state} outside symbolic model post set "
                    f"[{post_hr.get_lb()}, {post_hr.get_ub()}] for x={x_sym}, u={u_action_sym}"
                )

        # 5. Safety assertion: G(!obstacles)
        assert not is_in_hyperrects(next_state, obstacle_HRs), (
            f"SAFETY VIOLATION at step {step}: transitioned from {state} to obstacle {next_state} "
            f"via action {u}"
        )

        # 6. Check target satisfaction
        in_target = is_in_hyperrects(next_state, target_HRs)
        if in_target:
            if target_reached_step is None:
                target_reached_step = step + 1
            steps_in_target += 1
        else:
            if target_reached_step is not None:
                raise AssertionError(
                    f"INVARIANCE VIOLATION: Left target at step {step+1}: state {next_state} not in target"
                )

        if verbose:
            print(
                f"  Step {step:2d}: q={q_curr:2d} -> {controller.machine_state:2d} | "
                f"x={state} (sym {x_sym:2d}) + u={u} -> next_x={next_state} "
                f"| {'TARGET' if in_target else 'transit'}"
            )

        state = next_state
        trace.append(list(state))

        # Once target is reached and we've verified target invariance for post_target_steps, we succeed
        if target_reached_step is not None and steps_in_target >= post_target_steps:
            if verbose:
                print(f"  Target verified invariant for {steps_in_target} consecutive steps.")
            break

    assert target_reached_step is not None, f"LIVENESS FAILURE: Robot did not reach target within {max_steps} steps"
    assert steps_in_target >= post_target_steps, "Insufficient steps verified inside target"

    metrics = {
        "x_0": x_0,
        "target_reached_step": target_reached_step,
        "total_steps": len(trace) - 1,
        "steps_in_target": steps_in_target,
        "final_state": state,
        "safety_violations": 0,
        "trace": trace,
    }
    return metrics


def plot_trajectories(cfg_reader, all_metrics, output_png):
    """Generate visual verification plot for 2D robot paths."""
    fig, ax = plt.subplots(figsize=(8, 8))

    target_HRs = str2hyperrects(cfg_reader.get_value_string("system.states.subsets.mapping_target"))
    obstacle_HRs = str2hyperrects(cfg_reader.get_value_string("system.states.subsets.mapping_obstacles"))

    # Draw Obstacles (red)
    for i, hr in enumerate(obstacle_HRs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=1.8, edgecolor='darkred', facecolor='red', alpha=0.35,
            label='Obstacles' if i == 0 else None, hatch='//', zorder=2
        )
        ax.add_patch(rect)
        ax.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'Obstacle',
                color='darkred', weight='bold', fontsize=11, ha='center', va='center', zorder=3)

    # Draw Target (green)
    for i, hr in enumerate(target_HRs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=1.8, edgecolor='darkgreen', facecolor='forestgreen', alpha=0.35,
            label='Target' if i == 0 else None, zorder=2
        )
        ax.add_patch(rect)
        ax.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'Target',
                color='darkgreen', weight='bold', fontsize=13, ha='center', va='center', zorder=3)

    colors = ['royalblue', 'darkorange', 'purple', 'teal']
    for idx, m in enumerate(all_metrics):
        x_0 = m["x_0"]
        trace = m["trace"]
        color = colors[idx % len(colors)]
        xs = [pt[0] for pt in trace]
        ys = [pt[1] for pt in trace]

        # Draw trajectory
        ax.plot(
            xs, ys, '-o', color=color, linewidth=2.5, markersize=5,
            label=f'Path from x0={list(x_0)} ({m["target_reached_step"]} steps to target)',
            zorder=4
        )

        # Mark Start
        ax.plot(xs[0], ys[0], 's', color=color, markersize=10,
                markeredgecolor='black', markeredgewidth=1.5, zorder=5)
        offset_y = -16 if idx % 2 == 0 else 10
        ax.annotate(
            f'Start {list(x_0)}', (xs[0], ys[0]),
            textcoords="offset points", xytext=(-35, offset_y), fontsize=9, fontweight='bold',
            bbox=dict(boxstyle="round,pad=0.2", fc="white", ec=color, alpha=0.85), zorder=6
        )

        # Mark End point (in target)
        ax.plot(xs[-1], ys[-1], '*', color='gold', markersize=14,
                markeredgecolor='black', markeredgewidth=1.2, zorder=7)

    ax.set_xlim(-0.1, 4.3)
    ax.set_ylim(-0.1, 4.3)
    ax.set_xlabel('X coordinate', fontsize=12)
    ax.set_ylabel('Y coordinate', fontsize=12)
    ax.set_title('Robot2D Closed-Loop Trajectories\nLTL Spec: FG(target) & G(!(obstacles))', fontsize=13, fontweight='bold')
    ax.grid(True, linestyle='--', alpha=0.6)
    ax.set_aspect('equal')
    ax.legend(loc='upper left', framealpha=0.92, fontsize=10)

    plt.tight_layout()
    plt.savefig(output_png, dpi=150)
    plt.close(fig)
    print(f"Saved visual verification plot: {output_png}")


def main():
    parser = argparse.ArgumentParser(description="Headless closed-loop verification for 2D Robot")
    parser.add_argument("--cfg", default=os.path.join(SCRIPT_DIR, "robot.cfg"), help="Path to robot.cfg")
    parser.add_argument("--mdf", default=None, help="Path to robot.mdf (defaults to cfg simulation.controller_file)")
    parser.add_argument("--symmodel", default=None, help="Path to robot.symmodel (optional)")
    parser.add_argument("--max-steps", type=int, default=50, help="Maximum steps for simulation")
    parser.add_argument("--output-png", default=os.path.join(SCRIPT_DIR, "closedloop_trajectory.png"), help="Path to output plot PNG")
    parser.add_argument("--verbose", action="store_true", default=True, help="Verbose output")
    args = parser.parse_args()

    cfg_path = os.path.abspath(args.cfg)
    if not os.path.exists(cfg_path):
        print(f"[ERROR] Config file not found: {cfg_path}", file=sys.stderr)
        return 1

    print(f"=== Running Closed-Loop Verification: Robot2D ===")
    print(f"Loading configuration: {cfg_path}")
    cfg_reader = ConfigReader(cfg_path)

    # Controller path
    mdf_name = args.mdf or cfg_reader.get_value_string("simulation.controller_file")
    mdf_path = os.path.join(os.path.dirname(cfg_path), mdf_name)
    if not os.path.exists(mdf_path):
        print(f"[ERROR] Controller file not found: {mdf_path}", file=sys.stderr)
        return 1

    print(f"Loading controller: {mdf_path}")
    controller = Controller(mdf_path)

    # Optional symmodel
    sym_model = None
    symmodel_name = args.symmodel or cfg_reader.get_value_string("simulation.model_dump_file")
    if symmodel_name:
        symmodel_path = os.path.join(os.path.dirname(cfg_path), symmodel_name)
        if os.path.exists(symmodel_path):
            print(f"Loading symbolic model: {symmodel_path}")
            x_lb = parse_float_list(cfg_reader.get_value_string("system.states.first_symbol"))
            x_ub = parse_float_list(cfg_reader.get_value_string("system.states.last_symbol"))
            x_eta = parse_float_list(cfg_reader.get_value_string("system.states.quantizers"))
            u_lb = parse_float_list(cfg_reader.get_value_string("system.controls.first_symbol"))
            u_ub = parse_float_list(cfg_reader.get_value_string("system.controls.last_symbol"))
            u_eta = parse_float_list(cfg_reader.get_value_string("system.controls.quantizers"))
            qnt_x = Quantizer(x_lb, x_eta, x_ub)
            qnt_u = Quantizer(u_lb, u_eta, u_ub)
            sym_model = SymbolicModel(symmodel_path, qnt_x.get_num_symbols(), qnt_u.get_num_symbols())

    # Initial states to verify:
    # 1. Configured initial_state
    initial_states = []
    cfg_init_str = cfg_reader.get_value_string("simulation.initial_state")
    if cfg_init_str:
        initial_states.append(parse_float_list(cfg_init_str))

    # 2. Add all grid points inside initial_set
    initial_set_str = cfg_reader.get_value_string("system.states.initial_set")
    if initial_set_str:
        init_HRs = str2hyperrects(initial_set_str)
        # Search candidate grid states
        x_lb = parse_float_list(cfg_reader.get_value_string("system.states.first_symbol"))
        x_ub = parse_float_list(cfg_reader.get_value_string("system.states.last_symbol"))
        x_eta = parse_float_list(cfg_reader.get_value_string("system.states.quantizers"))
        nx = int((x_ub[0] - x_lb[0]) / x_eta[0]) + 1
        ny = int((x_ub[1] - x_lb[1]) / x_eta[1]) + 1
        for i in range(nx):
            for j in range(ny):
                cand = [x_lb[0] + i * x_eta[0], x_lb[1] + j * x_eta[1]]
                if is_in_hyperrects(cand, init_HRs):
                    if cand not in initial_states:
                        initial_states.append(cand)

    print(f"Initial states to verify: {initial_states}")

    all_metrics = []
    try:
        for x_0 in initial_states:
            metrics = simulate_closed_loop(
                x_0, cfg_reader, controller, sym_model=sym_model, max_steps=args.max_steps, verbose=args.verbose
            )
            all_metrics.append(metrics)

        print("\n=== Robot2D Closed-Loop Verification Summary ===")
        print(f"LTL Specification: {cfg_reader.get_value_string('specifications.ltl_formula')}")
        for m in all_metrics:
            print(
                f"  Initial state {m['x_0']}: Reached target at step {m['target_reached_step']} "
                f"| Total steps: {m['total_steps']} | Invariant steps: {m['steps_in_target']} "
                f"| Obstacle hits: {m['safety_violations']} | Status: PASS"
            )

        # Plot trajectories
        plot_trajectories(cfg_reader, all_metrics, os.path.abspath(args.output_png))

        print("[PASS] All initial states satisfied specification with 0 violations.\n")
        return 0

    except AssertionError as ae:
        print(f"\n[FAIL] Assertion error during verification: {ae}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"\n[FAIL] Unexpected error during verification: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
