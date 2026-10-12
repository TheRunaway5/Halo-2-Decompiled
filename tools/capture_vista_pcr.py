#!/usr/bin/env python3
"""Capture source-backed PC PCR panes with the explicit original fixture bank.

This is a native renderer/asset bridge capture, not a live game report or a
Vista/Xbox framebuffer comparison. The retained original PC spas shader gap is
recorded by the painter. No runtime completion or player/session state is added.
"""
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
    parser.add_argument("binary", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    binary, output = args.binary.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    files = [root / p for p in (
        "Halo 2 Vista installer/halo2.exe", "Halo 2 Vista installer/maps/mainmenu.map",
        "assets/disc/maps/mainmenu.map", "tests/fixtures/vista_pcr_results_original.tsv",
        "tests/fixtures/vista_pcr_appearance_original.jsonl", "tests/fixtures/vista_transition_assets.json",
        "src/frontend_painter.cpp", "src/frontend_vista.cpp", "src/vista_pcr_appearance.cpp",
        "src/vista_pcr_screen.cpp", "src/frontend_state.cpp", "include/halo2/frontend.hpp",
        "include/halo2/vista_pcr_screen.hpp", "include/halo2/vista_pcr_appearance.hpp",
        "tests/fixtures/vista_pcr_chrome_original.json",
        "tests/frontend_vista_pcr_painter_tests.cpp")]
    files += sorted((root / "Halo 2 Vista installer/maps/fonts").iterdir())
    files += sorted((root / "assets/disc/maps/fonts").iterdir())
    files += [binary]

    def pins():
        return {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in files if path.is_file()}

    before = pins()
    assert before[str(root / "Halo 2 Vista installer/halo2.exe")] == \
        "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
    env = dict(os.environ, SDL_VIDEODRIVER="x11")
    validation = root / ".tools/validation/usr"
    if validation.is_dir():
        env["VK_LAYER_PATH"] = str(validation / "share/vulkan/explicit_layer.d")
        env["LD_LIBRARY_PATH"] = str(validation / "lib")
    command = [str(binary), str(root / "assets/disc/maps"),
               str(root / "Halo 2 Vista installer/maps/mainmenu.map"),
               str(root / "tests/fixtures/vista_pcr_results_original.tsv"), str(output)]
    result = subprocess.run(command, cwd=root, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
    (output / "native.log").write_text(result.stdout)
    errors = re.findall(r"VUID-|Validation Error|validation error|Fatal", result.stdout)
    assert result.returncode == 0 and not errors, result.stdout
    report = json.loads((output / "report.json").read_text())
    for screen in (16, 185):
        sheet = Image.new("RGB", (3840, 1440))
        for pane in range(6):
            stem = f"pcr{screen}-pane{pane}"
            with Image.open(output / (stem + ".ppm")) as image:
                assert image.size == (1280, 720)
                image.save(output / (stem + ".png"))
                sheet.paste(image, ((pane % 3) * 1280, (pane // 3) * 720))
        sheet.save(output / f"pcr{screen}-six-panes.png")
    assert pins() == before, "capture inputs changed"
    report.update(pins=before, argv=command, returncode=result.returncode,
                  validation_errors=errors,
                  original_framebuffer_comparison=False, live_game_report=False)
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: 12 native actual-asset PCR pane captures; PC emblem shader gap retained")


if __name__ == "__main__":
    main()
