#!/usr/bin/env python3
"""Prepare a reviewable Vista gameplay conversion without activating it.

Keeping preparation separate prevents unresolved material, animation-owner and
parity work from replacing a working first-launch selection.
"""
from contextlib import ExitStack
import json
from pathlib import Path
import shutil
import uuid

from audit_rendering import Cache, FormatError, file_sha
from disc import Disc
from setup_assets import (VistaUiCache, XBOX_MAPS, export_pack, extract_vista,
                          inventory, regular, validate_xbox)
from vista_tag_conversion import convert_maps


def texture_bindings(vista_maps, xbox_root):
    """Record exact-path/frame correspondence, retaining original donor bytes.

    The runtime resolves these same exact paths and frame counts. A missing bank
    remains explicit; resampling Vista pixels would not recover Xbox artwork.
    """
    index = {}
    sources = {}
    with ExitStack() as stack:
        # Shared banks first; local map duplicates are checked for correspondence.
        names = ("shared", "single_player_shared", "mainmenu") + tuple(
            n for n in XBOX_MAPS if n not in ("shared", "single_player_shared", "mainmenu"))
        for name in names:
            path = xbox_root / "maps" / (name + ".map")
            cache = Cache(path)
            stack.callback(cache.close)
            sources[path.name] = file_sha(path)
            for tag in cache.tags.values():
                if tag["group"] != "bitm" or not tag["address"] or tag["name"].startswith("ui\\"):
                    continue
                _, at = cache.tag(tag["datum"], "bitm")
                frames = cache.block(at + 68, 116)
                candidate = dict(map=path.name, datum=f"{tag['datum']:08x}", frames=len(frames),
                    dimensions=[[cache.u16(p+4), cache.u16(p+6), cache.u16(p+8)] for p in frames])
                index.setdefault(tag["name"], []).append(candidate)
        rows = []
        for path in sorted(vista_maps.glob("*.map")):
            cache = VistaUiCache(path)
            try:
                for tag in cache.tags.values():
                    if (tag["group"] != "bitm" or not tag["address"] or not tag["size"] or
                            tag["name"].startswith("ui\\")):
                        continue
                    _, at = cache.tag(tag["datum"], "bitm")
                    frames = cache.block(at + 68, 116)
                    candidates = [c for c in index.get(tag["name"], []) if c["frames"] == len(frames)]
                    # Prefer the matching map, then shared sources. Never choose
                    # a different level's baked lighting merely by filename stem.
                    preferred = [c for c in candidates if c["map"] == path.name]
                    if not preferred:
                        preferred = [c for c in candidates if c["map"] in ("shared.map", "single_player_shared.map")]
                    row = dict(vista_map=path.name, vista_datum=f"{tag['datum']:08x}",
                               tag=tag["name"], frames=len(frames))
                    if preferred:
                        row["xbox"] = preferred[0]
                    else:
                        row["unresolved"] = "no exact-path and frame-count Xbox donor in the supplied retail disc"
                    rows.append(row)
            finally:
                cache.close()
    return dict(schema="H2_XBOX_TEXTURE_BINDINGS_V1", runtime_active=False,
        xbox_maps_sha256=sources, matched=sum("xbox" in r for r in rows),
        unresolved=sum("unresolved" in r for r in rows), banks=rows)


def prepare(root, vista_source, xbox_source, progress=print):
    root = Path(root).resolve()
    imports = root / "assets/imported"
    imports.mkdir(parents=True, exist_ok=True)
    identifier = "vista-" + uuid.uuid4().hex
    stage, final = imports / (".staging-" + identifier), imports / identifier
    stage.mkdir()
    try:
        progress("Extracting Vista maps, tags, HUD and menu resources…")
        gameplay = stage / "gameplay"
        vista_record = extract_vista(Path(vista_source), gameplay, progress, gameplay=True)
        for name in XBOX_MAPS:
            regular(gameplay / "maps" / (name + ".map"))
        source = Path(xbox_source).resolve()
        if source.is_file():
            with Disc(source) as disc:
                disc.extract(stage / "xbox")
            xbox = stage / "xbox"
            xbox_record = dict(path=str(source), kind="iso", sha256=file_sha(source))
        else:
            xbox = source.parent if source.name == "maps" else source
            xbox_record = dict(path=str(xbox), kind="extracted-directory")
        validate_xbox(xbox)
        # Relative paths survive publication from the staging directory when
        # the ISO was extracted beside gameplay. External folders stay exact.
        donor = "../xbox" if xbox == stage / "xbox" else str(xbox)
        (gameplay / "asset-donors.tsv").write_text("H2_VISTA_DONORS_V1\nxbox\t"+donor+"\n")
        ui = stage / "vista-ui"
        ui.mkdir()
        progress("Preparing Vista artwork with the current layouts…")
        with ExitStack() as stack:
            caches = []
            for cls, path in ((Cache, xbox / "maps/mainmenu.map"), (VistaUiCache, gameplay / "maps/mainmenu.map"),
                              (Cache, xbox / "maps/shared.map"), (VistaUiCache, gameplay / "maps/shared.map")):
                cache = cls(path)
                stack.callback(cache.close)
                caches.append(cache)
            menu = export_pack(caches[0], caches[1], ui, "pack.tsv", "ui\\")
            hud = export_pack(caches[2], caches[3], ui, "hud.tsv", "ui\\hud\\")
        shutil.copytree(gameplay / "maps/fonts", ui / "fonts")
        (ui / "maps").mkdir()
        shutil.copyfile(gameplay / "maps/mainmenu.map", ui / "maps/mainmenu.map")
        shutil.copytree(ui / "fonts", ui / "maps/fonts")
        progress("Matching original Xbox texture donors…")
        bindings = texture_bindings(gameplay / "maps", xbox)
        (stage / "texture-bindings.json").write_text(json.dumps(bindings, indent=2) + "\n")
        conversion = convert_maps(gameplay / "maps", progress)
        (stage / "tag-conversion.json").write_text(json.dumps(conversion, indent=2) + "\n")
        report = dict(schema="H2_VISTA_GAMEPLAY_PREPARATION_V1", runtime_active=False,
            full_tu15_parity=False,
            runtime_blocker="independent Vulkan material passes, Xbox unit-animation metadata adaptation, remaining resource consumers and full TU1.5 semantic audit",
            policy=dict(maps="vista", tags="vista with verified TU1.5 postprocessing",
                world_textures="xbox donor", ui_pixels_fonts="vista", ui_layout="existing",
                shaders="independently implemented Vulkan; Xbox material behavior reference"),
            vista_source=vista_record, xbox_source=xbox_record,
            xbox_root=str(final / "xbox" if xbox == stage / "xbox" else xbox),
            conversion_profile=conversion["profile"], menu=menu, hud=hud,
            texture_banks_matched=bindings["matched"], texture_banks_unresolved=bindings["unresolved"],
            files=inventory(stage))
        (stage / "provenance.json").write_text(json.dumps(report, indent=2) + "\n")
        stage.rename(final)
        # Deliberately no write to asset-sources.tsv. Activating an incompatible
        # cache would either break launch or quietly keep Xbox gameplay active.
        progress(f"Prepared Vista conversion: {final}")
        progress("Native Vista material/gameplay parity remains pending; the active game-file selection was preserved.")
        return final, report
    finally:
        if stage.exists():
            shutil.rmtree(stage)
