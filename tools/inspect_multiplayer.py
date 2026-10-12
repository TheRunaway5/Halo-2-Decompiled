#!/usr/bin/env python3
"""Inspect original Xbox multiplayer cache metadata without modifying game assets.

Writes only an optional host JSON report. The offsets are from the pinned
Assembly Halo2 schemas; output records file offsets so findings are reproducible.
No packet format, spawn weighting, or simulation behavior is inferred here.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
MAPS = ('ascension', 'beavercreek', 'burial_mounds', 'coagulation', 'colossus',
        'cyclotron', 'foundation', 'headlong', 'lockout', 'midship',
        'waterworks', 'zanzibar')


class Cache:
    def __init__(self, path):
        self.path = path
        self.stream = path.open('rb')
        self.data = mmap.mmap(self.stream.fileno(), 0, access=mmap.ACCESS_READ)
        assert self.u(0) == 0x68656164 and self.u(4) == 8
        assert self.u(8) == len(self.data) and self.text(288) == '02.09.27.09809'
        index = self.u(16)
        table = self.u(index + 8) - (self.u(index) - (index + 32))
        self.magic = self.u(table + 8) - (index + self.u(20))
        name_base, name_table = self.u(708), self.u(716)
        self.tags = {}
        for i in range(self.u(index + 24)):
            at = table + i * 16
            datum, address, size = struct.unpack_from('<III', self.data, at + 4)
            self.tags[datum] = dict(datum=f'{datum:08x}',
                                   group=self.data[at:at + 4][::-1].decode('ascii'),
                                   offset=address - self.magic if address else None,
                                   size=size,
                                   name=self.text(name_base + self.u(name_table + i * 4)))
        self.scenario = self.tags[self.u(index + 12)]['offset']

    def u(self, at): return struct.unpack_from('<I', self.data, at)[0]
    def i(self, at): return struct.unpack_from('<i', self.data, at)[0]
    def h(self, at): return struct.unpack_from('<h', self.data, at)[0]
    def f(self, at): return struct.unpack_from('<f', self.data, at)[0]

    def text(self, at):
        end = self.data.find(b'\0', at)
        assert 0 <= at <= end < len(self.data)
        return self.data[at:end].decode('utf-8', errors='replace')

    def block(self, at, stride, limit=65536):
        count = self.u(at)
        assert count <= limit
        if not count:
            return []
        start = self.u(at + 4) - self.magic
        assert 0 <= start and start + count * stride <= len(self.data)
        return [start + i * stride for i in range(count)]

    def reference(self, datum):
        if datum == 0xffffffff:
            return None
        tag = self.tags[datum]
        return {key: tag[key] for key in ('datum', 'group', 'name')}

    def collection(self, datum):
        result = self.reference(datum)
        if not result or result['group'] not in ('itmc', 'vehc'):
            return result
        at = self.tags[datum]['offset']
        result['spawn_seconds'] = self.h(at + 8)
        result['items'] = [dict(file_offset=f'{p:08x}', weight=self.f(p),
                                item=self.reference(self.u(p + 8)),
                                variant=f'{self.u(p + 12):08x}')
                           for p in self.block(at, 16)]
        return result

    def globals(self):
        root = next(t['offset'] for t in self.tags.values() if t['group'] == 'matg')
        mulg = self.u(root + 0x16c)
        rows = []
        for p in self.block(self.tags[mulg]['offset'] + 8, 0x568):
            rows.append(dict(file_offset=f'{p:08x}',
                             unit=self.reference(self.u(p + 0x14)),
                             primary=self.collection(self.u(p + 0x114)),
                             secondary=self.collection(self.u(p + 0x11c)),
                             frag_count=self.i(p + 0x120),
                             plasma_count=self.i(p + 0x124)))
        representations = [dict(index=i, file_offset=f'{p:08x}',
                                hands=self.reference(self.u(p + 4)),
                                unit=self.reference(self.u(p + 0xb4)))
                           for i, p in enumerate(self.block(root + 0x138, 0xbc))]
        grenades = [dict(index=i, maximum=self.h(p),
                        equipment=self.reference(self.u(p + 0x20)),
                        projectile=self.reference(self.u(p + 0x28)))
                    for i, p in enumerate(self.block(root + 0x100, 0x2c))]
        return dict(multiplayer=self.reference(mulg), runtime=rows,
                    player_representations=representations, grenades=grenades)

    def inspect_map(self):
        s = self.scenario
        starts = [dict(index=i, file_offset=f'{p:08x}',
                       position=list(struct.unpack_from('<3f', self.data, p)),
                       facing=self.f(p + 12), team=self.h(p + 0x10),
                       bsp=self.h(p + 0x12),
                       modes=list(struct.unpack_from('<4h', self.data, p + 0x14)),
                       spawn_types=list(struct.unpack_from('<4h', self.data, p + 0x1c)),
                       campaign_type=self.h(p + 0x2c))
                  for i, p in enumerate(self.block(s + 0x100, 0x34))]
        equipment = [dict(index=i, file_offset=f'{p:08x}', flags=self.u(p),
                          modes=list(struct.unpack_from('<4h', self.data, p + 4)),
                          team=self.h(p + 12), spawn_seconds=self.h(p + 14),
                          respawn_on_empty_seconds=self.h(p + 16),
                          timer_starts=self.h(p + 18), classification=self.data[p + 20],
                          position=list(struct.unpack_from('<3f', self.data, p + 0x40)),
                          orientation=list(struct.unpack_from('<3f', self.data, p + 0x4c)),
                          collection=self.collection(self.u(p + 0x5c)))
                     for i, p in enumerate(self.block(s + 0x120, 0x90))]
        profiles = [dict(index=i, name=self.text(p),
                         primary=self.reference(self.u(p + 0x2c)),
                         secondary=self.reference(self.u(p + 0x38)))
                    for i, p in enumerate(self.block(s + 0xf8, 0x44))]
        starting_equipment = [dict(index=i, file_offset=f'{p:08x}', flags=self.u(p),
                                   modes=list(struct.unpack_from('<4h', self.data, p + 4)),
                                   collections=[self.collection(self.u(p + 0x40 + n * 8))
                                                for n in range(6)])
                              for i, p in enumerate(self.block(s + 0x128, 0x9c))]
        self.stream.seek(0)
        return dict(map=self.path.name, sha256=hashlib.file_digest(self.stream, 'sha256').hexdigest(),
                    scenario_file_offset=f'{s:08x}', scenario_type=self.h(s + 0x1c),
                    starts=starts, equipment=equipment, campaign_profiles=profiles,
                    starting_equipment=starting_equipment,
                    globals=self.globals())

    def inspect_variants(self):
        schema = ET.parse(ROOT / 'tools/reference/assembly/src/Assembly/Plugins/Halo2/sily.xml')
        names = {int(e.get('value'), 0): e.get('name') for e in schema.getroot().find('enum32')}
        root = next(t['offset'] for t in self.tags.values() if t['group'] == 'matg')
        locale_at = root + 0x190
        count, size, indices, strings = struct.unpack_from('<4I', self.data, locale_at)
        assert count <= 65536 and strings + size <= len(self.data)
        locale = [(self.u(indices + i * 8), self.text(strings + self.u(indices + i * 8 + 4)))
                  for i in range(count)]

        def string_list(datum):
            at = self.tags[datum]['offset']
            start, length = struct.unpack_from('<HH', self.data, at + 16)
            assert start + length <= len(locale)
            return dict(locale[start:start + length])
        defaults = {}
        for tag in self.tags.values():
            if tag['group'] != 'sily':
                continue
            p = tag['offset']
            localized = string_list(self.u(p + 12))
            parameter = self.u(p)
            defaults[parameter] = dict(parameter=parameter, name=names.get(parameter),
                                       tag=tag['name'], file_offset=f'{p:08x}',
                                       serialized_value=self.i(p + 4), title=localized.get(self.u(p + 16)),
                                       choices=[dict(value=self.i(a + 4), label=localized.get(self.u(a + 8)))
                                                for a in self.block(p + 28, 12)])
        variants = []
        for tag in self.tags.values():
            if tag['group'] != 'goof':
                continue
            localized = string_list(self.u(tag['offset'] + 36))
            for p in self.block(tag['offset'] + 40, 20):
                overrides = {self.u(a): self.i(a + 4) for a in self.block(p + 8, 8)}
                variants.append(dict(name=localized.get(self.u(p)), engine=self.u(p + 4),
                                     file_offset=f'{p:08x}', overrides=overrides))
        return dict(map=self.path.name, parameters=defaults, variants=variants)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('caches', type=Path, nargs='*')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    paths = args.caches or [ROOT / f'assets/disc/maps/{name}.map' for name in MAPS]
    rows = []
    for path in paths:
        cache = Cache(path)
        rows.append(cache.inspect_variants() if path.name == 'mainmenu.map' else cache.inspect_map())
    output = json.dumps(rows, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output)
    else:
        print(output, end='')


if __name__ == '__main__':
    main()
