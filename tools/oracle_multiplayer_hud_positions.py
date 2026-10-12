#!/usr/bin/env python3
"""Execute pinned retail Xbox HUD bitmap geometry for every local-player cell.

No geometry or layout is calculated by a native implementation. The supplied
environment selects an ordinary gameplay session, explicit Spartan/Elite appearance, zero HUD
inputs (including score winning input), and no active player projection. Actual
Lockout nhdt/bitm metadata is copied unchanged except for host pointer relocation.
Execution stops at retail 22B299 after final bitmap quad construction, before
special-type shading, draw calls, scope expansion, and graphics APIs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from inspect_multiplayer_hud import HudCache, ROOT

sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows
from disasm import listing
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX,
                              UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                              UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
ARENA, STACK, STOP = 0x1000000, 0x200ff00, 0x3000000


class Retail:
    def __init__(self):
        x = Xbe(ROOT / 'assets/disc/default.xbe')
        assert hashlib.sha256(x.data).hexdigest() == PIN
        self.g = g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in x.sections:
            g.mem_write(section.va, x.section_bytes(section))
        g.mem_map(ARENA, 0x400000)
        g.mem_map(0x2000000, 0x10000)
        g.mem_map(STOP, 0x1000)
        self.next = ARENA
        self.flags = 0
        self.quad_ready = False
        self.appearance = 0
        g.hook_add(UC_HOOK_CODE, self.hook)
        self.w(0x4e6948, 0)  # no playback/cinematic override
        self.w(0x510c50, 0)  # no forced single-view overlay
        self.players = self.alloc(bytes(32))
        self.w(0x4e8c20, self.players)
        self.table = self.alloc(bytes(0x10000 * 16))
        self.w(0x4e3b44, self.table)

    def alloc(self, data):
        address = self.next
        self.next = (address + len(data) + 15) & ~15
        assert self.next <= ARENA + 0x400000
        self.g.mem_write(address, bytes(data))
        return address

    def w(self, at, value):
        self.g.mem_write(at, struct.pack('<I', value & 0xffffffff))

    def r(self, at):
        return struct.unpack('<I', self.g.mem_read(at, 4))[0]

    def read(self, at, form):
        return list(struct.unpack(form, self.g.mem_read(at, struct.calcsize(form))))

    def ret(self, eax):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        self.g.reg_write(UC_X86_REG_EAX, eax)
        self.g.reg_write(UC_X86_REG_EIP, self.r(sp))
        self.g.reg_write(UC_X86_REG_ESP, sp + 4)

    def hook(self, uc, address, size, _):
        if address == 0x330310:  # XGetVideoStandard: supplied NTSC environment
            self.ret(1)
        elif address == 0x330339:  # XGetVideoFlags: the test input
            self.ret(self.flags)
        elif address == 0x14ddc0:  # no active player; projected center is excluded
            self.ret(0)
        elif address == 0x22acb4:  # Spartan appearance; original selector branch
            self.ret(self.appearance)
        elif address == 0x22b299:
            self.quad_ready = True
            uc.emu_stop()

    def prepare(self, argv=(), registers=()):
        self.w(STACK, STOP)
        for i, value in enumerate(argv):
            self.w(STACK + 4 + i * 4, value)
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        self.g.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        for reg, value in registers:
            self.g.reg_write(reg, value)

    def run(self, address, stop=STOP, argv=(), registers=()):
        self.prepare(argv, registers)
        self.g.emu_start(address, stop, count=200000)
        assert self.g.reg_read(UC_X86_REG_EIP) == stop, hex(self.g.reg_read(UC_X86_REG_EIP))
        return self.g.reg_read(UC_X86_REG_EAX)

    def video(self, flags):
        self.flags = flags
        self.run(0x123b0)
        # Original branch chooses margins and calls original 16A30. Enter after
        # unrelated initialization 169F0 and stop before the device allocation.
        self.run(0x12578, stop=0x125b3, registers=[(UC_X86_REG_EBX, 0)])
        return dict(flags=flags, widescreen=bool(self.read(0x485ac2, '<B')[0]),
                    progressive_5=bool(self.read(0x485ac5, '<B')[0]),
                    progressive_6=bool(self.read(0x485ac6, '<B')[0]),
                    base_outer=self.read(0x485a8a, '<4h'),
                    base_safe=self.read(0x485a92, '<4h'))

    def cells(self, players):
        self.g.mem_write(self.players + 8, struct.pack('<h', players))
        # Retail chooses grid orientation before constructing camera state.
        self.run(0x2234e0, stop=0x223585)
        mode = self.r(self.g.reg_read(UC_X86_REG_ESP) + 0x10)
        grid, span, pos, outer, safe = [self.alloc(bytes(16)) for _ in range(5)]
        self.run(0x2f600, registers=[(UC_X86_REG_EDX, mode),
                                   (UC_X86_REG_ESI, players), (UC_X86_REG_EDI, grid)])
        cells = []
        for view in range(players):
            self.run(0x2f640, argv=[players, mode, pos],
                     registers=[(UC_X86_REG_ECX, grid), (UC_X86_REG_EDX, span), (UC_X86_REG_EAX, view)])
            self.run(0x2f6b0, argv=[grid, span, outer],
                     registers=[(UC_X86_REG_ESI, pos), (UC_X86_REG_EBX, safe)])
            cells.append(dict(view=view, grid=self.read(grid, '<2h'), span=self.read(span, '<2h'),
                              position=self.read(pos, '<2h'), outer=self.read(outer, '<4h'),
                              safe=self.read(safe, '<4h')))
        return mode, cells


def transplant(cache, retail, row):
    """Copy real metadata; only change pointers to relocated child blocks."""
    bitmap = cache.tags[int(row['bitmap']['datum'], 16)]
    p = bitmap['offset']
    header = bytearray(cache.data[p:p + 0x4c])
    images = cache.block(p + 0x44, 0x74)
    image_data = b''.join(cache.data[q:q + 0x74] for q in images)
    image_ptr = retail.alloc(image_data)
    sequences = cache.block(p + 0x3c, 0x3c)
    seq_data = bytearray()
    for q in sequences:
        seq = bytearray(cache.data[q:q + 0x3c])
        sprites = cache.block(q + 0x34, 0x20)
        if sprites:
            sprite_ptr = retail.alloc(b''.join(cache.data[s:s + 0x20] for s in sprites))
            struct.pack_into('<I', seq, 0x38, sprite_ptr)
        seq_data.extend(seq)
    seq_ptr = retail.alloc(seq_data)
    struct.pack_into('<I', header, 0x40, seq_ptr)
    struct.pack_into('<I', header, 0x48, image_ptr)
    tag_ptr = retail.alloc(header)
    retail.w(retail.table + (int(bitmap['datum'], 16) & 0xffff) * 16 + 8, tag_ptr)

    q = int(row['offset'], 16)
    widget = bytearray(cache.data[q:q + 0x64])
    effects = cache.block(q + 0x58, 0x68)
    effect_data = bytearray()
    for e in effects:
        effect = bytearray(cache.data[e:e + 0x68])
        for function in (4, 0x18, 0x2c, 0x40, 0x54):
            size = cache.u(e + function + 12)
            if size:
                at = cache.u(e + function + 16) - cache.magic
                ptr = retail.alloc(cache.data[at:at + size])
                struct.pack_into('<I', effect, function + 16, ptr)
        effect_data.extend(effect)
    if effect_data:
        struct.pack_into('<I', widget, 0x5c, retail.alloc(effect_data))
    return retail.alloc(widget)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', type=Path, default=ROOT / 'assets/disc/maps/lockout.map')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/hud-position-instruction-oracle.json')
    parser.add_argument('--tsv', type=Path, default=ROOT / 'analysis/multiplayer/hud-position-instruction-oracle.tsv')
    parser.add_argument('--instructions', type=Path,
                        default=ROOT / 'analysis/multiplayer/hud-position-retail-instructions.txt')
    parser.add_argument('--elite', action='store_true', help='Use actual dervish rows and Elite sprite selector')
    args = parser.parse_args()
    cache, retail = HudCache(args.map), Retail()
    retail.appearance = int(args.elite)
    tags = {t['name']: t for t in cache.tags.values() if t['group'] == 'nhdt'}
    requested = [('ui\\hud\\smg', name) for name in ('weapon_background_right', 'ammo_meter_right', 'crosshair')]
    requested += [('ui\\hud\\dervish' if args.elite else 'ui\\hud\\masterchief', name) for name in ('no_grenades', 'shield_meter', 'shield_mask', 'motion_tracker_background')]
    requested += [('ui\\hud\\scoreboard', 'player_arrow')]
    widgets = []
    for tag_name, name in requested:
        # First motion sensor widget is the sensor-enabled row; second is off.
        row = next(r for r in cache.hud(tags[tag_name])['bitmap'] if r['name'] == name)
        widgets.append((row, transplant(cache, retail, row)))
    status, inputs = retail.alloc(bytes(0x400)), retail.alloc(bytes(16))
    cases, videos = [], []
    for flags in (0, 1, 8, 9):
        video = retail.video(flags)
        videos.append(video)
        for players in range(1, 5):
            mode, cells = retail.cells(players)
            for cell in cells:
                retail.g.mem_write(0x4b9dd0, struct.pack('<4h', *cell['outer']))
                retail.g.mem_write(0x4b9dd8, struct.pack('<4h', *cell['safe']))
                for row, widget in widgets:
                    slot = retail.run(0x13a690, argv=[row['anchor']])
                    retail.quad_ready = False
                    retail.prepare([cell['view'], status, widget, inputs])
                    retail.g.emu_start(0x22aff1, STOP, count=200000)
                    pc = retail.g.reg_read(UC_X86_REG_EIP)
                    assert pc in (0x22b299, STOP), hex(pc)
                    quad, bounds, image = [], [0.] * 4, -1
                    if retail.quad_ready:
                        frame = retail.g.reg_read(UC_X86_REG_EBP)
                        quad = [retail.read(frame + 0xc + i * 8, '<2f') for i in range(4)]
                        image = retail.r(frame + 0x44)
                        bounds = [min(p[0] for p in quad), min(p[1] for p in quad),
                                  max(p[0] for p in quad), max(p[1] for p in quad)]
                    cases.append(dict(video_flags=flags, widescreen=video['widescreen'], progressive=video['progressive_5'],
                                      players=players, grid_mode=mode, **cell,
                                      widget_name=row['name'], widget_file_offset=row['offset'], anchor=row['anchor'],
                                      layout_slot=slot, sequence=row['sequences'][slot], image=image,
                                      valid=retail.quad_ready, bounds=bounds, quad=quad))
    assert len(cases) == 320
    assert [v['base_safe'] for v in videos] == [[36, 48, 444, 592], [36, 48, 444, 592],
                                               [24, 32, 456, 608], [24, 32, 456, 608]]
    # Source-instruction invariants, independent of the native implementation.
    for flags in (0, 1):
        grids = [c for c in cases if c['video_flags'] == flags and c['widget_name'] == 'crosshair']
        two = [c['outer'] for c in grids if c['players'] == 2]
        assert two == ([[0, 0, 240, 640], [240, 0, 480, 640]] if flags == 0 else
                       [[0, 0, 480, 320], [0, 320, 480, 640]])
        three = [c['outer'] for c in grids if c['players'] == 3]
        assert three == ([[0, 0, 240, 640], [240, 0, 480, 320], [240, 320, 480, 640]] if flags == 0 else
                         [[0, 0, 480, 320], [0, 320, 240, 640], [240, 320, 480, 640]])
    report = dict(xbe_sha256=PIN, map=str(args.map), map_sha256=hashlib.sha256(cache.data).hexdigest(),
                  entries=['000123b0', '00012578..000125b3', '002234e0..00223585',
                           '0002f600', '0002f640', '0002f6b0', '0013a690', '001396c7',
                           '0022ae21', '0022aff1..0022b299', '0022bcaf', '0022ace7', '0013b390', '0013bb40'],
                  supplied_environment=dict(video_standard='NTSC', no_session_override=True, spartan_appearance=not args.elite,
                                            no_active_player_projection=True, widget_inputs=[0., 0., 0., 0.]),
                  video_modes=videos, cases=cases,
                  scope='Original bitmap selection, anchors, grid, geometry, rounding and authored effects. No native code is imported. Visibility state predicates and graphics are excluded. valid means selected bitmap exists, not that gameplay predicates show this widget. Center projection, special-type shading, UV flips/scope expansion, font/text positions and final device scaling are excluded.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    lines = ['widescreen\tplayers\tview\twidget_name\tsequence\tleft\ttop\tright\tbottom']
    for c in cases:
        if c['progressive']:
            continue
        lines.append('\t'.join(map(str, [int(c['widescreen']), c['players'], c['view'], c['widget_name'],
                                        c['sequence'], *c['bounds']])))
    args.tsv.parent.mkdir(parents=True, exist_ok=True)
    args.tsv.write_text('\n'.join(lines) + '\n')
    inventory = read_rows(FUNCTIONS_CSV)
    xbe = Xbe(ROOT / 'assets/disc/default.xbe')
    instructions = [f'Untouched retail default.xbe SHA256 {PIN}']
    for address in (0x123b0, 0x12560, 0x2234e0, 0x223080, 0x2f600, 0x2f640,
                    0x2f6b0, 0x13a690, 0x1396c7, 0x22ae21, 0x22aff1,
                    0x22bcaf, 0x22ace7, 0x13b390, 0x13bb40):
        row = inventory[address]
        instructions.extend(['', f"{row['va']} size {row['size']} {row['status']}"])
        instructions.extend(listing(xbe.read(address, int(row['size'])), address, inventory))
    args.instructions.parent.mkdir(parents=True, exist_ok=True)
    args.instructions.write_text('\n'.join(instructions) + '\n')
    print(f'{len(cases)} original-instruction widget cases; JSON {args.output}; TSV {args.tsv}')


if __name__ == '__main__':
    main()
