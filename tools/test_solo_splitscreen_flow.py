#!/usr/bin/env python3
"""Exercise the installed assets through solo sign-in, countdown and GPU match.

Uses declared SDL key presses and a diagnostic capture at actual match tick90.
No frame limit or simulated tick limit bypasses the normal Vista lifecycle.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--players", type=int, choices=(1,2), default=1)
    parser.add_argument("--validation", action="store_true")
    parser.add_argument("--mute", action="store_true")
    parser.add_argument("--retry-surface-formats", action="store_true")
    args = parser.parse_args()
    launcher, probe, output = (p.resolve() for p in (args.launcher, args.probe, args.output))
    output.mkdir(parents=True, exist_ok=True)
    prefs = output / "prefs/Halo2Native/Halo2"
    prefs.mkdir(parents=True, exist_ok=True)
    (prefs / "menu.cfg").write_text(
        'halo2-native-menu-v1\nactive "Solo Pilot"\nprofile "Solo Pilot"\n'+
        ('profile "Second Pilot"\n' if args.players==2 else ''))
    events = output / "window.events"
    events.write_text("")
    env = os.environ.copy()
    env.update(LD_PRELOAD=str(probe), XDG_DATA_HOME=str(prefs.parents[1]),
               HALO2_WINDOW_PROBE_LOG=str(events), HALO2_WINDOW_PROBE_RESIZE="1",
               HALO2_MENU_EFFECT_SEED="305419896", HALO2_MENU_FOG_SEED="305419896")
    validation=Path(__file__).resolve().parents[1]/".tools/validation/usr"
    if args.validation and validation.is_dir():
        env["VK_LAYER_PATH"]=str(validation/"share/vulkan/explicit_layer.d")
        env["LD_LIBRARY_PATH"]=os.pathsep.join(filter(None,(str(validation/"lib"),env.get("LD_LIBRARY_PATH"))))
    if args.players==2:
        env["HALO2_WINDOW_PROBE_TWO_PLAYERS"]="1"
    if args.retry_surface_formats:
        env["HALO2_WINDOW_PROBE_FORMAT_RETRY"] = "1"
    inputs = output / "neutral-input.tsv"
    inputs.write_text("# Neutral local controls; every actual match tick still executes.\n")
    command = [str(launcher), "--menu-screen", "9", "--skip-intro",
               "--viewport", "1280", "720", "--mp-capture-tick", "90",
               "--mp-input-script", str(inputs),
               "--screenshot", str(output / "match.ppm")]
    if args.validation:
        command.append("--validation")
    if args.mute:
        command.append("--mute")
    with (output / "runtime.log").open("w") as log:
        result = subprocess.run(command, env=env, stdout=log,
                                stderr=subprocess.STDOUT, timeout=180)
    log = (output / "runtime.log").read_text()
    observed = [line.split() for line in events.read_text().splitlines()]
    creates = [row for row in observed if row[0] == "create"]
    destroys = [row for row in observed if row[0] == "destroy"]
    world = re.search(rf"Multiplayer frames=(\d+) ticks=90 players={args.players} ", log)
    checks = {
        "application_exited_successfully": result.returncode == 0,
        "entered_split_screen_lobby": "Menu screen 14\n" in log,
        "normal_countdown": "Vista lifecycle: actual offline lobby countdown started\n" in log,
        "selected_local_roster": f"Native Slayer: lockout.map local_players={args.players} " in log,
        "actual_world_activated": all(f"Vista lifecycle: actual native world running; controller{i} source datum=" in log for i in range(args.players)),
        "all_local_views_presented": f"split_views={args.players}" in log,
        "match_presented_through_tick90": bool(world and int(world[1]) > 0) and (output / "match.ppm").is_file(),
        "same_window_until_shutdown": len(creates) == len(destroys) == 1 and creates[0][1] == destroys[0][1],
        "user_resize_preserved": len(destroys) == 1 and destroys[0][2:] == ["1100", "650"],
        "no_runtime_or_validation_errors": "error:" not in log and "VUID-" not in log,
    }
    if args.retry_surface_formats:
        checks["incomplete_surface_formats_recovered"] = ["incomplete_formats"] in observed
    report = {
        "scope": "Installed launcher and asset registry; actual local-player countdown, loading, player activation and GPU presentation through tick90. No full-match or original framebuffer parity claim.",
        "launcher_sha256": hashlib.sha256(launcher.read_bytes()).hexdigest(),
        "probe_sha256": hashlib.sha256(probe.read_bytes()).hexdigest(),
        "command": command, "returncode": result.returncode,
        "window_events": observed, "checks": checks, "passed": all(checks.values()),
    }
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"checks": checks, "passed": report["passed"]}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
