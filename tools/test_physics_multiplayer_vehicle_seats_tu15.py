#!/usr/bin/env python3
"""Drive actual TU1.5 multiplayer seat/camera scenarios through ordinary SDL.

The native acceptance executable owns every body, seat transition and camera.
These cases specify test input only. Scorpion rider seats explicitly reject
players in original CB7A0; their NPC riding behavior is outside these cases.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from test_physics_multiplayer_visible_tu15 import inspect_mirror

ROOT = Path(__file__).resolve().parents[1]
CASES = {
    "warthog-passenger": ("warthog", 1, False, False),
    "warthog-turret": ("warthog", 0, True, False),
    "spectre-passenger1": ("spectre", 1, False, False),
    "spectre-passenger2": ("spectre", 2, False, False),
    "spectre-turret": ("spectre", 0, True, False),
    "gauss-turret": ("warthog_gauss", 0, True, False),
    **{f"scorpion-rejection{i}": ("scorpion", i, False, False) for i in range(1, 5)},
    "moving-warthog-passenger": ("warthog", 1, False, True),
    "moving-warthog-turret": ("warthog", 0, True, True),
    "moving-spectre-passenger1": ("spectre", 1, False, True),
    "moving-spectre-passenger2": ("spectre", 2, False, True),
    "moving-spectre-turret": ("spectre", 0, True, True),
    "moving-gauss-turret": ("warthog_gauss", 0, True, True),
}


def start_mirror(args, out):
    out.mkdir(parents=True, exist_ok=True)
    log = (out / "mirror.log").open("x")
    capture = subprocess.Popen([
        "ffmpeg", "-nostdin", "-hide_banner", "-loglevel", "warning",
        "-f", "x11grab", "-video_size", "960x540", "-framerate", "30",
        "-i", os.environ["DISPLAY"], "-an", "-c:v", "rawvideo", "-threads", "1",
        "-pix_fmt", "rgb24", "-f", "rawvideo", "pipe:1"],
        stdout=subprocess.PIPE, stderr=log)
    player = subprocess.Popen([
        "ffplay", "-hide_banner", "-loglevel", "warning", "-f", "rawvideo",
        "-pixel_format", "rgb24", "-video_size", "960x540", "-framerate", "30",
        "-window_title", "Halo2 TU1.5 live multiplayer", "-i", "pipe:0"],
        stdin=capture.stdout, stdout=log, stderr=log,
        env={**os.environ, "DISPLAY": args.visible_mirror_display,
             "SDL_VIDEODRIVER": "x11", "SDL_RENDER_DRIVER": "software"})
    capture.stdout.close()
    path = out / "mirror-process-info.json"
    path.write_text(json.dumps({"display": os.environ["DISPLAY"],
        "mirror_display": args.visible_mirror_display,
        "capture_pid": capture.pid, "mirror_pid": player.pid}, indent=2) + "\n")
    observed = argparse.Namespace(visible_mirror_display=args.visible_mirror_display,
                                  mirror_process_info=path)
    try:
        for attempt in range(50):
            try:
                inspect_mirror(observed)
                return path, (player, capture, log)
            except Exception:
                if attempt == 49 or player.poll() is not None or capture.poll() is not None:
                    raise
                time.sleep(.1)
    except Exception:
        stop_mirror((player, capture, log))
        raise


def stop_mirror(owned):
    if owned is None:
        return
    player, capture, log = owned
    for process in (player, capture):
        if process.poll() is None:
            process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ("runtime", "output", "xbe", "ui"):
        parser.add_argument(f"--{field}", type=Path, required=True)
    parser.add_argument("--case", action="append", choices=tuple(CASES))
    parser.add_argument("--visible-mirror-display")
    parser.add_argument("--mirror-process-info", type=Path)
    args = parser.parse_args()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    summary = {"reference": "English TU1.5 only", "full_physics_parity": False, "cases": []}
    for name in args.case or CASES:
        vehicle, seat, child, moving = CASES[name]
        env = os.environ.copy()
        for variable in ("HALO2_TU15_VEHICLE_CHILD", "HALO2_TU15_SEAT_DRIVER",
                         "HALO2_TU15_VEHICLE_FIXTURE_ONLY"):
            env.pop(variable, None)
        env["HALO2_TU15_VEHICLE_NAME"] = vehicle
        env["HALO2_TU15_VEHICLE_SEAT"] = str(seat)
        if child:
            env["HALO2_TU15_VEHICLE_CHILD"] = "1"
        if moving:
            env["HALO2_TU15_SEAT_DRIVER"] = "1"
        out = args.output / name
        command = [sys.executable, str(ROOT / "tools/test_physics_multiplayer_visible_tu15.py"),
                   "--runtime", str(args.runtime.resolve()), "--output", str(out),
                   "--xbe", str(args.xbe.resolve()), "--ui", str(args.ui.resolve()),
                   "--scenario", "lan-vehicle"]
        owned_mirror = None
        if args.visible_mirror_display:
            mirror_info = args.mirror_process_info
            if mirror_info is None:
                mirror_info, owned_mirror = start_mirror(args, out)
            command += ["--visible-mirror-display", args.visible_mirror_display,
                        "--mirror-process-info", str(mirror_info.resolve())]
        print(f"START {name}", flush=True)
        try:
            with (args.output / f"{name}.log").open("x") as log:
                result = subprocess.run(command, cwd=ROOT, env=env, stdout=log, stderr=subprocess.STDOUT)
        finally:
            stop_mirror(owned_mirror)
        row = {"case": name, "exit_code": result.returncode,
               "report": str(out / "lan-vehicle/report.json"),
               "execution": str(out / "lan-vehicle/execution.json")}
        if Path(row["report"]).exists():
            report = json.loads(Path(row["report"]).read_text())
            for field in ("acceptance_passed", "exact_lan_snapshot_comparisons",
                          "source_seat_entered", "source_seat_exited", "source_seat_rejection_verified",
                          "companion_driver_entered", "companion_original_camera_frames",
                          "submitted_seated_camera_frames", "submitted_seated_camera_look_turn_radians",
                          "moving_occupied_vehicle_distance"):
                if field in report:
                    row[field] = report[field]
        if moving:
            trace = out / "lan-vehicle/authority-player-bindings.jsonl"
            binding_rows = [json.loads(line) for line in trace.read_text().splitlines()] if trace.exists() else []
            row["original_team_birth_verified"] = bool(binding_rows) and all(
                player["unit_team"] == 0 for snapshot in binding_rows for player in snapshot["players"])
            if result.returncode == 0 and not row["original_team_birth_verified"]:
                row["exit_code"] = 2
                row["failure"] = "Cooperative original unit birth did not retain the selected team"
        if Path(row["execution"]).exists():
            execution = json.loads(Path(row["execution"]).read_text())
            row["mirror_continuously_verified"] = execution.get("mirror_continuously_verified", False)
            row["runtime_stable"] = execution["runtime_stable"]
        summary["cases"].append(row)
        (args.output / "seat-suite.json").write_text(json.dumps(summary, indent=2) + "\n")
        print(json.dumps(row), flush=True)
        if row["exit_code"]:
            return row["exit_code"]
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
