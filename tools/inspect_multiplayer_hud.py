#!/usr/bin/env python3
"""Read authored Halo 2 Xbox new-HUD metadata; never alter original assets."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
from inspect_multiplayer import Cache, ROOT


class HudCache(Cache):
    def sid(self, value):
        index = value & 0xffff
        if index >= self.u(356):
            return f'{value:08x}'
        return self.text(self.u(368) + self.u(self.u(364) + index * 4))

    def pairs(self, at, kind, count):
        return [list(struct.unpack_from('<' + kind * 2, self.data, at + i * struct.calcsize('<' + kind * 2))) for i in range(count)]

    def effects(self, at):
        result = []
        for p in self.block(at, 0x68):
            functions = []
            for q in (p + 4, p + 0x18, p + 0x2c, p + 0x40, p + 0x54):
                size = self.u(q + 12)
                address = self.u(q + 16) - self.magic if size else None
                functions.append(dict(input=self.sid(self.u(q)), range=self.sid(self.u(q + 4)),
                                      period=self.f(q + 8), data=self.data[address:address + size].hex() if size else ''))
            result.append(dict(offset=f'{p:x}', flags=self.u(p), functions=functions))
        return result

    def hud(self, t):
        p = t['offset']
        out = dict(t, link=self.reference(self.u(p + 4)), clip_cutoff=self.h(p + 0x18),
                   ammo_cutoff=self.h(p + 0x1a), age_cutoff=self.f(p + 0x1c))
        for kind, block, stride in (('bitmap', 8, 0x64), ('text', 0x10, 0x54)):
            rows = []
            for q in self.block(p + block, stride):
                row = dict(offset=f'{q:x}', name=self.sid(self.u(q)), inputs=list(self.data[q + 4:q + 8]),
                           yes=list(struct.unpack_from('<4H', self.data, q + 8)),
                           no=list(struct.unpack_from('<4H', self.data, q + 16)),
                           cutoffs=list(struct.unpack_from('<3B', self.data, q + 24)),
                           anchor=self.h(q + 0x1c), flags=self.h(q + 0x1e))
                if kind == 'bitmap':
                    row.update(bitmap=self.reference(self.u(q + 0x24)), shader=self.reference(self.u(q + 0x2c)),
                               sequences=list(struct.unpack_from('<3b', self.data, q + 0x30)),
                               offsets=self.pairs(q + 0x34, 'h', 3),
                               registration=self.pairs(q + 0x40, 'f', 3),
                               effects=self.effects(q + 0x58), special=self.h(q + 0x60))
                else:
                    row.update(shader=self.reference(self.u(q + 0x24)), text=self.sid(self.u(q + 0x28)),
                               justification=self.h(q + 0x2c), fonts=list(self.data[q + 0x30:q + 0x33]),
                               scales=list(struct.unpack_from('<3f', self.data, q + 0x34)),
                               offsets=self.pairs(q + 0x40, 'h', 3), effects=self.effects(q + 0x4c))
                rows.append(row)
            out[kind] = rows
        out['screen_effects'] = [dict(offset=f'{q:x}', bytes=self.data[q:q + 0x50].hex()) for q in self.block(p + 0x20, 0x50)]
        return out

    def bitmap(self, t):
        p = t['offset']
        images = [dict(index=i, offset=f'{q:x}', width=self.h(q + 4), height=self.h(q + 6),
                       format=self.h(q + 12), flags=self.h(q + 14), registration=self.pairs(q + 16, 'h', 1)[0],
                       mipmaps=self.h(q + 20), pixels=f'{self.u(q + 24):08x}', size=self.u(q + 0x34))
                  for i, q in enumerate(self.block(p + 0x44, 0x74))]
        sequences = []
        for i, q in enumerate(self.block(p + 0x3c, 0x3c)):
            sprites = []
            for s in self.block(q + 0x34, 0x20):
                image = self.h(s)
                uv = list(struct.unpack_from('<4f', self.data, s + 8))
                sprites.append(dict(image=image, uv=uv, registration=self.pairs(s + 24, 'f', 1)[0],
                                    size=[(uv[1] - uv[0]) * images[image]['width'], (uv[3] - uv[2]) * images[image]['height']]))
            sequences.append(dict(index=i, name=self.text(q), first=self.h(q + 0x20), count=self.h(q + 0x22), sprites=sprites))
        return dict(t, type=self.h(p), images=images, sequences=sequences)

    def shader(self, t):
        p = t['offset']
        params = []
        for q in self.block(p + 0x18, 0x28):
            params.append(dict(name=self.sid(self.u(q)), type=self.h(q + 4), bitmap=self.reference(self.u(q + 12)),
                               value=self.f(q + 16), color=list(struct.unpack_from('<3f', self.data, q + 20))))
        postprocess = []
        for q in self.block(p + 0x20, 0x7c):
            postprocess.append(dict(template=self.reference(self.u(q)),
                                    bitmaps=[dict(bitmap=self.reference(self.u(s)), index=self.i(s + 4)) for s in self.block(q + 4, 12)],
                                    pixel_constants=[f'{self.u(s):08x}' for s in self.block(q + 12, 4)],
                                    vertex_constants=[list(struct.unpack_from('<4f', self.data, s)) for s in self.block(q + 20, 16)],
                                    color_properties=[list(struct.unpack_from('<3f', self.data, s)) for s in self.block(q + 0x64, 12)],
                                    value_properties=[self.f(s) for s in self.block(q + 0x6c, 4)],
                                    overlays=[dict(input=self.sid(self.u(s)), range=self.sid(self.u(s + 4)), period=self.f(s + 8),
                                                   data=self.data[self.u(s + 16) - self.magic:self.u(s + 16) - self.magic + self.u(s + 12)].hex() if self.u(s + 12) else '')
                                              for s in self.block(q + 0x3c, 0x14)]))
        return dict(t, template=self.reference(self.u(p + 4)), flags=self.h(p + 0x16), parameters=params, postprocess=postprocess)

    def template(self, t):
        """Keep compact cache pass words intact; no PC D3D interpretation."""
        out = dict(t, passes=[])
        for pp in self.block(t['offset'] + 0x58, 0x28):
            for p in self.block(pp + 0x10, 10):
                tag = self.tags[self.u(p + 4)]
                passes = []
                for q in self.block(tag['offset'] + 0x1c, 0x58):
                    # Xbox uses compact 0x132 implementations and 4-byte
                    # fragments (retail 0x1BA00); XML's PC-sized 0x14A
                    # implementation must not be used for these caches.
                    passes.append(dict(offset=f'{q:x}',
                                       implementations=[dict(offset=f'{s:x}', bytes=self.data[s:s + 0x132].hex(),
                                                             program=[f'{self.u(s + 6 + i * 4):08x}' for i in range(60)])
                                                        for s in self.block(q, 0x132)],
                                       textures=[list(self.data[s:s + 4]) for s in self.block(q + 8, 4)],
                                       states=[dict(state=self.data[s], value=f'{self.u(s + 1):08x}') for s in self.block(q + 16, 5)],
                                       fragments=[list(struct.unpack_from('<hH', self.data, s)) for s in self.block(q + 32, 4)],
                                       combiners=[self.data[s:s + 32].hex() for s in self.block(q + 48, 32)],
                                       externs=[list(self.data[s:s + 4]) for s in self.block(q + 56, 4)]))
                out['passes'].append(dict(tag, postprocess=passes))
        return out

    def inspect(self):
        nhdts = [self.hud(t) for t in self.tags.values() if t['group'] == 'nhdt']
        datums = {int(r[k]['datum'], 16) for h in nhdts for kind in ('bitmap', 'text') for r in h[kind] for k in ('bitmap', 'shader') if r.get(k)}
        globals_root = next(t['offset'] for t in self.tags.values() if t['group'] == 'matg')
        interface = self.block(globals_root + 0x110, 0x98)[0]
        hudg = self.tags[self.u(interface + 0x34)]
        g = hudg['offset']
        datums.add(self.u(interface + 0x4c))
        selectors = []
        for t in self.tags.values():
            if t['group'] == 'weap':
                selectors.append(dict(tag=self.reference(int(t['datum'], 16)), hud=self.reference(self.u(t['offset'] + 0x2b4))))
            elif t['group'] == 'bipd' and 'masterchief_mp' in t['name']:
                selectors.append(dict(tag=self.reference(int(t['datum'], 16)), hud=[self.reference(self.u(p + 4)) for p in self.block(t['offset'] + 0x1a0, 8)]))
        shaders = [self.shader(self.tags[d]) for d in sorted(datums) if self.tags[d]['group'] == 'shad']
        templates = {int(s['template']['datum'], 16) for s in shaders}
        return dict(map=str(self.path), sha256=hashlib.sha256(self.data).hexdigest(), groups=dict(Counter(t['group'] for t in self.tags.values())),
                    selectors=selectors, hudg=dict(hudg, interface_offset=f'{interface:x}', multiplayer_bitmap=self.reference(self.u(interface + 0x4c)),
                                                  motion_range=self.f(g + 0x290), motion_speed=self.f(g + 0x294), motion_scale=self.f(g + 0x298),
                                                  motion_bounds=list(struct.unpack_from('<4h', self.data, g + 0x29c))),
                    nhdt=nhdts, bitmaps=[self.bitmap(self.tags[d]) for d in sorted(datums) if self.tags[d]['group'] == 'bitm'],
                    shaders=shaders, templates=[self.template(self.tags[d]) for d in sorted(templates)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('map', nargs='?', type=Path, default=ROOT / 'assets/disc/maps/lockout.map')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = HudCache(args.map).inspect()
    output = json.dumps(result, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output)
    else:
        print(output, end='')


if __name__ == '__main__':
    main()
