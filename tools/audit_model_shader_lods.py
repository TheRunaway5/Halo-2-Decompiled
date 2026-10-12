#!/usr/bin/env python3
"""Retain literal shader LOD banks for actual Slayer model collector families.

Placed-model collectors come from the untouched4C2B0 fixture, filtered by the
separately validated automatic Slayer gate. Player/FP references come from
matg representations2/3 and the three supported weapon tags; their complete
declared material tables are a documented superset of initial selected parts.
No expected material-preparation output comes from native code.
"""
import argparse
import collections
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
NAMES = "ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar containment warlock deltatap turf backwash dune elongation gemini triplicate".split()
NONE = 0xffffffff
WEAPONS = ("objects\\weapons\\rifle\\smg\\smg", "objects\\weapons\\rifle\\battle_rifle\\battle_rifle", "objects\\weapons\\rifle\\sniper_rifle\\sniper_rifle")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write-fixture", type=Path, required=True)
    args = parser.parse_args()
    geom = json.loads((ROOT / "analysis/multiplayer-shading/object-geometry-lod-original.json").read_text())
    placed = json.loads((ROOT / "analysis/multiplayer-shading/object-contact-inventory.json").read_text())["placements"]
    owners = {(p["map"], p["placement_type"], p["placement"]): p for p in placed}
    sections = {(s["map"], s["render_model"], s["section"]): s for s in geom["sections"]}
    frozen = ["# Literal model shader bank: C map shader implementation prt-render-model example-definition geometryLOD section role; L map shader index packed mask height-bits count ordered-ranges"]
    maps = []
    for ordinal, name in enumerate(NAMES):
        path = ROOT / ("assets/disc/maps" if ordinal < 12 else "assets/map-pack/maps") / (name + ".map")
        c = Cache(path)
        contexts = {}
        model_materials = {}
        owner_count = collections.Counter()
        omitted = []

        def materials(mode):
            if mode not in model_materials:
                _, root = c.tag(mode, "mode")
                model_materials[mode] = [c.u32(m+12) for m in c.block(root+0x60, 32)]
            return model_materials[mode]

        def add(shader, impl, prtmode, definition, lod, section, role):
            if shader != NONE:
                contexts.setdefault((shader, impl, prtmode), dict(shader=shader,
                    implementation=impl, prt_render_model=prtmode, definition=definition,
                    geometry_lod=lod, section=section, role=role))

        for g in (g for g in geom["placements"] if g["map"] == name):
            p = owners[(name, g["type"], g["placement"])]
            if g["type"] not in (6, 7, 11) or not (p["slayer_automatic"] if g["type"] == 6 else p["automatic"]):
                continue
            if name == "zanzibar" and g["type"] == 6 and g["placement"] in (19, 20, 21, 22):
                omitted.append(g["placement"])
                continue
            if g["render_model"] == NONE:
                continue
            owner_count[str(g["type"])] += 1
            mode = g["render_model"]
            ids = materials(mode)
            impl = 1 if g["type"] == 6 and p["baked"] else 2
            prtmode = mode if p["prt_count"] else NONE
            for lod, collector in enumerate(g["collectors"]):
                for section in collector:
                    s = sections[(name, mode, section)]
                    part = s["resources"][0]
                    assert part["type"] == 0 and part["channel"] == 72 and part["size"] % 72 == 0
                    with (ROOT / s["raw_file"]).open("rb") as stream:
                        stream.seek(part["file_offset"])
                        data = stream.read(part["size"])
                    assert len(data) == part["size"]
                    for off in range(0, len(data), 72):
                        material = struct.unpack_from("<H", data, off+4)[0]
                        assert material < len(ids)
                        add(ids[material], impl, prtmode, g["definition"], lod, section,
                            {6: "scenery", 7: "machine", 11: "prop"}[g["type"]])

        _, globals_root = c.tag(next(t["datum"] for t in c.tags.values() if t["group"] == "matg"), "matg")
        reps = c.block(globals_root+0x138, 0xbc)
        for index in (2, 3):
            representation = reps[index]
            unit, hands = c.u32(representation+0xb4), c.u32(representation+4)
            _, biped = c.tag(unit, "bipd")
            _, hlmt = c.tag(c.u32(biped+0x38), "hlmt")
            body_mode = c.u32(hlmt+4)
            for mode, definition, role, lod in ((body_mode, unit, "player_body", 5), (hands, hands, "fp_hands", 4)):
                for shader in materials(mode):add(shader, 2, NONE, definition, lod, NONE, role)
        for weapon in WEAPONS:
            tag = next(t for t in c.tags.values() if t["group"] == "weap" and t["name"] == weapon)
            _, root = c.tag(tag["datum"], "weap")
            _, hlmt = c.tag(c.u32(root+0x38), "hlmt")
            mode = c.u32(hlmt+4)
            for shader in materials(mode):add(shader, 2, NONE, tag["datum"], 5, NONE, "held_weapon")
            first_person = c.block(root+0x2a8, 16)
            assert first_person
            fp = c.u32(first_person[0]+4)
            for shader in materials(fp):add(shader, 2, NONE, fp, 4, NONE, "fp_weapon")

        records = []
        for context in sorted(contexts.values(), key=lambda x: (x["shader"], x["implementation"], x["prt_render_model"])):
            frozen.append("C " + name + " " + " ".join(f"{context[k]:x}" for k in ("shader", "implementation", "prt_render_model", "definition", "geometry_lod", "section")) + " " + context["role"])
        for datum in sorted({k[0] for k in contexts}):
            _, root = c.tag(datum, "shad")
            pp = c.block(root+32, 124)
            assert len(pp) == 1
            _, template = c.tag(c.u32(pp[0]), "stem")
            postprocess = c.block(template+88, 40)
            assert len(postprocess) == 1
            ranges = [c.u16(p) for p in c.block(postprocess[0]+8, 2)]
            for index, lod in enumerate(c.block(postprocess[0], 10)):
                packed, mask, height = c.u16(lod), c.u32(lod+2), c.u32(lod+6)
                selected = c.packed_range(packed, ranges)
                frozen.append("L " + name + " " + " ".join(f"{v:x}" for v in (datum, index, packed, mask, height, len(selected), *selected)))
                records.append(dict(datum=datum, shader=c.tags[datum]["name"], lod=index,
                    layer_range=packed, mask=mask, height_bits=height, layers=selected))
        maps.append(dict(map=name, cache_sha256=hashlib.sha256(c.data).hexdigest(),
            rendered_owners=dict(owner_count), omitted_unresolved_scenery=omitted,
            contexts=list(contexts.values()), records=records, shaders=len({k[0] for k in contexts}),
            lower_lods=sum(r["lod"] != 0 for r in records)))
        c.close()
    args.write_fixture.write_text("\n".join(frozen)+"\n")
    result = dict(source="literal original cache metadata + untouched original selected collector fixture + source Slayer admission", maps=maps,
        shaders=sum(m["shaders"] for m in maps), contexts=sum(len(m["contexts"]) for m in maps),
        lower_lods=sum(m["lower_lods"] for m in maps),
        fixture_sha256=hashlib.sha256(args.write_fixture.read_bytes()).hexdigest(),
        scope="Placed model references use exact admitted source collector parts across all6 geometryLODs. Four unresolved Zanzibar pillars are excluded. Both MP player/body-hands representations and supported SMG/BR/sniper held/FP declared material tables are an explicit asset superset, not proof every table entry is selected by a live player.")
    (ROOT / "analysis/multiplayer-shading/model-shader-lod-literal.json").write_text(json.dumps(result, indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k != "maps"}, indent=2))


if __name__ == "__main__":main()
