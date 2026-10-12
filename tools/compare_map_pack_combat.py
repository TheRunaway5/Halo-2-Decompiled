#!/usr/bin/env python3
"""Compare resident raw combat tag fields, before any executable processing.

Field layouts come from the pinned Assembly schemas. This is source research,
not a title-update implementation: matching cache values cannot establish that
the updated executable uses them unchanged.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

from inspect_multiplayer import Cache

ROOT = Path(__file__).resolve().parents[1]
SCHEMAS = ROOT / "tools/reference/assembly/src/Assembly/Plugins/Halo2"
FORMATS = {
    "float32": "f", "degree": "f", "rangef": "2f", "ranged": "2f",
    "int8": "b", "int16": "h", "int32": "i", "uint8": "B",
    "uint16": "H", "uint32": "I", "enum8": "B", "enum16": "H",
    "enum32": "I", "flags8": "B", "flags16": "H", "flags32": "I",
    "stringId": "I",
}


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def fields(cache, tag, schema):
    result = {}

    def decode(elements, base, prefix="", root=False):
        for e in elements:
            if "offset" not in e.attrib or e.get("visible") == "false":
                continue
            offset = int(e.get("offset"), 0)
            # Object geometry/appearance and player HUD references are outside
            # this combat-data comparison. Retain weapon aim and melee fields.
            if root and tag["group"] == "weap" and not 0x12c <= offset < 0x2a8:
                if e.tag != "tagblock" or e.get("name") not in ("Magazines", "Triggers", "Barrels"):
                    continue
            if root and tag["group"] == "proj" and not 0xbc <= offset < 0x184:
                continue
            if root and tag["group"] == "jpt!" and offset >= 0x6c:
                continue
            if root and tag["group"] == "matg" and offset != 0xd0:
                continue
            name = prefix + e.get("name") + "@" + hex(offset)
            at = base + offset
            if e.tag == "tagblock":
                entries = cache.block(at, int(e.get("elementSize"), 0))
                result[name + "/count"] = {"value": len(entries), "file_offset": at}
                for index, entry in enumerate(entries):
                    decode(e, entry, name + f"[{index}]/")
            elif e.tag == "tagRef":
                value = cache.reference(cache.u(at + 4))
                # Datum identifiers are cache-local. Compare authored paths.
                result[name] = {"value": None if value is None else
                                {k: value[k] for k in ("group", "name")},
                                "file_offset": at}
            elif e.tag == "stringId":
                # Cache-local string indices also differ across DLC caches.
                # This is Cache::string_id's shipped header table layout.
                datum = cache.u(at)
                index = datum & 0xffff
                count, size = cache.u(356), cache.u(360)
                indices, strings = cache.u(364), cache.u(368)
                assert index < count <= 1_000_000
                offset = cache.u(indices + index * 4)
                assert offset < size
                value = cache.text(strings + offset)
                assert len(value.encode()) + offset < size
                result[name] = {"value": value, "datum": hex(datum), "file_offset": at}
            elif e.tag in FORMATS:
                fmt = "<" + FORMATS[e.tag]
                size = struct.calcsize(fmt)
                raw = bytes(cache.data[at:at + size])
                values = struct.unpack(fmt, raw)
                result[name] = {"value": values[0] if len(values) == 1 else list(values),
                                "bits": raw.hex(), "file_offset": at}

    decode(schema, tag["offset"], root=True)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", type=Path, default=ROOT / "assets/disc/maps/lockout.map")
    parser.add_argument("--dlc", type=Path, default=ROOT / "assets/multiplayer-map-pack/demos/dlc")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer/map-pack/combat-cache-comparison.json")
    args = parser.parse_args()
    schemas = {g: ET.parse(SCHEMAS / (g + ".xml")).getroot() for g in ("weap", "proj", "jpt!", "matg")}

    def load(path):
        cache = Cache(path)
        try:
            return {(t["group"], t["name"]): fields(cache, t, schemas[t["group"]])
                    for t in cache.tags.values() if t["group"] in schemas and t["offset"]}
        finally:
            cache.data.close()
            cache.stream.close()

    baseline = load(args.base)
    rows = []
    for path in sorted(args.dlc.glob("*/*.map")):
        other = load(path)
        changes, compared = [], 0
        common = baseline.keys() & other.keys()
        for key in sorted(common):
            left, right = baseline[key], other[key]
            for name in sorted(left.keys() | right.keys()):
                a, b = left.get(name), right.get(name)
                compared += 1
                if a is None or b is None or a.get("bits", a["value"]) != b.get("bits", b["value"]):
                    changes.append({"group": key[0], "tag": key[1], "field": name,
                                    "base": a, "dlc": b})
        rows.append({"cache": str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),
                     "sha256": digest(path), "matching_tags": len(common),
                     "compared_fields": compared, "differences": changes})
    report = {"base_cache": str(args.base), "base_sha256": digest(args.base),
              "schema_sha256": {g: digest(SCHEMAS / (g + ".xml")) for g in schemas},
              "scope": "Visible weapon combat/aim fields, magazine/trigger/barrel blocks, projectile +BC..+183 and damage +0..+6B scalar/reference fields, and globals damage/armor table (+D0). Resident tags matched by group and authored name. Float bits compared exactly; references compared by group/name; StringIDs resolved through each cache's string table. Excludes executable processing, other tag groups, opaque functions and missing tags.",
              "maps": rows}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print(json.dumps({"output": str(args.output), "maps": [
        {k: row[k] for k in ("cache", "matching_tags", "compared_fields")} |
        {"changed_fields": len(row["differences"])} for row in rows]}, indent=2))


if __name__ == "__main__":
    main()
