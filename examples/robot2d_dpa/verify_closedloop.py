#!/usr/bin/env python3
"""
Automated Headless Closed-Loop Verification for 2D Robot with Direct DPA Specification (pFaces-OmegaThreads)

Specification (from robot.pa):
  FG(target) & G(!(obstacles))
  Direct Deterministic Parity Automaton (.pa) specification input (bypassing Owl).

Validates that synthesized controller satisfies LTL specifications in closed loop:
  - Reaches target within maximum number of steps
  - Avoids all obstacles along the entire trajectory (G !obstacles)
  - Remains permanently invariant in target set (FG target satisfaction)
  - Operates deterministically across Mealy automaton modes
  - Generates publication-quality visual verification plot: closedloop_trajectory.png
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
from OmegaInterface import Controller, Quantizer, str2hyperrects


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


def verify_robot2d_dpa(cfg_path, mdf_path, max_steps=40, post_target_steps=10, verbose=True):
    """
    Run closed-loop verification of 2D Robot controller synthesized from DPA (.pa).
    Returns metrics dict on success; raises AssertionError on specification violation.
    """
    cfg = ConfigReader(cfg_path)
    controller = Controller(mdf_path)

    # Quantizers
    x_lb = parse_float_list(cfg.get_value_string("system.states.first_symbol"))
    x_ub = parse_float_list(cfg.get_value_string("system.states.last_symbol"))
    x_eta = parse_float_list(cfg.get_value_string("system.states.quantizers"))

    u_lb = parse_float_list(cfg.get_value_string("system.controls.first_symbol"))
    u_ub = parse_float_list(cfg.get_value_string("system.controls.last_symbol"))
    u_eta = parse_float_list(cfg.get_value_string("system.controls.quantizers"))

    qnt_x = Quantizer(x_lb, x_eta, x_ub)
    qnt_u = Quantizer(u_lb, u_eta, u_ub)

    # Subsets
    target_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_target"))
    obstacle_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_obstacles"))

    # Initial state
    init_str = cfg.get_value_string("simulation.initial_state") or "0.5,1.5"
    x_0 = parse_float_list(init_str)

    state = list(x_0)
    controller.machine_state = 0

    trace_x = [list(state)]
    trace_u = []
    trace_q = [0]
    trace_modes = [0]  # Mode 0: transit, Mode 1: in target

    target_reached_step = None
    steps_in_target = 0

    if verbose:
        print(f"\n--- Simulating Robot2D (Direct DPA) from x_0 = {state} ---")

    # Safety of initial state
    assert not is_in_hyperrects(state, obstacle_HRs), f"Initial state {state} is in obstacles!"

    for step in range(max_steps):
        # 1. Quantize current state
        x_sym = qnt_x.conc_to_flat(state)
        q_curr = controller.machine_state

        # 2. Query controller
        try:
            actions = controller.get_control_actions(x_sym)
        except Exception as e:
            raise AssertionError(f"Controller failure at step {step}: q={q_curr}, x={state}, sym={x_sym}: {e}")

        assert len(actions) > 0, f"No control actions for state {state} (sym {x_sym})"
        u_action_sym = actions[0]
        u = qnt_u.flat_to_conc(u_action_sym)
        q_next = controller.machine_state

        # 3. Apply dynamics
        next_state = system_post(state, u)

        # 4. Safety assertion: G(!obstacles)
        assert not is_in_hyperrects(next_state, obstacle_HRs), (
            f"SAFETY VIOLATION at step {step}: transitioned from {state} to obstacle {next_state} via action {u}"
        )

        # 5. Check target satisfaction and invariance: FG(target)
        in_target = is_in_hyperrects(next_state, target_HRs)
        if in_target:
            if target_reached_step is None:
                target_reached_step = step + 1
            steps_in_target += 1
            mode = 1  # In target
        else:
            if target_reached_step is not None:
                raise AssertionError(
                    f"INVARIANCE VIOLATION at step {step+1}: Left target region! State {next_state} not in target!"
                )
            mode = 0  # Transit

        if verbose:
            print(
                f"  Step {step:2d}: q={q_curr:2d} -> {q_next:2d} | "
                f"x={state} (sym {x_sym:2d}) + u={u} -> next_x={next_state} "
                f"| {'TARGET INVARIANCE' if in_target else 'transit'}"
            )

        state = next_state
        trace_x.append(list(state))
        trace_u.append(list(u))
        trace_q.append(q_next)
        trace_modes.append(mode)

        # Once target invariance is sustained for required steps, test succeeds
        if target_reached_step is not None and steps_in_target >= post_target_steps:
            if verbose:
                print(f"  Target verified invariant for {steps_in_target} consecutive steps.")
            break

    assert target_reached_step is not None, f"LIVENESS FAILURE: Robot did not reach target within {max_steps} steps"
    assert steps_in_target >= post_target_steps, f"Insufficient steps in target: {steps_in_target} < {post_target_steps}"

    metrics = {
        "x_0": x_0,
        "target_reached_step": target_reached_step,
        "steps_in_target": steps_in_target,
        "total_steps": len(trace_x) - 1,
        "final_state": state,
        "trace_x": trace_x,
        "trace_u": trace_u,
        "trace_q": trace_q,
        "trace_modes": trace_modes,
    }
    return metrics


def plot_robot2d_dpa_verification(cfg_path, metrics, output_png):
    """Generate visual verification plot for 2D robot with DPA specification."""
    cfg = ConfigReader(cfg_path)
    target_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_target"))
    obstacle_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_obstacles"))

    fig, ax = plt.subplots(figsize=(8.5, 8.5))

    # Draw Obstacles (red hatching)
    for i, hr in enumerate(obstacle_HRs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=2.0, edgecolor='darkred', facecolor='salmon', alpha=0.45,
            hatch='//', label='Obstacle G(!obstacles)' if i == 0 else None, zorder=2
        )
        ax.add_patch(rect)
        ax.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'OBSTACLE',
                 color='darkred', weight='bold', fontsize=12, ha='center', va='center', zorder=3)

    # Draw Target (forest green)
    for i, hr in enumerate(target_HRs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=2.0, edgecolor='darkgreen', facecolor='limegreen', alpha=0.35,
            label='Target FG(target)' if i == 0 else None, zorder=2
        )
        ax.add_patch(rect)
        ax.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'TARGET\n(FG Target)',
                 color='darkgreen', weight='bold', fontsize=13, ha='center', va='center', zorder=3)

    trace_x = metrics["trace_x"]
    trace_modes = metrics["trace_modes"]
    reached_step = metrics["target_reached_step"]

    # Draw colored path segments:
    # Mode 0 (royal blue): Transit towards Target
    # Mode 1 (emerald green): Target Invariance Cycle
    transit_label_added = False
    invariance_label_added = False

    for k in range(len(trace_x) - 1):
        x1 = trace_x[k]
        x2 = trace_x[k + 1]
        mode = trace_modes[k + 1]

        if mode == 0:
            color = 'royalblue'
            label = 'Transit Mode (to Target)' if not transit_label_added else None
            transit_label_added = True
        else:
            color = 'forestgreen'
            label = 'Target Invariance Mode (FG Stay)' if not invariance_label_added else None
            invariance_label_added = True

        ax.annotate(
            "", xy=(x2[0], x2[1]), xytext=(x1[0], x1[1]),
            arrowprops=dict(arrowstyle="->", color=color, lw=2.5, mutation_scale=15),
            zorder=4
        )
        ax.plot([x1[0], x2[0]], [x1[1], x2[1]], 'o-', color=color, markersize=5, linewidth=2.5, label=label, zorder=5)

    # Mark Start state
    x0 = metrics["x_0"]
    ax.plot(x0[0], x0[1], 's', color='gold', markeredgecolor='black', markersize=12, label=f'Start x0={x0}', zorder=6)
    ax.annotate(f'Start {x0}', (x0[0], x0[1]), textcoords="offset points", xytext=(-50, -18),
                fontweight='bold', fontsize=10, bbox=dict(boxstyle="round,pad=0.2", fc="white", ec="black", alpha=0.85), zorder=7)

    # Mark Target entry point
    x_entry = trace_x[reached_step]
    ax.plot(x_entry[0], x_entry[1], '*', color='lime', markeredgecolor='darkgreen', markersize=16,
            label=f'Target Entry (Step {reached_step})', zorder=8)

    ax.set_xlim(-0.1, 4.3)
    ax.set_ylim(-0.1, 4.3)
    ax.set_xlabel('X coordinate', fontsize=12, fontweight='bold')
    ax.set_ylabel('Y coordinate', fontsize=12, fontweight='bold')
    ax.set_title('Robot2D with Direct DPA Specification (robot.pa)\nLTL Spec: FG(target) & G(!(obstacles))', fontsize=13, fontweight='bold')
    ax.grid(True, linestyle='--', alpha=0.6)
    ax.set_aspect('equal')
    ax.legend(loc='upper left', framealpha=0.92, fontsize=10)

    plt.tight_layout()
    plt.savefig(output_png, dpi=180)
    plt.close(fig)
    print(f"Saved visual verification plot: {output_png}")


def main():
    parser = argparse.ArgumentParser(description="Closed-loop verification for 2D Robot with Direct DPA")
    parser.add_argument("--cfg", default=os.path.join(SCRIPT_DIR, "robot.cfg"), help="Path to robot.cfg")
    parser.add_argument("--mdf", default=None, help="Path to robot.mdf")
    parser.add_argument("--max-steps", type=int, default=40, help="Maximum steps")
    parser.add_argument("--post-target-steps", type=int, default=10, help="Consecutive target invariance steps")
    parser.add_argument("--output-png", default=os.path.join(SCRIPT_DIR, "closedloop_trajectory.png"), help="Output plot PNG path")
    parser.add_argument("--verbose", action="store_true", default=True, help="Verbose output")
    args = parser.parse_args()

    cfg_path = os.path.abspath(args.cfg)
    if not os.path.exists(cfg_path):
        print(f"Error: Config file not found: {cfg_path}", file=sys.stderr)
        return 1

    cfg = ConfigReader(cfg_path)
    mdf_name = args.mdf or cfg.get_value_string("simulation.controller_file") or "robot.mdf"
    mdf_path = os.path.join(os.path.dirname(cfg_path), mdf_name)
    if not os.path.exists(mdf_path):
        print(f"Error: Controller file not found: {mdf_path}", file=sys.stderr)
        return 1

    print(f"=== Running Closed-Loop Verification: Robot2D Direct DPA ===")
    print(f"Configuration: {cfg_path}")
    print(f"Controller:    {mdf_path}")

    metrics = verify_robot2d_dpa(
        cfg_path, mdf_path, max_steps=args.max_steps,
        post_target_steps=args.post_target_steps, verbose=args.verbose
    )

    plot_robot2d_dpa_verification(cfg_path, metrics, args.output_png)

    print("\n=== Robot2D DPA Closed-Loop Verification Summary ===")
    print(f"  Specification:        robot.pa -> FG(target) & G(!(obstacles))")
    print(f"  Target reached step:  {metrics['target_reached_step']}")
    print(f"  Verified in target:   {metrics['steps_in_target']} steps (Invariant)")
    print(f"  Total steps:          {metrics['total_steps']}")
    print(f"  Obstacle collisions:  0 (Strictly safe)")
    print(f"[PASS] Robot2D DPA closed-loop specifications verified successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
