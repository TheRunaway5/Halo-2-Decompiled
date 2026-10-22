#!/usr/bin/env python3
"""Run visible real-match physics acceptance and retain unaltered window video.

The native runner drives normal SDL input. Physics is never replaced with
recorded state. Red/unavailable features remain nonzero results in the report.
Runtime assets come only from the selected prepared MCC cache. English Xbox
TU1.5 source/execution remains the offline behavioral reference.
"""
import argparse
import ctypes
import ctypes.util
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import threading
import time

from physics_test_cache import runtime_cache, runtime_map, retain_results

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def capture_window_state(display_name, window):
    class Attributes(ctypes.Structure):
        _fields_ = [(name, ctypes.c_int) for name in ('x','y','width','height','border','depth')] + [
            ('visual',ctypes.c_void_p),('root',ctypes.c_ulong),
            *[(name,ctypes.c_int) for name in ('window_class','bit_gravity','win_gravity','backing_store')],
            ('backing_planes',ctypes.c_ulong),('backing_pixel',ctypes.c_ulong),
            ('save_under',ctypes.c_int),('colormap',ctypes.c_ulong),
            ('map_installed',ctypes.c_int),('map_state',ctypes.c_int),
            ('all_event_masks',ctypes.c_long),('your_event_mask',ctypes.c_long),
            ('do_not_propagate_mask',ctypes.c_long),('override_redirect',ctypes.c_int),
            ('screen',ctypes.c_void_p)]
    library=ctypes.CDLL(ctypes.util.find_library('X11'))
    library.XOpenDisplay.argtypes=[ctypes.c_char_p];library.XOpenDisplay.restype=ctypes.c_void_p
    library.XGetWindowAttributes.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.POINTER(Attributes)]
    library.XTranslateCoordinates.argtypes=[ctypes.c_void_p,ctypes.c_ulong,ctypes.c_ulong,
        ctypes.c_int,ctypes.c_int,ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_int),ctypes.POINTER(ctypes.c_ulong)]
    library.XCloseDisplay.argtypes=[ctypes.c_void_p]
    display=library.XOpenDisplay(display_name.encode())
    if not display:raise RuntimeError('Could not inspect the actual recording display')
    try:
        attributes=Attributes();root=Attributes()
        if not library.XGetWindowAttributes(display,window,ctypes.byref(attributes)):
            raise RuntimeError('Could not inspect the actual recording window')
        library.XGetWindowAttributes(display,attributes.root,ctypes.byref(root))
        x=ctypes.c_int();y=ctypes.c_int();child=ctypes.c_ulong()
        library.XTranslateCoordinates(display,window,attributes.root,0,0,ctypes.byref(x),ctypes.byref(y),ctypes.byref(child))
        return dict(window=window,x=x.value,y=y.value,width=attributes.width,height=attributes.height,
            map_state=attributes.map_state,window_class=attributes.window_class,depth=attributes.depth,
            root_width=root.width,root_height=root.height)
    finally:library.XCloseDisplay(display)


def inspect_mirror(args):
    if not args.visible_mirror_display:
        return None
    if not args.mirror_process_info:
        raise RuntimeError("Isolated visible acceptance requires --mirror-process-info")
    info = json.loads(args.mirror_process_info.read_text())
    if info["display"] != os.environ["DISPLAY"] or info["mirror_display"] != args.visible_mirror_display:
        raise RuntimeError("Mirror source/destination does not match the actual displays")
    capture = Path(f"/proc/{int(info['capture_pid'])}")
    mirror = Path(f"/proc/{int(info['mirror_pid'])}")
    for label, process in (("capture", capture), ("player", mirror)):
        state = process.joinpath("stat").read_text().rpartition(") ")[2].split()[0]
        if state in {"T", "t", "Z", "X"}:
            raise RuntimeError(f"Mirror {label} pid={process.name} state={state} is stopped or terminated")
    capture_command = capture.joinpath("cmdline").read_bytes().split(b"\0")
    mirror_command = mirror.joinpath("cmdline").read_bytes().split(b"\0")
    if not capture_command or Path(os.fsdecode(capture_command[0])).name != "ffmpeg" or b"x11grab" not in capture_command:
        raise RuntimeError("Mirror has no actual X11 capture producer")
    if b"-i" not in capture_command or os.fsencode(info["display"]) != capture_command[capture_command.index(b"-i") + 1]:
        raise RuntimeError("Mirror captures a different display")
    if any(flag in capture_command for flag in (b"-vf", b"-filter_complex", b"-filter:v")):
        raise RuntimeError("Visible proof requires an unaltered capture stream")
    if not mirror_command or Path(os.fsdecode(mirror_command[0])).name != "ffplay":
        raise RuntimeError("Mirror player is not running")
    pipe = os.readlink(capture / "fd/1")
    if not pipe.startswith("pipe:[") or pipe != os.readlink(mirror / "fd/0"):
        raise RuntimeError("Capture producer is not connected to the mirror player")
    environment = dict(item.split(b"=", 1) for item in mirror.joinpath("environ").read_bytes().split(b"\0") if b"=" in item)
    if environment.get(b"DISPLAY") != os.fsencode(args.visible_mirror_display):
        raise RuntimeError("Mirror player uses a different desktop")
    env = {**os.environ, "DISPLAY": args.visible_mirror_display}
    clients = subprocess.run(["xprop", "-root", "_NET_CLIENT_LIST"], env=env,
        capture_output=True, text=True, check=True).stdout
    for identifier in re.findall(r"0x[0-9a-fA-F]+", clients):
        try:
            owner = subprocess.run(["xprop", "-id", identifier, "_NET_WM_PID"], env=env,
                capture_output=True, text=True, check=True).stdout
        except subprocess.CalledProcessError as error:
            # EWMH enumeration and per-window queries are separate requests;
            # unrelated clients may disappear between them. The owned mirror
            # must still be found and verified below.
            if "BadWindow" in (error.stderr or ""):
                continue
            raise
        if re.search(r"=\s*" + str(info["mirror_pid"]) + r"\s*$", owner):
            window = capture_window_state(args.visible_mirror_display, int(identifier, 16))
            if window["map_state"] != 2 or window["width"] <= 0 or window["height"] <= 0:
                raise RuntimeError("Mirror desktop window is not viewable")
            return {"capture_pid": info["capture_pid"], "mirror_pid": info["mirror_pid"],
                "pipe": pipe, "window": window}
    raise RuntimeError("Mirror player has no mapped desktop window")


def enforce_runtime_deadline(process, complete, seconds, expired):
    if complete.wait(seconds) or process.poll() is not None:
        return
    expired.append(f"Native physics acceptance exceeded {seconds:g} seconds")
    process.terminate()
    try:
        process.wait(timeout=10)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait(timeout=10)


def run(args, scenario):
    out = args.output / scenario
    out.mkdir(parents=True, exist_ok=False)
    map_path = runtime_map(args.maps, args.map or args.maps / ("coagulation.map" if scenario in ("vehicle", "lan-vehicle") else "lockout.map"))
    command = [str(args.runtime), str(map_path), str(args.maps), "native-mcc",
               str(args.mcc_root), str(out), scenario]
    env = os.environ.copy()
    env["SDL_VIDEODRIVER"] = "x11"  # This visible window is the recorded X11 surface.
    started = time.monotonic()
    record = None
    window = None
    record_command = None
    record_log = None
    runtime_before = digest(args.runtime)
    map_before = digest(map_path)
    runtime_executed = runtime_before
    mirror_checks = []
    mirror_errors = []
    mirror_error_observations = []
    mirror_stop = threading.Event()
    timeout_errors = []
    if args.visible_mirror_display:
        mirror_checks.append(inspect_mirror(args))
    with (out / "runtime.log").open("w") as log:
        process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True, bufsize=1)
        deadline = threading.Thread(target=enforce_runtime_deadline,
            args=(process, mirror_stop, args.timeout_seconds, timeout_errors), daemon=True)
        deadline.start()
        def monitor_mirror():
            while not mirror_stop.wait(.5):
                try:
                    observed = inspect_mirror(args)
                    observed["elapsed_seconds"] = time.monotonic() - started
                    mirror_checks.append(observed)
                except Exception as error:
                    mirror_errors.append(str(error))
                    mirror_error_observations.append({"elapsed_seconds": time.monotonic() - started,
                        "native_pid": process.pid, "native_exit_code_at_observation": process.poll(),
                        "error": str(error)})
                    if process.poll() is None:
                        process.terminate()
                    return
        monitor = None
        if args.visible_mirror_display:
            monitor = threading.Thread(target=monitor_mirror, daemon=True)
            monitor.start()
        try:
            # A shared build can replace the path while this process runs.
            # Linux retains the actually executed inode through /proc.
            runtime_executed = digest(Path(f"/proc/{process.pid}/exe"))
        except FileNotFoundError:
            pass  # A startup failure can exit before its executable is read.
        try:
            for line in process.stdout:
                log.write(line)
                log.flush()
                match = re.match(r"acceptance_x11_window=(\d+)", line)
                if match:
                    window = int(match[1])
                if line.startswith("acceptance_first_frame=") and window and args.record:
                    (out / "capture-window.json").write_text(json.dumps(capture_window_state(env["DISPLAY"],window),indent=2)+"\n")
                    record_command = ["ffmpeg", "-nostdin", "-hide_banner", "-loglevel", "warning",
                        "-f", "x11grab", "-framerate", "30", "-window_id", str(window),
                        "-draw_mouse", "0", "-i", env["DISPLAY"], "-an", "-c:v", "libx264",
                        "-preset", "ultrafast", "-crf", "18", "-pix_fmt", "yuv420p",
                        "-movflags", "+faststart", str(out / "window.mp4")]
                    record_log = (out / "recorder.log").open("w")
                    record = subprocess.Popen(record_command, stdout=record_log, stderr=subprocess.STDOUT)
                if line.startswith(("PASS visible", "RED visible")):
                    print(line.rstrip(), flush=True)
            exit_code = process.wait(timeout=15)
        finally:
            mirror_stop.set()
            deadline.join(timeout=15)
            if monitor is not None:
                monitor.join(timeout=5)
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
            if record is not None:
                # A destroyed native window can close x11grab first. Preserve
                # that exit code and independently validate the saved stream.
                if record.poll() is None:
                    record.send_signal(2)
                record.wait(timeout=10)
                record_log.close()
    runtime_after = digest(args.runtime)
    manifest = {"command": command, "exit_code": exit_code,
                "elapsed_seconds": time.monotonic() - started,
                "timeout_seconds": args.timeout_seconds, "timeout_errors": timeout_errors,
                "runtime_sha256": runtime_executed,
                "runtime_sha256_before": runtime_before, "runtime_sha256_after": runtime_after,
                "runtime_stable": runtime_before == runtime_executed == runtime_after,
                "native_display": env.get("DISPLAY"),
                "visible_mirror_display": args.visible_mirror_display,
                "mirror_checks": mirror_checks, "mirror_errors": mirror_errors,
                "mirror_error_observations": mirror_error_observations,
                "mirror_continuously_verified": bool(mirror_checks) and not mirror_errors,
                "runtime_cache_source": "MCC", "runtime_cache_inputs": args.cache_inputs,
                "reference": "English Xbox TU1.5 offline behavior and visual authority",
                "runtime_xbe": None, "xbox_vista_runtime_fallback_allowed": False,
                "map_sha256": digest(map_path), "map_sha256_before": map_before,
                "map_stable": map_before == digest(map_path), "visible_x11_window": window,
                "record_command": record_command, "record_exit_code": record.returncode if record else None}
    if (mirror_errors or timeout_errors) and exit_code == 0:
        manifest["native_exit_code"] = exit_code
        exit_code = manifest["exit_code"] = 1
    if not manifest["map_stable"] and exit_code == 0:
        manifest["input_stability_error"] = "Prepared MCC map changed during the actual runtime test"
        exit_code = manifest["exit_code"] = 1
    # Preserve the actual native result even if the window closed before the
    # recorder obtained its first frame or a later media inspection fails.
    (out / "execution.json").write_text(json.dumps(manifest, indent=2) + "\n")
    ppm = out / "frame.ppm"
    if ppm.exists():
        # Lossless format conversion for the app preview; no pixel edits.
        subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(ppm),
            "-frames:v", "1", str(out / "frame.png")], check=True)
        def pixels(path):
            return subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(path),
                "-frames:v", "1", "-pix_fmt", "rgb24", "-f", "rawvideo", "pipe:1"],
                capture_output=True, check=True).stdout
        if pixels(ppm) != pixels(out / "frame.png"):
            raise RuntimeError("Screenshot conversion changed pixels")
    video = out / "window.mp4"
    if video.exists():
        probe = subprocess.run(["ffprobe", "-v", "error", "-count_frames", "-select_streams", "v:0",
            "-show_entries", "stream=width,height,nb_read_frames,duration", "-of", "json", str(video)],
            capture_output=True, text=True, check=True)
        manifest["video_probe"] = json.loads(probe.stdout)
        manifest["video_sha256"] = digest(video)
        streams = manifest["video_probe"]["streams"]
        if not streams or int(streams[0].get("nb_read_frames", 0)) == 0:
            manifest["recording_error"] = "No real window frames saved"
        else:
            # A native frame from the recording, without overlays or interpolation.
            sample_index=min(60,int(streams[0]["nb_read_frames"])-1)
            manifest["video_sample_frame"]=sample_index
            subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(video),
                "-vf", f"select=eq(n\\,{sample_index})", "-frames:v", "1", str(out / "video-sample.png")], check=True)
    elif args.record:
        manifest["recording_error"] = "No video saved from the visible native window"
    if manifest.get("recording_error") and exit_code == 0:
        manifest["native_exit_code"] = exit_code
        exit_code = manifest["exit_code"] = 1
    if not manifest["runtime_stable"] and exit_code == 0:
        manifest["runtime_stability_error"] = "Runtime replaced during visible acceptance; rerun the built artifact"
        manifest["native_exit_code"] = exit_code
        exit_code = manifest["exit_code"] = 1
    (out / "execution.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return {"scenario": scenario, "exit_code": exit_code,
            "report": str(out / "report.json"), "execution": str(out / "execution.json")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--mcc-root", type=Path, required=True, help="Prepared imported MCC cache; no Xbox/Vista runtime fallback")
    parser.add_argument("--maps", type=Path, help="Optional exact maps directory inside the selected MCC cache")
    parser.add_argument("--map", type=Path, help="Explicit map inside the selected prepared MCC cache")
    parser.add_argument("--report-output", type=Path, help="Keep bounded reports outside disposable scratch")
    parser.add_argument("--retain-video", action="store_true", help="Explicitly retain the unaltered recordings and selected frames with reports")
    parser.add_argument("--scenario", action="append", choices=["movement", "objects", "lift", "vehicle", "lan-movement", "lan-lift", "lan-vehicle", "lan-objects", "lan-campaign"])
    parser.add_argument("--record", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--timeout-seconds", type=float, default=180,
                        help="Hard per-case native deadline; expiry is a failed result")
    parser.add_argument("--visible-mirror-display", help="Desktop displaying an unaltered live mirror when input is isolated")
    parser.add_argument("--mirror-process-info", type=Path, help="Owned live capture/player process metadata")
    args = parser.parse_args()
    if not 0 < args.timeout_seconds < 86400:
        parser.error("--timeout-seconds must be positive and less than one day")
    if os.environ.get("HALO2_TU15_OBJECT_FIELD_TARGET"):
        required = ("HALO2_TU15_PHANTOM_TRACE", "HALO2_TU15_PHANTOM_OBJECT_TRACE",
                    "HALO2_TU15_LIFT_DEFINITION")
        missing = [name for name in required if not os.environ.get(name)]
        if missing:
            parser.error("Object-field acceptance requires its actual field/member observations: "
                         + ", ".join(missing))
    for name in ("runtime", "output", "mcc_root"):
        setattr(args, name, getattr(args, name).resolve())
    args.maps, args.cache_inputs = runtime_cache(args.mcc_root, args.maps)
    if args.map:
        args.map = runtime_map(args.maps, args.map)
    owned = os.environ.get("HALO2_TEST_SCRATCH")
    if not owned or not args.output.is_relative_to(Path(owned).resolve()):
        raise SystemExit("Run physics validation through tools/run_test.py with --output under HALO2_TEST_SCRATCH")
    if args.retain_video and not args.report_output:
        parser.error("--retain-video requires --report-output")
    if not os.environ.get("DISPLAY"):
        raise SystemExit("A native DISPLAY is required; visible proof needs a desktop window or an unaltered live desktop mirror")
    if args.record and (not shutil.which("ffmpeg") or not shutil.which("ffprobe")):
        raise SystemExit("ffmpeg and ffprobe are required to retain visible-window evidence")
    args.output.mkdir(parents=True, exist_ok=True)
    # Independent live suites must not inject input into the same X11 display
    # together. Hold this lock through native shutdown and recording cleanup.
    display_key=hashlib.sha256(os.environ["DISPLAY"].encode()).hexdigest()[:16]
    lock_path=Path(tempfile.gettempdir())/f"halo2-tu15-visible-{os.getuid()}-{display_key}.lock"
    try:
        with lock_path.open("a") as display_lock:
            fcntl.flock(display_lock,fcntl.LOCK_EX)
            results = [run(args, name) for name in (args.scenario or ["movement", "objects", "lift", "vehicle", "lan-movement"])]
        (args.output / "suite.json").write_text(json.dumps({"target": "English TU1.5", "runtime_cache_inputs": args.cache_inputs,
            "results": results, "full_physics_parity": False}, indent=2) + "\n")
    finally:
        if args.report_output:
            retain_results(args.output, args.report_output, video=args.retain_video)
    raise SystemExit(0 if all(row["exit_code"] == 0 for row in results) else 2)


if __name__ == "__main__":
    main()
