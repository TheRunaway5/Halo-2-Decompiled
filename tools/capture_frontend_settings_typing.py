#!/usr/bin/env python3
"""Capture actual native source-derived Settings typing and aspect coverage."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="fresh output directory")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    paths = ["build-frontend/halo2_native", "src/frontend.cpp", "src/frontend_state.cpp",
             "src/frontend_painter.cpp", "src/frontend_content.cpp", "src/frontend_text_style.cpp",
             "src/frontend_typewriter.cpp", "include/halo2/frontend.hpp", "src/renderer.cpp",
             "src/bsp_atmosphere.cpp", "shaders/ui_frame.frag", "assets/disc/maps/mainmenu.map",
             "assets/disc/maps/ui/frame_edges/left-edge.dds",
             "assets/disc/maps/ui/frame_edges/right-edge.dds"]
    paths += [str(p.relative_to(root)) for p in sorted((root / "assets/disc/maps/fonts").iterdir())
              if p.is_file()]

    def identity():
        return {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths}

    pins = identity()
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER="x11", HALO2_MENU_FOG_SEED="305419896",
               VK_LAYER_PATH=str(root / ".tools/validation/usr/share/vulkan/explicit_layer.d"),
               LD_LIBRARY_PATH=str(root / ".tools/validation/usr/lib"))
    cases = [("settings-typing-16x9", 19, 1280, 720, 31, 100, 50, False),
             ("settings-complete-4x3", 19, 960, 720, 121, 100, 50, False),
             ("settings-complete-16x9", 19, 1280, 720, 121, 100, 50, False),
             ("settings-complete-32x9", 19, 2560, 720, 121, 100, 50, False),
             ("settings-resize", 19, 2560, 720, 121, 100, 50, True),
             ("keyboard-16x9", 167, 1280, 720, 3, 5000, 0, False),
             ("appearance-32x9", 47, 2560, 720, 3, 5000, 0, False)]
    report = {"scope": "Actual native typing and complete labels with source-derived fonts, glyph clipping and clocks. UI clocks advance per presented draw; world is diagnostic-frozen. Not an original framebuffer match.",
              "pins": pins, "runs": []}
    for name, screen, width, height, frames, start, step, resize in cases:
        assert identity() == pins, "capture inputs changed"
        command = [str(root / "build-frontend/Halo2-Title-Launcher"),
                   "--skip-intro", "--freeze-menu", "--menu-tick", "181",
                   "--menu-screen", str(screen), "--frames", str(frames),
                   "--menu-ui-ms", str(start), "--menu-ui-step-ms", str(step),
                   "--viewport", str(width), str(height), "--hidden", "--mute",
                   "--validation", "--screenshot", str(output / (name + ".ppm"))]
        if resize:
            command.append("--resize-test")
        run = subprocess.run(command, cwd=root, env=env, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True, timeout=180)
        (output / (name + ".log")).write_text(run.stdout)
        errors = re.findall(r"VUID-|Validation Error|validation error|Fatal|cannot initialize", run.stdout)
        expected_extent = [1000, 600] if resize else [width, height]
        row = {"name": name, "extent": expected_extent, "screen": screen,
               "capture_clock_ms": (start + (frames - 1) * step) & 0xffffffff,
               "argv": command, "returncode": run.returncode, "validation_errors": errors}
        if run.returncode == 0 and not errors:
            with Image.open(output / (name + ".ppm")) as image:
                assert list(image.size) == expected_extent
                image.save(output / (name + ".png"))
        report["runs"].append(row)
        (output / "runs.json").write_text(json.dumps(report, indent=2) + "\n")
        print(name, "exit", run.returncode, "validation errors", len(errors), flush=True)
        assert run.returncode == 0 and not errors
        assert identity() == pins, "capture inputs changed"
    print("PASS: native Settings typing and aspect captures with stable inputs", flush=True)


if __name__ == "__main__":
    main()
