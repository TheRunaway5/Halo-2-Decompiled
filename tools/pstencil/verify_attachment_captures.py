#!/usr/bin/env python3
"""Check real game and native-attachment render controls without changing pixels."""
import argparse
import json
from pathlib import Path
import re
import numpy as np
from PIL import Image


def compare(a, b):
    x, y = (np.asarray(Image.open(p)).astype(np.int16) for p in (a, b))
    return dict(brightened_channels=int((x > y).sum()), darkened_channels=int((x < y).sum()),
                changed_pixels=int(np.any(x != y, axis=2).sum()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bundle", type=Path, required=True)
    parser.add_argument("--baseline", type=Path, required=True)
    args = parser.parse_args()
    b = args.bundle
    status = json.loads((b / "gameplay-status.json").read_text())
    assert len(status) == 4 and all(x["exit_code"] == 0 for x in status), status
    assert json.loads((b / "native-attachment-status.json").read_text())["exit_code"] == 0
    result = dict(Original_vs_baseline=compare(b / "game-original/game.ppm", args.baseline / "game-original/game.ppm"),
                  gameplay_shadow=compare(b / "game-pstencil/game.ppm", b / "game-control/game.ppm"), simulation={})
    assert result["Original_vs_baseline"]["changed_pixels"] == 0
    shadow = result["gameplay_shadow"]
    assert shadow["darkened_channels"] > 100 and shadow["brightened_channels"] == 0, shadow
    for name in ("game-original", "game-pstencil", "game-control", "game-third"):
        line = next(x for x in (b / (name + ".log")).read_text().splitlines() if "Input capture tick=45" in x)
        result["simulation"][name] = {k: re.search(r"(?:^| )" + k + r"=([^ ]+)", line)[1] for k in
            ("capture_primary_final", "capture_auxiliary_final", "detonation_tick", "detonation_position", "player_position")}
        Image.open(b / name / "game.ppm").save(b / name / "game.png")
    assert len({json.dumps(result["simulation"][n], sort_keys=True) for n in
                ("game-original", "game-pstencil", "game-control")}) == 1
    # The separate camera has its own auxiliary visibility/effect work.
    for k in ("capture_primary_final", "detonation_tick", "detonation_position", "player_position"):
        assert result["simulation"]["game-third"][k] == result["simulation"]["game-original"][k]
    p = b / "native-plasma-rifle"
    result["native_attachment"] = dict(
        Original_roundtrip=compare(p / "illumination.ppm", p / "Original-restored.ppm"),
        shadow=compare(p / "pStencil-specular.ppm", p / "pStencil-no-shadows-specular.ppm"),
        light=compare(p / "pStencil-specular.ppm", p / "pStencil-no-point-light.ppm"),
        specular=compare(p / "pStencil-specular.ppm", p / "pStencil-diffuse.ppm"),
        independent_caster=compare(p / "pStencil-independent-dynamic-casters.ppm", p / "pStencil-no-dynamic-casters.ppm"))
    native = result["native_attachment"]
    assert native["Original_roundtrip"]["changed_pixels"] == 0
    for key in ("shadow", "independent_caster"):
        assert native[key]["darkened_channels"] > 100 and native[key]["brightened_channels"] == 0, native[key]
    assert native["light"]["changed_pixels"] > 100
    assert native["specular"]["darkened_channels"] == 0
    for name in ("pStencil-specular", "pStencil-no-shadows-specular", "illumination"):
        Image.open(p / (name + ".ppm")).save(p / (name + ".png"))
    (b / "attachment-verification.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "simulation"}, indent=2))


if __name__ == "__main__":
    main()
