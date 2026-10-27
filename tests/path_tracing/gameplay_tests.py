#!/usr/bin/env python3
"""Check actual prepared MCC gameplay in both renderer modes.

Run through tools/run_test.py. Frames, PNG previews, logs and private state stay
inside HALO2_TEST_SCRATCH until its owner cleans up, allowing an enclosing
coordinator to inspect the printed images before returning. This checks actual
frame submission; it makes no visual parity or frame budget claim.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import signal
import struct
import subprocess
import tempfile
import time
import zlib


CASES = {"lockout1": ("lockout", 1), "coagulation1": ("coagulation", 1),
         "coagulation2": ("coagulation", 2)}
RECEIPT_PREFIX = "Path Tracing frame: "
ACTIVE_PROCESS = None


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def ppm(path):
    data = path.read_bytes()
    match = re.match(rb"P6\s+(\d+)\s+(\d+)\s+255\s", data)
    if not match:
        raise ValueError("Native screenshot is not a P6 RGB image")
    width, height = map(int, match.groups())
    pixels = data[match.end():]
    if not width or not height or len(pixels) != width * height * 3:
        raise ValueError("Native screenshot has an invalid RGB payload size")
    return width, height, pixels


def png(path, width, height, pixels):
    """Losslessly encode the actual RGB capture for inspection using stdlib."""
    def chunk(kind, data):
        return (struct.pack(">I", len(data)) + kind + data +
                struct.pack(">I", zlib.crc32(kind + data)))

    rows = b"".join(b"\0" + pixels[y * width * 3:(y + 1) * width * 3]
                    for y in range(height))
    path.write_bytes(b"\x89PNG\r\n\x1a\n" +
                     chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
                     chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def stop(process):
    """Stop only this case's process group, including a private Xvfb server."""
    try:
        os.killpg(process.pid, signal.SIGTERM)
    except ProcessLookupError:
        pass
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        pass
    try:
        os.killpg(process.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    process.wait()


def interrupted(signum, _frame):
    if ACTIVE_PROCESS is not None:
        stop(ACTIVE_PROCESS)
    raise SystemExit(128 + signum)


def map_directory(root):
    for path in (root, root / "halo2/h2_maps_win64_dx11", root / "h2_maps_win64_dx11"):
        if (path / "shared.map").is_file():
            return path
    raise ValueError("MCC root has no prepared h2_maps_win64_dx11/shared.map")


def validation_environment():
    if os.environ.get("VK_LAYER_PATH"):
        return {}, {"source": "existing environment", "layer_path": os.environ["VK_LAYER_PATH"]}
    bundle = Path(__file__).resolve().parents[2] / ".tools/validation"
    layers, libraries = bundle / "usr/share/vulkan/explicit_layer.d", bundle / "usr/lib"
    if layers.is_dir() and libraries.is_dir():
        env = {"VK_LAYER_PATH": str(layers), "LD_LIBRARY_PATH": str(libraries) +
               (os.pathsep + os.environ["LD_LIBRARY_PATH"] if os.environ.get("LD_LIBRARY_PATH") else "")}
        return env, {"source": "existing repository bundle", "layer_path": str(layers)}
    return {}, {"source": "system Vulkan loader discovery"}


def inspect_log(text, mode, players, args, row):
    errors = row["errors"]
    summaries = re.findall(r"^Multiplayer frames=(\d+) ticks=(\d+) players=(\d+)\b", text, re.MULTILINE)
    if len(summaries) != 1:
        errors.append("Expected exactly one actual multiplayer frame summary")
    else:
        frames, ticks, connected = map(int, summaries[0])
        row.update(rendered_frames=frames, source_tick=ticks, connected_players=connected)
        if (args.capture_tick is None and frames != args.frames) or connected != players or frames <= 0:
            errors.append("Actual rendered frames or local players differ from the request")
        if ticks <= 0:
            errors.append("Actual source gameplay clock did not advance")
        if args.capture_tick is not None and ticks != args.capture_tick:
            errors.append("Actual source gameplay clock did not reach the requested capture tick")
    if args.capture_tick is not None:
        captures = re.findall(r"^Input capture tick=(\d+) .* capture_tick_cadence=(\d+)\b", text, re.MULTILINE)
        if captures != [(str(args.capture_tick), "1")]:
            errors.append("Actual finite source tick/cadence capture receipt is missing")
    validation = [line for line in text.splitlines()
                  if "Validation Error" in line or "VUID-" in line]
    row["validation_errors"] = len(validation)
    if validation:
        errors.append("Vulkan validation reported errors")
        row["validation_error_tail"] = [line[-2048:] for line in validation[-8:]]
    receipts = [line[len(RECEIPT_PREFIX):] for line in text.splitlines()
                if line.startswith(RECEIPT_PREFIX)]
    if mode == "Original":
        if receipts:
            errors.append("Original mode emitted a Path Tracing frame receipt")
        return
    if len(receipts) != 1:
        errors.append("Expected exactly one completed Path Tracing frame receipt")
        return
    try:
        receipt = json.loads(receipts[0])
    except (ValueError, TypeError) as error:
        errors.append(f"Path Tracing receipt is invalid JSON: {error}")
        return
    row["path_tracing_frame"] = receipt
    if not isinstance(receipt, dict):
        errors.append("Path Tracing frame receipt is not an object")
        return
    if receipt.get("completed") is not True:
        errors.append("Path Tracing frame did not complete")
    samples, bounces = receipt.get("samples_per_pixel"), receipt.get("max_bounces")
    query_bound = receipt.get("max_ray_queries_per_fragment")
    if (type(samples) is not int or not 1 <= samples <= 2 or
            type(bounces) is not int or not 1 <= bounces <= 2):
        errors.append("Path Tracing frame has an invalid sample or bounce budget")
    elif type(query_bound) is not int or query_bound != samples * (1 + 2 * bounces):
        errors.append("Path Tracing frame has an invalid per-fragment ray-query bound")
    for field in ("views", "opaque_draws", "triangles", "gpu_bytes"):
        value = receipt.get(field)
        if type(value) is not int or value <= 0:
            errors.append(f"Path Tracing frame has no positive {field}")
    energy = 0
    for field in ("lights", "emissive_triangles"):
        value = receipt.get(field)
        if type(value) is not int or value < 0:
            errors.append(f"Path Tracing frame has invalid {field}")
        else:
            energy += value
    environment = receipt.get("source_environment")
    if environment is not None:
        if (not isinstance(environment, list) or len(environment) != 3 or
                any(type(value) not in (int, float) or not math.isfinite(value) or value < 0
                    for value in environment)):
            errors.append("Path Tracing source environment is invalid")
        else:
            energy += sum(environment)
    if energy <= 0:
        errors.append("Path Tracing frame has no actual source lights, emissive triangles or environment energy")
    if (receipt.get("width"), receipt.get("height")) != (args.width, args.height):
        errors.append("Path Tracing receipt dimensions differ from the request")
    views = receipt.get("view_counts")
    if (not isinstance(views, list) or len(views) != players or
            receipt.get("views") != players):
        errors.append("Path Tracing did not retain every requested local view")
    elif any(not isinstance(view, dict) or type(view.get("opaque_draws")) is not int or
             view["opaque_draws"] <= 0 for view in views):
        errors.append("A Path Tracing local view submitted no opaque draws")


def run_case(name, mode, maps, work, args, validation_env):
    global ACTIVE_PROCESS
    map_name, players = CASES[name]
    owned = work / f"{name}-{mode}"
    owned.mkdir()
    for directory in ("config", "data", "cache", "tmp", "pipeline", "mesa", "nvidia", "savegame"):
        (owned / directory).mkdir()
    frame, preview, log = (owned / name for name in ("frame.ppm", "frame.png", "native.log"))
    row = {"case": name, "mode": mode, "players": players, "errors": [],
           "raw_frame": str(frame), "preview": str(preview), "log": str(log)}
    map_path = maps / f"{map_name}.map"
    if not map_path.is_file():
        row["errors"].append(f"Prepared map is missing: {map_path}")
        return row
    row["map"] = str(map_path)
    row["map_sha256"] = digest(map_path)
    with map_path.open("rb") as stream:
        if stream.read(8) != struct.pack("<II", 0x68656164, 13):
            row["errors"].append("Actual gameplay regression requires an MCC version13 cache")
            return row
    env = {key: value for key, value in os.environ.items() if not key.startswith("HALO2_")}
    env.update(HALO2_TEST_SCRATCH=os.environ["HALO2_TEST_SCRATCH"], SDL_AUDIODRIVER="dummy",
               XDG_CONFIG_HOME=str(owned / "config"), XDG_DATA_HOME=str(owned / "data"),
               XDG_CACHE_HOME=str(owned / "cache"), TMPDIR=str(owned / "tmp"),
               TMP=str(owned / "tmp"), TEMP=str(owned / "tmp"),
               HALO2_PIPELINE_CACHE_DIRECTORY=str(owned / "pipeline"),
               MESA_SHADER_CACHE_DIR=str(owned / "mesa"),
               __GL_SHADER_DISK_CACHE_PATH=str(owned / "nvidia"), **validation_env)
    command = [str(args.runtime), "--multiplayer", "--map", str(map_path), "--shared-maps", str(maps),
               "--local-players", str(players), "--mp-source-object-world", str(owned / "savegame"),
               "--viewport", str(args.width), str(args.height), "--render-size", str(args.width),
               str(args.height), "--graphics-mode", mode,
               "--mute", "--hidden", "--validation", "--screenshot", str(frame)]
    if args.capture_tick is None:
        command.extend(("--frames", str(args.frames)))
    else:
        neutral = owned / "neutral-input.tsv"
        neutral.write_text("# No scripted actions: retain the actual local player spawn and gameplay.\n")
        row["input_script_sha256"] = digest(neutral)
        command.extend(("--mp-input-script", str(neutral), "--mp-capture-tick", str(args.capture_tick),
                        "--mp-reference-tick-cadence"))
    if not os.environ.get("DISPLAY"):
        if not shutil.which("xvfb-run"):
            row["errors"].append("No DISPLAY or xvfb-run is available for actual native rendering")
            return row
        command = ["xvfb-run", "-a", "-s",
                   f"-screen 0 {args.width}x{args.height}x24 -nolisten tcp", *command]
        env["SDL_VIDEODRIVER"] = "x11"
    row["command"] = command
    started = time.monotonic()
    try:
        with log.open("wb") as output:
            process = subprocess.Popen(command, env=env, stdout=output, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            ACTIVE_PROCESS = process
            try:
                row["exit_code"] = process.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                stop(process)
                row.update(exit_code=124, timed_out=True)
            except BaseException:
                stop(process)
                raise
            finally:
                ACTIVE_PROCESS = None
    except OSError as error:
        row["errors"].append(f"Actual native process could not start: {error}")
        row["exit_code"] = None
    row["seconds"] = round(time.monotonic() - started, 3)
    if row.get("exit_code") != 0:
        row["errors"].append(f"Actual native process exited {row.get('exit_code')}")
    text = log.read_text(errors="replace") if log.exists() else ""
    row["log_tail"] = text[-8192:]
    inspect_log(text, mode, players, args, row)
    try:
        width, height, pixels = ppm(frame)
        row["frame_sha256"] = digest(frame)
        row["image_width"], row["image_height"] = width, height
        if (width, height) != (args.width, args.height):
            row["errors"].append("Native image dimensions differ from the request")
        nonuniform = any(pixels[at:at + 3] != pixels[:3] for at in range(3, len(pixels), 3))
        row["image_nonuniform"] = nonuniform
        if not nonuniform:
            row["errors"].append("Native screenshot is a uniform image")
        png(preview, width, height, pixels)
        row["preview_sha256"] = digest(preview)
        print(f"Gameplay capture {name} {mode}: {preview}", flush=True)
    except (OSError, ValueError) as error:
        row["errors"].append(f"Actual native framebuffer missing or invalid: {error}")
    row["passed"] = not row["errors"]
    print(f"Gameplay case {name} {mode}: {'PASS' if row['passed'] else 'FAIL'} "
          f"({row['seconds']:.3f}s)", flush=True)
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--mcc-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, help="Optional small JSON report outside scratch")
    limit = parser.add_mutually_exclusive_group()
    limit.add_argument("--frames", type=int, default=8)
    limit.add_argument("--capture-tick", type=int,
                       help="Optional actual source tick capture with neutral input and one frame per source tick")
    parser.add_argument("--timeout", type=float, default=180)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--height", type=int, default=480)
    parser.add_argument("--cases", nargs="+", choices=CASES, default=list(CASES))
    args = parser.parse_args()
    if not os.environ.get("HALO2_TEST_SCRATCH"):
        parser.error("Run this helper through tools/run_test.py")
    if not 1 <= args.frames <= 10000:
        parser.error("Frames must be between 1 and 10000")
    if args.capture_tick is not None and not 1 <= args.capture_tick <= 1000000:
        parser.error("Capture tick must be between 1 and 1000000")
    if not (64 <= args.width <= 4096 and 64 <= args.height <= 4096):
        parser.error("Width and height must each be between 64 and 4096")
    if not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("Timeout must be finite and positive")
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    args.runtime, args.mcc_root = args.runtime.resolve(), args.mcc_root.resolve()
    if not args.runtime.is_file() or not os.access(args.runtime, os.X_OK):
        parser.error("Runtime must be an existing executable; this helper does not build or select one")
    try:
        maps = map_directory(args.mcc_root)
    except ValueError as error:
        parser.error(str(error))
    work = Path(tempfile.mkdtemp(prefix="path-tracing-gameplay-", dir=scratch))
    output = args.output.resolve() if args.output else work / "report.json"
    validation_env, validation = validation_environment()
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    report = {"passed": False, "scope": "Actual offline prepared MCC gameplay and native frame submission; "
              "no visual parity or frame budget claim", "runtime": str(args.runtime),
              "runtime_sha256": digest(args.runtime), "helper_sha256": digest(Path(__file__)),
              "mcc_root": str(args.mcc_root), "maps": str(maps), "scratch": str(work),
              "requested_frames": args.frames if args.capture_tick is None else None,
              "capture_tick": args.capture_tick, "reference_tick_cadence": args.capture_tick is not None,
              "width": args.width, "height": args.height,
              "timeout_seconds": args.timeout, "validation": validation, "cases": [],
              "artifacts": "Raw frames, PNG previews, logs and private state remain under the enclosing "
              "HALO2_TEST_SCRATCH owner until its cleanup; no captures copied outside scratch"}
    started = time.monotonic()
    for name in args.cases:
        for mode in ("Original", "PathTracing"):
            try:
                row = run_case(name, mode, maps, work, args, validation_env)
            except Exception as error:
                row = {"case": name, "mode": mode, "passed": False,
                       "errors": [f"Case could not complete: {type(error).__name__}: {error}"]}
            report["cases"].append(row)
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(json.dumps(report, indent=2) + "\n")
    report["seconds"] = round(time.monotonic() - started, 3)
    report["runtime_sha256_after"] = digest(args.runtime)
    report["runtime_unchanged"] = report["runtime_sha256_after"] == report["runtime_sha256"]
    report["passed"] = report["runtime_unchanged"] and all(row.get("passed") for row in report["cases"])
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Gameplay report: {output}", flush=True)
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
