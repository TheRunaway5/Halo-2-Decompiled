#!/usr/bin/env python3
"""Run visible real-match physics acceptance and retain unaltered window video.

The native runner drives normal SDL input. Physics is never replaced with
recorded state. Red/unavailable features remain nonzero results in the report.
The pinned English TU1.5 executable is mandatory, including for LAN authority.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
TU15_SHA = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def run(args, scenario):
    out = args.output / scenario
    out.mkdir(parents=True, exist_ok=False)
    map_path = args.maps / ("coagulation.map" if scenario == "vehicle" else "lockout.map")
    command = [str(args.runtime), str(map_path), str(args.maps), str(args.xbe),
               str(args.ui), str(out), scenario]
    env = os.environ.copy()
    env["SDL_VIDEODRIVER"] = "x11"  # This visible window is the recorded X11 surface.
    started = time.monotonic()
    record = None
    window = None
    record_command = None
    record_log = None
    with (out / "runtime.log").open("w") as log:
        process = subprocess.Popen(command, cwd=ROOT, env=env, stdout=subprocess.PIPE,
                                   stderr=subprocess.STDOUT, text=True, bufsize=1)
        try:
            for line in process.stdout:
                log.write(line)
                log.flush()
                match = re.match(r"acceptance_x11_window=(\d+)", line)
                if match:
                    window = int(match[1])
                if line.startswith("acceptance_first_frame=") and window and args.record:
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
    manifest = {"command": command, "exit_code": exit_code,
                "elapsed_seconds": time.monotonic() - started,
                "runtime_sha256": digest(args.runtime), "tu15_xbe_sha256": digest(args.xbe),
                "map_sha256": digest(map_path), "visible_x11_window": window,
                "record_command": record_command, "record_exit_code": record.returncode if record else None}
    ppm = out / "frame.ppm"
    if ppm.exists():
        # Lossless format conversion for the app preview; no pixel edits.
        from PIL import Image
        with Image.open(ppm) as original:
            original.save(out / "frame.png")
            with Image.open(out / "frame.png") as converted:
                if original.tobytes() != converted.tobytes():
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
            raise RuntimeError(f"No real window frames saved: {out}")
        # A native frame from the recording, without overlays or interpolation.
        subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", str(video),
            "-vf", "select=eq(n\\,60)", "-frames:v", "1", str(out / "video-sample.png")], check=True)
    elif args.record:
        manifest["recording_error"] = "No video saved from the visible native window"
    (out / "execution.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return {"scenario": scenario, "exit_code": exit_code,
            "report": str(out / "report.json"), "execution": str(out / "execution.json")}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--maps", type=Path, default=ROOT / "assets/disc/maps")
    parser.add_argument("--xbe", type=Path, required=True)
    parser.add_argument("--ui", type=Path, required=True)
    parser.add_argument("--scenario", action="append", choices=["movement", "objects", "lift", "vehicle", "lan-movement"])
    parser.add_argument("--record", action=argparse.BooleanOptionalAction, default=True)
    args = parser.parse_args()
    for name in ("runtime", "output", "maps", "xbe", "ui"):
        setattr(args, name, getattr(args, name).resolve())
    if digest(args.xbe) != TU15_SHA:
        raise SystemExit("Acceptance requires the pinned English TU1.5 executable")
    if not os.environ.get("DISPLAY"):
        raise SystemExit("A visible desktop DISPLAY is required; do not run this proof under Xvfb")
    if args.record and (not shutil.which("ffmpeg") or not shutil.which("ffprobe")):
        raise SystemExit("ffmpeg and ffprobe are required to retain visible-window evidence")
    args.output.mkdir(parents=True, exist_ok=True)
    results = [run(args, name) for name in (args.scenario or ["movement", "objects", "lift", "vehicle", "lan-movement"])]
    (args.output / "suite.json").write_text(json.dumps({"target": "English TU1.5", "results": results,
        "full_physics_parity": False}, indent=2) + "\n")
    raise SystemExit(0 if all(row["exit_code"] == 0 for row in results) else 2)


if __name__ == "__main__":
    main()
