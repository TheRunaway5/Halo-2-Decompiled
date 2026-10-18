"""Measure actual same-frame campaign light/shadow capture controls.

Input PPMs come directly from the native renderer. This tool does not alter
their pixels. Counts distinguish positive darkening, brightening and unchanged
pixels; submission logs alone are not visual acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--directory", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()


def pixels(path):
    with Image.open(path) as image:
        return np.asarray(image.convert("RGB"), dtype=np.int16)


def compare(actual, control):
    if actual.shape != control.shape:
        raise ValueError("Native paired captures differ in dimensions")
    difference = actual - control
    return dict(
        changed_pixels=int(np.any(difference != 0, axis=2).sum()),
        darker_pixels=int(np.any(difference < 0, axis=2).sum()),
        brighter_pixels=int(np.any(difference > 0, axis=2).sum()),
        darker_channels=int((difference < 0).sum()),
        brighter_channels=int((difference > 0).sum()),
        minimum_channel_change=int(difference.min()),
        maximum_channel_change=int(difference.max()),
    )


rows = []
for path in sorted(args.directory.glob("tick-*-target-*.ppm")):
    if not path.stem.rsplit("-", 1)[-1].isdigit():
        continue
    lights = path.with_name(path.stem + "-without-projected-lights.ppm")
    shadows = path.with_name(path.stem + "-without-point-shadows.ppm")
    if not lights.is_file() or not shadows.is_file():
        rows.append(dict(image=path.name, complete_pair=False))
        continue
    actual, unlit, unshadowed = map(pixels, (path, lights, shadows))
    rows.append(dict(
        image=path.name,
        complete_pair=True,
        width=int(actual.shape[1]),
        height=int(actual.shape[0]),
        projected_lights=compare(actual, unlit),
        point_shadows=compare(actual, unshadowed),
        inputs={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                for p in (path, lights, shadows)},
    ))
material_rows = []
for control in sorted(args.directory.glob("tick-*-full-materials.ppm")):
    actual = control.with_name(control.name.replace("-full-materials", ""))
    complete = actual.is_file()
    row = dict(image=actual.name, complete_pair=complete)
    if complete:
        row.update(full_materials_exact=actual.read_bytes() == control.read_bytes(),
                   inputs={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in (actual, control)})
    material_rows.append(row)
mode_rows = []
for before in sorted(args.directory.glob("tick-*-Original-before.ppm")):
    restored = before.with_name(before.name.replace("-Original-before", "-Original-restored"))
    pstencil = before.with_name(before.name.replace("-Original-before", ""))
    complete = restored.is_file() and pstencil.is_file()
    row = dict(image=before.name, complete_pair=complete)
    if complete:
        original = before.read_bytes()
        row.update(restored_exact=original == restored.read_bytes(),
                   pstencil_changed=original != pstencil.read_bytes(),
                   original_sha256=hashlib.sha256(original).hexdigest())
    mode_rows.append(row)

result = dict(scope=__doc__, captures=rows, mode_checks=mode_rows,
              material_checks=material_rows,
              full_material_restore_pairs=sum(r.get("full_materials_exact", False) for r in material_rows),
              original_restore_pairs=sum(r.get("restored_exact", False) for r in mode_rows),
              pstencil_changed_pairs=sum(r.get("pstencil_changed", False) for r in mode_rows),
              positive_light_pairs=sum(r.get("projected_lights", {}).get("brighter_pixels", 0) > 0 for r in rows),
              positive_shadow_pairs=sum(r.get("point_shadows", {}).get("darker_pixels", 0) > 0 for r in rows),
              shadow_pairs_with_brightening=sum(r.get("point_shadows", {}).get("brighter_pixels", 0) > 0 for r in rows))
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps({k: v for k, v in result.items()
                  if k not in ("scope", "captures", "mode_checks", "material_checks")}))
