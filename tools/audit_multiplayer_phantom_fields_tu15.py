#!/usr/bin/env python3
"""Inventory placed original Xbox multiplayer phantom definitions and inputs.

This inventories supplied data. It does not admit an initially absent object,
classify every phantom as a player lift, or prove live force delivery.
"""
import argparse
import json
import struct
from pathlib import Path

from audit_rendering import Cache, file_sha
from audit_map_world_features import DISC, PACK


def inspect(path):
    cache = Cache(path)
    try:
        _, scenario = cache.tag(cache.scenario, 'scnr')
        placements = {}
        for offset, stride, group in ((0x50,0x5c,'scen'), (0xa8,0x48,'mach'),
                                       (0xb8,0x44,'ctrl'), (0x328,0x4c,'bloc')):
            palette = cache.block(scenario+offset+8,40)
            for ordinal, row in enumerate(cache.block(scenario+offset,stride)):
                index = struct.unpack_from('<h',cache.data,row)[0]
                assert 0 <= index < len(palette)
                datum = cache.u32(palette[index]+4)
                placements.setdefault(datum,[]).append(dict(group=group,ordinal=ordinal,
                    flags=cache.u32(row+4),
                    position=list(struct.unpack_from('<3f',cache.data,row+8))))
        fields = []
        for datum, instances in placements.items():
            definition, object_row = cache.tag(datum,cache.tags[datum]['group'])
            model_datum = cache.u32(object_row+0x38)
            if model_datum == 0xffffffff:
                continue
            _, model = cache.tag(model_datum,'hlmt')
            physics_datum = cache.u32(model+0x24)
            if physics_datum == 0xffffffff:
                continue
            _, physics = cache.tag(physics_datum,'phmo')
            phantoms = cache.block(physics+0x108,0x20)
            if not phantoms:
                continue
            # Whole TU1.5 1DA750 resolves phmo+44 body bindings to the
            # phmo+2C array and multiplies the selected index by68.
            force_types = cache.block(physics+0x28,0x68)
            fields.append(dict(definition=definition['name'],datum=datum,
                physics=cache.tags[physics_datum]['name'],placements=instances,
                phantom_shapes=[dict(type=cache.u16(row+0xc),index=cache.u16(row+0xe),
                    raw_hex=cache.data[row:row+0x20].hex()) for row in phantoms],
                force_types=[dict(flags=cache.u32(row),raw_hex=cache.data[row:row+0x68].hex(),
                    linear_coefficients=list(struct.unpack_from('<6f',cache.data,row+0x20)),
                    angular_coefficients=list(struct.unpack_from('<2f',cache.data,row+0x58)))
                    for row in force_types]))
        return dict(map=str(path),map_sha256=file_sha(path),fields=fields)
    finally:
        cache.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    maps = [inspect(Path(directory)/(name+'.map'))
            for names,directory in ((DISC,'assets/disc/maps'),(PACK,'assets/map-pack/maps'))
            for name in names]
    assert len(maps) == 21
    definitions = sum(len(row['fields']) for row in maps)
    placements = sum(len(field['placements']) for row in maps for field in row['fields'])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(
        boundary='All21 supplied Xbox multiplayer caches; placed SCEN/MACH/CTRL/BLOC phmo phantom blocks. Includes placements not automatically created. Not all spawned/dynamic types, BSP force owners, or live parity.',
        definitions=definitions,placements=placements,maps=maps),indent=2)+'\n')
    print(f'PASS inventory: {len(maps)} maps, {definitions} map-local definitions, {placements} authored placements')


if __name__ == '__main__':
    main()
