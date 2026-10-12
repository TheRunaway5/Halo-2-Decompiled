#!/usr/bin/env python3
"""Read literal BSP shader LOD ranges/masks from all supplied Xbox caches.

Expected values are cache bytes, never results of native material preparation.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
NAMES = "ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar containment warlock deltatap turf backwash dune elongation gemini triplicate".split()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-fixture", type=Path, required=True)
    args = parser.parse_args()
    frozen = ["# Literal Xbox BSP shader LOD: map datum index layer-range mask height-bits count ordered-layer-ranges"]
    maps = []
    for ordinal, name in enumerate(NAMES):
        path = ROOT / ("assets/disc/maps" if ordinal < 12 else "assets/map-pack/maps") / (name + ".map")
        cache = Cache(path)
        _, scenario = cache.tag(cache.scenario, "scnr")
        datums = set()
        for record in cache.block(scenario + 528, 68):
            start, size, memory = cache.u32(record), cache.u32(record + 4), cache.u32(record + 8)
            datums.update(cache.u32(p + 12) for p in cache.block(start + 180, 32, memory - start, (start, start + size)))
        records, masks, layers = [], collections.Counter(), collections.Counter()
        for datum in sorted(datums):
            _, shader = cache.tag(datum, "shad")
            definition = cache.block(shader + 32, 124)[0]
            template_datum = cache.u32(definition)
            _, template = cache.tag(template_datum, "stem")
            postprocess = cache.block(template + 88, 40)[0]
            ranges = [cache.u16(p) for p in cache.block(postprocess + 8, 2)]
            for index, lod in enumerate(cache.block(postprocess, 10)):
                packed, mask, height = cache.u16(lod), cache.u32(lod + 2), cache.u32(lod + 6)
                selected = cache.packed_range(packed, ranges)
                words = [datum, index, packed, mask, height, len(selected), *selected]
                frozen.append(name + " " + " ".join(f"{v:x}" for v in words))
                records.append(dict(datum=f"{datum:08X}", shader=cache.tags[datum]["name"],
                    template=cache.tags[template_datum]["name"], lod=index,
                    layer_range=f"{packed:04X}", mask=f"{mask:08X}",
                    height_bits=f"{height:08X}", layers=selected))
                if index:
                    masks[f"{mask:08X}"] += 1
                    for layer in range(32):
                        if mask & (1 << layer): layers[str(layer)] += 1
        maps.append(dict(map=name, cache_sha256=hashlib.sha256(cache.data).hexdigest(),
            shaders=len(datums), lods=len(records), lower_lods=sum(r["lod"] != 0 for r in records),
            lower_masks=dict(masks), lower_layers=dict(layers), records=records))
        cache.close()
    args.write_fixture.write_text("\n".join(frozen) + "\n")
    report = dict(source="literal original version-8 cache metadata", maps=maps,
        shaders=sum(m["shaders"] for m in maps), lods=sum(m["lods"] for m in maps),
        lower_lods=sum(m["lower_lods"] for m in maps),
        fixture_sha256=hashlib.sha256(args.write_fixture.read_bytes()).hexdigest())
    (ROOT / "analysis/multiplayer-shading/bsp-shader-lod-literal.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "maps"}, indent=2))


if __name__ == "__main__":
    main()
