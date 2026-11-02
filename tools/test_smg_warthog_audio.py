#!/usr/bin/env python3
"""Exercise unmuted authored SMG damage against a normally spawned Warthog.

The linked native fixture delivers ordinary SDL controls and observes completed
source state. This wrapper owns Xvfb, private preferences/caches and the finite
capture. Only bounded reports and a log tail leave its temporary directory.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time

from weapon_test_cache import require_mcc_map


EXACT_ERROR = "TU1.5 live world owner missing at 18fa45 -> 17e160 return 18fa4a"
SHADOW_FIELDS = ("world_native_shadow_generate_prepared_draws",
                 "world_native_shadow_apply_prepared_draws")


def sha256(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def require(value, message):
    if not value:
        raise RuntimeError(message)


def image_receipt(path):
    with path.open("rb") as stream:
        require(stream.readline() == b"P6\n", "Native capture lacks its P6 header")
        require(stream.readline() == b"640 480\n", "Native capture lost its requested drawable")
        require(stream.readline() == b"255\n", "Native capture has an unsupported channel extent")
        header = stream.tell()
    require(path.stat().st_size == header + 640 * 480 * 3,
            "Native capture does not contain a complete rendered surface")
    return dict(bytes=path.stat().st_size, sha256=sha256(path), width=640, height=480)


def exercise(args, scratch, report):
    native_report, log = scratch / "native.json", scratch / "native.log"
    timings, image = scratch / "renderer.tsv", scratch / "completed.ppm"
    command = ["xvfb-run", "-a", "-s", "-screen 0 800x600x24", str(args.fixture),
               "--map", str(args.map), "--shared-maps", str(args.shared_maps),
               "--shader-directory", str(args.shader_directory),
               "--report-output", str(native_report), "--capture-tick", str(args.capture_tick),
               "--screenshot", str(image)]
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith(("HALO2_", "H2_"))}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), SDL_VIDEODRIVER="x11",
                       SDL_AUDIODRIVER="dummy", HALO2_RENDER_TIMINGS=str(timings))
    for key, name in (("XDG_CONFIG_HOME", "config"), ("XDG_DATA_HOME", "data"),
                      ("XDG_CACHE_HOME", "cache"), ("HALO2_PIPELINE_CACHE_DIRECTORY", "pipelines"),
                      ("MESA_SHADER_CACHE_DIR", "mesa"), ("__GL_SHADER_DISK_CACHE_PATH", "nvidia")):
        environment[key] = str(scratch / name)
    report.update(command=command, mute=False, audio_driver="dummy")
    started = time.monotonic()
    process = None
    try:
        with log.open("wb") as stream:
            process = subprocess.Popen(command, cwd=args.root, env=environment,
                                       stdout=stream, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            try:
                report["exit_code"] = process.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                report["exit_code"] = "timeout"
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait()
    finally:
        if process is not None and process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait()
        report["elapsed_seconds"] = round(time.monotonic() - started, 3)
        output = log.read_text(errors="replace") if log.exists() else ""
        retained_log = args.report_output.with_suffix(".log")
        retained_log.write_text(output[-131072:])
        report.update(exact_audio_owner_error=EXACT_ERROR in output,
                      log_bytes=log.stat().st_size if log.exists() else 0,
                      log_sha256=sha256(log) if log.exists() else None,
                      retained_log=dict(path=str(retained_log), bytes=retained_log.stat().st_size,
                                        sha256=sha256(retained_log), tail_truncated=len(output) > 131072))
        if native_report.is_file():
            require(not native_report.is_symlink() and native_report.stat().st_size <= 524288,
                    "Native runtime receipt exceeds its bound")
            report["native"] = json.loads(native_report.read_text())
        delivered = scratch / "delivered-inputs.tsv"
        if delivered.is_file():
            require(not delivered.is_symlink() and 0 < delivered.stat().st_size <= 1048576,
                    "Delivered native input schedule exceeds its bound")
            rows = delivered.read_bytes()
            retained_inputs = args.report_output.with_suffix(".inputs.tsv")
            retained_inputs.write_bytes(rows)
            report["delivered_inputs"] = dict(path=str(retained_inputs), bytes=len(rows),
                sha256=sha256(retained_inputs), rows=rows.count(b"\n"),
                scope="Required regression reference: actual delivered ordinary control values; no gameplay result supplied")
    require(not report["exact_audio_owner_error"], EXACT_ERROR)
    require(report.get("exit_code") == 0, f"Native SMG case did not complete: {report.get('exit_code')}")
    native = report.get("native", {})
    require(isinstance(native, dict) and native.get("passed") is True,
            "Native SMG case did not satisfy its measured gameplay and audio gates")
    require(native.get("runtime_cache_source") == "MCC" and native.get("graphics_mode") == "Original",
            "Native SMG case lost its MCC/Original runtime inputs")
    require(report.get("delivered_inputs", {}).get("rows", 0) > 0,
            "Native SMG case lacks its actual delivered input schedule")
    delivered = native.get("delivered_inputs", {})
    birth_tick = native.get("birth_inventory", {}).get("tick")
    expected_rows = args.capture_tick - birth_tick if type(birth_tick) is int else -1
    require(delivered.get("closed") is True and delivered.get("io_error") is False and
            delivered.get("rows") == delivered.get("parser_roundtrip_rows") ==
            report["delivered_inputs"]["rows"] == expected_rows and
            delivered.get("bytes") == report["delivered_inputs"]["bytes"],
            "Delivered native input schedule is partial or differs from its parsed control values")
    rows = Path(report["delivered_inputs"]["path"]).read_text().splitlines()
    for step, row in enumerate(rows):
        fields = row.split()
        require(len(fields) >= 10 and fields[0] == str(step) and fields[1] == "0" and
                all(value in ("0", "1") for value in fields[2:5]) and fields[7] == "move" and
                len(set(fields[10:])) == len(fields[10:]) and
                all(value in ("jump", "crouch") for value in fields[10:]),
                "Delivered input schedule cannot be replayed by the native CLI parser")
    require(native.get("requested_capture_tick") == args.capture_tick and
            native.get("final_tick") == args.capture_tick and
            native.get("exit_reason") == "captured_tick" and
            type(native.get("submitted_frames")) is int and native["submitted_frames"] > 0 and
            native.get("capture_frame_count") == 1,
            "Native SMG case lacks its exact completed tick and submitted frame")
    require(not any(line.startswith(("error:", "Vulkan validation:")) for line in output.splitlines()),
            "Native SMG case emitted an error or Vulkan validation diagnostic")
    with timings.open() as stream:
        rows = list(csv.DictReader(stream, delimiter="\t"))
    require(rows and all(field in rows[0] for field in SHADOW_FIELDS),
            "Native SMG case lost actual shadow submission counters")
    report["shadow_draws"] = {field: sum(int(row[field]) for row in rows) for field in SHADOW_FIELDS}
    require(all(report["shadow_draws"].values()),
            "Native SMG case did not submit ordinary Generate/Apply shadow draws")
    report["capture"] = image_receipt(image)
    report["passed"] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--engine", type=Path)
    parser.add_argument("--map", type=Path, required=True)
    parser.add_argument("--shared-maps", type=Path, required=True)
    parser.add_argument("--shader-directory", type=Path, required=True)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--report-output", type=Path, required=True)
    parser.add_argument("--capture-tick", type=int, default=1200)
    parser.add_argument("--timeout", type=int, default=420)
    args = parser.parse_args()
    if "HALO2_TEST_SCRATCH" not in os.environ:
        parser.error("Run through tools/run_test.py for owned temporary storage")
    if not 600 <= args.capture_tick <= 1800 or not 1 <= args.timeout <= 600:
        parser.error("Use a capture tick in600..1800 and finite timeout in1..600 seconds")
    args.fixture, args.root = args.fixture.resolve(strict=True), args.root.resolve(strict=True)
    args.map = require_mcc_map(args.map)
    args.shared_maps = args.shared_maps.resolve(strict=True)
    args.shader_directory = args.shader_directory.resolve(strict=True)
    if args.map.parent != args.shared_maps or not args.shader_directory.is_dir():
        parser.error("Use the MCC map's actual shared directory and a compiled shader directory")
    if args.engine:
        args.engine = args.engine.resolve(strict=True)
    args.report_output = args.report_output.resolve()
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    inputs = [args.fixture, args.map] + ([args.engine] if args.engine else [])
    pins = {path: sha256(path) for path in inputs}
    report = dict(passed=False, scope=__doc__, fixture=str(args.fixture),
                  fixture_sha256=pins[args.fixture], map=str(args.map), map_sha256=pins[args.map],
                  shared_maps=str(args.shared_maps), shader_directory=str(args.shader_directory),
                  requested_capture_tick=args.capture_tick, test_sha256=sha256(__file__),
                  runtime_provenance="Linked native gameplay fixture; source/build cohort qualification",
                  retained_large_artifacts=[])
    if args.engine:
        report.update(cohort_engine=str(args.engine), cohort_engine_sha256=pins[args.engine])
    try:
        require(os.access(args.fixture, os.X_OK), "Native SMG fixture is not executable")
        with tempfile.TemporaryDirectory(prefix="smg-warthog-audio-", dir=os.environ["HALO2_TEST_SCRATCH"]) as directory:
            report["scratch"] = directory
            exercise(args, Path(directory), report)
    except Exception as error:
        report.update(passed=False, error=str(error))
    finally:
        report["scratch_removed"] = "scratch" in report and not Path(report["scratch"]).exists()
        report["inputs_unchanged"] = all(path.is_file() and sha256(path) == pin for path, pin in pins.items())
        report["passed"] &= report["scratch_removed"] and report["inputs_unchanged"]
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: report.get(key) for key in
                     ("passed", "exit_code", "elapsed_seconds", "exact_audio_owner_error", "error")}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
