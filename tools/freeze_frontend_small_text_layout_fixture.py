#!/usr/bin/env python3
"""Export final original glyph corners; atlas offsets remain pointer-free."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]


def word(value):
    return f"{struct.unpack('<I', struct.pack('<f', value))[0]:08x}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path, default=ROOT /
        "analysis/frontend/settings-text-source-audit/glyph-layout-original/original-layout.json")
    parser.add_argument("--output", type=Path, default=ROOT /
        "tests/fixtures/frontend_small_text_layout_original.tsv")
    args = parser.parse_args()
    original_raw = args.original.read_bytes()
    original = json.loads(original_raw)
    assert not original["patched_instructions"]
    origin_u, origin_v = original["supplied_atlas_origin"]
    rows = [
        "# Unchanged original13F0E0/13EF40/13F700/1F490/1F3A0; actual seven screen19 labels.",
        "# Full reveal, alpha1, explicit canonical projected bounds; supplied ready font cache/atlas.",
        "# Original JSON SHA256 " + hashlib.sha256(original_raw).hexdigest(),
        "# C label string_id glyph_count top left bottom right runtime_font scale_word",
        "# G label ordinal codepoint x0_word y0_word x1_word y1_word u0 v0 u1 v1 argb",
        "# U/V are integer offsets from the supplied original glyph atlas origin, not native packed UVs.",
    ]
    count = 0
    for case in original["cases"]:
        rows.append(" ".join(map(str, ["C", case["index"], case["string_id"],
            len(case["glyphs"]), *case["projected_bounds"], case["runtime_font"],
            case["scale_word"]])))
        for index, glyph in enumerate(case["glyphs"]):
            assert len(glyph["quads"]) == 1 and glyph["shadow"] == "00000000"
            vertices = glyph["quads"][0]["vertices"]
            a, b, c, d = vertices
            assert a["x"] == d["x"] and b["x"] == c["x"]
            assert a["y"] == b["y"] and c["y"] == d["y"]
            uv = [a["u"]-origin_u, a["v"]-origin_v,
                  c["u"]-origin_u, c["v"]-origin_v]
            assert all(int(v) == v for v in uv)
            assert all(v["color"] == int(glyph["color"],16) for v in vertices)
            rows.append(" ".join(map(str, ["G", case["index"], index,
                glyph["codepoint"], word(a["x"]), word(a["y"]),
                word(c["x"]), word(c["y"]), *map(int,uv), glyph["color"]])))
            count += 1
    assert len(original["cases"]) == 7 and count == 440
    args.output.write_text("\n".join(rows)+"\n")
    print(json.dumps(dict(cases=7, glyphs=count, path=str(args.output),
        sha256=hashlib.sha256(args.output.read_bytes()).hexdigest()),indent=2))


if __name__ == "__main__":
    main()
