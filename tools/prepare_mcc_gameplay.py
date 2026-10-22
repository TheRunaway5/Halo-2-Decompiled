#!/usr/bin/env python3
"""Import installed MCC data and prepare the verified subset of TU1.5 fixes.

Native MCC gameplay admission and full tag conversion remain unsupported. The
published cache/report never changes the working game's asset selection.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys
import uuid

from audit_rendering import FormatError, file_sha
from mcc_content import MccCache, directory, discover, texture_resource
from mcc_conversion_coverage import validate as validate_conversion_coverage
from mcc_multiplayer_catalog import apply_catalog_plan, plan_catalog
from mcc_instant_charge import apply_instant_charge_plan, plan_instant_charge
from mcc_player_screen_flashes import inventory_maps as inventory_player_screen_flashes
from mcc_movies import _cancellation
from vista_tag_conversion import TagGraph, TU15_SHA256, apply_combat, apply_netgame

PROFILE = "H2_MCC_TU15_PREPARATION_V1"
MAX_BITMAP_BYTES = 32 * 1024**3
MAX_BITMAP_PARTITIONS = 65536
BITMAP_DISK_RESERVE = 64 * 1024**2
EXCLUDED_SUFFIXES = {".dll", ".exe", ".xbe", ".iso", ".flt", ".pdb", ".spv", ".cso", ".fxo"}
BLOCKERS = [
    "Native world startup must replace executable-backed mutable runtime banks; immutable TU1.5 engine operands are compiled into the port",
    "Compiled collision/physics and remaining gameplay consumers need native 64-bit metadata support or checked Xbox-ABI conversion; animation/Classic audio adapters do not establish complete semantic parity",
    "All gameplay tag groups need a complete Xbox TU1.5 semantic conversion, beyond the two recovered postprocessors",
    "Changed launch/DLC bitmaps, bump-material parameters and existing shader-pass bindings require map-specific Xbox reference verification",
]


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n", encoding="utf-8")


def excluded(relative):
    return relative.parts[:2] == ("prebuild", "shaders_2_cache") or relative.suffix.lower() in EXCLUDED_SUFFIXES


def source_files(source):
    selected, omitted = [], []
    for path in sorted(source.rglob("*")):
        relative = path.relative_to(source)
        if path.is_symlink():
            raise FormatError(f"Symlink in MCC asset source: {relative}")
        if path.is_dir():
            continue
        if not path.is_file():
            raise FormatError(f"Nonregular MCC asset source: {relative}")
        if excluded(relative):
            omitted.append(relative.as_posix())
        else:
            selected.append((relative, path.stat()))
    if not selected:
        raise FormatError("Empty MCC asset installation")
    return selected, omitted


def source_directories(source):
    """Retain authored relative directories, including empty tags/data trees."""
    result=[]
    for path in sorted(source.rglob("*")):
        relative=path.relative_to(source)
        if path.is_symlink():
            raise FormatError(f"Symlink in MCC asset source: {relative}")
        if path.is_dir() and relative.parts[:2] != ("prebuild", "shaders_2_cache"):
            result.append(relative)
    return result


def copy_verified(source, target, files, progress):
    records = {}
    # Source-format folders retain their relative names/case. In particular a
    # loose-tag donor's tags/ and data/ trees are not content-addressed blobs.
    for relative in source_directories(source):
        (target/relative).mkdir(parents=True,exist_ok=True)
    total = sum(stat.st_size for _, stat in files)
    progress(f"Copying {len(files)} MCC asset files ({total / 1024**3:.2f} GiB) to the private PC-port cache…")
    copied = 0
    next_notice = 1024**3
    for relative, before in files:
        path, output = source / relative, target / relative
        current = path.stat()
        if path.is_symlink() or (current.st_size, current.st_mtime_ns, current.st_ino) != (
                before.st_size, before.st_mtime_ns, before.st_ino):
            raise FormatError(f"MCC source changed during import: {relative}")
        output.parent.mkdir(parents=True, exist_ok=True)
        digest = hashlib.sha256()
        with path.open("rb") as inp, output.open("xb") as out:
            while block := inp.read(1024 * 1024):
                digest.update(block)
                out.write(block)
        after = path.stat()
        if path.is_symlink() or (after.st_size, after.st_mtime_ns, after.st_ino) != (
                before.st_size, before.st_mtime_ns, before.st_ino) or output.stat().st_size != before.st_size:
            raise FormatError(f"MCC source changed during copy: {relative}")
        if file_sha(output) != digest.hexdigest():
            raise FormatError(f"MCC copied asset hash differs: {relative}")
        records[relative.as_posix()] = dict(bytes=before.st_size, sha256=digest.hexdigest())
        copied += before.st_size
        if copied >= next_notice:
            progress(f"Verified {copied / 1024**3:.1f} / {total / 1024**3:.1f} GiB…")
            next_notice = copied + 1024**3
    return records


def save_changed_cache(cache):
    # Publish a valid *uncompressed v13* private map if tag stores changed it.
    # Virtual pointers/resources keep decoded file offsets. No v8 relabeling.
    struct.pack_into("<H", cache.data, 28, cache.u16(28) & ~1)
    struct.pack_into("<4I", cache.data, 776, 0, 0, 0, 0)
    cache.path.write_bytes(cache.data)
    check = MccCache(cache.path)
    if check.data != cache.data:
        raise FormatError("MCC converted cache failed independent reopen")
    check.close()


def bitmap_partitions(cache):
    """Walk bounded bitmap records without decoding their pixel partitions."""
    for tag in cache.tags.values():
        if tag["group"] != "bitm" or tag["offset"] is None:
            continue
        low, high = tag["offset"], tag["offset"] + tag["size"]
        if tag["size"] < 76:
            raise FormatError("Truncated MCC bitmap root")
        for at in cache.block(low + 68, 168):
            if not low <= at <= at + 168 <= high:
                raise FormatError(f"MCC bitmap frame escapes tag: {tag['name']}")
            for partition in range(3):
                pointer, size = cache.u32(at + 56 + partition * 4), cache.u32(at + 80 + partition * 4)
                if size:
                    yield pointer, size


def plan_bitmaps(paths, textures, progress):
    # Decode one partition at a time to measure compressed resources exactly.
    # Retain only sizes/hashes; repeated references are charged once.
    plan, total = {}, 0
    for path in paths:
        progress(f"Checking aggregate MCC bitmap budget: {path.name}")
        cache = MccCache(path)
        try:
            for pointer, size in bitmap_partitions(cache):
                key = f"{pointer & 0x3fffffff:08x}-{size:08x}"
                if key in plan:
                    continue
                if len(plan) >= MAX_BITMAP_PARTITIONS:
                    raise FormatError("Excessive MCC bitmap partition count")
                payload = texture_resource(textures, pointer, size, decoded_limit=MAX_BITMAP_BYTES - total)
                total += len(payload)
                plan[key] = (len(payload), hashlib.sha256(payload).hexdigest())
                del payload
        finally:
            cache.close()
    return plan, total


def prepare_maps(maps, output, progress):
    paths = sorted(maps.glob("*.map"))
    if not paths or not (maps / "shared.map").is_file():
        raise FormatError("MCC maps/shared metadata is incomplete")
    before = {p.name: file_sha(p) for p in paths}
    # Project the two complete, authored scenario catalog records before bitmap
    # extraction so both the metadata and copied-pixel inventories describe the
    # same destination previews. This runs only in the private import stage.
    menu = MccCache(maps / "mainmenu.map")
    try:
        with (maps / "textures.dat").open("rb") as textures:
            catalog_projection = apply_catalog_plan(menu, plan_catalog(menu, maps, textures))
        if catalog_projection["changed_store_blocks"]:
            save_changed_cache(menu)
        catalog_projection["serialized_decoded_sha256"] = hashlib.sha256(menu.data).hexdigest()
    finally:
        menu.close()
    with (maps / "textures.dat").open("rb") as textures:
        bitmap_plan, bitmap_bytes = plan_bitmaps(paths, textures, progress)
    if shutil.disk_usage(output).free < bitmap_bytes + BITMAP_DISK_RESERVE:
        raise FormatError("Insufficient free space for aggregate decoded MCC bitmaps")
    shared = MccCache(maps / "shared.map")
    resources, banks, results, inventory, instant_charge = {}, [], [], [], []
    decoded_bytes = 0
    (output / "bitmap-resources").mkdir()
    try:
        with (maps / "textures.dat").open("rb") as textures:
            for path in paths:
                progress(f"Reading MCC Classic tags, textures and bump resources: {path.name}")
                cache = shared if path.name == "shared.map" else MccCache(path)
                try:
                    stores = list(catalog_projection["stores"]) if path.name == "mainmenu.map" else []
                    charge_plan = plan_instant_charge(cache)
                    charge_projection = apply_instant_charge_plan(cache, charge_plan)
                    instant_charge.append(dict(map=path.name, **charge_projection))
                    if charge_projection["changed_store_blocks"]:
                        tag = cache.tags[charge_plan.datum]
                        stores.append(dict(map=path.name, datum=f'{tag["datum"]:08x}', group="effe",
                            tag=tag["name"], address=tag["address"], file_offset=charge_plan.offset,
                            tag_offset=0, before=charge_plan.before.hex(), after=charge_plan.after.hex(),
                            operation="whole-qualified-original-instant-charge-flags"))
                    inventory.append(dict(map=path.name, groups=dict(Counter(t["group"] for t in cache.tags.values())),
                        resident_tags=sum(bool(t["address"] and t["size"]) for t in cache.tags.values()),
                        tags=[dict(datum=f'{t["datum"]:08x}', group=t["group"], name=t["name"],
                                   address=t["address"], size=t["size"]) for t in cache.tags.values()]))
                    for tag in cache.tags.values():
                        if tag["group"] != "bitm" or tag["offset"] is None:
                            continue
                        low, high = tag["offset"], tag["offset"] + tag["size"]
                        if tag["size"] < 76:
                            raise FormatError("Truncated MCC bitmap root")
                        frames = cache.block(low + 68, 168)
                        row = dict(map=path.name, datum=f'{tag["datum"]:08x}', tag=tag["name"], frames=[])
                        for at in frames:
                            if not low <= at <= at + 168 <= high:
                                raise FormatError(f"MCC bitmap frame escapes tag: {tag['name']}")
                            dimensions = list(struct.unpack_from("<2HB", cache.data, at + 4))
                            frame = dict(dimensions=dimensions, type=cache.u16(at + 10),
                                creation_flags=cache.data[at + 9],
                                format=cache.u16(at + 12), flags=cache.u16(at + 14),
                                additional_mips=cache.u16(at + 20), record_hex=cache.data[at:at+168].hex(), partitions=[])
                            for partition in range(3):
                                pointer, size = cache.u32(at + 56 + partition * 4), cache.u32(at + 80 + partition * 4)
                                if not size:
                                    frame["partitions"].append(None)
                                    continue
                                key = f"{pointer & 0x3fffffff:08x}-{size:08x}"
                                if key not in resources:
                                    if key not in bitmap_plan or len(resources) >= MAX_BITMAP_PARTITIONS:
                                        raise FormatError("MCC bitmap inventory changed after preflight")
                                    expected_size, expected_hash = bitmap_plan[key]
                                    payload = texture_resource(textures, pointer, size,
                                        decoded_limit=min(expected_size, MAX_BITMAP_BYTES - decoded_bytes))
                                    digest = hashlib.sha256(payload).hexdigest()
                                    if len(payload) != expected_size or digest != expected_hash:
                                        raise FormatError("MCC bitmap partition changed after preflight")
                                    if shutil.disk_usage(output).free < len(payload) + BITMAP_DISK_RESERVE:
                                        raise FormatError("Insufficient free space for MCC bitmap output")
                                    relative = f"bitmap-resources/{digest}.bin"
                                    destination = output / relative
                                    if not destination.exists():
                                        destination.write_bytes(payload)
                                    if file_sha(destination) != digest:
                                        raise FormatError("MCC decoded bitmap readback differs")
                                    resources[key] = dict(source_pointer=pointer, stored_bytes=size,
                                        decoded_bytes=len(payload), sha256=digest, path=relative)
                                    decoded_bytes += len(payload)
                                    del payload
                                frame["partitions"].append(key)
                            row["frames"].append(frame)
                        banks.append(row)
                    map_id = None
                    role = "resources"
                    if path.name not in ("shared.map", "single_player_shared.map"):
                        scenario, at = cache.tag(cache.u32(cache.index + 12), "scnr")
                        if scenario["size"] < 18:
                            raise FormatError("Truncated MCC scenario root")
                        role = "multiplayer" if cache.u16(at + 16) == 1 else "campaign-or-menu"
                        if role == "multiplayer":
                            graph = TagGraph(cache, shared)
                            map_id = apply_netgame(graph, scenario["address"])
                            apply_combat(graph, scenario["address"], graph.resolve(cache.u32(cache.index + 16), "matg"))
                            stores.extend(graph.stores)
                    if cache is not shared and any(s["map"] == path.name and s["before"] != s["after"] for s in stores):
                        save_changed_cache(cache)
                    results.append(dict(map=path.name, role=role, map_id=map_id,
                        stores=stores, changes=sum(s["before"] != s["after"] for s in stores)))
                finally:
                    if cache is not shared:
                        cache.close()
        if any(s["map"] == "shared.map" and s["before"] != s["after"] for r in results for s in r["stores"]):
            save_changed_cache(shared)
    finally:
        shared.close()
    write_json(output / "tag-inventory.json", dict(schema="H2_MCC_TAG_INVENTORY_V1", maps=inventory))
    write_json(output / "bitmap-resources.json", dict(schema="H2_MCC_BITMAP_RESOURCES_V1", banks=banks, resources=resources,
        policy="Decoded source partitions with original records, dimensions, formats and mips; no resampling, normal renormalization or channel reinterpretation"))
    report = dict(profile=PROFILE, reference_xbe_sha256=TU15_SHA256,
        reference_xbe_used_as_runtime_input=False,
        original_routines=["0011f060", "00165b30", "001a5590"], full_tu15_parity=False,
        multiplayer_catalog=catalog_projection,
        instant_charge=instant_charge,
        coverage="Recovered multiplayer netgame/combat postprocessors, two source-authored menu catalog projections and one whole-qualified instant-charge effect tuple; no full MCC-to-Xbox layout/semantic conversion",
        maps=[dict(**r, source_sha256=before[r["map"]], output_sha256=file_sha(maps / r["map"])) for r in results])
    write_json(output / "tag-conversion.json", report)
    return dict(maps=len(paths), multiplayer_maps=sum(r["role"] == "multiplayer" for r in results),
        tag_revisions={r["map"]: dict(source_sha256=r["source_sha256"], output_sha256=r["output_sha256"])
                       for r in report["maps"]},
        ordered_stores=sum(len(r["stores"]) for r in results), changes=sum(r["changes"] for r in results),
        bitmap_banks=len(banks), bitmap_frames=sum(len(b["frames"]) for b in banks),
        bitmap_partitions=len(resources), decoded_bitmap_bytes=sum(r["decoded_bytes"] for r in resources.values()),
        multiplayer_catalog=catalog_projection, instant_charge=instant_charge)


@_cancellation()
def prepare(root, mcc_source=None, progress=print, *, presentation=None):
    root = Path(root).resolve()
    import mcc_presentation
    presentation = presentation if presentation is not None else mcc_presentation.requirements(root)
    source = directory(mcc_source) if mcc_source is not None else discover()
    imports = root / "assets/imported"
    # Reject importing from inside our own output or recursively copying it.
    if source == imports or source in imports.parents or imports in source.parents:
        raise FormatError("MCC source and PC-port import directory must be separate")
    files, omitted = source_files(source)
    directories=source_directories(source)
    presentation_bytes = mcc_presentation.required_bytes(root, presentation)
    import controller_prompt_assets
    presentation_bytes += controller_prompt_assets.required_bytes(root)
    from native_variant_assets import source_bundle, install as install_variants
    variant_bytes = sum(map(len, source_bundle(root).values()))
    imports.mkdir(parents=True, exist_ok=True)
    from mcc_ui_layout import required_bytes as ui_layout_bytes, prepare as prepare_ui_layout
    needed = sum(stat.st_size for _, stat in files) + ui_layout_bytes(root) + presentation_bytes + variant_bytes
    if shutil.disk_usage(imports).free < needed + 2 * 1024**3:
        raise FormatError("Insufficient free space for MCC data and decoded bitmap cache")
    mcc_presentation.preflight(root, source, presentation)
    identifier = "mcc-" + uuid.uuid4().hex
    stage, final = imports / (".staging-" + identifier), imports / identifier
    try:
        stage.mkdir()
        progress(f"Found MCC Halo 2: {source}")
        record = copy_verified(source, stage / "halo2", files, progress)
        summary = prepare_maps(stage / "halo2/h2_maps_win64_dx11", stage, progress)
        from mcc_menu_inventory import verify as verify_menu_inventory
        menu_inventory = verify_menu_inventory(stage / "halo2", progress=progress,
            catalog_projection=summary.get("multiplayer_catalog"))
        write_json(stage / "vista-menu-inventory.json", menu_inventory)
        from mcc_menu_assets import prepare as prepare_menu_assets
        progress("Assembling verified MCC menu images and font ownership…")
        menu_assets = prepare_menu_assets(stage, menu_inventory)
        from mcc_loading_assets import prepare as prepare_loading
        progress("Preparing MCC loading resources and retaining authored custom loading textures…")
        loading = prepare_loading(root, stage)
        presentation_report = mcc_presentation.prepare(root, stage, source, presentation, progress=progress)
        presentation_report['controller_prompts']=controller_prompt_assets.prepare(root,stage,progress=progress)
        ui_layout = prepare_ui_layout(root, stage, progress=progress)
        install_variants(root, stage, progress=progress)
        from preserve_native_shaders import copy_selected as preserve_shaders
        progress("Preserving the selected existing native shader library…")
        shaders = preserve_shaders(root, stage)
        from mcc_graphics_corrections import apply_selected as restore_graphics
        progress("Applying verified graphics corrections…")
        graphics = restore_graphics(root, stage, tag_revisions=summary.get("tag_revisions"))
        # Damage flashes are inline jpt! Player Responses, not a shader-name
        # closure. Include shared definitions and exact same-address aliases;
        # a zero-sized directory record does not mean an absent flash.
        flashes = inventory_player_screen_flashes(stage / "halo2/h2_maps_win64_dx11", progress=progress)
        write_json(stage / "player-screen-flashes.json", flashes)
        # Refuse to publish an inconsistent mixed-version snapshot after a Steam
        # update changes the set, size or timestamp of any selected source file.
        now, omitted_now = source_files(source)
        identity = lambda rows: [(p.as_posix(), s.st_size, s.st_mtime_ns, s.st_ino) for p, s in rows]
        if identity(now) != identity(files) or omitted_now != omitted or source_directories(source)!=directories:
            raise FormatError("MCC installation changed during preparation; retry after Steam finishes updating")
        # A successful copy is insufficient: every stock map must retain one
        # source-bound conversion result, even if that map needed no stores.
        # Nested/mod owners stay explicit copied inputs, never implied converted
        # definitions or basename aliases of a stock map.
        conversion = json.loads((stage / "tag-conversion.json").read_text())
        _, conversion_coverage = validate_conversion_coverage(record, conversion)
        report = dict(schema=PROFILE, source=str(source), source_files=record, excluded_files=omitted,
            conversion_coverage=conversion_coverage,
            source_directories=[p.as_posix() for p in directories],
            desired_runtime_role="vanilla Xbox TU1.5 definitions after complete conversion",
            donor_runtime_role="offline import input; never a runtime mod overlay",
            external_installation_requirement="Halo MCC only",
            reference_only_inputs=["OG Xbox Halo 2 ISO", "Xbox TU1.5 patch", "Vista executable loading bank"],
            runtime_active=False, full_tu15_parity=False, summary=summary, blockers=BLOCKERS,
            player_screen_flashes=dict(path="player-screen-flashes.json", complete=flashes["complete"],
                sha256=file_sha(stage / "player-screen-flashes.json"), **flashes["summary"]),
            match_loading=loading,
            **presentation_report,
            native_ui_layout=ui_layout,
            menu_resources=dict(path="ui/mcc-menu-assets.json", complete=menu_assets["complete"],
                frame_owners=menu_assets["frame_owners"], unique_dds_pages=menu_assets["unique_dds_pages"],
                fonts=len(menu_assets["fonts"]), runtime_activation=False),
            vista_menu_inventory=dict(path="vista-menu-inventory.json", complete=menu_inventory["complete"],
                banks=menu_inventory["banks"], frame_owners=menu_inventory["frame_owners"],
                unique_dds_pages=menu_inventory["unique_dds_pages"], fonts=len(menu_inventory["fonts"])),
            native_shader_library=shaders,
            graphics_corrections=graphics,
            policy=dict(assets="Private verified MCC copy", shaders="Native port renderer remains selected; MCC binaries/shader cache excluded",
                prompt_artwork="Port-owned Xbox/PlayStation control rasters per local player; verified MCC fonts retain ordinary text and logical metrics",
                embedded_material_metadata="Retained in original caches for inspection, never admitted as native shader programs",
                visuals="No guessed correction; launch and DLC differences require separate reference checks"))
        if shaders.get("maps"):
            from mcc_native_map_owner_publication import publish_staged_owner_index
            progress("Binding final converted map paths to their retained native libraries…")
            report = publish_staged_owner_index(stage, report)
        write_json(stage / "provenance.json", report)
        mcc_presentation.recheck(presentation)
        stage.rename(final)
        progress(f"Prepared MCC cache: {final}")
        progress("Native MCC playback/full TU1.5 conversion remain incomplete. The active game-file selection was preserved.")
        return final, report
    finally:
        if stage.exists():
            shutil.rmtree(stage)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--mcc-root", type=Path)
    args = parser.parse_args()
    try:
        prepare(args.root, args.mcc_root)
        return 0
    except (OSError, ValueError) as error:
        print(f"MCC preparation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
