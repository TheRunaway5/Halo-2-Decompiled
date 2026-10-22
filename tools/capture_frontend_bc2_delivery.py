#!/usr/bin/env python3
"""Capture the rebuilt native frontend; this is not an original-frame comparison."""
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
    parser.add_argument("output", type=Path, help="new directory for immutable captures")
    parser.add_argument("--case", action="append", help="capture only the named case; repeat to select several")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    env = os.environ.copy()
    env.update(SDL_VIDEODRIVER="x11", HALO2_MENU_FOG_SEED="305419896",
               VK_LAYER_PATH=str(root / ".tools/validation/usr/share/vulkan/explicit_layer.d"),
               LD_LIBRARY_PATH=str(root / ".tools/validation/usr/lib"))

    def digest(path):
        data = path.read_bytes()
        return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}

    paths = ["build-frontend/halo2_native", "build-frontend/Halo2-Title-Launcher",
             "src/app/main.cpp", "src/ui/frontend.cpp", "src/ui/frontend_painter.cpp",
             "src/ui/frontend_state.cpp", "src/ui/menu_noise_texture.cpp",
             "include/halo2/frontend.hpp", "include/halo2/frontend_material.hpp",
             "include/halo2/frontend_exit_animation.hpp", "src/rendering/renderer/renderer.cpp",
             "include/halo2/renderer.hpp", "src/ui/mainmenu_bsp_frame.cpp",
             "include/halo2/mainmenu_bsp_frame.hpp", "src/ui/mainmenu_portal_state.cpp",
             "src/rendering/world/bsp_atmosphere.cpp", "include/halo2/bsp_atmosphere.hpp",
             "src/ui/menu_projection.cpp", "include/halo2/menu_projection.hpp",
             "src/rendering/world/source_portal_visibility.cpp", "src/rendering/world/source_bsp_admission.cpp",
             "assets/disc/maps/mainmenu.map", "assets/disc/default.xbe",
             "assets/disc/maps/ui/frame_edges/left-edge.dds",
             "assets/disc/maps/ui/frame_edges/right-edge.dds"]
    identity = {p: digest(root / p) for p in paths}
    (output / "identity.json").write_text(json.dumps(identity, indent=2) + "\n")

    # The same ordinary creation and Split Screen route used by the source test.
    lobby = ["accept", "accept", "text:Capture Test", "commit", "back",
             "select:2", "accept", "accept"]
    transition = lobby + ["select:1", "accept", "accept"]
    cases = [("title-4x3", 9, 960, 720, [], None),
             ("appearance-32x9", 47, 2560, 720, [], None),
             ("profile-4x3", 10, 960, 720, [], None),
             ("keyboard-4x3", 167, 960, 720, [], None),
             ("lobby-4x3", 9, 960, 720, lobby, None),
             ("setup-map-125ms-4x3", 9, 960, 720, transition, 125),
             ("setup-map-249ms-4x3", 9, 960, 720, transition, 249),
             ("setup-map-125ms-32x9", 9, 2560, 720, transition, 125)]
    if args.case:
        unknown = set(args.case) - {case[0] for case in cases}
        if unknown:
            parser.error("unknown capture case: " + ", ".join(sorted(unknown)))
        cases = [case for case in cases if case[0] in args.case]
    report = {"scope": "Actual native launcher frames, including an ordinary disposable MenuState route. "
                        "Explicit diagnostic elapsed time is held across rendered frames; no original "
                        "GPU framebuffer equality or ordinary host update cadence is claimed.",
              "deterministic_native_fog_seed": 305419896, "scene_tick": 181, "runs": []}
    for name, screen, width, height, actions, ui_ms in cases:
        assert identity == {p: digest(root / p) for p in paths}, "capture inputs changed"
        command = [str(root / "build-frontend/Halo2-Title-Launcher"),
                   "--skip-intro", "--freeze-menu", "--menu-tick", "181",
                   "--menu-screen", str(screen), "--frames", "3", "--viewport",
                   str(width), str(height), "--hidden", "--mute", "--validation",
                   "--screenshot", str(output / (name + ".ppm"))]
        for action in actions:
            command.extend(["--menu-action", action])
        if ui_ms is not None:
            command.extend(["--menu-ui-ms", str(ui_ms)])
        run = subprocess.run(command, cwd=root, env=env, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT, text=True, timeout=180)
        (output / (name + ".log")).write_text(run.stdout)
        errors = re.findall(r"VUID-|Validation Error|validation error|Fatal|cannot initialize", run.stdout)
        row = {"name": name, "screen_option": screen, "requested_extent": [width, height],
               "actions": actions, "ui_ms": ui_ms, "argv": command,
               "returncode": run.returncode, "validation_errors": errors,
               "route_screens": [int(x) for x in re.findall(r"Menu capture route screen=(\d+)", run.stdout)]}
        if run.returncode == 0 and not errors:
            with Image.open(output / (name + ".ppm")) as picture:
                assert picture.size == (width, height), picture.size
                picture.save(output / (name + ".png"))
            row["png"] = digest(output / (name + ".png"))
            seed = re.search(r"Retail patchy-fog random seed: (\d+)", run.stdout)
            assert seed and int(seed.group(1)) == 305419896
            if actions:
                assert row["route_screens"][-1] == (206 if ui_ms is not None else 14)
        report["runs"].append(row)
        (output / "runs.json").write_text(json.dumps(report, indent=2) + "\n")
        print(name, "exit", run.returncode, "validation errors", len(errors), flush=True)
        if run.returncode or errors:
            raise SystemExit(1)
        assert identity == {p: digest(root / p) for p in paths}, "capture inputs changed"
    print("PASS: all native captures; pinned source, binary and DDS files unchanged", flush=True)


if __name__ == "__main__":
    main()
