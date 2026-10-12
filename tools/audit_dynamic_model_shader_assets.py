#!/usr/bin/env python3
"""Freeze literal player/body/held/FP shader and 72-byte part identities.

This reads the supplied Xbox caches/resources only. Geometry/pose admission
is the tested caller's explicit input; no expected native prepared material
or selector output is used to create these asset records.
"""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import struct

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
MAPS = "ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar containment warlock deltatap turf backwash dune elongation gemini triplicate".split()
WEAPONS = ("smg", "battle_rifle", "sniper_rifle")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--write-fixture", type=Path, required=True)
    a = p.parse_args()
    frozen = ["# Literal Xbox dynamic models: M map role input-datum render-model material-count shaders; R map mode section part shader 18raw-part-words; L map shader lod bias packed mask height count layer-ranges; P map shader lod layer template-pass implementation spas-name vertex-shader-name"]
    maps = []
    for index, name in enumerate(MAPS):
        path = ROOT / ("assets/disc/maps" if index < 12 else "assets/map-pack/maps") / (name + ".map")
        c = Cache(path)
        roles = []
        _, matg = c.tag(next(t["datum"] for t in c.tags.values() if t["group"] == "matg"), "matg")
        representations = c.block(matg + 0x138, 0xBC)
        for representation, race in ((2, "chief"), (3, "elite")):
            at = representations[representation]
            unit, hands = c.u32(at + 0xB4), c.u32(at + 4)
            _, biped = c.tag(unit, "bipd")
            _, hlmt = c.tag(c.u32(biped + 0x38), "hlmt")
            roles.extend([(race + "_body", unit, c.u32(hlmt + 4)), (race + "_hands", hands, hands)])
        for weapon in WEAPONS:
            datum = next(t["datum"] for t in c.tags.values() if t["group"] == "weap" and t["name"] == "objects\\weapons\\rifle\\" + weapon + "\\" + weapon)
            _, tag = c.tag(datum, "weap")
            _, hlmt = c.tag(c.u32(tag + 0x38), "hlmt")
            fp = [c.u32(at + 4) for at in c.block(tag + 0x2A8, 16)]
            assert len(fp) == 2 and fp[0] == fp[1]
            roles.extend([(weapon + "_held", datum, c.u32(hlmt + 4)), (weapon + "_fp", fp[0], fp[0])])
        shaders, streams, counts = set(), [], collections.Counter()
        for role, definition, mode in roles:
            _, root = c.tag(mode, "mode")
            materials = [c.u32(at + 12) for at in c.block(root + 0x60, 32)]
            frozen.append("M " + name + " " + role + " " + " ".join(f"{v:x}" for v in [definition, mode, len(materials), *materials]))
            shaders.update(s for s in materials if s != 0xFFFFFFFF)
            counts["models"] += 1
            for section_index, section in enumerate(c.block(root + 0x24, 0x5C)):
                resources = c.block(section + 0x48, 16)
                if not resources:
                    continue
                pointer, size, header = c.u32(section + 0x38), c.u32(section + 0x3C), c.u32(section + 0x44)
                location, offset = pointer >> 30, pointer & 0x3FFFFFFF
                source = path if location == 0 else path.parent / ("", "mainmenu.map", "shared.map", "single_player_shared.map")[location]
                if not source.exists():
                    source = ROOT / "assets/disc/maps" / source.name
                with source.open("rb") as f:
                    f.seek(offset)
                    raw = f.read(size)
                assert len(raw) == size
                part = resources[0]
                assert c.u16(part + 4) == 0 and c.u16(part + 6) == 72
                first, length = len(raw) - header - 4 + c.u32(part + 12), c.u32(part + 8)
                assert 0 <= first <= len(raw) - length and length % 72 == 0
                streams.append(dict(mode=mode, section=section_index, file=str(source.relative_to(ROOT)), file_offset=offset + first,
                    size=length, sha256=hashlib.sha256(raw[first:first + length]).hexdigest()))
                for ordinal in range(length // 72):
                    at = first + ordinal * 72
                    material = struct.unpack_from("<h", raw, at + 4)[0]
                    shader = materials[material] if 0 <= material < len(materials) else 0xFFFFFFFF
                    frozen.append("R " + name + " " + " ".join(f"{v:x}" for v in [mode, section_index, ordinal, shader, *struct.unpack_from("<18I", raw, at)]))
                    counts["raw_parts"] += 1
        # A mode may be referenced through multiple roles; keep its literal
        # part record only once. Both role identities remain present above.
        frozen = list(dict.fromkeys(frozen))
        for datum in sorted(shaders):
            _, shader = c.tag(datum, "shad")
            post = c.block(shader + 32, 124)
            assert len(post) == 1
            _, stem = c.tag(c.u32(post[0]), "stem")
            template = c.block(stem + 88, 40)
            assert len(template) == 1
            tp = template[0]
            layers, passes = c.block(tp + 8, 2), c.block(tp + 16, 10)
            for lod_index, lod in enumerate(c.block(tp, 10)):
                packed = c.u16(lod)
                selected = layers[packed & 511:(packed & 511) + (packed >> 9)]
                assert len(selected) == packed >> 9
                mask = c.u32(lod + 2)
                frozen.append("L " + name + " " + " ".join(f"{v:x}" for v in [datum, lod_index, c.u16(shader + 0x3C), packed, mask, c.u32(lod + 6), len(selected), *(c.u16(at) for at in selected)]))
                counts["shader_lods"] += 1
                for layer, at in enumerate(selected):
                    if not mask & (1 << layer):
                        continue
                    layer_range = c.u16(at)
                    for pass_index in range(layer_range & 511, (layer_range & 511) + (layer_range >> 9)):
                        pass_at = passes[pass_index]
                        _, spas = c.tag(c.u32(pass_at + 4), "spas")
                        definitions = c.block(spas + 28, 88)
                        assert len(definitions) == 1
                        impl = c.block(definitions[0], 306)
                        for implementation in range(c.u16(pass_at + 8) >> 9):
                            vrtx = c.u32(impl[implementation] + 256)
                            frozen.append("P " + name + " " + " ".join(f"{v:x}" for v in [datum, lod_index, layer, pass_index, implementation])
                                + " " + c.tags[c.u32(pass_at + 4)]["name"] + " " + ("NONE" if vrtx == 0xFFFFFFFF else c.tags[vrtx]["name"]))
                            counts["literal_passes"] += 1
        maps.append(dict(map=name, cache_sha256=hashlib.sha256(c.data).hexdigest(), source_streams=streams, **counts))
        c.close()
    a.write_fixture.write_text("\n".join(frozen) + "\n")
    totals = {key: sum(m[key] for m in maps) for key in ("models", "raw_parts", "shader_lods", "literal_passes")}
    report = dict(maps=maps, **totals, fixture_sha256=hashlib.sha256(a.write_fixture.read_bytes()).hexdigest(),
        oracle=str(Path(__file__).relative_to(ROOT)), oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scope="Literal all21 matg representations2/3 (body and hands), supported SMG/BR/sniper held and shared Chief/Elite FP resource identities, all original mode section 72-byte parts and shad/stem/spas/vrtx selected-LOD references. Geometry selected by the integration caller remains explicit; this asset census does not infer live owner/camera/pose/visibility/shaderLOD admission.")
    a.write_fixture.with_suffix(".provenance.json").write_text(json.dumps(report, indent=2) + "\n")
    (ROOT / "analysis/multiplayer-shading/dynamic-model-shader-assets.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: value for key, value in report.items() if key != "maps"}, indent=2))


if __name__ == "__main__":
    main()
