#!/usr/bin/env python3
"""Clone an existing prepared MCC cache and apply only qualified data deltas.

This never imports an installation, extracts the complete bitmap catalogue,
selects a cache, writes runtime capabilities, or starts the game. Linux reflinks
are mandatory: an unsupported filesystem fails rather than falling back to
hardlinks or a second full asset copy. The original prepared cache is retained.
"""
from __future__ import annotations

import argparse
from collections import Counter
import copy
import csv
import ctypes
import fcntl
import json
import os
from pathlib import Path
import shutil
import stat
import struct
import sys
import tempfile

from audit_rendering import FormatError, file_sha
from export_vista_menu_ui import dds_image
from mcc_content import MccCache
from mcc_instant_charge import apply_instant_charge_plan, plan_instant_charge
from mcc_menu_assets import _owned_file
from mcc_menu_catalog_inventory import PAGES, projected_page, qualified_pages
from mcc_menu_inventory import INVENTORY
from mcc_movies import _cancellation
from mcc_multiplayer_catalog import (apply_catalog_plan, bitmap, digest,
                                     plan_catalog)
from prepare_mcc_gameplay import PROFILE, save_changed_cache, write_json

SCHEMA = "H2_MCC_PREPARED_CACHE_DELTA_V1"
FICLONE = 0x40049409
MAPS = Path("halo2/h2_maps_win64_dx11")


def require(condition, message):
    if not condition:
        raise FormatError("MCC prepared-cache delta: " + message)


def _identity(st):
    return (st.st_dev, st.st_ino, st.st_mode, st.st_nlink, st.st_size,
            st.st_mtime_ns, st.st_ctime_ns)


def _directory(path):
    path = Path(path).absolute()
    require(not path.is_symlink() and path.is_dir(), "directory is absent or symbolic: " + str(path))
    require(all(not p.is_symlink() for p in (path, *path.parents)),
            "directory has a symbolic ancestor: " + str(path))
    return path.resolve(strict=True)


def _snapshot(root):
    """No content scan of the 36GB cache; preserve every actual inode owner."""
    entries = {".": _identity(root.lstat())}
    for path in sorted(root.rglob("*")):
        info = path.lstat()
        require(stat.S_ISDIR(info.st_mode) or stat.S_ISREG(info.st_mode),
                "nonregular/symbolic cache entry: " + str(path))
        if stat.S_ISREG(info.st_mode):
            require(info.st_nlink == 1, "cache file has a shared hardlink owner: " + str(path))
        entries[path.relative_to(root).as_posix()] = _identity(info)
    return entries


def _snapshot_sha(entries):
    return digest(json.dumps(entries, sort_keys=True, separators=(",", ":")).encode())


def _clone_tree(source, stage, snapshot):
    files, logical_bytes = 0, 0
    for name, identity in snapshot.items():
        if name == ".":
            continue
        target = stage / name
        if stat.S_ISDIR(identity[2]):
            target.mkdir(parents=True, exist_ok=True)
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        flags = os.O_RDONLY | os.O_NOFOLLOW
        with os.fdopen(os.open(source / name, flags), "rb") as inp:
            require(_identity(os.fstat(inp.fileno())) == identity,
                    "source changed before reflink: " + name)
            with target.open("xb") as out:
                fcntl.ioctl(out.fileno(), FICLONE, inp.fileno())
                clone = os.fstat(out.fileno())
                require(clone.st_dev == identity[0] and clone.st_ino != identity[1] and
                        clone.st_nlink == 1 and clone.st_size == identity[4],
                        "reflink did not create a distinct private file: " + name)
            require(_identity(os.fstat(inp.fileno())) == identity and
                    _identity((source / name).lstat()) == identity,
                    "source changed during reflink: " + name)
        os.chmod(target, stat.S_IMODE(identity[2]))
        os.utime(target, ns=(identity[5], identity[5]))
        files += 1
        logical_bytes += identity[4]
    return dict(files=files, logical_bytes=logical_bytes, copy_method="Linux FICLONE",
                distinct_inodes=True, hardlinks=False, symlinks=False)


def _publish(stage, destination):
    # rename(2) may replace an existing empty directory. RENAME_NOREPLACE also
    # closes that final race and leaves any concurrent destination untouched.
    libc = ctypes.CDLL(None, use_errno=True)
    rename = libc.renameat2
    rename.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_int,
                       ctypes.c_char_p, ctypes.c_uint)
    rename.restype = ctypes.c_int
    if rename(-100, os.fsencode(stage), -100, os.fsencode(destination), 1):
        error = ctypes.get_errno()
        raise OSError(error, os.strerror(error), str(destination))


def _transaction(source, destination, convert, progress=lambda message: None):
    """Own the unpublished clone; an exception can never alter an existing cache."""
    source = _directory(source)
    raw = Path(destination).absolute()
    require(raw.name not in ("", ".", ".."), "invalid destination")
    parent = _directory(raw.parent)
    destination = parent / raw.name
    require(not destination.exists() and not destination.is_symlink(), "destination already exists")
    require(destination != source and not destination.is_relative_to(source) and
            not source.is_relative_to(destination), "source and destination overlap")
    require(parent.stat().st_dev == source.stat().st_dev,
            "source and derivative must share a reflink-capable filesystem")
    before = _snapshot(source)
    stage = Path(tempfile.mkdtemp(prefix=".mcc-delta-", dir=parent))
    owner = stage.stat().st_ino, stage.stat().st_dev
    try:
        progress("Reflinking the retained prepared MCC cache; no full import or extraction")
        cloned = _clone_tree(source, stage, before)
        result = convert(stage, source)
        require(_snapshot(source) == before, "original prepared cache changed during derivation")
        # Reject accidental links introduced by the converter as well.
        final = _snapshot(stage)
        result.update(clone=cloned, source_tree_snapshot_sha256=_snapshot_sha(before),
                      output_files=sum(stat.S_ISREG(v[2]) for v in final.values()),
                      output_logical_bytes=sum(v[4] for v in final.values() if stat.S_ISREG(v[2])),
                      original_cache_unchanged=True)
        require((stage.stat().st_ino, stage.stat().st_dev) == owner, "stage owner changed")
        _publish(stage, destination)
        return result
    finally:
        if stage.exists():
            require(not stage.is_symlink() and (stage.stat().st_ino, stage.stat().st_dev) == owner,
                    "refusing cleanup after stage owner replacement: " + str(stage))
            shutil.rmtree(stage)


def _json(pack, name, schema=None):
    path = _owned_file(pack, name)
    value = json.loads(path.read_text())
    if schema is not None:
        require(value.get("schema", value.get("profile")) == schema, "unsupported " + name)
    return value


def _validate_inherited_resources(pack, bitmap_index, inventory, assets):
    """Check complete inherited ownership without decoding or rebuilding pages.

    Kernel reflinks retain the unchanged bytes. Content-addressed filenames,
    recorded sizes and complete private paths bind those copied resources to
    their existing prepared inventories; the two changed pages are separately
    rehashed against the authenticated projection below.
    """
    for resource in bitmap_index["resources"].values():
        expected = "bitmap-resources/" + resource["sha256"] + ".bin"
        require(resource["path"] == expected, "inherited bitmap resource lost content identity")
        require(_owned_file(pack, expected).stat().st_size == resource["decoded_bytes"],
                "inherited bitmap resource is absent or truncated")
    pages = {(p["cache"], p["name"], p["datum"], p["frame"]): p for p in inventory["pages"]}
    require(len(pages) == len(inventory["pages"]), "duplicate inherited menu page identity")
    seen = set()
    for asset in assets["pages"]:
        key = (asset["cache"], asset["name"], asset["datum"], asset["frame"])
        require(key in pages and key not in seen, "inherited assembled menu page identity differs")
        seen.add(key)
        extra = {"path", "bytes", "source_record_hex", "source_partition"}
        require({k: v for k, v in asset.items() if k not in extra} == pages[key],
                "inherited assembled menu page differs from its inventory")
        expected = "ui/bitmaps/" + asset["dds_sha256"] + ".dds"
        require(asset["path"] == expected and _owned_file(pack, expected).stat().st_size == asset["bytes"],
                "inherited menu DDS lost its complete copied owner")
    require(seen == set(pages), "inherited menu resource catalogue is partial")
    require(assets["fonts"] == {"halo2/" + k: v for k, v in inventory["fonts"].items()},
            "inherited menu font catalogue differs")
    for name, record in assets["fonts"].items():
        require(_owned_file(pack, name).stat().st_size == record["bytes"],
                "inherited menu font is absent or truncated")


def _one(rows, predicate, message):
    selected = [r for r in rows if predicate(r)]
    require(len(selected) == 1, message)
    return selected[0]


def _frame(raw):
    pointers = struct.unpack_from("<3I", raw, 56)
    sizes = struct.unpack_from("<3I", raw, 80)
    return dict(dimensions=list(struct.unpack_from("<2HB", raw, 4)),
                type=struct.unpack_from("<H", raw, 10)[0], creation_flags=raw[9],
                format=struct.unpack_from("<H", raw, 12)[0],
                flags=struct.unpack_from("<H", raw, 14)[0],
                additional_mips=struct.unpack_from("<H", raw, 20)[0],
                record_hex=raw.hex(),
                partitions=[f"{p & 0x3fffffff:08x}-{s:08x}" if s else None
                            for p, s in zip(pointers, sizes)])


def _menu_delta(pack, menu, projection, old_frames):
    """Replace exactly two page owners, retaining all other copied resources."""
    bitmap_index = _json(pack, "bitmap-resources.json", "H2_MCC_BITMAP_RESOURCES_V1")
    inventory = _json(pack, "vista-menu-inventory.json", "H2_MCC_VISTA_MENU_INVENTORY_V1")
    assets = _json(pack, "ui/mcc-menu-assets.json", "H2_MCC_MENU_RESOURCE_CACHE_V1")
    require(inventory.get("complete") is True and assets.get("complete") is True,
            "base cache lacks complete menu preparation")
    require(assets["menu_inventory_sha256"] == file_sha(pack / "vista-menu-inventory.json") and
            assets["bitmap_inventory_sha256"] == file_sha(pack / "bitmap-resources.json"),
            "base menu manifests are incoherent")
    require(inventory.get("inventory_sha256") == file_sha(INVENTORY),
            "base copied-pixel inventory authority differs")
    require(len(inventory["pages"]) == len(assets["pages"]) == inventory["frame_owners"] == assets["frame_owners"],
            "base menu frame owner counts differ")
    _validate_inherited_resources(pack, bitmap_index, inventory, assets)
    specs = qualified_pages(menu, projection)
    with INVENTORY.open(newline="", encoding="utf-8") as stream:
        reference = list(csv.DictReader(stream, delimiter="\t"))
    changes = []
    for spec in PAGES:
        key = (spec["name"], 0)
        require(specs.get(key) == spec, "qualified preview owner differs")
        identity = lambda row: row.get("cache") == "mainmenu.map" and row.get("name") == spec["name"] and row.get("frame") == 0
        page = _one(inventory["pages"], identity, "missing/duplicate inventory preview owner")
        asset = _one(assets["pages"], identity, "missing/duplicate assembled preview owner")
        bank = _one(bitmap_index["banks"], lambda row: row["map"] == "mainmenu.map" and
                    row["tag"] == spec["name"] and row["datum"] == spec["datum"],
                    "missing/duplicate copied bitmap preview owner")
        require(bank["frames"] == [_frame(old_frames[spec["name"]])],
                "base copied bitmap frame differs from its original cache")
        old_extra = {"path", "bytes", "source_record_hex", "source_partition"}
        require({k: v for k, v in asset.items() if k not in old_extra} == page,
                "base assembled page does not match its inventory")
        require(asset["source_record_hex"] == bank["frames"][0]["record_hex"] and
                asset["source_partition"] == bank["frames"][0]["partitions"][0],
                "base assembled page has a stale bitmap owner")
        old_dds = _owned_file(pack, asset["path"])
        require(asset["path"] == "ui/bitmaps/" + page["dds_sha256"] + ".dds" and
                old_dds.stat().st_size == asset["bytes"] and file_sha(old_dds) == page["dds_sha256"],
                "base copied preview DDS differs")
        _, _, raw = bitmap(menu, int(spec["datum"], 16))
        frame = _frame(raw)
        resource = bitmap_index["resources"].get(frame["partitions"][0])
        require(isinstance(resource, dict) and resource == dict(
            source_pointer=spec["pointer"], stored_bytes=spec["stored_bytes"], decoded_bytes=182160,
            sha256=spec["pixel_sha256"], path="bitmap-resources/" + spec["pixel_sha256"] + ".bin"),
            "authored copied preview partition is absent or differs")
        pixels = _owned_file(pack, resource["path"]).read_bytes()
        require(len(pixels) == resource["decoded_bytes"] and digest(pixels) == resource["sha256"],
                "authored copied preview pixels differ")
        dds, _ = dds_image(220, 207, 0, 11, pixels)
        require(digest(dds) == spec["dds_sha256"], "authored preview DDS differs")
        row = _one(reference, lambda row: row["mcc_cache"] == "mainmenu.map" and
                   row["mcc_name"] == spec["name"] and int(row["mcc_frame"]) == 0,
                   "missing/duplicate original stock preview authority")
        new_page = dict(name=spec["name"], frame=0, cache="mainmenu.map", datum=spec["datum"],
                        width=220, height=207, dds_sha256=spec["dds_sha256"],
                        textures_dat_address=spec["pointer"] & 0x3fffffff)
        new_page.update(projected_page(spec, row, digest(dds), digest(pixels)))
        # Only the original stock state or this exact qualified completed state
        # is admitted. A cache cannot launder arbitrary old page provenance.
        stock_page = dict(name=spec["name"], frame=0, cache="mainmenu.map", datum=spec["datum"],
                          width=220, height=207, dds_sha256=row["dds_sha256"],
                          source_comparison=row["status"], textures_dat_address=0x02399a80)
        require(page in (stock_page, new_page), "base preview inventory is partial/customized")
        relative = "ui/bitmaps/" + spec["dds_sha256"] + ".dds"
        target = pack / relative
        if target.exists():
            require(not target.is_symlink() and target.read_bytes() == dds, "existing authored DDS differs")
        else:
            with target.open("xb") as stream:
                stream.write(dds)
        changed = page != new_page
        page.clear(); page.update(new_page)
        bank["frames"] = [frame]
        asset.clear(); asset.update(new_page, path=relative, bytes=len(dds),
                                    source_record_hex=raw.hex(), source_partition=frame["partitions"][0])
        changes.append(dict(name=spec["name"], changed=changed, path=relative,
                            bytes=len(dds), sha256=digest(dds), resource=resource["path"]))
    inventory["comparisons"] = dict(Counter(p["source_comparison"] for p in inventory["pages"]))
    inventory["unique_dds_pages"] = len({p["dds_sha256"] for p in inventory["pages"]})
    write_json(pack / "bitmap-resources.json", bitmap_index)
    write_json(pack / "vista-menu-inventory.json", inventory)
    assets.update(menu_inventory_sha256=file_sha(pack / "vista-menu-inventory.json"),
                  bitmap_inventory_sha256=file_sha(pack / "bitmap-resources.json"),
                  unique_dds_pages=inventory["unique_dds_pages"])
    write_json(pack / "ui/mcc-menu-assets.json", assets)
    return dict(pages=changes, frame_owners=inventory["frame_owners"],
                unique_dds_pages=inventory["unique_dds_pages"],
                complete=True, full_menu_extraction_repeated=False)


def _upgrade(pack, source):
    provenance = _json(pack, "provenance.json", PROFILE)
    conversion = _json(pack, "tag-conversion.json", PROFILE)
    implementation = {p.name: file_sha(p) for p in (Path(__file__),
        *(Path(__file__).parent / name for name in (
            "mcc_multiplayer_catalog.py", "mcc_menu_catalog_inventory.py",
            "mcc_instant_charge.py", "mcc_menu_inventory.py", "mcc_menu_assets.py",
            "mcc_content.py", "prepare_mcc_gameplay.py", "export_vista_menu_ui.py")), INVENTORY)}
    maps = pack / MAPS
    records = conversion["maps"]
    require(len(records) == len({r["map"] for r in records}), "duplicate map conversion owner")
    paths = sorted(maps.glob("*.map"))
    require({p.name for p in paths} == {r["map"] for r in records}, "prepared map catalogue differs")
    for path in paths:
        require(path.name == Path(path.name).name, "unsafe map name")
        record = _one(records, lambda row: row["map"] == path.name, "map conversion owner missing")
        require(file_sha(path) == record["output_sha256"], "prepared map output hash differs: " + path.name)
    before_files = {name: file_sha(pack / name) for name in (
        "provenance.json", "tag-conversion.json", "bitmap-resources.json",
        "vista-menu-inventory.json", "ui/mcc-menu-assets.json")}
    all_stores, map_changes, charge_profiles = [], [], []
    projection, old_frames = None, {}
    for path in paths:
        cache = MccCache(path)
        try:
            require(cache.u32(4) == 13, "non-MCC cache: " + path.name)
            original = bytes(cache.data)
            stores = []
            if path.name == "mainmenu.map":
                for spec in PAGES:
                    old_frames[spec["name"]] = bitmap(cache, int(spec["datum"], 16))[2]
                with (maps / "textures.dat").open("rb") as textures:
                    # Donor paths deliberately refer to the retained parent,
                    # never the unpublished staging pathname in a receipt.
                    projection = apply_catalog_plan(cache, plan_catalog(cache, source / MAPS, textures))
                stores.extend(projection["stores"])
            charge = plan_instant_charge(cache)
            charge_profiles.append(dict(map=path.name, **apply_instant_charge_plan(cache, charge)))
            if charge.status == "ready":
                tag = cache.tags[charge.datum]
                stores.append(dict(map=path.name, datum=f"{charge.datum:08x}", group="effe", tag=tag["name"],
                    address=tag["address"], file_offset=charge.offset, tag_offset=0,
                    before=charge.before.hex(), after=charge.after.hex(),
                    operation="whole-qualified-original-instant-charge-flags"))
            if stores:
                expected = bytearray(original)
                for store in stores:
                    offset, old = store["file_offset"], bytes.fromhex(store["before"])
                    require(expected[offset:offset + len(old)] == old, "metadata store before-image differs")
                    expected[offset:offset + len(old)] = bytes.fromhex(store["after"])
                require(expected == cache.data, "converter wrote outside its complete store manifest")
                serializers = []
                for offset, value in ((28, struct.pack("<H", cache.u16(28) & ~1)), (776, bytes(16))):
                    old = bytes(expected[offset:offset + len(value)])
                    if old != value:
                        serializers.append(dict(map=path.name, file_offset=offset, before=old.hex(),
                                                after=value.hex(), operation="canonical-uncompressed-v13-header"))
                    expected[offset:offset + len(value)] = value
                source_file_sha = file_sha(path)
                save_changed_cache(cache)
                require(cache.data == expected, "serializer wrote outside its original v13 header contract")
                current = _one(records, lambda row: row["map"] == path.name, "map record missing")
                current["stores"].extend(stores)
                current["changes"] += len(stores)
                current["output_sha256"] = file_sha(path)
                all_stores.extend(stores)
                map_changes.append(dict(map=path.name, before_file_sha256=source_file_sha,
                    after_file_sha256=current["output_sha256"], before_decoded_sha256=digest(original),
                    after_decoded_sha256=digest(cache.data), stores=stores, serialization_stores=serializers))
            if path.name == "mainmenu.map":
                projection["serialized_decoded_sha256"] = digest(cache.data)
                menu_resources = _menu_delta(pack, cache, projection, old_frames)
        finally:
            cache.close()
    require(projection is not None, "mainmenu cache is missing")
    conversion.update(multiplayer_catalog=projection, instant_charge=charge_profiles)
    write_json(pack / "tag-conversion.json", conversion)
    summary = provenance["summary"]
    summary.update(ordered_stores=sum(len(r["stores"]) for r in records),
                   changes=sum(r["changes"] for r in records),
                   multiplayer_catalog=projection, instant_charge=charge_profiles)
    for name in ("menu_resources", "vista_menu_inventory"):
        provenance[name].update(complete=True, frame_owners=menu_resources["frame_owners"],
                                unique_dds_pages=menu_resources["unique_dds_pages"])
    result = dict(schema=SCHEMA, source=str(source), selected=False, published_as_selection=False,
                  capabilities_written=False, game_executed=False, full_game_parity=False,
                  source_provenance_sha256=before_files["provenance.json"],
                  metadata_store_blocks=len(all_stores),
                  metadata_store_bytes=sum(len(bytes.fromhex(s["after"])) for s in all_stores),
                  changed_maps=map_changes, multiplayer_catalog=projection,
                  instant_charge=charge_profiles, menu_resources=menu_resources,
                  before_metadata_sha256=before_files,
                  implementation_sha256=implementation,
                  inheritance="Unchanged prepared resources retained by private reflink; no full map/bitmap/menu preparation repeated")
    provenance.update(runtime_active=False, full_tu15_parity=False,
                      prepared_cache_derivation=copy.deepcopy(result))
    write_json(pack / "provenance.json", provenance)
    result["after_metadata_sha256"] = {name: file_sha(pack / name) for name in before_files}
    require(implementation == {p.name: file_sha(p) for p in (Path(__file__),
        *(Path(__file__).parent / name for name in (
            "mcc_multiplayer_catalog.py", "mcc_menu_catalog_inventory.py",
            "mcc_instant_charge.py", "mcc_menu_inventory.py", "mcc_menu_assets.py",
            "mcc_content.py", "prepare_mcc_gameplay.py", "export_vista_menu_ui.py")), INVENTORY)},
        "conversion implementation changed during derivation")
    write_json(pack / "prepared-cache-delta.json", result)
    return result


@_cancellation()
def prepare_derivative(source, destination, progress=print):
    """Create an absent derivative; the caller separately qualifies/selects it."""
    result = _transaction(source, destination, _upgrade, progress)
    result["destination"] = str(Path(destination).resolve(strict=True))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-cache", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    try:
        report = prepare_derivative(args.source_cache, args.destination)
        if args.report_output:
            write_json(args.report_output, report)
        print(json.dumps({k: report[k] for k in ("schema", "destination", "metadata_store_blocks",
              "metadata_store_bytes", "clone", "original_cache_unchanged")}))
        return 0
    except (OSError, ValueError, AttributeError, KeyError, TypeError) as error:
        print("MCC derivative preparation failed: " + str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
