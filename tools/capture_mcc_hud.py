#!/usr/bin/env python3
"""Capture the selected MCC HUD and Vista at the same native 4K match state."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT/"analysis/multiplayer/mcc-hud")
    args = parser.parse_args()
    out = args.output.resolve();out.mkdir(parents=True, exist_ok=True)
    sources = dict(line.split("\t", 1) for line in (ROOT/"assets/asset-sources.tsv").read_text().splitlines()[1:])
    vista = Path(sources["vista"]);vista = vista if vista.is_absolute() else ROOT/vista
    info = subprocess.check_output([str(ROOT/"Halo2-Launcher"), "--launcher-info"], text=True)
    selected = dict(line.split(": ", 1) for line in info.splitlines() if ": " in line)
    runtime = Path(selected["Runtime"]);pack = Path(selected["Selected HUD/fonts"])
    assert (pack/"hud-source.tsv").read_text().startswith("H2_MCC_CLASSIC_HUD_V1\n")
    runtime_sha = sha(runtime);manifest_sha = sha(pack/"manifest.json")
    inputs = out/"idle-input.tsv";inputs.write_text("0 0 0 0\n")
    env = os.environ.copy();env["SDL_VIDEODRIVER"] = "x11"
    env["VK_LAYER_PATH"] = str(ROOT/".tools/validation/usr/share/vulkan/explicit_layer.d")
    env["LD_LIBRARY_PATH"] = str(ROOT/".tools/validation/usr/lib") + (":"+env["LD_LIBRARY_PATH"] if env.get("LD_LIBRARY_PATH") else "")
    captures = []
    for label in ("vista", "mcc"):
        screenshot = out/(label+"-4k.ppm");log = out/(label+"-4k.log")
        command = ["xvfb-run", "-a", "-n", "223", "-s", "-screen 0 3840x2160x24",
            str(ROOT/"Halo2-Launcher"), "--multiplayer", "--mp-starting-weapon", "5",
            "--local-players", "1", "--viewport", "3840", "2160",
            "--mp-input-script", str(inputs), "--mp-capture-tick", "40",
            "--mp-reference-auxiliary-seed", "305419896", "--screenshot", str(screenshot),
            "--hidden", "--validation", "--mute"]
        if label == "vista":
            command += ["--hud-ui-assets", str(vista)]
        with log.open("w") as stream:
            subprocess.run(command, cwd=ROOT, env=env, stdout=stream, stderr=subprocess.STDOUT, check=True)
        assert sha(runtime) == runtime_sha and sha(pack/"manifest.json") == manifest_sha
        text = log.read_text();assert "Validation Error" not in text and "VUID-" not in text
        state = next(line for line in text.splitlines() if line.startswith("Input capture tick="))
        image = Image.open(screenshot).convert("RGB");assert image.size == (3840, 2160)
        png = screenshot.with_suffix(".png");image.save(png)
        captures.append(dict(label=label, command=command, image=str(png), sha256=sha(png), state=state))
        print(label+": 3840x2160 at tick40", flush=True)
    assert captures[0]["state"] == captures[1]["state"], "Native capture states differ"
    a,b = (Image.open(c["image"]) for c in captures)
    changed = np.any(np.array(a) != np.array(b), axis=2);allowed = np.zeros(changed.shape, bool)
    regions = {"grenades":(100,80,800,350), "ammo":(3050,70,3810,600),
        "radar-shield":(80,1440,880,2140), "reticle":(1700,1080,2140,1460),
        "score":(3050,1650,3810,2050)}
    for name,rect in regions.items():
        x0,y0,x1,y1 = rect;allowed[y0:y1,x0:x1] = True
        w,h = x1-x0,y1-y0;canvas = Image.new("RGB", (w*2,h+40), (15,20,30));draw = ImageDraw.Draw(canvas)
        draw.text((12,12), "Vista", fill="white");draw.text((w+12,12), "MCC Halo 2 Classic", fill="white")
        canvas.paste(a.crop(rect), (0,40));canvas.paste(b.crop(rect), (w,40));canvas.save(out/(name+"-comparison.png"))
    outside = int((changed & ~allowed).sum());assert outside == 0, "World pixels changed outside HUD"
    report = dict(passed=True, runtime=str(runtime), runtime_sha256=runtime_sha,
        selected_pack=str(pack), manifest_sha256=manifest_sha, launcher_info=info,
        captures=captures, changed_pixels=int(changed.sum()), changed_pixels_outside_hud=outside,
        claim="Matched native-port frames; original MCC source identity is verified separately. No retail framebuffer parity claim.")
    (out/"capture-comparison.json").write_text(json.dumps(report, indent=2)+"\n")
    print("Matched world pixels outside HUD unchanged", flush=True)


if __name__ == "__main__":
    main()
