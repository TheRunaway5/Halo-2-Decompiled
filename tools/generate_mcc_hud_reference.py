#!/usr/bin/env python3
"""Generate the finite scoreboard logical-metadata profile; no asset pixels."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DATA_SHA256 = "1aa51cfbb643358aad49d94d45cb33abac4f687c174e134423b11fe23a83a906"
GEOMETRY_SHA256 = "d79593fecefdb3eea55a0b999502ac8974fb79c43567263debcb9e73f126197e"


def generate(raw):
    if hashlib.sha256(raw).hexdigest() != DATA_SHA256:
        raise ValueError("unrecognized scoreboard metadata authority")
    data = json.loads(raw)
    if data["schema"] != "H2_MCC_SCOREBOARD_LOGICAL_REFERENCE_V1":
        raise ValueError("unsupported scoreboard metadata schema")
    lines = ["// Finite authored logical metadata only; no bitmap/font pixels or shader programs."]
    for field, symbol in (("before", "hud_before"), ("after", "hud_after")):
        payload = json.dumps(data[field], sort_keys=True, separators=(",", ":"))
        lines.append(f'constexpr std::string_view {symbol}=R"H2HUD({payload})H2HUD";')
    patches = data["patches"]
    lines.append(f"constexpr std::array<HudPatch,{len(patches)}> hud_patches{{{{")
    lines.extend("    {" + ",".join(hex(v) + "u" for v in row) + "}," for row in patches)
    lines.append("}};")
    hashes = ["// Entire known allocation guards; source typed names and closures are also exact."]
    for field, symbol in (("preimages", "hud_preimages"), ("postimages", "hud_postimages")):
        values = list(dict.fromkeys(data[field]))
        hashes.append(f"constexpr std::array<std::string_view,{len(values)}> {symbol}" + "{" + ",".join(json.dumps(v) for v in values) + "};")
    return "\n".join(lines) + "\n", "\n".join(hashes) + "\n"


def publish(path, text):
    with tempfile.NamedTemporaryFile(mode="w", dir=path.parent, delete=False) as file:
        pending = Path(file.name)
        file.write(text)
    try:
        os.replace(pending, path)
    finally:
        pending.unlink(missing_ok=True)


def generate_geometry(raw):
    if hashlib.sha256(raw).hexdigest() != GEOMETRY_SHA256:
        raise ValueError("unrecognized HUD geometry metadata authority")
    data = json.loads(raw)
    if data["schema"] != "H2_MCC_NHDT_GEOMETRY_REFERENCE_V1":
        raise ValueError("unsupported HUD geometry metadata schema")
    names = set()
    for profile in data["profiles"]:
        if profile["name"] in names or profile["name"] == "ui\\hud\\scoreboard":
            raise ValueError("duplicate or scoreboard geometry owner")
        names.add(profile["name"])
        if not profile["preimages"] or not profile["postimages"]:
            raise ValueError("incomplete HUD geometry allocation guards")
        for kind, row, effect, function, offset, width, before, after in profile["patches"]:
            offsets = width == 2 and offset % 2 == 0 and (
                kind == 0 and 52 <= offset < 64 or kind == 1 and 64 <= offset < 76)
            registration = kind == 0 and width == 4 and offset % 4 == 0 and 64 <= offset < 88
            if not (offsets or registration) or effect != 0xffffffff or function != 0xffffffff:
                raise ValueError("HUD geometry patch escapes its field domain")
    payload = json.dumps(data["profiles"], sort_keys=True, separators=(",", ":"))
    return ('// Finite complete canonical nhdt geometry metadata only; no pixels/programs.\n'
            'constexpr std::string_view hud_geometry_profiles=R"H2HUDG(' + payload + ')H2HUDG";\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, default=ROOT / "tools/data/mcc-hud-reference.json")
    parser.add_argument("--output", type=Path, default=ROOT / "src/ui/hud/mcc_hud_reference.inc")
    parser.add_argument("--hash-output", type=Path, default=ROOT / "src/ui/hud/mcc_hud_reference_hashes.inc")
    parser.add_argument("--geometry-data", type=Path, default=ROOT / "tools/data/mcc-hud-geometry-reference.json")
    parser.add_argument("--geometry-output", type=Path, default=ROOT / "src/ui/hud/mcc_hud_geometry_reference.inc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        values = generate(args.data.read_bytes())
        for path, value in zip((args.output, args.hash_output), values):
            if args.check:
                if path.read_text() != value:
                    raise ValueError(f"scoreboard reference output is stale: {path.name}")
            else:
                publish(path, value)
        geometry = generate_geometry(args.geometry_data.read_bytes())
        if args.check:
            if args.geometry_output.read_text() != geometry:
                raise ValueError("HUD geometry reference output is stale")
        else:
            publish(args.geometry_output, geometry)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
