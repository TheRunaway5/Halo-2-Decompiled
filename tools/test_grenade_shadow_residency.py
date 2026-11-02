#!/usr/bin/env python3
"""Exercise native grenade births through ordinary single-view shadow rendering.

Run through tools/run_test.py. A real input-script grenade pulse must produce
salted native projectiles, detonate, and complete the requested rendered frame.
The neutral and receiver cases supply ordinary model/world shadow views without
a grenade pulse. Only bounded JSON/log evidence leaves scratch; inputs,
profiles, timings and captures do not.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

from weapon_test_cache import require_mcc_map


EXACT_ERROR = "shadow scene selected a section that was not preloaded"
SHADOW_FIELDS = ("world_native_shadow_generate_prepared_draws",
                 "world_native_shadow_apply_prepared_draws")
GRENADE = re.compile(r"^pStencil grenade input: tick=(\d+) definition=(\d+) "
                     r"object=(\d+) type=(\d+) admitted=([^\n]*)$", re.MULTILINE)


def sha256(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def require(value, message):
    if not value:
        raise RuntimeError(message)


def image_receipt(path, width, height):
    with path.open("rb") as stream:
        require(stream.readline() == b"P6\n", "Capture lacks its actual P6 header")
        require(stream.readline() == f"{width} {height}\n".encode(),
                "Capture differs from the requested drawable extent")
        require(stream.readline() == b"255\n", "Capture has an unsupported channel extent")
        header = stream.tell()
    require(path.stat().st_size == header + width * height * 3,
            "Capture does not contain a complete rendered surface")
    return {"bytes": path.stat().st_size, "sha256": sha256(path)}


def exercise(args, scratch, report):
    inputs = scratch / "grenade-inputs.tsv"
    pulse = int(args.case in ("frag", "plasma"))
    script = ("0 0 0 0 0 1.35 -0.6\n"
              f"20 0 0 0 {pulse} 1.35 -0.6\n"
              "21 0 0 0 0 1.35 -0.6\n")
    inputs.write_text(script)
    image = scratch / "completed.ppm"
    timings = scratch / "renderer.tsv"
    log = scratch / "native.log"
    command = [str(args.runtime), "--multiplayer", "--graphics-mode", "Original",
               "--map", str(args.map), "--shared-maps", str(args.shared_maps),
               "--local-players", "1", "--mp-input-script", str(inputs),
               "--mp-capture-tick", str(args.capture_tick), "--mp-reference-tick-cadence",
               "--screenshot", str(image), "--mute", "--viewport", str(args.width), str(args.height)]
    # Retain ordinary source model/shadow ownership. In particular, no root,
    # reference-player or shadow-disable option may replace that path.
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith(("HALO2_", "H2_"))}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), SDL_VIDEODRIVER="x11",
                       SDL_AUDIODRIVER="dummy", HALO2_RENDER_TIMINGS=str(timings))
    for key, name in (("XDG_CONFIG_HOME", "config"), ("XDG_DATA_HOME", "data"),
                      ("XDG_CACHE_HOME", "cache"), ("HALO2_PIPELINE_CACHE_DIRECTORY", "pipelines"),
                      ("MESA_SHADER_CACHE_DIR", "mesa"), ("__GL_SHADER_DISK_CACHE_PATH", "nvidia")):
        environment[key] = str(scratch / name)
    report.update(command=command, inputs=script, expected_grenade_pulses=pulse)
    started = time.monotonic()
    try:
        with log.open("wb") as stream:
            result = subprocess.run(command, cwd=args.root, env=environment, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=args.timeout)
        report["exit_code"] = result.returncode
    except subprocess.TimeoutExpired:
        report["exit_code"] = "timeout"
    finally:
        report["elapsed_seconds"] = round(time.monotonic() - started, 3)
        output = log.read_text(errors="replace") if log.exists() else ""
        report.update(exact_missing_section_error=EXACT_ERROR in output,
                      log_bytes=log.stat().st_size if log.exists() else 0,
                      log_sha256=sha256(log) if log.exists() else None)
        retained_log = args.report_output.with_suffix(".log")
        retained_log.write_text(output[-65536:])
        report["retained_log"] = {"path": str(retained_log), "bytes": retained_log.stat().st_size,
                                  "sha256": sha256(retained_log),
                                  "tail_truncated": len(output) > 65536}
    projectiles = [{"tick": int(tick), "definition": int(definition), "object": int(owner),
                    "type": int(kind), "admitted": admitted == "1"}
                   for tick, definition, owner, kind, admitted in GRENADE.findall(output)]
    # This source observation is emitted from actual grenade snapshots, not
    # from the script parser. Retain compact identities rather than every tick.
    report["native_grenade_observations"] = len(projectiles)
    report["grenade_owners"] = sorted({row["object"] for row in projectiles})
    report["grenade_definitions"] = sorted({row["definition"] for row in projectiles})
    report["grenade_types"] = sorted({row["type"] for row in projectiles})
    report["grenade_ticks"] = sorted({row["tick"] for row in projectiles})
    report["native_grenade_admitted_observations"] = sum(row["admitted"] for row in projectiles)
    require(not report["exact_missing_section_error"], EXACT_ERROR)
    require(report["exit_code"] == 0, f"Native match did not complete: {report['exit_code']}")
    require(not any(line.startswith(("error:", "Vulkan validation:")) for line in output.splitlines()),
            "Native match emitted an error or Vulkan validation diagnostic")
    match = re.search(r"Multiplayer frames=(\d+) ticks=(\d+) players=(\d+) "
                      r"connected=(\d+) peak_players=(\d+)", output)
    require(match is not None, "Native match lacks its actual submitted-frame summary")
    frames, ticks, players, connected, peak = map(int, match.groups())
    report.update(frames=frames, ticks=ticks, players=players)
    require(frames > 0 and ticks > 0 and players == connected == peak == 1,
            "Native frame/tick/local-owner completion differs from the requested match")
    captures = re.findall(r"^Input capture tick=.*$", output, re.MULTILINE)
    require(len(captures) == 1 and captures[0].startswith(f"Input capture tick={args.capture_tick} "),
            "The requested native tick was not captured exactly once")
    require(" capture_tick_cadence=1 " in captures[0], "Native one-frame-per-tick cadence was lost")
    detonation = re.search(r"\bdetonations=(\d+)\b", captures[0])
    require(detonation is not None, "Capture lacks its native detonation cursor")
    report["detonation_serial"] = int(detonation[1])
    if args.case in ("neutral", "receiver"):
        require(not projectiles and report["detonation_serial"] == 0,
                "No-throw control unexpectedly admitted a grenade or detonation")
    else:
        expected_type = 0 if args.case == "frag" else 1
        require(len(report["grenade_ticks"]) >= 2 and report["grenade_ticks"][0] >= 20,
                "The actual grenade lifecycle did not advance after the throw pulse")
        require(report["grenade_types"] == [expected_type],
                "Map spawn inventory did not produce the requested authored grenade type")
        require(all(row["admitted"] for row in projectiles),
                "An actual native grenade snapshot was not admitted")
        require(all(row["object"] != 0xffffffff and row["object"] >> 16 and
                    row["definition"] != 0xffffffff and row["definition"] >> 16
                    for row in projectiles), "Native grenade lost its full salted owner/definition")
        require(report["detonation_serial"] > 0,
                "A constructed grenade never reached its native release/detonation lifecycle")
    with timings.open() as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    require(rows and all(field in rows[0] for field in SHADOW_FIELDS),
            "Renderer timings lost their actual native shadow submission counters")
    report["shadow_draws"] = {field: sum(int(row[field]) for row in rows) for field in SHADOW_FIELDS}
    require(all(report["shadow_draws"].values()),
            "The ordinary native Generate/Apply GPU path did not submit draws")
    report["capture"] = image_receipt(image, args.width, args.height)
    report["checks"] = {"exact_tick_and_complete_surface": True, "actual_shadow_gpu_submissions": True,
                        "native_grenade_lifecycle_or_neutral_control": True,
                        "no_missing_shadow_section": True}
    report["passed"] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--shared-maps", type=Path, required=True)
    parser.add_argument("--case", choices=("neutral", "frag", "plasma", "receiver"), required=True)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--report-output", type=Path, required=True)
    parser.add_argument("--capture-tick", type=int, default=160)
    parser.add_argument("--width", type=int, default=1280)
    parser.add_argument("--height", type=int, default=720)
    parser.add_argument("--timeout", type=int, default=240)
    args = parser.parse_args()
    if "HALO2_TEST_SCRATCH" not in os.environ:
        parser.error("Run through tools/run_test.py for owned temporary storage")
    if not 80 <= args.capture_tick <= 300 or not 1 <= args.timeout <= 420:
        parser.error("Use a capture tick in80..300 and finite timeout in1..420 seconds")
    if not 1 <= args.width <= 3840 or not 1 <= args.height <= 2160:
        parser.error("Use a positive drawable no larger than3840x2160")
    args.runtime, args.root = args.runtime.resolve(strict=True), args.root.resolve(strict=True)
    args.map = require_mcc_map(args.map)
    args.shared_maps = args.shared_maps.resolve(strict=True)
    if args.map.parent != args.shared_maps:
        parser.error("The MCC map must belong to the explicitly selected shared-map directory")
    args.report_output = args.report_output.resolve()
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    report = {"passed": False, "scope": __doc__, "case": args.case, "graphics_mode": "Original",
              "runtime": str(args.runtime), "map": str(args.map), "shared_maps": str(args.shared_maps),
              "capture_tick": args.capture_tick, "viewport": [args.width, args.height],
              "retained_large_artifacts": []}
    initial = {args.runtime: sha256(args.runtime), args.map: sha256(args.map)}
    report.update(runtime_sha256=initial[args.runtime], map_sha256=initial[args.map],
                  test_sha256=sha256(__file__))
    try:
        require(os.access(args.runtime, os.X_OK), "Native runtime is not executable")
        with tempfile.TemporaryDirectory(prefix="grenade-shadow-", dir=os.environ["HALO2_TEST_SCRATCH"]) as directory:
            report["scratch"] = directory
            exercise(args, Path(directory), report)
    except Exception as error:
        report["passed"] = False
        report["error"] = str(error)
    finally:
        report["scratch_removed"] = "scratch" in report and not Path(report["scratch"]).exists()
        report["runtime_and_map_unchanged"] = all(path.is_file() and sha256(path) == pin
                                                 for path, pin in initial.items())
        report["passed"] &= report["scratch_removed"] and report["runtime_and_map_unchanged"]
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: report.get(key) for key in
                     ("passed", "case", "frames", "ticks", "native_grenade_observations",
                      "detonation_serial", "exact_missing_section_error", "error")}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
