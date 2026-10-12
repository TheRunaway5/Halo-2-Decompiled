#!/usr/bin/env python3
"""Freeze Elite body/FP-body original section/part compiled-class selection.

Uses unchanged 1CAA0/1CB20 with literal section and part inputs. Downstream
state consumers are intercepted only for observation, as in the FP route
audit. This covers authored sections, not live section/character selection.
"""
from pathlib import Path
import collections
import hashlib
import json
import struct
from audit_first_person_vertex_routes import OriginalClass
from audit_rendering import Cache
from audit_dynamic_model_shader_assets import MAPS
from audit_world_vertex_declarations import raw_model_streams
from xbe import Xbe

ROOT = Path(__file__).resolve().parents[1]


def main():
    original = Xbe(ROOT / 'assets/disc/default.xbe')
    selector = OriginalClass(original)
    owners = []
    for name in MAPS:
        maps = 'assets/map-pack/maps' if (ROOT / f'assets/map-pack/maps/{name}.map').is_file() else 'assets/disc/maps'
        path = ROOT / maps / (name + '.map')
        cache = Cache(path)
        for model_name in ('objects\\characters\\elite\\elite', 'objects\\characters\\elite\\fp_body\\fp_body'):
            tag = next(t for t in cache.tags.values() if t['group'] == 'mode' and t['name'] == model_name)
            _, root = cache.tag(tag['datum'], 'mode')
            sections = cache.block(root + 0x24, 0x5c)
            owner = dict(map=name, model=f"{tag['datum']:08X}", name=model_name, sections=[])
            for record in raw_model_streams(cache, tag['datum'], original):
                section = sections[record['section']]
                mode = cache.u16(section)
                opaque, transparent = cache.data[section + 0x14:section + 0x16]
                resources = cache.block(section + 0x48, 16)
                entry = next(r for r in resources if cache.u16(r + 4) == 0 and cache.u16(r + 6) == 72)
                pointer, size, header = cache.u32(section + 0x38), cache.u32(section + 0x3c), cache.u32(section + 0x44)
                source = path if pointer >> 30 == 0 else path.parent / ('', 'mainmenu.map', 'shared.map', 'single_player_shared.map')[pointer >> 30]
                if not source.exists(): source = ROOT / 'assets/disc/maps' / source.name
                with source.open('rb') as stream:
                    stream.seek(pointer & 0x3fffffff)
                    raw = stream.read(size)
                first, length = len(raw) - header - 4 + cache.u32(entry + 12), cache.u32(entry + 8)
                assert length % 72 == 0 and 0 <= first <= len(raw) - length
                parts = []
                for part in range(length // 72):
                    at = first + part * 72
                    kind = struct.unpack_from('<H', raw, at)[0]
                    result = selector.run(mode, opaque, transparent, kind)
                    expected = mode if mode < 3 else (transparent if kind in (4, 5) else opaque) + 2
                    assert result == expected
                    parts.append(dict(part=part, type=kind, selected_class=result,
                                      raw_words=[f'{w:08X}' for w in struct.unpack_from('<18I', raw, at)]))
                owner['sections'].append(dict(section=record['section'], classification=mode,
                    opaque_influences=opaque, transparent_influences=transparent,
                    part_file=str(source.relative_to(ROOT)), file_offset=(pointer & 0x3fffffff) + first,
                    part_sha256=hashlib.sha256(raw[first:first + length]).hexdigest(), parts=parts))
            owners.append(owner)
        cache.close()
    counts = collections.Counter(p['selected_class'] for o in owners for s in o['sections'] for p in s['parts'])
    report = dict(executable_sha256=hashlib.sha256(original.data).hexdigest(),
        untouched_functions=['1CAA0', '1CB20'], intercepted_state_consumers=['1C590', '1CB70', '1C8C0'],
        maps=len(MAPS), models=len(owners), sections=sum(len(o['sections']) for o in owners),
        parts=sum(counts.values()), class_counts=dict(counts), owners=owners,
        boundary='Every authored Elite body/FP-body section is an explicit asset superset. No native Elite selection, live region/LOD admission or controller phase is inferred.')
    out = ROOT / 'analysis/multiplayer-shading/elite-body-vertex-classes-original.json'
    out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'owners'}, indent=2))


if __name__ == '__main__': main()
