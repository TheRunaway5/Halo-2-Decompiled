#!/usr/bin/env python3
"""Inventory equipment definitions, placements and authored shaders in owned Xbox caches.

This is resource evidence, not pickup, respawn, shader execution or framebuffer
acceptance. In particular a resident powerup tag is not a placed powerup.
"""
import argparse
import collections
import hashlib
import json
from pathlib import Path

from audit_dynamic_model_shader_assets import MAPS
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
NONE = 0xffffffff


def inspect(path):
    c = Cache(path)
    try:
        definitions = []
        for tag in c.tags.values():
            if tag['group'] != 'eqip':
                continue
            _, definition = c.tag(tag['datum'], 'eqip')
            row = dict(datum=tag['datum'], name=tag['name'],
                       powerup_type=c.u16(definition + 0x12c),
                       grenade_type=c.u16(definition + 0x12e),
                       duration_bits=c.u32(definition + 0x130),
                       pickup_sound=c.u32(definition + 0x138),
                       model=c.u32(definition + 0x38), shaders=[])
            if row['model'] != NONE:
                _, model = c.tag(row['model'], 'hlmt')
                render_model = c.u32(model + 4)
                row['render_model'] = render_model
                if render_model != NONE:
                    _, render = c.tag(render_model, 'mode')
                    row['shaders'] = [c.u32(at + 12) for at in c.block(render + 0x60, 32)]
                    for shader in row['shaders']:
                        if shader != NONE:
                            c.shader(shader)
            definitions.append(row)
        _, scenario = c.tag(c.scenario, 'scnr')
        placements = []
        for ordinal, at in enumerate(c.block(scenario + 0x120, 0x90)):
            collection = c.u32(at + 0x5c)
            if collection == NONE:
                continue
            # The same scenario block also contains vehicle collections.
            if c.tags[collection]['group'] != 'itmc':
                continue
            tag, item_collection = c.tag(collection, 'itmc')
            for item in c.block(item_collection, 0x10):
                datum = c.u32(item + 8)
                if datum == NONE or c.tags[datum]['group'] != 'eqip':
                    continue
                placements.append(dict(row=ordinal, collection=collection,
                                       collection_name=tag['name'], definition=datum,
                                       modes=[c.u16(at + 4 + n * 2) for n in range(4)],
                                       classification=c.data[at + 0x14],
                                       respawn_seconds=c.u16(at + 0xe),
                                       collection_respawn_seconds=c.u16(item_collection + 8),
                                       flags=c.u32(at),
                                       position_bits=[c.u32(at + 0x40 + n * 4) for n in range(3)]))
        def manifest(records):
            return {datum: dict(name=row['name'], metadata_sha256=hashlib.sha256(
                json.dumps(row, sort_keys=True).encode()).hexdigest()) for datum, row in records.items()}
        return dict(map=path.stem, sha256=hashlib.sha256(c.data).hexdigest(),
                    equipment=definitions, placements=placements,
                    shaders=manifest(c.shaders), templates=manifest(c.templates), passes=manifest(c.passes),
                    vertex_shaders=manifest(c.vertex_shaders), bitmaps=manifest(c.bitmaps))
    finally:
        c.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    maps = [inspect(ROOT / ('assets/disc/maps' if n < 12 else 'assets/map-pack/maps') /
                    (name + '.map')) for n, name in enumerate(MAPS)]
    types = collections.Counter(row['powerup_type'] for m in maps for row in m['equipment'])
    report = dict(schema='equipment-assets-v1', scope=__doc__, maps=maps,
                  summary=dict(maps=len(maps), definitions=sum(types.values()),
                               types=dict(sorted(types.items())),
                               placed_equipment_choices=sum(len(m['placements']) for m in maps),
                               equipment_rows=sum(len({p['row'] for p in m['placements']}) for m in maps),
                               placed_by_type=dict(sorted(collections.Counter(
                                   next(e['powerup_type'] for e in m['equipment'] if e['datum'] == p['definition'])
                                   for m in maps for p in m['placements']).items()))),
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  gameplay_verified=False, shading_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(report['summary'], sort_keys=True))


if __name__ == '__main__':
    main()
