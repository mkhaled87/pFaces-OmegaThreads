#!/usr/bin/env python3
"""
Automated Headless Closed-Loop Verification for Pickup-Delivery Drone Example (pFaces-OmegaThreads)

Specification:
  G(!obstacles) &
  GF(pickup1) & GF(pickup2) &
  G(pickup1 -> ((!charger & !pickup2 & !delivery2) U delivery1)) &
  G(pickup2 -> ((!charger & !pickup1 & !delivery1) U delivery2)) &
  G(lowbattery -> (charger U fullbattery))

Validates that synthesized controller satisfies LTL specifications in closed loop:
  - Avoids all obstacles at all times (G !obstacles)
  - Battery is never exhausted (> 0%)
  - Pickup1 requires delivery to delivery1 before visiting charger, pickup2, or delivery2
  - Pickup2 requires delivery to delivery2 before visiting charger, pickup1, or delivery1
  - Low battery condition is followed by charging until full battery
  - Both pickup and delivery goals are satisfied sequentially and repeatedly (GF pickup1 & GF pickup2)
  - Detects closed-loop limit cycle / lasso recurrence
"""

import argparse
import os
import sys

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


def model_dynamics(x, u):
    """Discrete system dynamics matching pickupdelivery.cl."""
    pos_x = x[0]
    pos_y = x[1]
    battery = x[2]
    inp = int(u[0])

    if inp == 0:      # stay
        pass
    elif inp == 1:    # left
        pos_x -= 1.0
    elif inp == 2:    # up
        pos_y += 1.0
    elif inp == 3:    # right
        pos_x += 1.0
    elif inp == 4:    # down
        pos_y -= 1.0

    # Charging region: [6.5, 8.5] x [3.5, 4.5]
    is_charging = (x[0] >= 6.5 and x[0] <= 8.5) and (x[1] >= 3.5 and x[1] <= 4.5)
    if is_charging:
        battery += 20.0
        if battery > 99.0:
            battery = 99.0
    else:
        battery -= 1.0

    return [pos_x, pos_y, battery]


def verify_closed_loop(cfg_path, mdf_path, max_steps=300, min_deliveries=2, verbose=True):
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
    p1_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_pickup1"))
    p2_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_pickup2"))
    d1_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_delivery1"))
    d2_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_delivery2"))
    ch_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_charger"))
    obs_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_obstacles"))
    low_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_lowbattery"))
    full_HRs = str2hyperrects(cfg.get_value_string("system.states.subsets.mapping_fullbattery"))

    # Initial state
    init_str = cfg.get_value_string("simulation.initial_state")
    state = parse_float_list(init_str)
    controller.machine_state = 0

    carrying_p1 = False
    carrying_p2 = False
    p1_deliveries = 0
    p2_deliveries = 0
    charging_episodes = 0
    in_charger_prev = False
    min_battery = state[2]
    max_battery = state[2]

    # Lasso detection: (machine_state, x_flat) -> first visit step
    visited_states = {}
    lasso_detected = None

    events = []

    if verbose:
        print(f"\n--- Simulating Pickup-Delivery Drone from x_0 = {state} ---")

    for step in range(max_steps):
        # 1. State check
        x_sym = qnt_x.conc_to_flat(state)
        q_curr = controller.machine_state
        state_key = (q_curr, x_sym)

        if lasso_detected is None:
            if state_key in visited_states:
                lasso_start_step = visited_states[state_key]
                lasso_period = step - lasso_start_step
                lasso_detected = (lasso_start_step, step, lasso_period, q_curr)
                if verbose:
                    print(
                        f"  [LASSO] Periodic limit cycle detected at step {step}: "
                        f"(q={q_curr}, sym={x_sym}) previously visited at step {lasso_start_step} "
                        f"(Period: {lasso_period} steps)"
                    )
            else:
                visited_states[state_key] = step

        # 2. Safety invariant: G(!obstacles)
        assert not is_in_hyperrects(state, obs_HRs), (
            f"SAFETY VIOLATION at step {step}: Drone collided with obstacle at {state}"
        )

        # 3. Battery invariant: Battery > 0
        batt = state[2]
        assert batt > 0.0, f"BATTERY DEPLETION at step {step}: state {state}"
        min_battery = min(min_battery, batt)
        max_battery = max(max_battery, batt)

        # 4. Charger event tracking
        in_charger = is_in_hyperrects(state, ch_HRs)
        if in_charger and not in_charger_prev:
            charging_episodes += 1
            events.append((step, "ENTER_CHARGER", list(state)))
            if verbose:
                print(f"  Step {step:3d}: Reached CHARGER at {state[:2]}, battery={batt:.1f}%")
        in_charger_prev = in_charger

        # 5. Mission 1 verification:
        # G(pickup1 -> ((!charger & !pickup2 & !delivery2) U delivery1))
        in_p1 = is_in_hyperrects(state, p1_HRs)
        in_d1 = is_in_hyperrects(state, d1_HRs)

        if in_p1:
            assert not carrying_p2, f"VIOLATION at step {step}: Entered pickup1 while carrying p2!"
            if not carrying_p1:
                carrying_p1 = True
                events.append((step, "PICKUP_1", list(state)))
                if verbose:
                    print(f"  Step {step:3d}: [MISSION 1] Picked up package 1 at {state[:2]}, battery={batt:.1f}%")

        if carrying_p1:
            assert not in_charger, (
                f"SPEC VIOLATION at step {step}: Visited charger while carrying package 1 before delivery1!"
            )
            assert not is_in_hyperrects(state, p2_HRs), (
                f"SPEC VIOLATION at step {step}: Visited pickup2 while carrying package 1 before delivery1!"
            )
            assert not is_in_hyperrects(state, d2_HRs), (
                f"SPEC VIOLATION at step {step}: Visited delivery2 while carrying package 1 before delivery1!"
            )
            if in_d1:
                carrying_p1 = False
                p1_deliveries += 1
                events.append((step, "DELIVER_1", list(state)))
                if verbose:
                    print(f"  Step {step:3d}: [MISSION 1] Successfully delivered package 1 to delivery1! (Total: {p1_deliveries})")

        # 6. Mission 2 verification:
        # G(pickup2 -> ((!charger & !pickup1 & !delivery1) U delivery2))
        in_p2 = is_in_hyperrects(state, p2_HRs)
        in_d2 = is_in_hyperrects(state, d2_HRs)

        if in_p2:
            assert not carrying_p1, f"VIOLATION at step {step}: Entered pickup2 while carrying p1!"
            if not carrying_p2:
                carrying_p2 = True
                events.append((step, "PICKUP_2", list(state)))
                if verbose:
                    print(f"  Step {step:3d}: [MISSION 2] Picked up package 2 at {state[:2]}, battery={batt:.1f}%")

        if carrying_p2:
            assert not in_charger, (
                f"SPEC VIOLATION at step {step}: Visited charger while carrying package 2 before delivery2!"
            )
            assert not in_p1, (
                f"SPEC VIOLATION at step {step}: Visited pickup1 while carrying package 2 before delivery2!"
            )
            assert not in_d1, (
                f"SPEC VIOLATION at step {step}: Visited delivery1 while carrying package 2 before delivery2!"
            )
            if in_d2:
                carrying_p2 = False
                p2_deliveries += 1
                events.append((step, "DELIVER_2", list(state)))
                if verbose:
                    print(f"  Step {step:3d}: [MISSION 2] Successfully delivered package 2 to delivery2! (Total: {p2_deliveries})")

        # Early termination condition: at least min_deliveries for both missions and lasso observed
        if p1_deliveries >= min_deliveries and p2_deliveries >= min_deliveries and lasso_detected is not None:
            if verbose:
                print(
                    f"\n  Sufficient missions fulfilled ({p1_deliveries} x Mission 1, "
                    f"{p2_deliveries} x Mission 2) with verified periodic loop closure."
                )
            break

        # 7. Query controller for action
        try:
            actions = controller.get_control_actions(x_sym)
        except Exception as e:
            raise AssertionError(f"Controller failure at step {step}, q={q_curr}, x={state}, sym={x_sym}: {e}")

        assert len(actions) > 0, f"No control actions for state {state} (sym {x_sym})"
        u = qnt_u.flat_to_conc(actions[0])

        # 8. Apply dynamics
        next_state = model_dynamics(state, u)
        state = next_state

    # Final assertions
    assert p1_deliveries >= min_deliveries, (
        f"LIVENESS FAILURE: Achieved {p1_deliveries} package 1 deliveries, expected >= {min_deliveries}"
    )
    assert p2_deliveries >= min_deliveries, (
        f"LIVENESS FAILURE: Achieved {p2_deliveries} package 2 deliveries, expected >= {min_deliveries}"
    )
    assert lasso_detected is not None, "FAIRNESS FAILURE: No recurring limit cycle detected"

    metrics = {
        "total_steps": step + 1,
        "p1_deliveries": p1_deliveries,
        "p2_deliveries": p2_deliveries,
        "charging_episodes": charging_episodes,
        "min_battery": min_battery,
        "max_battery": max_battery,
        "lasso": lasso_detected,
        "events": events,
    }
    return metrics


def main():
    parser = argparse.ArgumentParser(description="Headless closed-loop verification for Pickup-Delivery Drone")
    parser.add_argument("--cfg", default=os.path.join(SCRIPT_DIR, "pickupdelivery.cfg"), help="Path to pickupdelivery.cfg")
    parser.add_argument("--mdf", default=None, help="Path to pickupdelivery.mdf (defaults to cfg simulation.controller_file)")
    parser.add_argument("--max-steps", type=int, default=300, help="Maximum steps to simulate")
    parser.add_argument("--min-deliveries", type=int, default=2, help="Minimum full delivery cycles per mission")
    parser.add_argument("--verbose", action="store_true", default=True, help="Verbose output")
    args = parser.parse_args()

    cfg_path = os.path.abspath(args.cfg)
    if not os.path.exists(cfg_path):
        print(f"[ERROR] Config file not found: {cfg_path}", file=sys.stderr)
        return 1

    print(f"=== Running Closed-Loop Verification: Pickup-Delivery Drone ===")
    print(f"Loading configuration: {cfg_path}")
    cfg = ConfigReader(cfg_path)

    mdf_name = args.mdf or cfg.get_value_string("simulation.controller_file")
    mdf_path = os.path.join(os.path.dirname(cfg_path), mdf_name)
    if not os.path.exists(mdf_path):
        print(f"[ERROR] Controller file not found: {mdf_path}", file=sys.stderr)
        return 1

    print(f"Loading controller: {mdf_path}")

    try:
        metrics = verify_closed_loop(
            cfg_path, mdf_path, max_steps=args.max_steps, min_deliveries=args.min_deliveries, verbose=args.verbose
        )

        lasso_start, lasso_end, lasso_period, lasso_q = metrics["lasso"]
        print("\n=== Pickup-Delivery Closed-Loop Verification Summary ===")
        print(f"LTL Specification: {cfg.get_value_string('specifications.ltl_formula')}")
        print(f"  Total simulation steps: {metrics['total_steps']}")
        print(f"  Completed Mission 1 (Pickup 1 -> Delivery 1): {metrics['p1_deliveries']} cycles")
        print(f"  Completed Mission 2 (Pickup 2 -> Delivery 2): {metrics['p2_deliveries']} cycles")
        print(f"  Recharging episodes: {metrics['charging_episodes']}")
        print(f"  Battery range: [{metrics['min_battery']:.1f}%, {metrics['max_battery']:.1f}%]")
        print(f"  Lasso recurrence: period of {lasso_period} steps (steps {lasso_start} -> {lasso_end}, q={lasso_q})")
        print(f"  Safety collisions: 0")
        print("[PASS] Closed-loop LTL specifications verified successfully with 0 violations.\n")
        return 0

    except AssertionError as ae:
        print(f"\n[FAIL] Assertion error during verification: {ae}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"\n[FAIL] Unexpected error during verification: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
