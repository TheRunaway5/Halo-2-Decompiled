#!/usr/bin/env python3
"""Verify prompt clicks through native SDL window events and Vulkan captures.

Coordinates use the independently checked original subtitle positions or the
real-painter action regression. MenuState setup is disposable; the final clicks
are submitted as SDL events, through the ordinary platform input handler.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--case", action="append")
    args = parser.parse_args()
    launcher, output = args.launcher.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, SDL_VIDEODRIVER="x11", HALO2_MENU_FOG_SEED="305419896")
    env["VK_LAYER_PATH"] = str(ROOT / ".tools/validation/usr/share/vulkan/explicit_layer.d")
    env["LD_LIBRARY_PATH"] = str(ROOT / ".tools/validation/usr/lib")
    create = ["accept", "accept", "text:Mouse Capture", "commit"]
    appearance = create + ["select:1", "accept"]
    icon = appearance + ["select:5", "accept"]
    # A source-supported unavailable engine reaches the production error pane.
    error = create + ["back", "select:2", "accept", "accept", "select:1", "accept",
                      "select:1", "accept", "select:1", "accept", "select:0", "accept", "select:0", "accept"]
    sizes = [("4x3", 640, 480), ("16x9", 1280, 720), ("16x10", 1280, 800),
             ("21x9", 1680, 720), ("user", 2560, 1437), ("odd", 1001, 601)]
    # name, initial screen, setup, source-canvas clicks, final screen, expected
    # (action, binding-capture) SDL dispatches, viewport width/height.
    cases = [("appearance-done-" + name, 9, appearance, [(545, 432)], 38, [(2, 0)], w, h)
             for name, w, h in sizes]
    cases += [
        ("title-start", 9, [], [(230, 282)], 10, [(1, 0)], 1280, 720),
        ("error-continue", 9, error, [(360, 332)], 14, [(1, 0)], 2560, 1437),
        ("error-blocks-lobby", 9, error, [(150, 78)], 8, [], 1280, 800),
        ("icon-toggle", 9, icon, [(136, 394)], 49, [(3, 0)], 1280, 720),
        ("icon-cancel", 9, icon, [(310, 394)], 47, [(2, 0)], 1280, 720),
        ("keyboard-done", 9, ["accept", "accept", "text:Mouse Name"], [(430, 372)], 38, [(0, 0)], 1280, 800),
        ("keyboard-cancel", 9, ["accept", "accept"], [(526, 426)], 9, [(2, 0)], 1280, 720),
        ("vista-select", 65555, ["select:5"], [(530, 446)], 65833, [(1, 0)], 1680, 720),
        ("vista-about-back", 65555, ["select:5", "accept"], [(468, 336)], 65555, [(2, 0)], 1280, 720),
        ("vista-secondary-cancel", 65789, ["select:1"], [(242, 46), (458, 46)], 65789, [(3, 1), (2, 0)], 1280, 720),
        ("vista-delete", 65789, ["select:1"], [(486, 446)], 65789, [(6, 0)], 1280, 720),
        ("vista-edit-clear", 65789, ["select:1"], [(540, 446), (360, 46)], 65789, [(1, 1), (6, 0)], 1280, 800),
        ("vista-done", 65794, ["select:0", "accept", "select:3", "accept"],
         [(580, 446)], 65779, [(2, 0)], 1280, 720),
    ]
    if args.case:
        assert set(args.case) <= {case[0] for case in cases}, "unknown case"
        cases = [case for case in cases if case[0] in args.case]
    info = subprocess.check_output([str(launcher), "--launcher-info"], cwd=ROOT, text=True)
    runtime = Path(next(line.removeprefix("Runtime: ") for line in info.splitlines() if line.startswith("Runtime: ")))
    pin = hashlib.sha256(runtime.read_bytes()).hexdigest()
    launcher_pin = hashlib.sha256(launcher.read_bytes()).hexdigest()
    report = {"scope": __doc__, "launcher": str(launcher), "launcher_sha256": launcher_pin,
              "runtime": str(runtime), "runtime_sha256": pin, "runs": []}
    for name, screen, setup, clicks, expected, dispatches, width, height in cases:
        assert hashlib.sha256(runtime.read_bytes()).hexdigest() == pin, "runtime changed during capture bank"
        assert hashlib.sha256(launcher.read_bytes()).hexdigest() == launcher_pin, "launcher changed during capture bank"
        image = output / (name + ".ppm")
        command = [str(launcher), "--skip-intro", "--freeze-menu", "--menu-tick", "181", "--menu-ui-ms", "10000",
                   "--menu-screen", str(screen), "--viewport", str(width), str(height), "--frames", "3",
                   "--hidden", "--mute", "--validation", "--screenshot", str(image)]
        scale = min(width / 640, height / 480)
        left, top = (width - 640 * scale) / 2, (height - 480 * scale) / 2
        for action in setup:
            command += ["--menu-action", action]
        for x, y in clicks:
            command += ["--menu-action", f"click:{round(left + x * scale)}:{round(top + y * scale)}"]
        run = subprocess.run(command, cwd=ROOT, env=env, text=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, timeout=180)
        (output / (name + ".log")).write_text(run.stdout)
        errors = re.findall(r"VUID-|Validation Error|validation error|Fatal|^error:", run.stdout, re.M)
        actual = [(int(a), int(b)) for a, b in re.findall(r"Menu click action=(\d+) screen=\d+ binding_capture=(\d+)", run.stdout)]
        row = {"name": name, "argv": command, "returncode": run.returncode, "validation_errors": errors,
               "expected_screen": expected, "expected_dispatches": dispatches, "actual_dispatches": actual}
        report["runs"].append(row)
        (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
        assert run.returncode == 0 and not errors, (name, run.stdout[-2000:])
        assert actual == dispatches, (name, actual, dispatches)
        assert f"Menu ready screen={expected} " in run.stdout, (name, run.stdout[-1500:])
        assert hashlib.sha256(runtime.read_bytes()).hexdigest() == pin, "runtime changed during capture"
        assert hashlib.sha256(launcher.read_bytes()).hexdigest() == launcher_pin, "launcher changed during capture"
        with Image.open(image) as frame:
            assert frame.size == (width, height)
            frame.save(image.with_suffix(".png"))
        print(f"PASS {name}: SDL click -> screen {expected}", flush=True)
    print(f"{len(cases)} native SDL mouse routes passed", flush=True)


if __name__ == "__main__":
    main()
