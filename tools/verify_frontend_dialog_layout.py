#!/usr/bin/env python3
"""Capture native dialog composition through disposable production menu routes.

Checks capture completion, route destinations, image sizes and Vulkan errors.
The painter regressions separately check backdrop boundaries and text bounds;
these captures do not establish complete original/native framebuffer parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case", action="append", help="capture only this named case; repeatable")
    args = parser.parse_args()
    launcher, output = args.launcher.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, SDL_VIDEODRIVER="x11", HALO2_MENU_FOG_SEED="305419896")
    env["VK_LAYER_PATH"] = str(ROOT / ".tools/validation/usr/share/vulkan/explicit_layer.d")
    env["LD_LIBRARY_PATH"] = str(ROOT / ".tools/validation/usr/lib")
    create = ["accept", "accept", "text:Overlap Test", "commit"]
    elite = create + ["select:1", "accept", "select:0", "accept", "select:1", "accept", "back", "back"]
    # The native biped admission gate can grow independently of UI layout.
    # King remains an unavailable launch and reaches the same error dialog.
    error = elite + ["select:2", "accept", "accept", "select:1", "accept",
                     "select:1", "accept", "select:1", "accept", "select:0",
                     "accept", "select:0", "accept"]
    cases = [(f"error-{name}", 9, error, 8, w, h) for name, w, h in (
        ("4x3", 640, 480), ("16x9", 1280, 720), ("16x10", 1280, 800),
        ("21x9", 1680, 720), ("user-size", 2560, 1437))]
    cases += [("error-dismiss", 9, error + ["accept"], 14, 1280, 720),
              ("profile", 9, ["accept"], 10, 1280, 720),
              ("keyboard", 9, ["accept", "accept"], 167, 1280, 720),
              ("confirmation", 7, [], 7, 1280, 720),
              ("sensitivity", 42, [], 42, 1280, 720),
              ("vista-about", 19, ["select:5", "accept"], 0x10000 | 297, 1280, 720),
              ("vista-video", 0x10000 | 256, [], 0x10000 | 256, 1280, 720),
              ("vista-bindings", 0x10000 | 253, [], 0x10000 | 253, 1280, 720)]
    if args.case:
        assert set(args.case) <= {case[0] for case in cases}, "unknown capture case"
        cases = [case for case in cases if case[0] in args.case]
    info = subprocess.run([str(launcher), "--launcher-info"], cwd=ROOT, text=True,
                          stdout=subprocess.PIPE, check=True).stdout
    runtime = Path(next(line.removeprefix("Runtime: ") for line in info.splitlines() if line.startswith("Runtime: ")))
    runtime_pin = hashlib.sha256(runtime.read_bytes()).hexdigest()
    report = {"scope": __doc__, "launcher": str(launcher), "runtime": str(runtime),
              "runtime_sha256": runtime_pin, "runs": []}
    thumbnails = []
    for name, screen, actions, expected, width, height in cases:
        assert hashlib.sha256(runtime.read_bytes()).hexdigest() == runtime_pin, "runtime changed during capture bank"
        target = output / f"{name}.ppm"
        command = [str(launcher), "--skip-intro", "--freeze-menu", "--menu-tick", "181",
                   "--menu-screen", str(screen), "--menu-ui-ms", "10000",
                   "--viewport", str(width), str(height), "--frames", "3", "--hidden",
                   "--mute", "--validation", "--screenshot", str(target)]
        for action in actions:
            command += ["--menu-action", action]
        # Also keep preview-only captures from reading the user's preferences.
        if not actions:
            command += ["--menu-action", "select:0"]
        run = subprocess.run(command, cwd=ROOT, env=env, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
        (output / f"{name}.log").write_text(run.stdout)
        errors = re.findall(r"VUID-|Validation Error|validation error|Fatal|^error:", run.stdout, re.M)
        row = {"name": name, "argv": command, "returncode": run.returncode,
               "validation_errors": errors, "expected_screen": expected}
        report["runs"].append(row)
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        assert run.returncode == 0 and not errors, (name, run.stdout[-2000:])
        assert hashlib.sha256(runtime.read_bytes()).hexdigest() == runtime_pin, "runtime changed during capture"
        assert f"Menu ready screen={expected} " in run.stdout, (name, run.stdout[-1000:])
        with Image.open(target) as image:
            assert image.size == (width, height)
            image.save(target.with_suffix(".png"))
            preview = image.convert("RGB")
            preview.thumbnail((480, 300))
            thumbnails.append((name, preview.copy()))
        row["png_sha256"] = hashlib.sha256(target.with_suffix(".png").read_bytes()).hexdigest()
        print(f"{name}: screen {expected}, {width}x{height}, no Vulkan errors", flush=True)
    sheet = Image.new("RGB", (1440, 340 * ((len(thumbnails) + 2) // 3)), "#111821")
    draw = ImageDraw.Draw(sheet)
    for i, (name, thumbnail) in enumerate(thumbnails):
        x, y = (i % 3) * 480, (i // 3) * 340
        draw.text((x + 10, y + 10), name, fill="white")
        sheet.paste(thumbnail, (x, y + 30))
    sheet.save(output / "contact-sheet.png")
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
