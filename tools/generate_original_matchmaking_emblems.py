#!/usr/bin/env python3
"""Retain all96 exact original32px emblem pages for scoped matchmaking views.

Run through run_test.py. The output include must be inside its owned scratch;
publish that qualified source artifact explicitly after checking the receipt.
No pixels are redrawn, resampled or decoded by this generator. Native texture
preparation consumes the original format8/9 swizzled source records directly.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import zlib

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
MAINMENU_SHA256 = "1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b"
BANK_SHA256 = "97b73f3dd00026573f0a0e78e838ec0dcad2f9f3becd98444ef23e1e0c0ddabb"
OWNERS = (("ui\\global_bitmaps\\emblems\\background_small", 32),
          ("ui\\global_bitmaps\\emblems\\foreground_small", 64))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mainmenu", type=Path, default=ROOT / "assets/disc/maps/mainmenu.map")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    owned = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    if not args.output.resolve().is_relative_to(owned):
        raise ValueError("generated emblem include must stay in owned scratch")
    cache = Cache(args.mainmenu)
    rows, raw = [], bytearray()
    try:
        if digest(cache.data) != MAINMENU_SHA256:
            raise ValueError("unrecognized original emblem source")
        shader = next(t for t in cache.tags.values() if t["group"] == "shad" and
                      t["name"] == "ui\\hud\\shaders\\ui_small_emblem")
        _, at = cache.tag(shader["datum"], "shad")
        postprocess = cache.block(at + 32, 124)
        if len(postprocess) != 1:
            raise ValueError("original emblem shader property owner changed")
        bitmaps = cache.block(postprocess[0] + 4, 12)
        if len(bitmaps) != 2:
            raise ValueError("original emblem shader stage owner changed")
        for stage, ((name, count), bitmap) in enumerate(zip(OWNERS, bitmaps)):
            datum, index = cache.u32(bitmap), cache.u32(bitmap + 4)
            tag, base = cache.tag(datum, "bitm")
            if tag["name"] != name or index != 0:
                raise ValueError("original emblem shader bitmap binding changed")
            frames = cache.block(base + 68, 116)
            if len(frames) != count:
                raise ValueError("original emblem page count changed")
            for index, page in enumerate(frames):
                pointer, size = cache.u32(page + 28), cache.u32(page + 52)
                location = pointer >> 30
                if location not in (0, 1):
                    raise ValueError("original emblem page escaped its pinned mainmenu owner")
                offset = pointer & 0x3fffffff
                cache.require(offset, size)
                pixels = cache.data[offset:offset + size]
                record = dict(stage=stage, index=index, width=cache.u16(page + 4),
                              height=cache.u16(page + 6), format=cache.u16(page + 12),
                              flags=cache.u16(page + 14), additional_mips=cache.u16(page + 20),
                              offset=len(raw), bytes=size)
                if (record["width"], record["height"], record["flags"], record["additional_mips"], size) != (32, 32, 4617, 0, 2048) or record["format"] not in (8, 9):
                    raise ValueError("original emblem page descriptor changed")
                rows.append(record)
                raw.extend(pixels)
    finally:
        cache.close()
    if len(raw) != 196608 or digest(raw) != BANK_SHA256:
        raise ValueError("original emblem page bytes changed")
    compressed = zlib.compress(raw, 9)
    if len(compressed) > 16000 or zlib.decompress(compressed) != raw:
        raise ValueError("original emblem compression exceeds finite bound")
    encoded = compressed.hex()
    chunks = [encoded[i:i + 4096] for i in range(0, len(encoded), 4096)]
    lines = ["// Exact pinned original mainmenu32px emblem banks; all96pages, no resampling.",
             f"constexpr std::size_t original_emblem_bytes={len(raw)};",
             f"constexpr std::size_t original_emblem_compressed_bytes={len(compressed)};",
             f'constexpr std::string_view original_emblem_sha256="{BANK_SHA256}";',
             f"constexpr std::array<std::string_view,{len(chunks)}> original_emblem_chunks{{{{"]
    lines.extend(f'    "{chunk}",' for chunk in chunks)
    lines.append("}};")
    lines.append(f"constexpr std::array<OriginalEmblemPage,{len(rows)}> original_emblem_pages{{{{")
    for row in rows:
        lines.append("    OriginalEmblemPage{" + ",".join(str(row[key]) for key in
                     ("stage", "index", "width", "height", "format", "flags", "additional_mips", "offset", "bytes")) + "},")
    lines.append("}};")
    generated = ("\n".join(lines) + "\n").encode()
    if len(generated) > 40000:
        raise ValueError("generated original emblem include exceeds its bound")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(generated)
    report = dict(schema="H2_ORIGINAL_MATCHMAKING_EMBLEM_PAYLOAD_V1", passed=True,
                  mainmenu_sha256=MAINMENU_SHA256, raw_bytes=len(raw), raw_sha256=BANK_SHA256,
                  compressed_bytes=len(compressed), compressed_sha256=digest(compressed),
                  pages=len(rows), background_pages=32, foreground_pages=64,
                  output_bytes=len(generated), output_sha256=digest(generated),
                  generator_sha256=digest(Path(__file__).read_bytes()), scratch=str(owned),
                  source_names=[name for name, _ in OWNERS], large_test_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(f"original emblem bank: {len(rows)} pages, {len(compressed)} compressed bytes")


if __name__ == "__main__":
    main()
