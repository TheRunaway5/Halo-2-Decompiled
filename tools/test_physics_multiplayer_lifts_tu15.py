#!/usr/bin/env python3
"""Exercise authored original-map lift placements in visible TU1.5 multiplayer."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from test_physics_multiplayer_vehicle_seats_tu15 import start_mirror, stop_mirror

ROOT = Path(__file__).resolve().parents[1]
CASES = {
    'dune-tower': ('dune', 'scenarios\\objects\\covenant\\military\\cov_watchtower\\cov_watchtower_base\\cov_watchtower_dune'),
    'dune-pod': ('dune', 'scenarios\\objects\\covenant\\military\\cov_watchtower\\cov_watchtower_pod\\cov_watchtower_pod'),
    'ascension': ('ascension', 'scenarios\\objects\\multi\\ascension\\ascension_jumppad\\ascension_jumppad'),
    'coagulation': ('coagulation', 'scenarios\\objects\\multi\\halo\\coagulation\\powerhaus\\powerhaus'),
    'backwash-hvac': ('backwash', 'scenarios\\objects\\multi\\cyclotron\\hvac_lift_short\\hvac_lift_short'),
    'gemini-jumppad': ('gemini', 'scenarios\\objects\\multi\\marathon\\gemini\\jump_pad\\jump_pad'),
    'backwash-machine': ('backwash', 'scenarios\\objects\\multi\\backwash\\backwash_lift\\backwash_lift'),
    "colossus-high": ("colossus", "scenarios\\objects\\multi\\colossus\\col_power_lift_high\\col_power_lift_high"),
    "cyclotron": ("cyclotron", "scenarios\\objects\\multi\\cyclotron\\hvac_lift\\hvac_lift"),
    "cyclotron-short": ("cyclotron", "scenarios\\objects\\multi\\cyclotron\\hvac_lift_short\\hvac_lift_short"),
    "foundation": ("foundation", "scenarios\\objects\\multi\\marathon\\foundation\\foun_hvac_lift_short\\foun_hvac_lift_short"),
    "headlong": ("headlong", "scenarios\\objects\\multi\\headlong\\steam_lift\\steam_lift"),
    "lockout": ("lockout", "scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube"),
    "midship": ("midship", "scenarios\\objects\\multi\\midship\\cov_power_lift\\cov_power_lift"),
    "midpillar": ("midship", "scenarios\\objects\\multi\\midship\\midpillar_lift\\midpillar_lift"),
    "warlock": ("warlock", "scenarios\\objects\\multi\\halo\\warlock\\warlock_lift\\warlock_lift"),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("runtime", "output", "xbe", "ui"):
        parser.add_argument(f"--{name}", required=True, type=Path)
    parser.add_argument("--case", action="append", choices=tuple(CASES))
    parser.add_argument("--instance", type=int, default=0)
    parser.add_argument("--approach", action="store_true", help="Walk into the lower field through ordinary SDL, without jumping")
    parser.add_argument("--approach-distance-heights", type=float, help="Declared approach clearance in authored player heights")
    parser.add_argument("--clear-approach", action="store_true", help="Require a clear cached BSP straight approach and survey32 additional directions")
    parser.add_argument("--lower-field-floor", action="store_true", help="Query the floor from the authored lower field bound, below an upper landing")
    parser.add_argument("--jump-into-field", action="store_true", help="Deliver a normal jump near a raised field; requires actual phantom force and further rise")
    parser.add_argument("--jump-distance-heights", type=float, help="Declared distance for delivering the normal jump input")
    parser.add_argument("--create-initially-absent", action="store_true", help="Create the actual authored absent placement before the LAN join")
    parser.add_argument("--visible-mirror-display", required=True)
    args = parser.parse_args()
    if args.jump_into_field and not args.approach:
        parser.error("--jump-into-field requires --approach")
    if args.instance < 0:
        parser.error("Placement ordinal must be nonnegative")
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    summary = {"reference": "English TU1.5 only", "full_physics_parity": False, "cases": []}
    for name in args.case or CASES:
        map_name, definition = CASES[name]
        map_path = ROOT / "assets/disc/maps" / f"{map_name}.map"
        if map_name in ("warlock", "backwash", "gemini", "dune"):
            map_path = ROOT / "assets/map-pack/maps" / f"{map_name}.map"
        out = args.output / name
        env = {**os.environ, "HALO2_TU15_LIFT_DEFINITION": definition,
               "HALO2_TU15_LIFT_INSTANCE": str(args.instance)}
        if args.approach:
            env["HALO2_TU15_LIFT_APPROACH"] = "1"
        if args.approach_distance_heights is not None:
            env["HALO2_TU15_LIFT_APPROACH_HEIGHTS"] = str(args.approach_distance_heights)
        if args.clear_approach:
            env["HALO2_TU15_LIFT_CLEAR_APPROACH"] = "1"
        if args.lower_field_floor:
            env["HALO2_TU15_LIFT_LOWER_FLOOR"] = "1"
        if args.jump_into_field:
            env["HALO2_TU15_LIFT_JUMP_INPUT"] = "1"
            env["HALO2_TU15_PHANTOM_TRACE"] = "1"
        if args.jump_distance_heights is not None:
            env["HALO2_TU15_LIFT_JUMP_HEIGHTS"] = str(args.jump_distance_heights)
        if args.create_initially_absent:
            env["HALO2_TU15_LIFT_CREATE_INPUT"] = "1"
        mirror, owned = start_mirror(args, out)
        command = [sys.executable, str(ROOT / "tools/test_physics_multiplayer_visible_tu15.py"),
                   "--runtime", str(args.runtime.resolve()), "--output", str(out),
                   "--map", str(map_path), "--xbe", str(args.xbe.resolve()), "--ui", str(args.ui.resolve()),
                   "--scenario", "lan-lift", "--visible-mirror-display", args.visible_mirror_display,
                   "--mirror-process-info", str(mirror)]
        print(f"START {name} placement {args.instance}", flush=True)
        try:
            with (args.output / f"{name}.log").open("x") as log:
                result = subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        finally:
            stop_mirror(owned)
        row = {"case": name, "placement_ordinal": args.instance, "exit_code": result.returncode,
               "report": str(out / "lan-lift/report.json")}
        report_path = Path(row["report"])
        if report_path.exists():
            report = json.loads(report_path.read_text())
            for field in ("acceptance_passed", "exact_lan_snapshot_comparisons", "lift_travel_observed",
                          "initial_z", "peak_z", "maximum_upward_velocity"):
                if field in report:
                    row[field] = report[field]
        execution_path = out / "lan-lift/execution.json"
        if execution_path.exists():
            execution = json.loads(execution_path.read_text())
            row["mirror_continuously_verified"] = execution.get("mirror_continuously_verified", False)
            row["runtime_stable"] = execution["runtime_stable"]
        summary["cases"].append(row)
        (args.output / "lift-suite.json").write_text(json.dumps(summary, indent=2) + "\n")
        print(json.dumps(row), flush=True)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
