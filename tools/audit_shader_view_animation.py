#!/usr/bin/env python3
"""Read-only original-cache animated-mask versus per-view extern inventory.

MODEL contexts come from the literal admitted collector/material census.
Every available independent layer is retained as a conservative pass superset;
layer3 uses that context's actual baked1/dynamic2 implementation. BSP records
include both authored static0/1 variants. These are asset contexts, not draws.
"""
import collections
import hashlib
import json
import struct
from pathlib import Path
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
PIXEL_MASKS = [0xff0000, 0xff00, 0xff, 0xff000000, 0xffffff, 0xff0000, 0xff00, 0xff]
# Literal17000 jump-table masks, also used after17960 computes its values.
VERTEX_MASKS = [1, 2, 4, 8, 7, 1, 2, 4, 8, 3, 12, 3, 12, 15, 11, 11, 7, 7, 15, 15, 15]
LIGHT = {14, 15, 16, 17, 23, 24, 35}
MODEL = ROOT / "tests/fixtures/retail_model_shader_lods.tsv"
BSP = ROOT / "tests/fixtures/retail_bsp_shader_lods.tsv"


def main():
    contexts = collections.defaultdict(set)
    for line in MODEL.read_text().splitlines():
        word = line.split()
        if word and word[0] == "C":
            contexts[word[1]].add(("model", int(word[2], 16), int(word[3], 16)))
    for line in BSP.read_text().splitlines():
        word = line.split()
        if word and not word[0].startswith("#"):
            for impl in (0, 1):
                contexts[word[0]].add(("bsp", int(word[1], 16), impl))
    totals = collections.Counter()
    records, overlaps, provenance = [], [], []
    for name, bank in sorted(contexts.items()):
        path = ROOT / "assets/disc/maps" / (name + ".map")
        if not path.exists():
            path = ROOT / "assets/map-pack/maps" / (name + ".map")
        c = Cache(path)
        provenance.append(dict(map=name, cache_sha256=hashlib.sha256(c.data).hexdigest()))
        try:
            for kind, datum, requested in sorted(bank):
                tag, s = c.tag(datum, "shad")
                totals[kind + "_shader_contexts"] += 1
                for d in c.block(s + 32, 124):
                    _, stem = c.tag(c.u32(d), "stem")
                    t = c.block(stem + 88, 40)[0]
                    templates = c.block(t + 16, 10)
                    layers = [c.u16(p) for p in c.block(t + 8, 2)]
                    animation_passes = c.block(d + 44, 2)
                    animation_impls = c.block(d + 52, 10)
                    animation_maps = c.block(d + 84, 4)
                    overlays = c.block(d + 60, 20)
                    switches = [0] * 8
                    switches[4:6] = [c.u16(s + 0x3e), c.u16(s + 0x40)]
                    for lod_index, lod in enumerate(c.block(t, 10)):
                        active_layers = c.packed_range(c.u16(lod), layers)
                        for layer, packed in enumerate(active_layers):
                            if not c.u32(lod + 2) & (1 << layer):
                                continue
                            implementation = requested if layer == 3 else 0
                            for ordinal in range(packed & 511, (packed & 511) + (packed >> 9)):
                                template = templates[ordinal]
                                declared = c.u16(template + 8) >> 9
                                if implementation >= declared:
                                    continue
                                _, spas = c.tag(c.u32(template + 4), "spas")
                                pp = c.block(spas + 28, 88)[0]
                                source = c.block(pp, 306)[implementation]
                                externs = c.block(pp + 56, 4)
                                fragment_bank = c.block(pp + 32, 4)
                                permutation_bank = c.block(pp + 40, 6)
                                fragments = c.packed_range(c.u16(source + 246), fragment_bank)
                                offsets, stage = [], 0
                                for fragment in fragments:
                                    switch = struct.unpack_from("<h", c.data, fragment)[0]
                                    choice = 0 if switch == -1 else switches[switch]
                                    permutation = c.packed_range(c.u16(fragment + 2), permutation_bank)[choice]
                                    offsets.append(stage)
                                    stage += (c.u16(permutation + 4) >> 9) - int(bool(c.u16(permutation + 2) & 1))

                                def pixel_write(address):
                                    dest, component, param = struct.unpack_from("<HBB", c.data, address)
                                    slot = (dest >> 6) & 15
                                    if fragments:
                                        fragment = dest & 7
                                        switch = struct.unpack_from("<h", c.data, fragments[fragment])[0]
                                        if switch != -1 and ((dest >> 3) & 7) != switches[switch]:
                                            return None
                                        if slot != 8:
                                            slot += offsets[fragment]
                                    reg = 43 + ((dest >> 10) & 1) if slot == 8 else 10 + ((dest >> 10) & 1) * 8 + slot
                                    assert 0 <= slot <= 8
                                    return dict(register=reg, mask=PIXEL_MASKS[component & 7],
                                        source=param, destination=dest, component=component)

                                def vertex_write(address):
                                    dest, component, param = struct.unpack_from("<HBB", c.data, address)
                                    mode = (dest >> 5) & 31
                                    assert mode < len(VERTEX_MASKS)
                                    return dict(bank=(dest >> 4) & 1, register=dest & 15,
                                        mask=VERTEX_MASKS[mode], source=param, destination=dest, component=component)

                                pe = [pixel_write(e) for e in c.packed_range(c.u16(source + 0x122), externs)]
                                pe = [e for e in pe if e and ((e["source"] in LIGHT and
                                    ((e["component"] & 7) == 4 or (e["source"] == 23 and (e["component"] & 7) < 4))) or
                                    (e["source"] == 0 and (e["component"] & 7) == 4))]
                                ve = [vertex_write(e) for e in c.packed_range(c.u16(source + 0x124), externs)]
                                ve = [e for e in ve if e["source"] in LIGHT]
                                totals[kind + "_pass_contexts"] += 1
                                if not (pe or ve):
                                    continue
                                totals[kind + "_passes_with_view_externs"] += 1
                                totals[kind + "_vertex_lighting_passes"] += bool(ve)
                                totals[kind + "_pixel_lighting_passes"] += any(e["source"] in LIGHT for e in pe)
                                totals[kind + "_pixel_camera_passes"] += any(e["source"] == 0 for e in pe)
                                pixel, vertex = [], []
                                if overlays and ordinal < len(animation_passes):
                                    candidates = c.packed_range(c.u16(animation_passes[ordinal]), animation_impls)
                                    if implementation < len(candidates):
                                        ai = candidates[implementation]
                                        pixel = [pixel_write(a) for a in c.packed_range(c.u16(ai + 6), animation_maps)]
                                        pixel = [a for a in pixel if a]
                                        vertex = [vertex_write(a) for a in c.packed_range(c.u16(ai + 8), animation_maps)]
                                totals[kind + "_passes_with_view_externs_and_animation"] += bool(pixel or vertex)
                                matches = []
                                for e in pe:
                                    for a in pixel:
                                        if e["register"] == a["register"] and e["mask"] & a["mask"]:
                                            matches.append(dict(bank="pixel", extern=e, animation=a, mask=e["mask"] & a["mask"]))
                                for e in ve:
                                    for a in vertex:
                                        if (e["bank"], e["register"]) == (a["bank"], a["register"]) and e["mask"] & a["mask"]:
                                            matches.append(dict(bank="vertex", extern=e, animation=a, mask=e["mask"] & a["mask"]))
                                record = dict(map=name, kind=kind, shader=datum, name=tag["name"],
                                    shader_lod=lod_index, layer=layer, template_pass=ordinal, implementation=implementation,
                                    pixel_view_externs=pe, vertex_view_externs=ve,
                                    pixel_animated_writes=pixel, vertex_animated_writes=vertex, overlap=matches)
                                records.append(record)
                                if matches:
                                    totals[kind + "_overlap_pass_contexts"] += 1
                                    overlaps.append(record)
        finally:
            c.close()
    for kind in ("model", "bsp"):
        totals.setdefault(kind + "_overlap_pass_contexts", 0)
    report = dict(summary=dict(totals), records=records, overlaps=overlaps, maps=provenance,
        model_fixture_sha256=hashlib.sha256(MODEL.read_bytes()).hexdigest(),
        bsp_fixture_sha256=hashlib.sha256(BSP.read_bytes()).hexdigest(),
        scope="Literal original selected shaderLOD/layer/default-fragment masks for admitted MODEL references and BSP references. Every independent layer is conservatively included; these are a pass superset, not visible draw counts. Player/FP complete declared material tables are a documented superset. Four unresolved Zanzibar pillars are excluded. Baked/static contexts may contain lighting extern metadata without a native ordinary-light query. Native order remains a separate inspection; no native code result forms these authored masks.")
    output = ROOT / "analysis/multiplayer-shading/shader-view-animation-inventory.json"
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report["summary"], indent=2))
    print("Overlap contexts:", len(overlaps))


if __name__ == "__main__":
    main()
