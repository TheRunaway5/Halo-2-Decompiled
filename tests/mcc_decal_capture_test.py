#!/usr/bin/env python3
"""Actual native cached-decal draw control; run through tools/run_test.py.

The ordinary source world owns geometry, textures, cameras and visibility.
Enabled/hidden/enabled native frames are controls, not Xbox frame references.
Raw frames/logs remain under HALO2_TEST_SCRATCH unless explicitly retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import time


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def ppm(path):
    data = path.read_bytes()
    match = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", data)
    if not match:
        raise RuntimeError("Native capture is not an unaltered P6 RGB frame")
    width, height = map(int, match.groups())
    pixels = data[match.end():]
    if len(pixels) != width * height * 3:
        raise RuntimeError("Native frame is truncated or has unexpected trailing bytes")
    return width, height, pixels


def compare(first, second, players):
    width, height, left = first
    if first[:2] != second[:2]:
        raise RuntimeError("Native capture dimensions changed between controls")
    right = second[2]
    changed = [0] * players
    maximum = 0
    for y in range(height):
        for x in range(width):
            at = (y * width + x) * 3
            delta = max(abs(left[at + channel] - right[at + channel]) for channel in range(3))
            maximum = max(maximum, delta)
            if delta:
                # Current hud_viewport/source_view_grid has equal 2x2 outer
                # cells for four players at640x480. This is a spatial count,
                # not an injected view/camera or a per-draw observer.
                cell = 0 if players == 1 else (2 if y >= height // 2 else 0) + (x >= width // 2)
                changed[cell] += 1
    return {"changed_pixels": sum(changed), "changed_pixels_by_cell": changed,
            "maximum_channel_delta": maximum,
            "all_cells_have_surviving_pixels": all(changed)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--shader-dir", type=Path, required=True,
                        help="Exact configured/staged native shader directory, inventoried without overriding the runtime")
    parser.add_argument("--report", type=Path, required=True)
    parser.add_argument("--tick", type=int, default=60)
    parser.add_argument("--timeout", type=float, default=240)
    parser.add_argument("--views", type=int, nargs="+", default=[1, 4], choices=[1, 4])
    parser.add_argument("--require-each-view", action="store_true",
                        help="Require positive pixels in every local cell; ordinary spawn cameras may not face a visible decal")
    parser.add_argument("--retain-captures", type=Path,
                        help="Explicitly retain the six raw frames and logs at this path")
    args = parser.parse_args()
    if "HALO2_TEST_SCRATCH" not in os.environ:
        parser.error("Run this helper through tools/run_test.py")
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]) / "mcc-decal-capture"
    scratch.mkdir()
    args.runtime = args.runtime.resolve(); args.map = args.map.resolve()
    args.shader_dir = args.shader_dir.resolve(); args.report = args.report.resolve()
    if args.retain_captures:args.retain_captures = args.retain_captures.resolve()
    report = {"passed": False, "cases": [], "scope":
              "Actual offline prepared MCC source world and native GPU enabled/hidden/enabled controls; no original Xbox pixels or network replication claim",
              "camera": "Ordinary actual local-player cameras, no camera/root override",
              "view_cells": "Actual standard640x480 full surface; four-player spatial counts use its equal2x2 outer cells",
              "shader_directory": str(args.shader_dir), "shader_directory_override": False,
              "capture_tick": args.tick, "full_goal_complete": False}
    retention_created = False
    try:
        if args.retain_captures:
            args.retain_captures.mkdir(parents=True, exist_ok=False)
            retention_created = True
        for path in (args.runtime, args.map):
            if not path.is_file():
                raise RuntimeError(f"Required native input is missing: {path}")
        with args.map.open("rb") as stream:
            header = stream.read(8)
        if len(header) != 8 or struct.unpack("<II", header) != (0x68656164, 13):
            raise RuntimeError("Actual runtime control requires an MCC version13 cache")
        report["cache_version"] = 13
        shaders = sorted(args.shader_dir.rglob("*.spv"))
        if not shaders:
            raise RuntimeError("Exact native shader directory has no compiled programs")
        pins = {str(path): digest(path) for path in [args.runtime, args.map, *shaders]}
        report["pins"] = pins
        validation_env = {}
        bundled_validation = Path(__file__).resolve().parents[1] / ".tools" / "validation"
        bundled_layers = bundled_validation / "usr/share/vulkan/explicit_layer.d"
        bundled_libraries = bundled_validation / "usr/lib"
        if os.environ.get("VK_LAYER_PATH"):
            report["validation_layer_provenance"] = "Existing VK_LAYER_PATH environment"
            report["validation_layer_path"] = os.environ["VK_LAYER_PATH"]
        elif bundled_layers.is_dir() and bundled_libraries.is_dir():
            validation_env["VK_LAYER_PATH"] = str(bundled_layers)
            validation_env["LD_LIBRARY_PATH"] = str(bundled_libraries) + (
                os.pathsep + os.environ["LD_LIBRARY_PATH"] if os.environ.get("LD_LIBRARY_PATH") else "")
            report["validation_layer_provenance"] = "Existing repository bundled validation layer; no installation or download"
            report["validation_layer_path"] = str(bundled_layers)
            report["validation_library_path_prefix"] = str(bundled_libraries)
        else:
            report["validation_layer_provenance"] = "System Vulkan loader discovery; no existing repository bundle found"
        script = scratch / "neutral.tsv"
        script.write_text("# Neutral controls; actual native simulation continues.\n")
        report["input_sha256"] = digest(script)
        failures = []
        if not os.environ.get("DISPLAY"):
            raise RuntimeError("Native GPU control requires DISPLAY; wrap this helper in xvfb-run when headless")
        for players in args.views:
            case = {"local_players": players, "runs": []}; report["cases"].append(case)
            images = []
            for mode in ("enabled", "hidden", "repeat"):
                stem = f"{players}-{mode}"
                frame, log = scratch / f"{stem}.ppm", scratch / f"{stem}.log"
                env = dict(os.environ)
                env.update(validation_env)
                env.pop("WAYLAND_DISPLAY", None)
                env.pop("HALO2_REFERENCE_CACHE_DECALS_HIDE", None)
                env["SDL_VIDEODRIVER"] = "x11"
                env["XDG_CONFIG_HOME"] = str(scratch / "config")
                for variable, directory in (
                        ("HALO2_PIPELINE_CACHE_DIRECTORY", "pipeline-cache"),
                        ("MESA_SHADER_CACHE_DIR", "mesa-shader-cache"),
                        ("__GL_SHADER_DISK_CACHE_PATH", "nvidia-shader-cache")):
                    cache = scratch / directory
                    cache.mkdir(exist_ok=True)
                    env[variable] = str(cache)
                if mode == "hidden":
                    env["HALO2_REFERENCE_CACHE_DECALS_HIDE"] = "1"
                command = [str(args.runtime), "--multiplayer", "--map", str(args.map),
                           "--shared-maps", str(args.map.parent), "--mp-starting-weapon", "5",
                           "--viewport", "640", "480", "--local-players", str(players),
                           "--mute", "--hidden", "--validation", "--mp-input-script", str(script),
                           "--mp-capture-tick", str(args.tick), "--mp-reference-tick-cadence",
                           "--mp-reference-render-fraction", "0", "--mp-reference-auxiliary-seed", "305419896",
                           "--screenshot", str(frame)]
                row = {"mode": mode, "command": command}; case["runs"].append(row)
                started = time.monotonic()
                with log.open("wb") as output:
                    try:
                        run = subprocess.run(command, env=env, stdout=output, stderr=subprocess.STDOUT,
                                             timeout=args.timeout)
                        row["exit_code"] = run.returncode
                    except subprocess.TimeoutExpired:
                        row["exit_code"] = 124; row["timeout"] = True
                row["seconds"] = round(time.monotonic() - started, 3)
                text = log.read_text(errors="replace")
                row["validation_errors"] = [line[:500] for line in text.splitlines()
                                            if "VUID-" in line or "Validation Error" in line][:20]
                observations = re.findall(r"Source cached decals: frame=(\d+) view=(\d+) draws=(\d+) quads=(\d+)", text)
                latest = {}
                for native_frame, view, draws, quads in observations:
                    latest[int(view)] = {"frame": int(native_frame), "draws": int(draws), "quads": int(quads)}
                row["last_source_decal_view_submissions"] = latest
                row["frame_present"] = frame.is_file()
                if row["exit_code"] or row["validation_errors"] or not frame.is_file():
                    row["diagnostic_tail"] = text[-4000:]
                    failures.append(f"{stem}: native frame failed")
                    images.append(None)
                else:
                    image = ppm(frame)
                    if image[:2] != (640, 480):
                        raise RuntimeError("Native screenshot did not preserve the declared640x480 surface")
                    row["frame_sha256"] = digest(frame)
                    images.append(image)
                if args.retain_captures:
                    for path in (frame, log):
                        if path.is_file():shutil.copy2(path, args.retain_captures / path.name)
            if all(image is not None for image in images):
                case["enabled_hidden"] = compare(images[0], images[1], players)
                case["enabled_repeat"] = compare(images[0], images[2], players)
                positive = case["enabled_hidden"]["changed_pixels"] > 0
                repeat = case["enabled_repeat"]["changed_pixels"] == 0
                submitted = all(len(row["last_source_decal_view_submissions"]) == players and
                                any(item["quads"] for item in row["last_source_decal_view_submissions"].values())
                                for row in case["runs"])
                case["passed"] = positive and repeat and submitted
                if args.require_each_view:
                    case["passed"] &= case["enabled_hidden"]["all_cells_have_surviving_pixels"]
                if not positive:failures.append(f"{players} views: hiding the source cached-decal consumer changes no surviving pixel")
                if not repeat:failures.append(f"{players} views: unchanged enabled controls are not pixel-identical")
                if not submitted:failures.append(f"{players} views: actual source decal draw submissions are absent")
                if args.require_each_view and not case["enabled_hidden"]["all_cells_have_surviving_pixels"]:
                    failures.append(f"{players} views: one or more ordinary cameras have no surviving decal pixel")
            else:
                case["passed"] = False
        changed = [str(path) for path in [args.runtime, args.map, *shaders]
                   if not path.is_file() or digest(path) != pins[str(path)]]
        report["changed_inputs"] = changed
        if changed:failures.append("Native input bytes changed during the controls")
        report["failures"] = failures
        report["passed"] = not failures and all(case["passed"] for case in report["cases"])
    except Exception as error:
        report["error"] = str(error)
    if retention_created:
        report["retained_artifacts"] = {str(path.resolve()): path.stat().st_size
            for path in args.retain_captures.iterdir() if path.is_file()}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": report["passed"], "report": str(args.report),
                      "failures": report.get("failures", []), "error": report.get("error")}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    sys.exit(main())
