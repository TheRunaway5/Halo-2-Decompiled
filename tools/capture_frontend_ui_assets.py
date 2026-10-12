#!/usr/bin/env python3
"""Capture opt-in 2D texture pack against the ordinary frozen native frontend."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from PIL import Image, ImageChops


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack", type=Path)
    parser.add_argument("output", type=Path, help="fresh output directory")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    pack, output = args.pack.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    files = [root / p for p in (
        "build-frontend/halo2_native", "src/frontend.cpp", "src/frontend_painter.cpp",
        "src/frontend_ui_assets.cpp", "include/halo2/frontend_ui_assets.hpp",
        "include/halo2/frontend.hpp", "src/frontend_content.cpp", "src/renderer.cpp",
        "src/bsp_atmosphere.cpp", "shaders/ui_frame.frag", "assets/disc/maps/mainmenu.map",
        "assets/disc/maps/ui/frame_edges/left-edge.dds",
        "assets/disc/maps/ui/frame_edges/right-edge.dds")]
    files += [pack / "pack.tsv", pack / "manifest.json", pack / "pack-manifest.json"]
    pack_rows = [line.split("\t") for line in (pack / "pack.tsv").read_text().splitlines()[1:]
                 if line]
    expected_banks = len({row[0] for row in pack_rows})
    files += sorted({pack / row[5] for row in pack_rows})
    files += sorted((root / "assets/disc/maps/fonts").iterdir())

    def identity():
        return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.is_file()}

    pins = identity()
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER="x11", HALO2_MENU_FOG_SEED="305419896",
               VK_LAYER_PATH=str(root / ".tools/validation/usr/share/vulkan/explicit_layer.d"),
               LD_LIBRARY_PATH=str(root / ".tools/validation/usr/lib"))
    cases = [("title-16x9", 9, 1280, 720, 3, 5000, 0),
             ("settings-32x9", 19, 2560, 720, 121, 100, 50),
             ("appearance-16x9", 47, 1280, 720, 3, 5000, 0),
             ("lobby-16x9", 14, 1280, 720, 3, 5000, 0)]
    report = {"scope": "Native optional texture-only pack captures; Xbox layout/font/world inputs retained. No Xbox whole-frame equality claim.",
              "pins": pins, "runs": [], "pairs": []}
    for name, screen, width, height, frames, clock, step in cases:
        rendered = []
        for variant in ("stock", "vista"):
            assert identity() == pins, "capture inputs changed"
            stem = name + "-" + variant
            command = [str(root / "build-frontend/Halo2-Title-Launcher"),
                       "--skip-intro", "--freeze-menu", "--menu-tick", "181",
                       "--menu-screen", str(screen), "--frames", str(frames),
                       "--menu-ui-ms", str(clock), "--menu-ui-step-ms", str(step),
                       "--viewport", str(width), str(height), "--hidden", "--mute",
                       "--validation", "--screenshot", str(output / (stem + ".ppm"))]
            if variant == "vista":
                command += ["--menu-ui-assets", str(pack)]
            run = subprocess.run(command, cwd=root, env=env, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, text=True, timeout=180)
            (output / (stem + ".log")).write_text(run.stdout)
            errors = re.findall(r"VUID-|Validation Error|validation error|Fatal|cannot initialize", run.stdout)
            report["runs"].append(dict(name=stem, argv=command, returncode=run.returncode,
                                       validation_errors=errors))
            (output / "runs.json").write_text(json.dumps(report, indent=2) + "\n")
            print(stem, "exit", run.returncode, "validation errors", len(errors), flush=True)
            assert run.returncode == 0 and not errors
            assert variant == "stock" or (
                f"Optional frontend 2D texture pack: {expected_banks} image banks" in run.stdout)
            with Image.open(output / (stem + ".ppm")) as image:
                assert image.size == (width, height)
                rendered.append(image.convert("RGB"))
                image.save(output / (stem + ".png"))
            assert identity() == pins, "capture inputs changed"
        difference = ImageChops.difference(*rendered)
        changed = sum(1 for pixel in difference.getdata() if pixel != (0, 0, 0))
        assert changed, "optional pack did not change captured sampled art"
        report["pairs"].append(dict(name=name, changed_pixels=changed,
                                    changed_bbox=difference.getbbox()))
        (output / "runs.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: four native stock/optional pairs, stable inputs, zero validation errors", flush=True)


if __name__ == "__main__":
    main()
