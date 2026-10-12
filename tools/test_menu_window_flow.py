#!/usr/bin/env python3
"""Drive real SDL menu input and verify one-profile, one-window match launch.

The preload library observes window lifetime and injects declared test key
presses. It does not replace simulation, map data, rendering or hit handling.
Run under a real display or xvfb-run. Linux-only SDL observation test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--maps", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--validation", action="store_true")
    args = parser.parse_args()
    runtime, probe, maps, output = (p.resolve() for p in
                                  (args.runtime, args.probe, args.maps, args.output))
    output.mkdir(parents=True, exist_ok=True)
    prefs = output / "prefs/Halo2Native/Halo2"
    prefs.mkdir(parents=True, exist_ok=True)
    (prefs / "menu.cfg").write_text(
        'halo2-native-menu-v1\nactive "Player One"\nprofile "Player One"\n')
    events = output / "window.events"
    events.write_text("")
    env = os.environ.copy()
    env.update(LD_PRELOAD=str(probe), XDG_DATA_HOME=str(prefs.parents[1]),
               HALO2_WINDOW_PROBE_LOG=str(events), HALO2_WINDOW_PROBE_RESIZE="1")
    command = [str(runtime), "--menu", "--menu-screen", "9", "--menu-maps", str(maps),
               "--skip-intro", "--frames", "3", "--hidden", "--mute",
               "--viewport", "1280", "720", "--screenshot", str(output / "match.ppm")]
    if args.validation:
        command.append("--validation")
    with (output / "runtime.log").open("w") as log:
        result = subprocess.run(command, env=env, stdout=log,
                                stderr=subprocess.STDOUT, timeout=180)
    log = (output / "runtime.log").read_text()
    observed = [line.split() for line in events.read_text().splitlines()]
    creates = [row for row in observed if row[0] == "create"]
    destroys = [row for row in observed if row[0] == "destroy"]
    checks = {
        "application_exited_successfully": result.returncode == 0,
        "entered_authored_lobby": "Menu screen 14\n" in log,
        "one_selected_profile_one_player": "Native Slayer: lockout.map local_players=1 " in log,
        "one_window": len(creates) == len(destroys) == 1,
        "same_window_until_shutdown": len(creates) == len(destroys) == 1 and creates[0][1] == destroys[0][1],
        "user_resize_preserved": len(destroys) == 1 and destroys[0][2:] == ["1100", "650"],
        "match_rendered": "Multiplayer frames=3 " in log and (output / "match.ppm").is_file(),
        "no_validation_errors": "VUID-" not in log,
    }
    report = {
        "scope": "Actual native menu-to-local-match transition with declared SDL key inputs and resize; not full lobby/countdown or gameplay parity.",
        "runtime_sha256": hashlib.sha256(runtime.read_bytes()).hexdigest(),
        "probe_sha256": hashlib.sha256(probe.read_bytes()).hexdigest(),
        "command": command, "returncode": result.returncode,
        "window_events": observed, "checks": checks, "passed": all(checks.values()),
    }
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"checks": checks, "passed": report["passed"]}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
