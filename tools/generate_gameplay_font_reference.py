#!/usr/bin/env python3
"""Emit finite, source-authenticated font layout metadata; no font pixel data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DATA_SHA256 = "11acb229ef47207f7b4584cc225cacc1a582026d4706802494a3aa73ac7184a1"


def generate(raw):
    if hashlib.sha256(raw).hexdigest() != DATA_SHA256:
        raise ValueError("unrecognized original/MCC font metadata authority")
    data = json.loads(raw)
    if data["schema"] != "H2_MCC_GAMEPLAY_FONT_LAYOUT_V1":
        raise ValueError("unsupported gameplay font metadata")
    output = [
        "// Authored layout metadata only. No original font pixels or compressed streams.",
        "// Input identities and extraction receipt: .local/mcc-gameplay-font-source-20261017/v1/source-authority.json",
        "constexpr std::array<std::string_view,12> mcc_names{" + ",".join(json.dumps(n) for n in data["mcc_names"]) + "};",
        "constexpr std::array<std::string_view,11> original_names{" + ",".join(json.dumps(n) for n in data["original_names"]) + "};",
        "constexpr std::array mcc_files{",
    ]
    for name, row in data["mcc_files"].items():
        output.append(f'    FontFile{{"{name}",{row["bytes"]},"{row["sha256"]}",0x{row["fnv64"]}ull}},')
    output.append("};")
    for index, profile in enumerate(data["profiles"]):
        for kind, field, suffix in (("FontLookup", "lookup", "lookup"), ("FontMetric", "glyph_metrics", "metrics"), ("FontKerning", "kerning", "kerning")):
            rows = profile[field]
            output.append(f"constexpr std::array<{kind},{len(rows)}> font_{index}_{suffix}{{{{")
            output.extend("    {" + ",".join(str(value) for value in row) + "}," for row in rows)
            output.append("}};")
    output.append("constexpr std::array profiles{")
    for index, profile in enumerate(data["profiles"]):
        ascent, descent, leading, horizontal = profile["metrics"]
        output.append(f'    FontProfile{{"{profile["name"]}","{profile["source_sha256"]}",0x{profile["source_fnv64"]}ull,{ascent},{descent},{leading},{horizontal},{profile["default_record"]},font_{index}_lookup,font_{index}_metrics,font_{index}_kerning}},')
    output.append("};")
    return "\n".join(output) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=Path, default=ROOT / "tools/data/gameplay-font-reference.json")
    parser.add_argument("--output", type=Path, default=ROOT / "src/ui/gameplay_font_reference.inc")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    try:
        result = generate(args.data.read_bytes())
        if args.check:
            if args.output.read_text() != result:
                raise ValueError("gameplay font reference output is stale")
        else:
            with tempfile.NamedTemporaryFile(mode="w", dir=args.output.parent, delete=False) as file:
                pending = Path(file.name)
                file.write(result)
            try:
                os.replace(pending, args.output)
            finally:
                pending.unlink(missing_ok=True)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
