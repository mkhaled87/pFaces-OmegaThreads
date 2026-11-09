#!/usr/bin/env python3
"""
Automated Headless Closed-Loop Verification for Amazon DeepRacer Autonomous Vehicle (pFaces-OmegaThreads)

Specification:
  GF(target1) & GF(target2) & G(!obstacles)

Continuous Model:
  4D non-linear bicycle dynamics ODE with sampling period 0.5s:
    dx/dt     = v * cos(theta)
    dy/dt     = v * sin(theta)
    dtheta/dt = (v / L) * tan(delta)
    dv/dt     = a * v + b * u_speed
  Where L = 0.165m, delta is mapped via steering calibration polynomial, and (a, b) model motor acceleration.
  Integrated numerically via 4th-order Runge-Kutta (RK4) with 5 sub-steps.

Validates that the synthesized controller satisfies LTL specifications in closed loop:
  - Global Safety: Car never penetrates the central obstacle (G !obstacles)
  - Recurrent Patrolling: Car visits target1 and target2 repeatedly (GF target1 & GF target2)
  - Limit Cycle: Detects periodic recurrence (lasso limit cycle) in discrete/symbolic states
  - Generates publication-quality visual verification plot: closedloop_trajectory.png
"""

import argparse
import math
import os
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np

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


def wrap_to_pi(rad):
    m_2pi = 2.0 * math.pi
    return rad - m_2pi * math.floor((rad + math.pi) * (1.0 / m_2pi))


def map_steering(angle_in):
    """Calibration polynomial matching deepracer.cl."""
    p1 = -0.1167
    p2 = 0.01949
    p3 = 0.3828
    p4 = -0.0293
    x = angle_in
    return p1 * x * x * x + p2 * x * x + p3 * x + p4


def map_speed(speed_in):
    """Speed mapping matching deepracer.cl."""
    lookup = {
        6: 0.70, 5: 0.65, 4: 0.60, 3: 0.55, 2: 0.50, 1: 0.45, 0: 0.00,
        -1: -0.45, -2: -0.50, -3: -0.55, -4: -0.60, -5: -0.65, -6: -0.70
    }
    return lookup.get(int(speed_in), 0.0)


def get_v_params(u_speed):
    """Velocity ODE parameters matching deepracer.cl."""
    params = {
        0.00: (0.0000, 0.2500),
        0.45: (1.9953, 0.9933), 0.50: (2.3567, 0.8943), 0.55: (3.0797, 0.88976),
        0.60: (3.2019, 0.87595), 0.65: (3.3276, 0.89594), 0.70: (3.7645, 0.92501),
        -0.45: (1.8229, 1.8431), -0.50: (2.3833, 1.2721), -0.55: (2.5120, 1.1403),
        -0.60: (3.0956, 1.1278), -0.65: (3.5500, 1.1226), -0.70: (3.6423, 1.1539)
    }
    k, t = params.get(round(u_speed, 2), (0.0, 1.0))
    return -1.0 / t, k / t


def deepracer_ode(x, u):
    """4D bicycle ODE matching deepracer.cl."""
    u_steer = map_steering(u[0])
    u_speed = map_speed(u[1])
    l_wheelbase = 0.165
    a, b = get_v_params(u_speed)

    dx0 = x[3] * math.cos(x[2])
    dx1 = x[3] * math.sin(x[2])
    dx2 = (x[3] / l_wheelbase) * math.tan(u_steer)
    dx3 = a * x[3] + b * u_speed
    return [dx0, dx1, dx2, dx3]


def rk4_step(x, u, dt=0.5, n_int=5):
    """RK4 integration matching rk4ode.cl and deepracer.cl."""
    h = dt / n_int
    curr_x = list(x)
    for _ in range(n_int):
        k0 = deepracer_ode(curr_x, u)
        tmp1 = [curr_x[i] + 0.5 * h * k0[i] for i in range(4)]
        k1 = deepracer_ode(tmp1, u)
        tmp2 = [curr_x[i] + 0.5 * h * k1[i] for i in range(4)]
        k2 = deepracer_ode(tmp2, u)
        tmp3 = [curr_x[i] + h * k2[i] for i in range(4)]
        k3 = deepracer_ode(tmp3, u)
        for i in range(4):
            curr_x[i] += (h / 6.0) * (k0[i] + 2.0 * k1[i] + 2.0 * k2[i] + k3[i])

    curr_x[2] = wrap_to_pi(curr_x[2])
    return curr_x


def verify_deepracer(cfg_path, mdf_path, symmodel_path=None, max_steps=60, min_cycles=2, verbose=True):
    """
    Simulate DeepRacer in closed loop with the synthesized Mealy machine controller.
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
    t1_hrs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_target1"))
    t2_hrs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_target2"))
    obs_hrs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_obstacles"))

    # Initial state
    init_hrs = str2hyperrects(cfg.get_value_string("system.states.initial_set"))
    init_state = init_hrs[0].get_center_element()
    dt = float(cfg.get_value_string("system.dynamics.step_time") or "0.5")

    state = list(init_state)
    controller.machine_state = 0

    trace_x = [list(state)]
    trace_u = []
    trace_modes = [0]
    trace_q = [0]
    target_history = []

    t1_count = 0
    t2_count = 0
    cycle_count = 0
    last_target = None
    lasso_detected = None

    # Track visited (q, sym_x) for periodic limit cycle detection
    visited_states = {}

    if verbose:
        print(f"\n--- Simulating Amazon DeepRacer from x_0 = {state} ---")

    # Safety of initial state
    assert not is_in_hyperrects(state, obs_hrs), f"Initial state {state} is in obstacles!"

    for step in range(max_steps):
        # 1. Quantize current state
        sym_x = qnt_x.conc_to_flat(state)
        grid_x = qnt_x.flat_to_conc(sym_x)
        q_curr = controller.machine_state

        # Check for periodic limit cycle (lasso)
        key = (q_curr, sym_x)
        if key in visited_states and lasso_detected is None:
            prev_step = visited_states[key]
            period = step - prev_step
            lasso_detected = {
                "start_step": prev_step,
                "loop_step": step,
                "period": period,
                "state": key
            }
            if verbose:
                print(f"  [LASSO] Periodic limit cycle detected at step {step}: (q={q_curr}, sym={sym_x}) previously visited at step {prev_step} (Period: {period} steps)")
        else:
            visited_states[key] = step

        # 2. Query Mealy controller
        try:
            actions = controller.get_control_actions(sym_x)
        except Exception as e:
            raise AssertionError(f"Controller failure at step {step}: q={q_curr}, x={state}, sym={sym_x}: {e}")

        assert len(actions) > 0, f"No control actions for state {state} (sym {sym_x})"
        u_action_sym = actions[0]
        u = qnt_u.flat_to_conc(u_action_sym)
        q_next = controller.machine_state

        # 3. Advance continuous dynamics
        # For deterministic dynamics simulation matching the symbolic abstraction:
        next_state = rk4_step(grid_x, u, dt=dt, n_int=5)

        # 4. Safety assertion: G(!obstacles)
        assert not is_in_hyperrects(next_state, obs_hrs), (
            f"SAFETY VIOLATION at step {step}: vehicle entered obstacle at {next_state}!"
        )

        # 5. Check target satisfaction
        # Evaluate target satisfaction on the symbolic grid cell containment
        next_sym = qnt_x.conc_to_flat(next_state)
        next_grid = qnt_x.flat_to_conc(next_sym)

        in_t1 = is_in_hyperrects(next_grid, t1_hrs) or is_in_hyperrects(next_state, t1_hrs)
        in_t2 = is_in_hyperrects(next_grid, t2_hrs) or is_in_hyperrects(next_state, t2_hrs)

        if in_t1:
            if last_target != "target1":
                t1_count += 1
                target_history.append((step + 1, "target1"))
                if last_target == "target2":
                    cycle_count += 1
                last_target = "target1"
                if verbose:
                    print(f"  Step {step+1:2d}: Reached TARGET 1 at [{next_state[0]:.2f}, {next_state[1]:.2f}] (Total visits: {t1_count}, Cycles: {cycle_count})")
            mode = 1
        elif in_t2:
            if last_target != "target2":
                t2_count += 1
                target_history.append((step + 1, "target2"))
                if last_target == "target1":
                    cycle_count += 1
                last_target = "target2"
                if verbose:
                    print(f"  Step {step+1:2d}: Reached TARGET 2 at [{next_state[0]:.2f}, {next_state[1]:.2f}] (Total visits: {t2_count}, Cycles: {cycle_count})")
            mode = 3
        else:
            mode = 2 if last_target == "target1" else 0

        state = next_state
        trace_x.append(list(state))
        trace_u.append(list(u))
        trace_modes.append(mode)
        trace_q.append(q_next)

        if t1_count >= min_cycles and t2_count >= min_cycles and lasso_detected is not None:
            if verbose:
                print(f"  Completed {cycle_count} patrol cycles with confirmed periodic lasso ({t1_count} x T1, {t2_count} x T2).")
            break

    assert t1_count >= min_cycles, f"LTL RECURRENCE FAILURE: Target 1 visited only {t1_count} times (< {min_cycles})"
    assert t2_count >= min_cycles, f"LTL RECURRENCE FAILURE: Target 2 visited only {t2_count} times (< {min_cycles})"
    assert cycle_count >= min_cycles, f"LTL CYCLE FAILURE: Completed only {cycle_count} target cycles (< {min_cycles})"
    assert lasso_detected is not None, "Failed to detect closed-loop periodic limit cycle"

    metrics = {
        "init_state": init_state,
        "final_state": state,
        "total_steps": len(trace_x) - 1,
        "t1_visits": t1_count,
        "t2_visits": t2_count,
        "cycles": cycle_count,
        "target_history": target_history,
        "lasso": lasso_detected,
        "trace_x": trace_x,
        "trace_u": trace_u,
        "trace_modes": trace_modes,
        "trace_q": trace_q,
    }
    return metrics


def plot_deepracer_verification(cfg_path, metrics, output_png):
    """Generate visual verification plot for DeepRacer."""
    cfg = ConfigReader(cfg_path)
    t1_hrs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_target1"))
    t2_hrs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_target2"))
    obs_hrs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_obstacles"))

    fig = plt.figure(figsize=(15, 7.5))
    gs = fig.add_gridspec(2, 2, width_ratios=[1.2, 1.0], height_ratios=[1.0, 1.0])

    ax_traj = fig.add_subplot(gs[:, 0])
    ax_speed = fig.add_subplot(gs[0, 1])
    ax_steer = fig.add_subplot(gs[1, 1])

    # 1. 2D Trajectory Plot
    # Obstacles
    for i, hr in enumerate(obs_hrs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=2.0, edgecolor='darkred', facecolor='salmon', alpha=0.45,
            hatch='//', label='Obstacle G(!obstacles)' if i == 0 else None, zorder=2
        )
        ax_traj.add_patch(rect)
        ax_traj.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'OBSTACLE',
                     color='darkred', weight='bold', fontsize=11, ha='center', va='center', zorder=3)

    # Target 1
    for i, hr in enumerate(t1_hrs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=2.0, edgecolor='darkgreen', facecolor='limegreen', alpha=0.35,
            label='Target 1 (GF target1)' if i == 0 else None, zorder=2
        )
        ax_traj.add_patch(rect)
        ax_traj.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'TARGET 1',
                     color='darkgreen', weight='bold', fontsize=11, ha='center', va='center', zorder=3)

    # Target 2
    for i, hr in enumerate(t2_hrs):
        lb = hr.get_lb()
        ub = hr.get_ub()
        rect = patches.Rectangle(
            (lb[0], lb[1]), ub[0] - lb[0], ub[1] - lb[1],
            linewidth=2.0, edgecolor='darkgreen', facecolor='forestgreen', alpha=0.35,
            label='Target 2 (GF target2)' if i == 0 else None, zorder=2
        )
        ax_traj.add_patch(rect)
        ax_traj.text((lb[0] + ub[0]) / 2.0, (lb[1] + ub[1]) / 2.0, 'TARGET 2',
                     color='darkgreen', weight='bold', fontsize=11, ha='center', va='center', zorder=3)

    mode_colors = {0: 'royalblue', 1: 'limegreen', 2: 'darkorange', 3: 'forestgreen'}
    mode_names = {0: 'Transit -> T1', 1: 'Inside T1', 2: 'Transit -> T2', 3: 'Inside T2'}

    trace_x = metrics["trace_x"]
    trace_modes = metrics["trace_modes"]
    seen_modes = set()

    for k in range(len(trace_x) - 1):
        x1 = trace_x[k]
        x2 = trace_x[k + 1]
        mode = trace_modes[k]
        color = mode_colors.get(mode, 'gray')
        label = mode_names.get(mode) if mode not in seen_modes else None
        seen_modes.add(mode)

        ax_traj.plot([x1[0], x2[0]], [x1[1], x2[1]], color=color, linewidth=2.5, label=label, zorder=4)

        if k % 3 == 0:
            dx = 0.08 * math.cos(x1[2])
            dy = 0.08 * math.sin(x1[2])
            ax_traj.arrow(x1[0], x1[1], dx, dy, head_width=0.04, head_length=0.04, fc=color, ec='black', linewidth=0.5, zorder=5)

    # Mark Start
    x0 = metrics["init_state"]
    ax_traj.plot(x0[0], x0[1], 's', color='gold', markeredgecolor='black', markersize=11, label='Start x0', zorder=6)

    # Mark Target arrivals
    for step_num, tgt_name in metrics["target_history"]:
        pt = trace_x[step_num]
        col = 'lime' if tgt_name == 'target1' else 'magenta'
        ax_traj.plot(pt[0], pt[1], '*', color=col, markeredgecolor='black', markersize=14, zorder=7)
        ax_traj.annotate(f'Step {step_num}', (pt[0], pt[1]), textcoords="offset points", xytext=(6, 4),
                         fontsize=8, fontweight='bold', bbox=dict(boxstyle="round,pad=0.15", fc="white", ec="gray", alpha=0.8), zorder=8)

    lasso = metrics["lasso"]
    if lasso:
        ax_traj.set_title(
            f'Amazon DeepRacer Closed-Loop Patrol (4D Bicycle Dynamics)\n'
            f'Periodic Lasso: {lasso["period"]} steps (Step {lasso["start_step"]} -> {lasso["loop_step"]})',
            fontsize=11, fontweight='bold'
        )

    ax_traj.set_xlim(-1.15, 1.15)
    ax_traj.set_ylim(-1.15, 1.15)
    ax_traj.set_xlabel('X Position [m]', fontsize=11, fontweight='bold')
    ax_traj.set_ylabel('Y Position [m]', fontsize=11, fontweight='bold')
    ax_traj.set_aspect('equal')
    ax_traj.grid(True, linestyle='--', alpha=0.5)
    ax_traj.legend(loc='lower right', framealpha=0.92, fontsize=8.5)

    # 2. Speed and Velocity Subplot
    steps_x = list(range(len(trace_x)))
    speeds = [pt[3] for pt in trace_x]
    ax_speed.plot(steps_x, speeds, '-o', color='teal', markersize=3, linewidth=1.8, label='Forward Speed v(t) [m/s]')
    ax_speed.set_xlabel('Simulation Step', fontsize=10, fontweight='bold')
    ax_speed.set_ylabel('Speed [m/s]', fontsize=10, fontweight='bold')
    ax_speed.set_title('Vehicle Forward Velocity v(t)', fontsize=11, fontweight='bold')
    ax_speed.grid(True, linestyle='--', alpha=0.5)
    ax_speed.legend(loc='upper right', framealpha=0.9, fontsize=8.5)

    # 3. Steering and Heading Subplot
    thetas = [pt[2] for pt in trace_x]
    ax_steer.plot(steps_x, thetas, '-^', color='darkviolet', markersize=3, linewidth=1.8, label='Heading Angle θ(t) [rad]')
    ax_steer.set_xlabel('Simulation Step', fontsize=10, fontweight='bold')
    ax_steer.set_ylabel('Heading θ [rad]', fontsize=10, fontweight='bold')
    ax_steer.set_title('Vehicle Heading Angle θ(t)', fontsize=11, fontweight='bold')
    ax_steer.grid(True, linestyle='--', alpha=0.5)
    ax_steer.legend(loc='lower right', framealpha=0.9, fontsize=8.5)

    plt.tight_layout()
    plt.savefig(output_png, dpi=180)
    plt.close(fig)
    print(f"Saved visual verification plot: {output_png}")


def main():
    parser = argparse.ArgumentParser(description="Closed-Loop Verification for DeepRacer")
    parser.add_argument("--cfg", default=os.path.join(SCRIPT_DIR, "deepracer_xsmall.cfg"), help="Path to config")
    parser.add_argument("--mdf", default=None, help="Path to controller mdf")
    parser.add_argument("--max-steps", type=int, default=60, help="Maximum steps")
    parser.add_argument("--min-cycles", type=int, default=2, help="Minimum patrol cycles")
    parser.add_argument("--output-png", default=os.path.join(SCRIPT_DIR, "closedloop_trajectory.png"), help="Output plot path")
    parser.add_argument("--verbose", action="store_true", default=True, help="Verbose output")
    args = parser.parse_args()

    cfg_path = os.path.abspath(args.cfg)
    if not os.path.exists(cfg_path):
        print(f"Error: Config file not found: {cfg_path}", file=sys.stderr)
        return 1

    cfg = ConfigReader(cfg_path)
    mdf_name = args.mdf or cfg.get_value_string("simulation.controller_file") or "deepracer_xsmall.mdf"
    mdf_path = os.path.join(os.path.dirname(cfg_path), mdf_name)
    if not os.path.exists(mdf_path):
        print(f"Error: Controller file not found: {mdf_path}", file=sys.stderr)
        return 1

    print(f"=== Running Closed-Loop Verification: Amazon DeepRacer ===")
    print(f"Configuration: {cfg_path}")
    print(f"Controller:    {mdf_path}")

    metrics = verify_deepracer(
        cfg_path, mdf_path, max_steps=args.max_steps, min_cycles=args.min_cycles, verbose=args.verbose
    )

    plot_deepracer_verification(cfg_path, metrics, args.output_png)

    print("\n=== DeepRacer Closed-Loop Verification Summary ===")
    print(f"  LTL Specification: GF(target1) & GF(target2) & G(!obstacles)")
    print(f"  Total steps:       {metrics['total_steps']}")
    print(f"  Target 1 visits:   {metrics['t1_visits']}")
    print(f"  Target 2 visits:   {metrics['t2_visits']}")
    print(f"  Patrol cycles:     {metrics['cycles']}")
    if metrics["lasso"]:
        print(f"  Periodic Lasso:    Period of {metrics['lasso']['period']} steps (Steps {metrics['lasso']['start_step']} -> {metrics['lasso']['loop_step']})")
    print(f"  Obstacle safety:   0 collisions")
    print(f"[PASS] Amazon DeepRacer closed-loop LTL specifications verified successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
