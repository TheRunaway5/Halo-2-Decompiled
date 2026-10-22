"""Inventory every MCC damage-response flash, including whole-tag aliases.

TU1.5 156840 reads jpt!+6C/70, 4C-byte rows. Its authored admission gates
use duration+08 and intensity+10, even when the authored flash type is NONE.
No bitmap, shader or other tag reference occurs inside the flash definition.
"""
from bisect import bisect_right
from collections import Counter
import argparse
import json
import math
from pathlib import Path
import struct

from audit_rendering import Cache, FormatError, file_sha
from mcc_content import MccCache, directory

SCHEMA = "H2_MCC_PLAYER_SCREEN_FLASHES_V1"
FLASH_GATE = struct.unpack("<f", struct.pack("<I", 0x3d4ccccd))[0]


def inventory_map(local, shared=None):
    # Match the native cache's unloaded-only, complete salted-datum merge.
    slots = {t["datum"] & 65535: (local, t) for t in local.tags.values()}
    if shared is not None and any(not t["address"] for t in local.tags.values()):
        for tag in shared.tags.values():
            if not tag["address"]:
                continue
            index = tag["datum"] & 65535
            current = slots.get(index)
            if current and any(current[1][k] != tag[k] for k in ("datum", "group", "name")):
                raise FormatError("Player flash shared datum/group/path collision")
            if current is None or not current[1]["address"]:
                slots[index] = shared, tag
    regions = sorted(((t["address"], owner, t) for owner, t in slots.values()
                      if t["address"] and t["size"]), key=lambda row: row[0])
    starts = [r[0] for r in regions]
    end = 0
    for address, _, tag in regions:
        if address < end:
            raise FormatError("Player flash metadata allocations overlap")
        end = address + tag["size"]

    def read(address, size):
        index = bisect_right(starts, address) - 1
        if index < 0:
            raise FormatError("Player flash pointer precedes owned metadata")
        base, owner, tag = regions[index]
        if size > tag["size"] or address - base > tag["size"] - size:
            raise FormatError("Player flash pointer escapes owned metadata")
        at = owner.pointer(address, owner.magic, size)
        return bytes(owner.data[at:at + size])

    tags = []
    for _, tag in sorted(slots.values(), key=lambda pair: pair[1]["name"]):
        if tag["group"] != "jpt!" or not tag["address"]:
            continue
        backing = tag
        if not tag["size"]:
            aliases = [t for _, t in slots.values() if t["size"] and
                       t["address"] == tag["address"] and t["group"] == tag["group"]]
            if len(aliases) != 1:
                raise FormatError("Player flash alias lacks one exact typed owner: " + tag["name"])
            backing = aliases[0]
        if backing["size"] < 0x74:
            raise FormatError("Truncated damage-response root: " + tag["name"])
        root = read(tag["address"], 0x74)
        count, pointer = struct.unpack_from("<II", root, 0x6c)
        if count > 256:
            raise FormatError("Excessive player response count")
        block = read(pointer, count * 0x4c) if count else b""
        rows = []
        for index in range(count):
            at = index * 0x4c
            fields = [struct.unpack_from("<h", block, at)[0],
                      *struct.unpack_from("<hh", block, at + 4),
                      struct.unpack_from("<I", block, at + 8)[0],
                      struct.unpack_from("<h", block, at + 12)[0],
                      *struct.unpack_from("<5I", block, at + 16)]
            duration, intensity, *color = struct.unpack_from("<f4xf4f", block, at + 8)
            if not all(math.isfinite(v) for v in (duration, intensity, *color)):
                raise FormatError("Nonfinite player flash definition")
            rows.append(dict(fields=fields, flash=duration > FLASH_GATE and intensity > FLASH_GATE))
        tags.append(dict(datum=tag["datum"], name=tag["name"], backing_datum=backing["datum"], rows=rows))
    return dict(map=local.path.name, sha256=file_sha(local.path), tags=tags,
                damage_tags=len(tags), aliases=sum(t["datum"] != t["backing_datum"] for t in tags),
                flash_tags=sum(any(row["flash"] for row in t["rows"]) for t in tags),
                flash_rows=sum(row["flash"] for t in tags for row in t["rows"]))


def inventory_maps(maps, *, progress=lambda _: None, reference=None):
    maps = Path(maps)
    shared = MccCache(maps / "shared.map")
    results, comparisons = [], []
    try:
        for path in sorted(maps.glob("*.map")):
            progress("Checking MCC player screen flashes: " + path.name)
            local = shared if path.name == "shared.map" else MccCache(path)
            try:
                row = inventory_map(local, None if path.stem in ("shared", "single_player_shared") else shared)
                results.append(row)
            finally:
                if local is not shared:
                    local.close()
            if reference is None or not (Path(reference) / path.name).is_file():
                continue
            original = Cache(Path(reference) / path.name)
            try:
                old = inventory_map(original)
            finally:
                original.close()
            current = {t["name"]: t for t in row["tags"]}
            missing, changed = [], []
            for tag in old["tags"]:
                if not any(r["flash"] for r in tag["rows"]):
                    continue
                if tag["name"] not in current:
                    missing.append(tag["name"])
                elif tag["rows"] != current[tag["name"]]["rows"]:
                    changed.append(tag["name"])
            comparisons.append(dict(map=path.name, original_sha256=old["sha256"],
                                    original_flash_tags=old["flash_tags"],
                                    original_flash_rows=old["flash_rows"], missing=missing, changed=changed))
    finally:
        shared.close()
    names = Counter(t["name"] for row in results for t in row["tags"] if any(r["flash"] for r in t["rows"]))
    return dict(schema=SCHEMA, complete=True,
                field_order=["response_type", "flash_type", "priority", "duration_bits", "curve",
                             "intensity_bits", "alpha_bits", "red_bits", "green_bits", "blue_bits"],
                summary=dict(maps=len(results), damage_tags=sum(r["damage_tags"] for r in results),
                             flash_tags=sum(r["flash_tags"] for r in results),
                             flash_rows=sum(r["flash_rows"] for r in results), unique_flash_tags=len(names)),
                flash_tag_names=sorted(names), maps=results,
                original_comparisons=comparisons,
                original_flash_definitions_equivalent=bool(comparisons) and
                    all(not r["missing"] and not r["changed"] for r in comparisons),
                full_screen_effect_parity=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mcc-root", type=Path, required=True)
    parser.add_argument("--reference-maps", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = inventory_maps(directory(args.mcc_root) / "h2_maps_win64_dx11",
                            progress=print, reference=args.reference_maps)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(json.dumps(report["summary"]))
    if args.reference_maps and not report["original_flash_definitions_equivalent"]:
        raise FormatError("MCC player flash definitions differ from the compared originals")


if __name__ == "__main__":
    main()
