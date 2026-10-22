#!/usr/bin/env python3
"""Compare authored Xbox rocket/Ghost inputs with prepared MCC metadata.

This is an offline metadata check, not flight, lock-on or explosion acceptance.
Run through tools/run_test.py; retain only the small explicit JSON report.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from inspect_multiplayer import Cache, MAPS
from mcc_content import MccCache, directory
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

NAMES = (("weap", "rocket_launcher\\rocket_launcher"), ("weap", "ghost\\weapon\\ghost_gun"),
         ("proj", "rocket_launcher\\projectiles\\rocket"), ("proj", "ghost\\weapon\\ghost_bolt"),
         ("MGS2", "effects\\objects\\vehicles\\ghost\\ghost_bolt"))


def sha(path):
    with Path(path).open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def inspect(cache, tag, mcc=False):
    word = cache.u32 if mcc else cache.u
    at = tag["offset"]

    def reference(value):
        target = cache.tags.get(value)
        return [target["group"], target["name"]] if target else None

    def symbol(value):
        if not value:
            return ""
        index = value & 65535
        count, table, strings = ((48, 60, 52) if mcc else (356, 364, 368))
        if index >= word(count):
            raise ValueError("Attachment function exceeds its owning StringID bank")
        location = word(strings) + word(word(table) + index * 4)
        end = cache.data.find(b"\0", location)
        if end < location:
            raise ValueError("Unterminated attachment function name")
        return cache.data[location:end].decode("utf-8")

    if tag["group"] == "MGS2":
        def curve(offset):
            rows = cache.block(offset, 1)
            return cache.data[rows[0]:rows[0] + len(rows)].hex() if rows else ""

        volumes = []
        for volume in cache.block(at + 8, 0x98):
            aspects = []
            for aspect in cache.block(volume + 0x38, 0x1c):
                aspects.append({"curves": [curve(aspect), curve(aspect + 8)],
                                "words": [word(aspect + offset) for offset in (0x10, 0x14, 0x18)]})
            volumes.append({"flags": word(volume), "bitmap": reference(word(volume + 8)),
                            "sprites": word(volume + 0xc),
                            "curves": [curve(volume + 0x10 + index * 8) for index in range(5)],
                            "aspects": aspects})
        return {"falloff_cutoff_words": [word(at), word(at + 4)], "volumes": volumes}
    if tag["group"] == "weap":
        return {
            "first_person_offset_words": [word(at + 0x308 + i * 4) for i in range(3)],
            "trigger_inputs_and_behaviors": [list(struct.unpack_from("<3h", cache.data, p + 4))
                for p in cache.block(at + 0x2c8, 0x40)],
            "barrel_projectiles": [reference(word(p + 0x90)) for p in cache.block(at + 0x2d0, 0xec)],
        }
    return {
        "projectile_words": {hex(offset): word(at + offset) for offset in
            (0xbc, 0xcc, 0xd4, 0xd8, 0xe0, 0x164, 0x16c, 0x17c, 0x180,
             0x184, 0x188, 0x18c, 0x190, 0x194, 0x198)},
        "effects_and_damage_references": {hex(offset): reference(word(at + offset + 4))
            for offset in range(0xe4, 0x15c, 8) if word(at + offset) in (0x65666665, 0x6a707421)},
        "attachments": [{"tag": reference(word(p + 4)), "flags": word(p + 0xc) & 65535,
            "function": symbol(word(p + 0x10))} for p in cache.block(at + 0x94, 0x18)],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xbox-maps", type=Path, default=Path("assets/disc/maps"))
    parser.add_argument("--xbe", type=Path, default=NEW_DEFAULT)
    parser.add_argument("--mcc-root", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get("HALO2_TEST_SCRATCH"):
        parser.error("Run through tools/run_test.py")
    if sha(args.xbe) != NEW_PIN:
        raise ValueError("The reference executable differs from pinned English TU1.5")
    maps = directory(args.mcc_root) / "h2_maps_win64_dx11"
    shared = MccCache(maps / "shared.map")
    rows, files = [], []
    try:
        for name in MAPS:
            original = Cache(args.xbox_maps / (name + ".map"))
            imported = MccCache(maps / (name + ".map"))
            try:
                files.append({"map": name, "xbox_sha256": sha(original.path), "mcc_sha256": sha(imported.path)})
                for group, suffix in NAMES:
                    find = lambda cache: next((t for t in cache.tags.values()
                        if t["group"] == group and t["name"].endswith(suffix) and t["offset"] is not None), None)
                    a, b, donor = find(original), find(imported), imported
                    if b is None:
                        b, donor = find(shared), shared
                    if a is None or b is None:
                        raise ValueError(f"Missing authored {suffix} in {name}")
                    xbox, mcc = inspect(original, a), inspect(donor, b, True)
                    differences = [key for key in xbox if xbox[key] != mcc[key]]
                    rows.append({"map": name, "name": suffix, "matching": not differences,
                                 "differences": differences, "xbox": xbox, "mcc": mcc})
            finally:
                original.data.close()
                original.stream.close()
                imported.close()
    finally:
        shared.close()
    report = {"passed": all(row["matching"] for row in rows), "maps": len(MAPS),
        "tu15_xbe_sha256": NEW_PIN, "mcc_shared_sha256": sha(maps / "shared.map"),
        "tool_sha256": sha(__file__), "source_files": files, "cases": rows,
        "rendered_or_live_gameplay_verified": False, "scope": __doc__}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    data = json.dumps(report, indent=2) + "\n"
    if len(data.encode()) > 1024 * 1024:
        raise ValueError("Reference report exceeds the small-report budget")
    args.report.write_text(data)
    print(json.dumps({"passed": report["passed"], "maps": len(MAPS), "cases": len(rows),
                      "report": str(args.report)}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
