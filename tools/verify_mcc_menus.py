#!/usr/bin/env python3
"""Verify imported menu bytes, complete MCC inventory and native owner mapping."""
import argparse
import json
from pathlib import Path
import struct

from audit_rendering import Cache, file_sha
from export_vista_menu_ui import VistaCache, dds_image, mip_shapes
from import_mcc_hud import MccCache, digest, texture_resource
from import_mcc_menus import menu_banks, identity, focus_page_proofs, focus_subset, PACK
from setup_assets import banks


def verify(pack):
    report = json.loads((pack/"manifest.json").read_text())
    source = Path(report["source"])
    maps = source/"h2_maps_win64_dx11"
    assert {p.name for p in maps.glob("*.map")} == {r["file"] for r in report["maps"]}
    assert file_sha(maps/"textures.dat") == report["textures"]["sha256"]
    exported = {b["name"]:b for b in report["bitmaps"]}
    signatures, effective, pages = {}, {}, 0
    with (maps/"textures.dat").open("rb") as stream:
        for record in report["maps"]:
            path = maps/record["file"]
            assert file_sha(path) == record["sha256"]
            cache = MccCache(path)
            local = menu_banks(cache)
            assert len(local) == record["local_banks"]
            for name, bank in local.items():
                sig = identity(cache,bank)
                if name in signatures:
                    assert signatures[name] == sig
                else:
                    signatures[name] = sig
                    effective[name] = bank["effective"]
                actual = exported[name]
                if actual["source_map"] != path.name:
                    continue
                assert bank["sequences"] == actual["sequences"]
                for at, frame in zip(bank["frames"], actual["frames"], strict=True):
                    assert cache.data[at:at+168].hex() == frame["record_hex"]
                    width, height = struct.unpack_from("<2H",cache.data,at+4)
                    fmt, mips = cache.u16(at+12),cache.u16(at+20)
                    levels = mip_shapes(width,height,mips,fmt)
                    payload, resource = texture_resource(stream,cache,at,sum(m["bytes"] for m in levels))
                    assert json.loads(json.dumps(resource)) == frame["resource"]
                    expected, _ = dds_image(width,height,mips,fmt,payload)
                    actual_dds = (pack/frame["dds"]).read_bytes()
                    assert actual_dds == expected and digest(actual_dds) == frame["dds_sha256"]
                    pages += 1
            del cache
    assert set(exported) == set(signatures)
    proofs=focus_page_proofs(Path(report["owners"]["vista.tsv"]["source"]).parent.parent,exported,pack)
    assert proofs==report["focus_page_proofs"]
    for filename, owner in report["owners"].items():
        source_cache = Path(owner["source"])
        assert file_sha(source_cache) == owner["source_sha256"]
        cache = (Cache if filename == "pack.tsv" else VistaCache)(source_cache)
        try:
            original = banks(cache,"ui\\")
            lines = [PACK]
            for row in owner["mapping"]:
                name, ordinal = row["name"],row["frame"]
                frames, seqs = original[name]
                bank = exported[name]
                assert seqs == effective[name]
                subset=focus_subset(name,frames,seqs,dict(frames=bank["frames"],effective=effective[name]),bank,proofs,cache)
                assert len(frames)==len(bank["frames"]) or filename=="pack.tsv" and subset
                assert row["ownership"]==("verified normal/selected subset" if subset else "exact bank")
                assert row["logical"] == [cache.u16(frames[ordinal]+4),cache.u16(frames[ordinal]+6)]
                frame = bank["frames"][ordinal]
                assert row["source_frame"] == ordinal and row["physical"] == [frame["width"],frame["height"]]
                lines.append("\t".join((name,str(ordinal),str(len(frames)),*(str(n) for n in row["logical"]),frame["dds"])))
            assert (pack/filename).read_text() == "\n".join(lines)+"\n"
        finally:
            cache.close()
    for name, record in report["fonts"].items():
        original, imported = (source/"h2_fonts"/name).read_bytes(),(pack/"fonts"/name).read_bytes()
        assert original == imported and len(imported) == record["bytes"] and digest(imported) == record["sha256"]
    return dict(passed=True, maps=len(report["maps"]), banks=len(exported), pages=pages,
        font_files=report["summary"]["font_files"],font_tables=report["summary"]["font_tables"],
        owners={n:dict(banks=o["banks"],pages=o["pages"],fallback_banks=len(o["retained_current"])) for n,o in report["owners"].items()},
        manifest_sha256=file_sha(pack/"manifest.json"),
        claim="Complete Classic menu bitmap inventory; byte-identical DDS payloads/mips and fonts; exact native page/sprite ownership")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack",type=Path)
    parser.add_argument("--output",type=Path)
    args=parser.parse_args()
    result=json.dumps(verify(args.pack),indent=2)+"\n"
    if args.output:
        args.output.write_text(result)
    print(result,end="")


if __name__ == "__main__":
    main()
