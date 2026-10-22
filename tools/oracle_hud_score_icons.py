#!/usr/bin/env python3
"""Observe unchanged disc/TU1.5 score-icon special dispatch and disc geometry.

Actual Lockout widget/bitmap/effect and palette bytes are relocated. Salted
player appearance, selected score identities/colors and viewport inputs are
explicit stimuli. Dispatch, appearance lookup/copy, palette packing and quad
construction execute original instructions. Graphics and a live Xbox frame
are outside this CPU evidence. Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EBP, UC_X86_REG_ECX
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, STOP, transplant
from inspect_multiplayer_hud import HudCache
from xbe import Xbe

TU_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


class Dispatch(Retail):
    def __init__(self, path, cache):
        self.image = image = Xbe(path)
        self.digest = hashlib.sha256(image.data).hexdigest()
        if self.digest == PIN:
            self.version = 'disc'
            self.begin, self.end = 0x22b2d0, 0x22b5cb
            hud, pool, globals_, scenario = 0x510c4c, 0x4e8c24, 0x4e034c, 0x4e0350
            self.color = 0x4e6990
        elif self.digest == TU_PIN:
            self.version = 'TU1.5'
            self.begin, self.end = 0x231cd0, 0x231fcb
            hud, pool, globals_, scenario = 0x51df4c, 0x4f54f4, 0x4ecc1c, 0x4ecc20
            self.color = 0x4f3260
        else:
            raise ValueError('Expected the owned disc or English TU1.5 executable')
        self.g = g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(0x1000000, 0x400000)
        g.mem_map(0x2000000, 0x10000)
        g.mem_map(STOP, 0x1000)
        self.next = 0x1000000
        self.hud = self.alloc(bytes(0x220)); self.w(hud, self.hud)
        player_pool = self.alloc(bytes(0x80))
        self.players = self.alloc(bytes(16 * 0x21c))
        self.w(pool, player_pool); self.w(player_pool + 0x44, self.players)
        matg = next(t['offset'] for t in cache.tags.values() if t['group'] == 'matg')
        header = self.alloc(cache.data[matg:matg + 0x168])
        palette = self.alloc(b''.join(cache.data[p:p + 12] for p in cache.block(matg + 0x160, 12)))
        self.w(header + 0x164, palette); self.w(globals_, header); self.w(scenario, 1)
        self.frame, self.status, self.widget = [self.alloc(bytes(0x400)) for _ in range(3)]

    def dispatch(self, special, datum, color):
        self.w(self.frame + 0x74, self.status); self.w(self.frame + 0x78, self.widget)
        self.g.mem_write(self.widget + 0x60, struct.pack('<H', special))
        offset = 0x188 if special == 1 else 0x1d4
        self.w(self.status + offset, datum)
        self.g.mem_write(self.status + offset + 4, struct.pack('<3I', *color))
        self.run(self.begin, stop=self.end,
                 registers=[(UC_X86_REG_EBP, self.frame), (UC_X86_REG_ECX, self.widget)])
        return dict(selected_player=self.r(self.hud + 0x1b4),
                    score_color=list(struct.unpack('<3I', self.g.mem_read(self.color, 12))),
                    appearance=bytes(self.g.mem_read(self.hud + 0x1c0, 16)).hex() if datum != 0xffffffff else None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tu15', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    args = parser.parse_args()
    cache = HudCache(ROOT / 'assets/disc/maps/lockout.map')
    scoreboard = next(t for t in cache.tags.values() if t['group'] == 'nhdt' and t['name'] == 'ui\\hud\\scoreboard')
    widgets = [w for w in cache.hud(scoreboard)['bitmap'] if w['special'] in (1, 2)]
    appearances = [bytes([1, 2, 3, 5, 0, 63, 31, 0]) + bytes(8),
                   bytes([4, 2, 3, 5, 1, 17, 12, 3]) + bytes(8)]
    colors = [(0x3f800000, 0, 0), (0, 0, 0x3f800000)]
    versions = []
    for path in (ROOT / 'assets/disc/default.xbe', args.tu15):
        r = Dispatch(path, cache)
        for slot, appearance in zip((0, 15), appearances):
            r.g.mem_write(r.players + slot * 0x21c + 0x84, appearance)
        cases = []
        for special in (1, 2):
            for slot in (0, 15, -1):
                for color in colors:
                    datum = 0xffffffff if slot < 0 else ((0x7000 + slot) << 16) | slot
                    result = r.dispatch(special, datum, color)
                    assert result['selected_player'] == datum and result['score_color'] == list(color)
                    if slot >= 0:
                        assert result['appearance'] == appearances[int(slot == 15)].hex()
                    cases.append(dict(special=special, player=datum, color=color, result=result))
        versions.append(dict(version=r.version, xbe_sha256=r.digest,
                             entry=f'{r.begin:08x}', stop=f'{r.end:08x}', cases=cases))
    assert versions[0]['cases'] == versions[1]['cases']
    retail = Retail()
    transplanted = [(w, transplant(cache, retail, w)) for w in widgets]
    status, inputs = retail.alloc(bytes(0x400)), retail.alloc(bytes(16))
    lines = ['# Unchanged disc22AFF1 geometry for real Lockout score icon widgets.',
             '# winning widescreen players view widget left top right bottom']
    for winning in (0, 1):
        retail.g.mem_write(inputs, struct.pack('<4f', 0, winning, 0, 1))
        for flags in (0, 1):
            video = retail.video(flags)
            for players in range(1, 5):
                _, cells = retail.cells(players)
                for cell in cells:
                    retail.g.mem_write(0x4b9dd0, struct.pack('<4h', *cell['outer']))
                    retail.g.mem_write(0x4b9dd8, struct.pack('<4h', *cell['safe']))
                    for row, widget in transplanted:
                        retail.quad_ready = False
                        retail.prepare([cell['view'], status, widget, inputs])
                        retail.g.emu_start(0x22aff1, STOP, count=200000)
                        assert retail.quad_ready
                        frame = retail.g.reg_read(UC_X86_REG_EBP)
                        quad = [retail.read(frame + 0xc + i * 8, '<2f') for i in range(4)]
                        bounds = [min(p[0] for p in quad), min(p[1] for p in quad),
                                  max(p[0] for p in quad), max(p[1] for p in quad)]
                        lines.append(' '.join(map(str, [winning, int(video['widescreen']), players,
                                                       cell['view'], row['name'], *bounds])))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text('\n'.join(lines) + '\n')
    report = dict(scope=__doc__, map_sha256=hashlib.sha256(cache.data).hexdigest(),
                  source_versions=versions, widgets=widgets, dispatch_cases=48,
                  geometry_cases=len(lines) - 2, instruction_patches=[],
                  supplied_returns=dict(dispatch=[], geometry=['NTSC video standard', 'video flags0/1',
                      'no active projected unit', 'Spartan HUD appearance']),
                  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f"PASS 48 disc/TU1.5 special dispatch cases, {len(lines)-2} original score-icon quads")


if __name__ == '__main__':
    main()
