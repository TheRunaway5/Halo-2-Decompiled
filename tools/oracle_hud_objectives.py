#!/usr/bin/env python3
"""Execute unchanged disc/TU1.5 objective queries and HUD quad construction.

Actual Lockout metadata/palette and explicit retained-bank stimuli are supplied.
Only platform environment, appearance selection, and the graphics submission
are supplied returns. Query, color packing, meter repetition and geometry run
original instructions. Run through tools/run_test.py; no raw banks are saved.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, STOP, STACK, transplant
from oracle_hud_score_icons import TU_PIN
from inspect_multiplayer_hud import HudCache
from xbe import Xbe


class Original(Retail):
    def __init__(self, path):
        x = Xbe(path)
        self.digest = hashlib.sha256(x.data).hexdigest()
        assert self.digest in (PIN, TU_PIN)
        self.tu = self.digest == TU_PIN
        self.delta = 0x6a00 if self.tu else 0
        self.g = g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in x.sections:
            g.mem_write(section.va, x.section_bytes(section))
        g.mem_map(0x1000000, 0x400000)
        g.mem_map(0x2000000, 0x20000)
        g.mem_map(STOP, 0x1000)
        self.next = 0x1000000
        self.table = self.alloc(bytes(0x10000 * 16))
        self.w(0x4f0414 if self.tu else 0x4e3b44, self.table)
        self.outer, self.safe = (0x4c65f0, 0x4c65f8) if self.tu else (0x4b9dd0, 0x4b9dd8)
        self.options_global = 0x4f3218 if self.tu else 0x4e6948
        self.player_global = 0x4f54f4 if self.tu else 0x4e8c24
        self.engine_global = 0x4f63b8 if self.tu else 0x4e9ae8
        self.object_global = 0x4ecbd0 if self.tu else 0x4e0300
        self.color = 0x4f3260 if self.tu else 0x4e6990
        self.inputs = 0x4f3250 if self.tu else 0x4e6980
        self.locals = self.alloc(bytes(32))
        self.w(0x4f54f0 if self.tu else 0x4e8c20, self.locals)
        self.w(0x51df50 if self.tu else 0x510c50, 0)
        self.w(self.options_global, 0)
        self.appearance, self.wide, self.geometry_only = 0, False, False
        self.quads = []
        self.quad_ready = False
        g.hook_add(UC_HOOK_CODE, self.hook)

    def hook(self, uc, address, size, _):
        if address == (0x16a70 if self.tu else 0x16a90):
            self.ret(int(self.wide))
        elif address == (0x1508e0 if self.tu else 0x14ddc0):
            self.ret(0)
        elif address == 0x22acb4 + self.delta:
            self.ret(self.appearance)
        elif address == 0x22b299 + self.delta and self.geometry_only:
            self.quad_ready = True
            uc.emu_stop()
        elif address == 0x22a664 + self.delta:
            p = uc.reg_read(UC_X86_REG_ECX)
            self.quads.append(dict(points=self.read(p, '<8f'),
                colors=self.read(self.color, '<6I'), inputs=self.read(self.inputs, '<4I')))
            self.ret(0)
            uc.reg_write(UC_X86_REG_ESP, uc.reg_read(UC_X86_REG_ESP) + 12)

    def geometry(self, widget, status, inputs, view, players, cell, wide, elite=False, ordinary=False):
        self.wide, self.appearance, self.geometry_only = wide, int(elite), ordinary
        self.g.mem_write(self.locals + 8, struct.pack('<h', players))
        for dest, key in ((self.outer, 'outer'), (self.safe, 'safe')):
            self.g.mem_write(dest, struct.pack('<4h', *cell[key]))
        self.quads, self.quad_ready = [], False
        self.prepare([view, status, widget, inputs])
        self.g.emu_start(0x22aff1 + self.delta, STOP, count=200000)
        if ordinary:
            assert self.quad_ready
            frame = self.g.reg_read(UC_X86_REG_EBP)
            return [dict(points=self.read(frame + 0xc, '<8f'))]
        assert self.g.reg_read(UC_X86_REG_EIP) == STOP
        return self.quads

    def setup_query(self, palette):
        self.options = self.alloc(bytes(0x800)); self.w(self.options_global, self.options)
        self.engine = self.alloc(bytes(0x400)); self.w(self.engine_global, self.engine)
        self.pool = self.alloc(bytes(0x80)); self.w(self.player_global, self.pool)
        self.rows = self.alloc(bytes(16 * 0x21c)); self.w(self.pool + 0x44, self.rows)
        self.bitmap = self.alloc(bytes(4)); self.w(self.pool + 0x48, self.bitmap)
        self.w(self.pool + 0x20, 16); self.w(self.pool + 0x24, 0x21c); self.w(self.pool + 0x38, 16)
        matg = self.matg = self.alloc(bytes(0x200)); mulg = self.alloc(bytes(0x40))
        universal = self.alloc(bytes(0x30)); rgb = self.alloc(struct.pack('<' + 'I' * len(palette), *palette))
        self.w(0x4ecc1c if self.tu else 0x4e034c, matg)
        self.w(matg + 0x16c, 1); self.w(self.table + 16 + 8, mulg)
        self.w(mulg + 4, universal); self.w(universal + 0x10, len(palette) // 3); self.w(universal + 0x14, rgb)
        self.object_pool = self.alloc(bytes(0x80)); self.w(self.object_global, self.object_pool)
        self.headers = self.alloc(bytes(24)); self.w(self.object_pool + 0x44, self.headers)
        self.unit = self.alloc(bytes(0x400)); self.w(self.headers + 8, self.unit)
        self.weapon = self.alloc(bytes(0x200)); self.w(self.headers + 20, self.weapon)
        definition = self.alloc(bytes(0x300)); self.w(self.table + 2 * 16 + 8, definition)
        self.definition = definition; self.w(self.weapon, 2)
        self.bombs = self.alloc(bytes(0x200)); self.w(0x52d710 if self.tu else 0x51ec80, self.bombs)
        self.clock = self.alloc(bytes(16)); self.w(0x51df54 if self.tu else 0x510c54, self.clock)
        self.output = self.alloc(bytes(0xfc))

    def query(self, case):
        engine, local, alive, active, count, players, indices, owners, bomb, armed, index, ticks, seconds, duration, missing_palette = case
        self.w(self.matg + 0x16c, 0xffffffff if missing_palette else 1)
        self.w(self.options + 0x180, engine)
        self.g.mem_write(self.options + 0x244, struct.pack('<h', seconds))
        self.w(self.clock + 4, duration); self.w(self.bitmap, active)
        for n, (member, capturing, capture) in enumerate(players):
            row = self.rows + n * 0x21c
            self.g.mem_write(row, struct.pack('<H', 0x7000 + n))
            self.g.mem_write(row + 2, b'\0')
            self.w(row + 0x2c, 0x71000000 if alive else 0xffffffff)
            self.g.mem_write(row + 0xc0, struct.pack('<b', member))
            self.g.mem_write(self.engine + 0x191 + n * 8, struct.pack('<bB', capturing, capture))
        self.g.mem_write(self.engine + 0x210, struct.pack('<h', count))
        self.g.mem_write(self.engine + 0x15c, struct.pack('<8h', *indices))
        self.g.mem_write(self.engine + 0x16c, struct.pack('<8I', *owners))
        self.g.mem_write(self.unit + 0x212, b'\0' if bomb else b'\xff')
        self.w(self.unit + 0x218, 0x72000001)
        self.g.mem_write(self.definition + 0x290, struct.pack('<h', int(bomb)))
        self.g.mem_write(self.weapon + 0x17e, struct.pack('<h', index))
        if index >= 0:
            self.g.mem_write(self.bombs + 0x170 + index, bytes([8 if armed else 0]))
            self.g.mem_write(self.bombs + 0x1a0 + index * 2, struct.pack('<h', ticks))
        self.g.mem_write(self.output, bytes(0xfc))
        self.prepare(registers=[(UC_X86_REG_EAX, self.options), (UC_X86_REG_EBP, self.output)])
        self.w(STACK + 0x20, self.rows + local * 0x21c)
        self.w(STACK + 0x38, local); self.w(STACK + 0x4a0, self.output)
        # The selected subrange begins after the score fields. Recreate the
        # enclosing callee's unchanged stack frame and its real return slot.
        self.w(STACK + 0x49c, STOP)
        try:
            self.g.emu_start(0x162399 if self.tu else 0x15f919, STOP, count=200000)
        except Exception as error:
            raise RuntimeError(f'Objective query tu={self.tu} pc={self.g.reg_read(UC_X86_REG_EIP):08x} case={case}') from error
        assert self.g.reg_read(UC_X86_REG_EIP) == STOP
        return (self.read(self.output + 0xb0, '<B')[0], self.r(self.output + 0xb4),
            self.r(self.output + 0xb8), *self.read(self.output + 0xbc, '<16I'))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--tu15', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--geometry', type=Path, required=True)
    p.add_argument('--queries', type=Path, required=True)
    args = p.parse_args()
    cache = HudCache(ROOT / 'assets/disc/maps/lockout.map')
    tags = {t['name']: t for t in cache.tags.values() if t['group'] == 'nhdt'}
    rows = [(0, w) for w in cache.hud(tags['ui\\hud\\scoreboard'])['bitmap']
            if w['name'] == 'bomb_defusal' or w['special'] == 7]
    for elite, species in enumerate(('masterchief', 'dervish')):
        rows += [(elite, next(w for w in cache.hud(tags['ui\\hud\\' + species])['bitmap']
                    if w['name'] == name)) for name in ('shield_meter', 'shield_mask', 'motion_tracker_background')]
    originals = [Original(path) for path in (ROOT / 'assets/disc/default.xbe', args.tu15)]
    widgets = [[transplant(cache, r, row) for _, row in rows] for r in originals]
    statuses = [r.alloc(bytes(0x400)) for r in originals]
    inputs = [r.alloc(bytes(16)) for r in originals]
    environment = Retail()
    lines = ['# disc/TU1.5 original geometry: elite wide players view widget count index left top right bottom']
    cases = 0
    for wide in (False, True):
        environment.video(int(wide))
        for players in range(1, 5):
            _, cells = environment.cells(players)
            for cell in cells:
                for n, (elite, row) in enumerate(rows):
                    for count in (range(9) if row['special'] == 7 else (0,)):
                        actual = []
                        for v, r in enumerate(originals):
                            r.w(statuses[v] + 0x22c, count)
                            for k in range(8):
                                r.w(statuses[v] + 0x230 + k * 4, (0xff0000, 0xff, 0x00ff00)[k % 3])
                                r.w(statuses[v] + 0x250 + k * 4, struct.unpack('<I', struct.pack('<f', k / 7))[0])
                            actual.append(r.geometry(widgets[v][n], statuses[v], inputs[v], cell['view'], players,
                                cell, wide, elite, row['special'] == 5))
                        assert actual[0] == actual[1], (row['name'], wide, players, cell, actual)
                        assert len(actual[0]) == (count if row['special'] == 7 else 1)
                        for k, q in enumerate(actual[0]):
                            xy = q['points']; bounds = (min(xy[::2]), min(xy[1::2]), max(xy[::2]), max(xy[1::2]))
                            lines.append(' '.join(map(str, [elite, int(wide), players, cell['view'], row['name'], count, k, *bounds])))
                            if row['special'] == 7:
                                assert q['colors'][3:] == [0, 0, 0]
                        cases += 1
    # Actual multiplayer team palette, consumed unchanged by original83050.
    mulg = next(t for t in cache.tags.values() if t['group'] == 'mulg')
    universal = cache.block(mulg['offset'], 0x20)[0]
    palette = [cache.u(q + c * 4) for q in cache.block(universal + 0x10, 12) for c in range(3)]
    for r in originals:
        r.setup_query(palette)
    rng = random.Random(0x161e20)
    queries = ['# palette RGB bits followed by source query stimuli and unchanged output bits', ' '.join(map(str, palette))]
    query_cases = []
    for n in range(168):
        engine = 8 if n < 112 else 9
        local = n % 16
        players = [(rng.choice((-1, 0, 1, 3, 7, 31)), rng.randrange(-1, 8), rng.choice((0, 1, 31, 63, 127, 255))) for _ in range(16)]
        players[local] = (0, n % 9 - 1, (0, 1, 31, 63, 255)[n % 5])
        count = n % 9
        indices = [rng.choice((-1, i)) if i < count else -1 for i in range(8)]
        owners = [0xffffffff if rng.randrange(2) else ((0x7000 + i) << 16) | i for i in range(8)]
        duration = struct.unpack('<I', struct.pack('<f', (1 / 30, 1 / 60)[n % 2]))[0]
        case = (engine, local, n % 7 != 0, rng.randrange(65536), count, players, indices, owners,
                n % 5 != 0, n % 11 == 0, (-1, 0, 1, 7)[n % 4], (-3, 0, 1, 30, 255, 32767)[n % 6],
                (0, 1, 3, -2)[n % 4], duration, n % 13 == 0)
        if n >= 160:
            # Actual neutral objective row8, including armed/unarmed and
            # negative/zero/positive progress with both original clocks.
            armed, ticks, seconds = (
                (False, -3, 0), (False, 0, 1), (False, 1, 3), (False, 30, 1),
                (False, 30, -2), (True, -3, 0), (True, 0, 1), (True, 32767, 3),
            )[n - 160]
            case = (9, local, True, 1 << local, count, players, indices, owners,
                    True, armed, 8, ticks, seconds, duration, False)
        actual = [r.query(case) for r in originals]
        assert actual[0] == actual[1], (n, actual)
        flat = [*case[:5], *(v for row in players for v in row), *indices, *owners, *case[8:], *actual[0]]
        queries.append(' '.join(str(int(v)) for v in flat))
        query_cases.append(actual[0])
    for path, content in ((args.geometry, lines), (args.queries, queries)):
        path.parent.mkdir(parents=True, exist_ok=True); path.write_text('\n'.join(content) + '\n')
    report = dict(scope=__doc__, executable_sha256=[r.digest for r in originals],
        map_sha256=hashlib.sha256(cache.data).hexdigest(), geometry_dispatch_cases=cases,
        geometry_quads=len(lines) - 1, query_cases=len(query_cases), instruction_patches=[],
        geometry_sha256=hashlib.sha256(args.geometry.read_bytes()).hexdigest(),
        queries_sha256=hashlib.sha256(args.queries.read_bytes()).hexdigest())
    args.output.parent.mkdir(parents=True, exist_ok=True); args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS disc/TU1.5: {cases} dispatches, {len(lines)-1} quads, {len(query_cases)} objective queries')


if __name__ == '__main__':
    main()
