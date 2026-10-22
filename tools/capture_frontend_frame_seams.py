#!/usr/bin/env python3
"""Retain actual native frame-strip joins; no original framebuffer claim."""
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
    paths = ["build-frontend/halo2_native", "src/rendering/renderer/renderer.cpp", "src/ui/frontend.cpp",
             "src/rendering/world/bsp_atmosphere.cpp", "include/halo2/renderer.hpp", "CMakeLists.txt",
             "src/ui/frontend_painter.cpp", "include/halo2/cache.hpp",
             "shaders/ui.vert", "shaders/ui.frag",
             "assets/disc/maps/ui/frame_edges/left-edge.dds",
             "assets/disc/maps/ui/frame_edges/right-edge.dds"]
    paths += [p for p in ["shaders/ui_frame.frag",
                         "include/halo2/frontend_frame_sampling.hpp"]
              if (root / p).exists()]

    def identity():
        return {p: hashlib.sha256((root / p).read_bytes()).hexdigest() for p in paths}

    pins = identity()
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER="x11", HALO2_MENU_FOG_SEED="305419896",
               VK_LAYER_PATH=str(root / ".tools/validation/usr/share/vulkan/explicit_layer.d"),
               LD_LIBRARY_PATH=str(root / ".tools/validation/usr/lib"))
    cases = [("settings-4x3", 19, 960, 720),
             ("settings-16x9", 19, 1280, 720),
             ("settings-user-extent", 19, 2560, 1370),
             ("settings-32x9", 19, 2560, 720),
             ("settings-fractional", 19, 1001, 601),
             ("appearance-32x9", 47, 2560, 720),
             ("settings-5x4", 19, 1000, 800)]
    report = {"scope": "Actual native extension seam captures. Stock center layout and DDS bytes retained; not an original-frame comparison.",
              "pins": pins, "runs": []}
    for name, screen, width, height in cases:
        assert identity() == pins, "capture inputs changed"
        command = [str(root / "build-frontend/Halo2-Title-Launcher"),
                   "--skip-intro", "--freeze-menu", "--menu-tick", "181",
                   "--menu-screen", str(screen), "--frames", "3", "--viewport",
                   str(width), str(height), "--hidden", "--mute", "--validation",
                   "--screenshot", str(output / (name + ".ppm"))]
        run = subprocess.run(command, cwd=root, env=env, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True, timeout=180)
        (output / (name + ".log")).write_text(run.stdout)
        errors = re.findall(r"VUID-|Validation Error|validation error|Fatal|cannot initialize", run.stdout)
        row = {"name": name, "extent": [width, height], "screen": screen,
               "argv": command, "returncode": run.returncode, "validation_errors": errors}
        if run.returncode == 0 and not errors:
            with Image.open(output / (name + ".ppm")) as image:
                assert image.size == (width, height)
                image.save(output / (name + ".png"))
        report["runs"].append(row)
        (output / "runs.json").write_text(json.dumps(report, indent=2) + "\n")
        print(name, "exit", run.returncode, "validation errors", len(errors), flush=True)
        assert run.returncode == 0 and not errors
        assert identity() == pins, "capture inputs changed"
    print("PASS: native frame joins captured with stable inputs", flush=True)


if __name__ == "__main__":
    main()
