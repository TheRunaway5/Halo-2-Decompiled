#!/usr/bin/env python3
"""Import installed MCC Halo 2 Classic menu bitmaps/fonts without resampling.

Exact tag names, page counts and effective sprite ownership bind each native
Xbox/Vista menu owner. Missing or incompatible banks retain their current art.
All available menu banks are retained, including currently unreferenced pages.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import tempfile
import uuid

from audit_rendering import Cache, FormatError, file_sha
from export_vista_menu_ui import VistaCache, sequences, dds_image, mip_shapes, expand_resource
from import_mcc_hud import (ROOT, HUD, MccCache, discover, mcc_directory,
    export_bank, import_fonts, selected_xbox_maps, selected_menu_assets)
from setup_assets import banks, effective_sequences, resource

PACK = "H2UI_MENU_SAMPLING_PACK_V1"
MARKER = "H2_MCC_CLASSIC_MENUS_V1\n"
FOCUS_BANKS = {"ui\\screens\\game_shell\\virtual_keyboard\\button_"+n for n in ("1","2","4","5","6")} | {
    "ui\\screens\\game_shell\\pregame_lobby\\button"}


def focus_page_proofs(vista_assets, exports, output):
    """PC63FF21: normal0, selected1, hovered2; PC63ADB1: latched1.

    The supported 11081 source getter and Xbox keyboard counterpart are
    retained in the source audit. Prove MCC has these exact Vista pages,
    rather than assuming that ordinal correspondence survives a new bank.
    """
    cache = VistaCache(vista_assets/"maps/mainmenu.map")
    proof = {}
    try:
        original = banks(cache)
        for name in sorted(FOCUS_BANKS):
            if name not in original or name not in exports:
                continue
            frames, effective = original[name]
            sampled = exports[name]["frames"]
            if len(frames) != 3 or len(sampled) != 3 or effective:
                continue
            for p, q in zip(frames, sampled):
                w,h,fmt,mips = (cache.u16(p+i) for i in (4,6,12,20))
                levels = mip_shapes(w,h,mips,fmt)
                payload = expand_resource(resource(cache,p),sum(l["bytes"] for l in levels))
                dds,_ = dds_image(w,h,mips,fmt,payload)
                if dds != (output/q["dds"]).read_bytes():
                    break
            else:
                proof[name] = dict(roles=["normal","selected/latched","mouse hover"],
                    vista_dds_identity=True, selected_source_frames=[0,1])
    finally:
        cache.close()
    return proof


def focus_subset(name, frames, effective, bank, exported, proofs, cache):
    return (name in proofs and len(frames)==2 and len(bank["frames"])==3 and
        not effective and not bank["effective"] and all(
            [q["width"],q["height"]]==[cache.u16(p+4)*2,cache.u16(p+6)*2]
            for p,q in zip(frames,exported["frames"][:2])))


def menu_banks(cache):
    result = {}
    for name, (frames, effective) in banks(cache, "ui\\").items():
        if name.startswith(HUD):
            continue
        tag = next(t for t in cache.tags.values() if t["name"] == name and t["group"] == "bitm")
        result[name] = dict(tag=tag, frames=frames, effective=effective,
            sequences=sequences(cache, tag["offset"], len(frames)))
    return result


def identity(cache, bank):
    # Compare authored sprite owners and actual textures.dat resource identity,
    # excluding map-relative metadata pointers and transient runtime handles.
    return (bank["effective"], [bytes(cache.data[p+4:p+28]) +
        bytes(cache.data[p+56:p+68]) + bytes(cache.data[p+80:p+92]) for p in bank["frames"]])


def mapping(cache, donor, exports, output, filename, proofs):
    rows, admitted, fallback = [PACK], [], {}
    for name, (frames, effective) in sorted(banks(cache, "ui\\").items()):
        if name.startswith(HUD):
            continue
        bank = donor.get(name)
        subset = bool(bank and filename=="pack.tsv" and
            focus_subset(name,frames,effective,bank,exports[name],proofs,cache))
        reason = ("absent from MCC Classic" if bank is None else
            "page count differs" if len(frames) != len(bank["frames"]) and not subset else
            "sprite ownership differs" if effective != bank["effective"] else
            "empty source bank" if not frames else None)
        if reason:
            fallback[name] = reason
            continue
        for ordinal, at in enumerate(frames):
            frame = exports[name]["frames"][ordinal]
            logical = [cache.u16(at+4), cache.u16(at+6)]
            rows.append("\t".join((name, str(ordinal), str(len(frames)),
                *(str(n) for n in logical), frame["dds"])))
            admitted.append(dict(name=name, frame=ordinal, logical=logical,
                physical=[frame["width"], frame["height"]], source_frame=ordinal,
                ownership="verified normal/selected subset" if subset else "exact bank"))
    (output/filename).write_text("\n".join(rows)+"\n")
    return dict(source=str(cache.path), source_sha256=file_sha(cache.path),
        banks=len({r["name"] for r in admitted}), pages=len(admitted),
        mapping=admitted, retained_current=fallback)


def export(mcc, xbox_maps, vista_assets, output, progress=print):
    maps = mcc/"h2_maps_win64_dx11"
    output.mkdir(parents=True)
    donor, identities, exports, scanned = {}, {}, {}, []
    # Prefer the menu owner for provenance; verify every other Classic cache.
    paths = [maps/"mainmenu.map"] + sorted(p for p in maps.glob("*.map") if p.name != "mainmenu.map")
    with (maps/"textures.dat").open("rb") as stream:
        for path in paths:
            progress("Reading MCC Classic menu artwork: "+path.name)
            cache = MccCache(path)
            local = menu_banks(cache)
            scanned.append(dict(file=path.name, sha256=cache.provenance["sha256"], local_banks=len(local)))
            for name, bank in sorted(local.items()):
                signature = identity(cache, bank)
                if name in identities:
                    if identities[name] != signature:
                        raise FormatError("Conflicting map-specific MCC menu bitmap: "+path.name+": "+name)
                else:
                    identities[name] = signature
                    donor[name] = dict(frames=bank["frames"], effective=bank["effective"])
                    exports[name] = export_bank(cache, bank, stream, output)
            del cache
    proofs = focus_page_proofs(vista_assets,exports,output)
    owners = {}
    for filename, source, reader in (("pack.tsv", xbox_maps/"mainmenu.map", Cache),
            ("vista.tsv", vista_assets/"maps/mainmenu.map", VistaCache),
            ("pause.tsv", vista_assets/"maps/01a_tutorial.map", VistaCache)):
        cache = reader(source)
        try:
            owners[filename] = mapping(cache, donor, exports, output, filename,proofs)
        finally:
            cache.close()
    fonts = import_fonts(mcc/"h2_fonts", output)
    (output/"menu-source.tsv").write_text(MARKER)
    (output/"menu-ui-source.tsv").write_text("H2_HUD_MENU_SOURCE_V1\nvista\t"+str(vista_assets.resolve())+"\n")
    report = dict(schema="halo2-mcc-classic-menu-import-v1", complete=True,
        importer_sha256=file_sha(Path(__file__)), source=str(mcc), maps=scanned,
        textures=dict(path=str(maps/"textures.dat"), bytes=(maps/"textures.dat").stat().st_size,
            sha256=file_sha(maps/"textures.dat")), bitmaps=list(exports.values()), fonts=fonts,
        owners=owners, focus_page_proofs=proofs,
        page_role_source=dict(path="multiplayer/docs/online-vista-assets.md",
            sha256=file_sha(ROOT/"multiplayer/docs/online-vista-assets.md"),
            base_getter="PC63FF21",keyboard_getter="PC63ADB1",xbox_keyboard="0x2377f6"),
        summary=dict(maps=len(scanned), imported_banks=len(exports),
            imported_pages=sum(len(b["frames"]) for b in exports.values()),
            font_files=sum(not n.startswith("font_table") for n in fonts),
            font_tables=sum(n.startswith("font_table") for n in fonts)),
        policy=dict(pixels="original MCC payloads and all authored mips", fonts="byte-identical MCC files and tables",
            image_generation=False, artwork_resampling=False, logical_layout="existing original menu owners",
            unmatched="retain existing source art; no guessed page remapping", gameplay="unchanged"))
    (output/"manifest.json").write_text(json.dumps(report, indent=2)+"\n")
    return report


def activate(root, output):
    path = str(output.resolve())
    if any(c in path for c in "\t\r\n\0"):
        raise FormatError("Menu source path contains control characters")
    registry = root/"assets/menu-source.tsv"
    temporary = registry.with_name(".menu-source-"+uuid.uuid4().hex)
    temporary.write_text("H2_MENU_SOURCE_V1\nmcc\t"+path+"\n")
    os.replace(temporary, registry)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--mcc-root", type=Path)
    parser.add_argument("--xbox-maps", type=Path)
    parser.add_argument("--vista-assets", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--activate", action="store_true")
    args = parser.parse_args()
    root = args.root.resolve()
    mcc = mcc_directory(args.mcc_root) if args.mcc_root else discover()
    output = (args.output or root/"assets/imported"/("mcc-menus-"+uuid.uuid4().hex)).resolve()
    if output.exists():
        parser.error("Output already exists; select a fresh directory")
    vista = args.vista_assets or selected_menu_assets(root)
    if vista is None:
        parser.error("Import the original menu sources first or pass --vista-assets")
    output.parent.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix=".mcc-menus-", dir=output.parent))
    try:
        report = export(mcc, args.xbox_maps or selected_xbox_maps(root), vista, stage/"pack")
        (stage/"pack").rename(output)
        if args.activate:
            activate(root, output)
        print(json.dumps(report["summary"], indent=2))
        for name, owner in report["owners"].items():
            print(name, owner["banks"], "banks", owner["pages"], "pages;", len(owner["retained_current"]), "retained current")
        print("Imported: "+str(output))
    finally:
        shutil.rmtree(stage)


if __name__ == "__main__":
    main()
