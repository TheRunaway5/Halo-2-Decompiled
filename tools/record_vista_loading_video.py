#!/usr/bin/env python3
"""Record the native Vista loading presentation with its original ambience.

This is a 24-second renderer replay with a supplied zero sweep owner and an
end fade. It does not pretend that a real map took this long to load.
"""
import argparse
from array import array
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess
import sys

from preserve_vista_loading import selected, validate

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(executable, output, assets):
    validate(assets)
    output.mkdir(parents=True, exist_ok=False)
    video = output / "vista-match-loading-with-sound.mp4"
    environment = dict(os.environ, SDL_AUDIODRIVER="dummy")
    validation = ROOT / ".tools/validation/usr"
    if validation.is_dir():
        environment["VK_LAYER_PATH"] = str(validation / "share/vulkan/explicit_layer.d")
        environment["LD_LIBRARY_PATH"] = os.pathsep.join(filter(None, (
            str(validation / "lib"), environment.get("LD_LIBRARY_PATH"))))
    paths = [executable, Path(__file__).resolve(), ROOT / "tools/capture_vista_loading_video.cpp",
             ROOT / "src/rendering/renderer/renderer.cpp", ROOT / "src/platform/media/native_loading_presentation.cpp",
             ROOT / "src/ui/vista/vista_loading_geometry.cpp", ROOT / "src/ui/vista/native_vista_loading_audio.cpp",
             ROOT / "src/ui/vista/vista_loading_audio.cpp", ROOT / "shaders/vista_loading.vert",
             ROOT / "shaders/vista_loading.frag", *sorted(assets.glob("*")),
             executable.parent / "shaders/vista_loading.vert.spv",
             executable.parent / "shaders/vista_loading.frag.spv"]
    pins = {str(path): digest(path) for path in paths if path.is_file()}
    capture_argv = [str(executable), str(assets), str(output / "capture")]
    encode_argv = ["ffmpeg", "-hide_banner", "-loglevel", "warning", "-f", "rawvideo",
                   "-pixel_format", "rgb24", "-video_size", "1280x720", "-framerate", "30",
                   "-i", "pipe:0", "-an", "-c:v", "libx264", "-preset", "medium",
                   "-crf", "16", "-pix_fmt", "yuv420p", str(output / "silent.mp4")]
    with (output / "capture.log").open("wb") as log:
        capture = subprocess.Popen(capture_argv, stdout=subprocess.PIPE, stderr=log, env=environment)
        encode = None
        try:
            encode = subprocess.Popen(encode_argv, stdin=capture.stdout, stdout=log, stderr=log)
            capture.stdout.close()
            encode_code = encode.wait(timeout=600)
            capture_code = capture.wait(timeout=30)
        finally:
            if capture.poll() is None:
                capture.terminate()
                capture.wait(timeout=10)
            if encode is not None and encode.poll() is None:
                encode.terminate()
                encode.wait(timeout=10)
        if encode_code or capture_code:
            raise RuntimeError(f"Loading capture/encoding failed: capture={capture_code}, encoder={encode_code}; see {log.name}")
    log_text = (output / "capture.log").read_text(errors="replace")
    if "Validation Error" in log_text or "VUID-" in log_text:
        raise RuntimeError("Vulkan validation rejected the recording; see capture.log")
    samples = array("f")
    samples.frombytes((output / "capture/audio.f32le").read_bytes())
    if sys.byteorder != "little":
        samples.byteswap()
    if len(samples) != 24 * 48000 * 2 or any(not math.isfinite(value) for value in samples):
        raise RuntimeError("Incomplete or invalid captured loading PCM")
    peak = max(map(abs, samples))
    rms = math.sqrt(sum(value * value for value in samples) / len(samples))
    if peak < 0.00001 or peak > 1:
        raise RuntimeError("Captured loading PCM is silent or outside the native clipping range")
    mux_argv = ["ffmpeg", "-hide_banner", "-loglevel", "warning", "-i", str(output / "silent.mp4"),
                "-f", "f32le", "-ar", "48000", "-ac", "2", "-i", str(output / "capture/audio.f32le"),
                "-c:v", "copy", "-c:a", "aac", "-b:a", "192k", "-movflags", "+faststart", str(video)]
    subprocess.run(mux_argv, check=True)
    probe = json.loads(subprocess.check_output([
        "ffprobe", "-v", "error", "-show_streams", "-show_format", "-of", "json", str(video)], text=True))
    visual = [row for row in probe["streams"] if row["codec_type"] == "video"]
    audio = [row for row in probe["streams"] if row["codec_type"] == "audio"]
    if len(visual) != 1 or len(audio) != 1 or visual[0].get("nb_frames") != "720" or abs(float(probe["format"]["duration"]) - 24) > 0.05:
        raise RuntimeError("Loading recording has incomplete video/audio streams")
    changed = [path for path, expected in pins.items() if digest(Path(path)) != expected]
    report = dict(schema="H2_VISTA_LOADING_AUDIOVISUAL_REPLAY_V1", scope=__doc__,
                  assets=str(assets), pins=pins, changed_inputs=changed,
                  capture_argv=capture_argv, encode_argv=encode_argv, mux_argv=mux_argv,
                  probe=probe, video=str(video), video_sha256=digest(video),
                  pcm_peak=peak, pcm_rms=rms, validation_errors=0,
                  physical_windows_comparison=False, actual_map_loading=False,
                  full_mcc_conversion=False, sweep_progress_bits="0x00000000")
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    if changed:
        raise RuntimeError("Recording inputs changed during capture")
    print(video)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--assets", type=Path)
    args = parser.parse_args()
    record(args.executable.resolve(), args.output.resolve(),
           args.assets.resolve() if args.assets else selected(ROOT).resolve())


if __name__ == "__main__":
    main()
