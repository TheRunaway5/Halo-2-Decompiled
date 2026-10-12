#!/usr/bin/env python3
"""Reopen MCC source records/resources and prove imported byte identity."""
import argparse
import json
from pathlib import Path
import struct

from audit_rendering import file_sha
from export_vista_menu_ui import dds_image, mip_shapes
from import_mcc_hud import MccCache, bitmap_banks, digest, texture_resource


def verify(pack):
    manifest = json.loads((pack / "manifest.json").read_text())
    source = Path(manifest["source"])
    maps = source / "h2_maps_win64_dx11"
    assert {p.name for p in maps.glob("*.map")} == {m["file"] for m in manifest["maps"]}
    for record in manifest["maps"]:
        assert file_sha(maps / record["file"]) == record["sha256"]
    assert file_sha(maps / "textures.dat") == manifest["textures"]["sha256"]
    cache = MccCache(maps / "shared.map")
    banks = bitmap_banks(cache)
    assert set(banks) == {b["name"] for b in manifest["bitmaps"]}
    pages = 0
    with (maps / "textures.dat").open("rb") as stream:
        for bank in manifest["bitmaps"]:
            actual = banks[bank["name"]]
            assert len(actual["frames"]) == len(bank["frames"])
            assert actual["sequences"] == bank["sequences"]
            for frame, at in zip(bank["frames"], actual["frames"]):
                assert cache.data[at:at+168].hex() == frame["record_hex"]
                width, height = struct.unpack_from("<2H", cache.data, at+4)
                fmt, mips = cache.u16(at+12), cache.u16(at+20)
                levels = mip_shapes(width, height, mips, fmt)
                payload, resource = texture_resource(stream, cache, at, sum(m["bytes"] for m in levels))
                assert json.loads(json.dumps(resource)) == frame["resource"]
                expected, _ = dds_image(width, height, mips, fmt, payload)
                dds = (pack / frame["dds"]).read_bytes()
                assert dds == expected and digest(dds) == frame["dds_sha256"]
                assert dds[128:] == payload
                pages += 1
    source_fonts = source / "h2_fonts"
    assert {p.name for p in source_fonts.iterdir() if p.is_file()} == set(manifest["fonts"])
    for name, record in manifest["fonts"].items():
        original, imported = (source_fonts/name).read_bytes(), (pack/"fonts"/name).read_bytes()
        assert original == imported and len(imported) == record["bytes"] and digest(imported) == record["sha256"]
    return dict(passed=True, maps=len(manifest["maps"]), banks=len(banks), pages=pages,
        font_files=manifest["summary"]["font_files"], font_tables=manifest["summary"]["font_tables"],
        manifest_sha256=file_sha(pack/"manifest.json"),
        claim="Every imported DDS payload/mip and font file equals the currently installed MCC Classic source bytes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    report = verify(args.pack)
    encoded = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded)
    print(encoded, end="")


if __name__ == "__main__":
    main()
