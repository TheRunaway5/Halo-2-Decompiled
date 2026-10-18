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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, default=ROOT / "tools/data/mcc-hud-reference.json")
    parser.add_argument("--output", type=Path, default=ROOT / "src/mcc_hud_reference.inc")
    parser.add_argument("--hash-output", type=Path, default=ROOT / "src/mcc_hud_reference_hashes.inc")
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
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
