#!/usr/bin/env python3
"""Verify visible native body and held-weapon grenade lighting independently.

All runs use real source constructors, inputs and grenade delivery. This proves
the joined native light contribution, not matched OG Xbox framebuffer parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
XBE = Path.home() / ".local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
XBE_SHA = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--shaders", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--width", type=int, default=960)
    parser.add_argument("--height", type=int, default=540)
    args = parser.parse_args()
    out = args.output.resolve()
    assert not out.exists(), out
    assert sha(XBE) == XBE_SHA, "Reference is not the verified OG Xbox TU1.5 executable"
    out.mkdir(parents=True)
    binary = out / "runtime"
    shutil.copy2(args.binary, binary)
    shader_dir = out / "shaders"
    shutil.copytree(args.shaders, shader_dir)
    shaders = {p.name: sha(p) for p in shader_dir.glob("*.spv")}
    assert len(shaders) == 64, "The capture requires the complete 64-module renderer bank"
    manifest = dict(scope=__doc__, full_goal_complete=False, original_framebuffer_parity=False,
                    runtime_sha256=sha(binary), original_xbe_sha256=XBE_SHA,
                    shader_modules_sha256=shaders, tool_sha256=sha(__file__), runs=[])
    env = os.environ.copy()
    for key in tuple(env):
        if key.startswith("HALO2_REFERENCE_DISABLE_"):
            del env[key]
    env.update(DISPLAY=":0", SDL_VIDEODRIVER="x11",
               TMPDIR=str(ROOT / ".local/shader-parity-tmp-20261014"),
               VK_DRIVER_FILES="/usr/share/vulkan/icd.d/nvidia_icd.json",
               VK_LAYER_PATH=str(ROOT / ".tools/validation/usr/share/vulkan/explicit_layer.d"),
               LD_LIBRARY_PATH=str(ROOT / ".tools/validation/usr/lib"))
    controls = (("no-unit-light", None), ("no-body-light", "unit"), ("no-weapon-light", "weapon"))
    for name, receiver_field in (("positive", None), *controls):
        # A real native world plus the Vulkan upload can use about 31 GiB.
        # Other workspace captures retain their own devices/worlds; wait for
        # room without interrupting their work or accepting a partial image.
        deadline = time.monotonic() + 180
        while True:
            available = next(int(line.split()[1]) for line in Path("/proc/meminfo").read_text().splitlines()
                             if line.startswith("MemAvailable:"))
            if available >= 32 * 1024 * 1024:
                break
            assert time.monotonic() < deadline, "Insufficient free memory for a complete native GPU capture"
            time.sleep(1)
        run_env = env.copy()
        removed = []
        receiver = None
        if name != "positive":
            positive_report = json.loads((out / "positive/report.json").read_text())
            removed = sorted({light["identity"] for light in positive_report["model_light_sources"]})
            assert removed, "No proven grenade light identities to remove"
            run_env["HALO2_REFERENCE_DISABLE_UNIT_EFFECT_LIGHTS"] = ",".join(map(str, removed))
            if receiver_field:
                target = next(p for p in positive_report["initial_players"]
                              if p["slot"] == positive_report["target_slot"])
                receiver = target[receiver_field]
                run_env["HALO2_REFERENCE_DISABLE_UNIT_EFFECT_LIGHT_OWNER"] = str(receiver)
        command = [str(binary), str(ROOT / "assets/disc/maps/lockout.map"),
                   str(ROOT / "assets/disc/maps"), str(XBE), str(args.width), str(args.height),
                   str(out / name), str(shader_dir)]
        with (out / (name + ".log")).open("w") as log:
            result = subprocess.run(command, cwd=ROOT, env=run_env, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=300)
        errors = [line for line in (out / (name + ".log")).read_text().splitlines()
                  if "Validation Error" in line or "VUID-" in line]
        manifest["runs"].append(dict(name=name, command=command, exit_code=result.returncode,
                                     validation_errors=errors, unit_light_consumer_disabled=name != "positive",
                                     removed_receiver=receiver,
                                     removed_unit_light_identities=removed))
        (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        assert result.returncode == 0, f"Capture failed: {out / (name + '.log')}"
        assert not errors, errors[:5]
        Image.open(out / name / "unit.ppm").save(out / name / "unit.png")
    a = json.loads((out / "positive/report.json").read_text())
    fields = ["source_owners", "initial_players", "target_frames", "spawn_pool", "session_random_seed", "source_selected_spawn_indices",
              "source_inputs", "draws", "grenade_events", "other_unit_lights",
              "model_light_tick", "model_light_packets", "model_light_sources", "final_view_geometry", "pinned_shader_directory"]
    receivers = {owner for light in a["model_light_sources"] for owner in light["receivers"]}
    assert receivers == set(a["source_owners"]), "Capture does not illuminate both actual receivers"
    bases = {p.name: sha(p) for p in (out / "positive").glob("base-*.bin")}
    assert set(bases) == {f"base-{owner}.bin" for owner in receivers}, "Missing actual final submitted receiver base"
    for name, _ in controls:
        b = json.loads((out / name / "report.json").read_text())
        for field in fields:
            assert a[field] == b[field], f"{name} changed original source inputs: {field}"
        assert bases == {p.name: sha(p) for p in (out / name).glob("base-*.bin")}, f"{name} changed submitted base vertices"
    assert shaders == {p.name: sha(p) for p in shader_dir.glob("*.spv")}, "Pinned shader bank changed"
    assert manifest["runtime_sha256"] == sha(binary), "Pinned runtime changed"
    positive = Image.open(out / "positive/unit.png").convert("RGB")
    contributions = {}
    for name, receiver_field in controls:
        negative = Image.open(out / name / "unit.png").convert("RGB")
        diff = ImageChops.difference(positive, negative)
        values = list(diff.get_flattened_data() if hasattr(diff, "get_flattened_data") else diff.getdata())
        count = sum(any(pixel) for pixel in values)
        contributions[name] = dict(changed_rgb_pixels=count, bounds=diff.getbbox(),
                                   changed_rgb_channels=sum(v != 0 for p in values for v in p),
                                   maximum_rgb_contribution=max(max(p) for p in values))
        diff.save(out / (name + "-difference.png"))
    manifest["receiver_contributions"] = contributions
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    for name, _ in controls:
        assert contributions[name]["changed_rgb_pixels"], f"{name}: receiver produced no visible RGB contribution"
    negative = Image.open(out / "no-unit-light/unit.png").convert("RGB")
    diff = ImageChops.difference(positive, negative)
    bounds = diff.getbbox()
    diff.save(out / "difference.png")
    cx, cy = (bounds[0] + bounds[2]) // 2, (bounds[1] + bounds[3]) // 2
    half_w = max(100, (bounds[2] - bounds[0]) // 2 + 12)
    half_h = max(75, (bounds[3] - bounds[1]) // 2 + 12)
    crop = (max(0, cx - half_w), max(0, cy - half_h),
            min(positive.width, cx + half_w), min(positive.height, cy + half_h))
    w, h = crop[2] - crop[0], crop[3] - crop[1]
    comparison = Image.new("RGB", (2 * w, h + 26), "#202020")
    comparison.paste(negative.crop(crop), (0, 26))
    comparison.paste(positive.crop(crop), (w, 26))
    draw = ImageDraw.Draw(comparison)
    draw.text((6, 6), "Unit light off", fill="white")
    draw.text((w + 6, 6), "Actual grenade", fill="white")
    comparison.save(out / "comparison-crop.png")
    manifest.update(same_source_fields=fields, base_vertices_sha256=bases,
                    model_light_tick=a["model_light_tick"], model_light_packets=a["model_light_packets"],
                    **contributions["no-unit-light"], all_requested_receivers_verified=True,
                    passed=True)
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps({k: manifest[k] for k in ("passed", "model_light_tick", "model_light_packets",
                                              "changed_rgb_pixels", "bounds")}))


if __name__ == "__main__":
    main()
