#!/usr/bin/env python3
"""Compare actual native loading captures with retained original D3D9 frames.

Run inside an X display (xvfb-run is sufficient). This compares the supplied
clock/progress fixtures and black input framebuffer, not transition lifetime,
live progress publication, physical Windows, or the entire audiovisual flow.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def metrics(reference, actual):
    if reference.shape != actual.shape:
        raise RuntimeError(f"Capture extent changed: {actual.shape} != {reference.shape}")
    delta = actual.astype(np.int16) - reference.astype(np.int16)
    absolute = np.abs(delta)
    changed = np.any(delta != 0, axis=2)
    rows, columns = np.nonzero(changed)
    return dict(exact_rgb=not bool(changed.any()), changed_pixels=int(changed.sum()),
                total_pixels=int(changed.size), max_channel_error=int(absolute.max()),
                mean_channel_error=float(absolute.mean()),
                signed_channel_bias=[float(v) for v in delta.mean(axis=(0, 1))],
                pixels_above_one=int(np.any(absolute > 1, axis=2).sum()),
                changed_bounds=[int(columns.min()), int(rows.min()),
                                int(columns.max()), int(rows.max())] if rows.size else None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable", type=Path)
    parser.add_argument("reference", type=Path)
    parser.add_argument("output", type=Path, help="new output directory")
    parser.add_argument("--assets", type=Path, default=ROOT/"assets/menu-ui/vista/loading")
    parser.add_argument("--source-root", type=Path, default=ROOT,
                        help="exact source tree compiled into the executable")
    parser.add_argument("--offset", action="append", help="x,y pixel offsets; default: 0,0 and 0.5,0.5")
    parser.add_argument("--case", type=int, action="append", help="optional selected original case indices")
    args = parser.parse_args()
    if not os.environ.get("DISPLAY"):
        parser.error("Run inside a real or virtual X display")
    executable, reference, output = args.executable.resolve(), args.reference.resolve(), args.output.resolve()
    source_root = args.source_root.resolve()
    output.mkdir(parents=True, exist_ok=False)
    original = json.loads((reference/"report.json").read_text())
    if original["schema"] != "H2V_ORIGINAL_LOADING_D3D9_REPLAY_V1" or original["returncode"] != 0:
        raise RuntimeError("Reference is not a successful original D3D9 replay")
    # Verify the actual reference source, shaders, decoder, and raw framebuffer
    # pins before using them as authority. An absent pin is an error.
    for name, expected in original["pins"].items():
        if digest(Path(name)) != expected:
            raise RuntimeError(f"Original replay input changed: {name}")
    cases = json.loads((reference/"original-commands.json").read_text())
    offsets = [tuple(map(float, value.split(","))) for value in (args.offset or ["0,0", "0.5,0.5"])]
    if any(len(value) != 2 for value in offsets):
        parser.error("Offsets must contain exactly x,y")
    pins = [executable, Path(__file__).resolve(), reference/"report.json", reference/"original-commands.json",
            source_root/"src/renderer.cpp", source_root/"src/native_loading_presentation.cpp",
            source_root/"src/vista_loading_geometry.cpp", source_root/"src/vista_loading.cpp",
            source_root/"include/halo2/cache.hpp", source_root/"tests/native_loading_presentation_capture.cpp",
            source_root/"shaders/vista_loading.vert", source_root/"shaders/vista_loading.frag",
            *sorted(args.assets.resolve().glob("*")),
            executable.parent/"shaders/vista_loading.vert.spv",
            executable.parent/"shaders/vista_loading.frag.spv"]
    initial_pins = {str(path): digest(path) for path in pins if path.is_file()}
    environment = dict(os.environ, SDL_VIDEODRIVER="x11")
    validation = ROOT/".tools/validation/usr"
    if validation.is_dir():
        environment["VK_LAYER_PATH"] = str(validation/"share/vulkan/explicit_layer.d")
        environment["LD_LIBRARY_PATH"] = os.pathsep.join(filter(None, (
            str(validation/"lib"), environment.get("LD_LIBRARY_PATH"))))
        for path in validation.rglob("*"):
            if path.is_file() and (path.suffix == ".json" or ".so" in path.name):
                initial_pins[str(path)] = digest(path)
    results = []
    for offset_index, offset in enumerate(offsets):
        directory = output/f"offset-{offset_index}"
        directory.mkdir()
        for index, case in enumerate(cases):
            if args.case is not None and index not in args.case:
                continue
            record = original["images"][index]
            raw = reference/f"frame-{index:02d}.bgra"
            if digest(raw) != record["raw_sha256"]:
                raise RuntimeError("Original raw framebuffer changed")
            expected = np.frombuffer(raw.read_bytes(), dtype=np.uint8).reshape(case["height"], case["width"], 4)
            expected = expected[:, :, [2, 1, 0]]
            fade = "none"
            if case["active"] and case["draws"][-1]["shader"] == 3:
                draw = case["draws"][-1]
                if draw["shader"] != 3:
                    raise RuntimeError("Active original frame has no final fade draw")
                fade = "0x" + draw["vertices"][0]["words"][6]
            progress = struct.unpack("<I", struct.pack("<f", case["supplied_sweep_progress"]))[0]
            ppm = directory/f"frame-{index:02d}.ppm"
            argv = [str(executable), str(args.assets.resolve()), str(ppm), str(case["width"]),
                    str(case["height"]), str(case["elapsed_ms"]), hex(progress), fade,
                    str(offset[0]), str(offset[1])]
            if "draw_prefix" in case:
                argv.append(str(case["draw_prefix"]))
            process = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     text=True, timeout=90, cwd=ROOT, env=environment)
            (directory/f"frame-{index:02d}.log").write_text(process.stdout)
            if process.returncode or not ppm.is_file():
                raise RuntimeError(f"Native capture {index} failed: {process.stdout}")
            actual = np.array(Image.open(ppm).convert("RGB"))
            png = ppm.with_suffix(".png")
            Image.fromarray(actual).save(png)
            difference = np.abs(actual.astype(np.int16) - expected.astype(np.int16))
            Image.fromarray(np.minimum(difference*16, 255).astype(np.uint8)).save(directory/f"frame-{index:02d}-diff16.png")
            result = dict(index=index, width=case["width"], height=case["height"],
                          elapsed_ms=case["elapsed_ms"], active=case["active"], offset_pixels=offset,
                          argv=argv, returncode=process.returncode,
                          validation_messages=len(re.findall(r"VUID-|Validation Error", process.stdout)),
                          native_ppm_sha256=digest(ppm), native_png_sha256=digest(png),
                          **metrics(expected, actual))
            results.append(result)
            print(f"offset={offset} frame={index:02d} changed={result['changed_pixels']}/{result['total_pixels']} "
                  f"max={result['max_channel_error']} mean={result['mean_channel_error']:.8f}", flush=True)
    changed = [name for name, expected in initial_pins.items() if digest(Path(name)) != expected]
    report = dict(schema="H2V_NATIVE_LOADING_FRAME_COMPARISON_V1", scope=__doc__, pins=initial_pins,
                  source_directory=str(source_root),
                  changed_inputs=changed, frames=results,
                  exact_rgb=bool(results) and all(row["exact_rgb"] for row in results),
                  validation_messages=sum(row["validation_messages"] for row in results))
    (output/"report.json").write_text(json.dumps(report, indent=2)+"\n")
    if changed:
        raise RuntimeError("Capture inputs changed during comparison")
    print(f"Compared {len(results)} actual native frames; exact RGB for all={report['exact_rgb']}: {output}")


if __name__ == "__main__":
    main()
