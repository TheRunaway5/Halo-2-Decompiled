#!/usr/bin/env python3
"""Regenerate the read-only native pre-optimization BSP comparison class.

Requires the independently retained source snapshots from the measured run.
Does not edit production files or obtain proprietary executable/map payloads.
"""
from pathlib import Path
import hashlib

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "analysis/multiplayer/integration/render-cpu-profile-20261008"
PINS = {
    "source_bsp_shader_scene_before.cpp": "b643b3bc07bbf1a2da50e34812c7f6943b3d63134948f941f73f6e83c79884a7",
    "source_bsp_shader_scene_before.hpp": "6e40982eff2bd3d522d4c1a1d4fd638f74ef9282f8b372d3a92f073389011263",
}


def main():
    reference = OUTPUT / "reference"
    reference.mkdir(exist_ok=True)
    for name, pin in PINS.items():
        data = (OUTPUT / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != pin:
            raise ValueError(f"Frozen native source snapshot differs: {name}")
        text = data.decode().replace("SourceBspShaderScene", "SourceBspShaderSceneReference")
        text = text.replace('"halo2/source_bsp_shader_scene.hpp"', '"bsp_shader_scene_reference.hpp"')
        extension = "cpp"
        if name.endswith("hpp"):
            extension = "hpp"
            begin = text.index("struct SourceBspShaderView")
            end = text.index("// Resident ordinary")
            text = text[:begin] + text[end:]
            text = text.replace('#include "halo2/source_bsp_admission.hpp"', '#include "halo2/source_bsp_shader_scene.hpp"')
        (reference / f"bsp_shader_scene_reference.{extension}").write_text(text)
    print("Verified both frozen native source hashes; comparison class regenerated.")


if __name__ == "__main__":
    main()
