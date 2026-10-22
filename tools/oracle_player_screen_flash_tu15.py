#!/usr/bin/env python3
"""Execute unchanged TU1.5 damage-flash selection, slot admission and sampling.

Synthetic response rows and player/object banks are explicit inputs. Selection
stops before rumble, sound and camera impulse. No instructions are patched.
Run through tools/run_test.py; only bounded JSON/TSV evidence is retained.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_multiplayer_hud_positions import Retail, ROOT, STACK, STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from xbe import Xbe


def bits(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


class Original(Retail):
    def __init__(self):
        self.x = x = Xbe(NEW_DEFAULT)
        assert hashlib.sha256(x.data).hexdigest() == NEW_PIN
        self.g = g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in x.sections:
            g.mem_write(section.va, x.section_bytes(section))
        g.mem_map(0x1000000, 0x400000)
        g.mem_map(0x2000000, 0x10000)
        g.mem_map(STOP, 0x1000)
        self.next = 0x1000000
        self.clock = self.alloc(struct.pack('<hhfI', 0, 30, 1 / 30, 0))
        self.bank = self.alloc(bytes(0x2b0))
        self.slot = self.bank + 0x30
        self.w(self.bank + 4, 0xffff)
        self.w(0x51df54, self.clock)
        self.w(0x51df5c, self.bank)
        table = self.alloc(bytes(32))
        self.tag = self.alloc(bytes(0xc8))
        self.w(table + 8, self.tag)
        self.w(0x4f0414, table)
        self.response = self.alloc(bytes(0x4c * 3))
        self.w(self.tag + 0x70, self.response)
        users = self.alloc(bytes(32))
        self.w(users + 0xc, 0)
        self.w(0x4f54f0, users)
        players = self.alloc(bytes(0x48))
        player = self.alloc(bytes(0x21c))
        self.w(players + 0x44, player)
        self.w(player + 0x2c, 0)
        self.w(0x4f54f4, players)
        objects = self.alloc(bytes(0x48))
        headers = self.alloc(bytes(12))
        self.unit = self.alloc(bytes(0x400))
        self.w(objects + 0x44, headers)
        self.w(headers + 8, self.unit)
        self.w(0x4ecbd0, objects)
        self.result = self.alloc(bytes(24))
        self.packed_input = self.alloc(bytes(16))
        self.selection = False
        g.hook_add(UC_HOOK_CODE, self.hook)

    def hook(self, uc, address, size, _):
        if self.selection and address in (0x156a33, 0x156a58):
            uc.emu_stop()

    def select(self, shield, seated, scale, rows):
        self.g.mem_write(self.slot, bytes(0xa0))
        self.w(self.unit + 0xf0, bits(shield))
        self.g.mem_write(self.unit + 0x1fc, struct.pack('<h', 0 if seated else -1))
        self.w(self.tag + 0x6c, len(rows))
        for i, (kind, duration, amount) in enumerate(rows):
            row = self.response + i * 0x4c
            self.g.mem_write(row, struct.pack('<hhhhfhhf4f', kind, 0, 6, 2,
                duration, 5, 0, amount, .7, .2, .3, .4))
        self.selection = True
        self.prepare([0, 0, 0, 0, bits(scale), bits(1), 0])
        self.g.emu_start(0x156840, STOP, count=20000)
        assert self.g.reg_read(UC_X86_REG_EIP) in (0x156a33, 0x156a58)
        self.selection = False

    def sample(self, ticks):
        self.g.mem_write(self.slot + 0x7e, struct.pack('<h', ticks))
        self.g.mem_write(self.result, bytes(24))
        self.run(0x156e40, registers=[(UC_X86_REG_EAX, 0), (UC_X86_REG_ESI, self.result)])
        result = self.read(self.result, '<6I')
        intensity = struct.unpack('<f', struct.pack('<I', result[1]))[0]
        colors = struct.unpack('<4f', struct.pack('<4I', *result[2:]))
        self.g.mem_write(self.packed_input, struct.pack('<4f', *(x * intensity for x in colors)))
        packed = self.run(0x134960, argv=[self.packed_input])
        return [*result, packed]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    original = Original()
    lines = ['# shield seated scale response_type duration amount sample_tick type intensity alpha red green blue packed_argb']
    cases = []
    for shield in (0., .01, 1.):
        for seated in (False, True):
            for scale in (0., .1, .5, 1., 2.):
                for kind, duration, amount in ((0, .05, 1.), (0, .2, .05),
                        (0, .2, 1.), (1, .8, 1.), (2, .5, .5), (2, .50001, .5)):
                    original.select(shield, seated, scale, [(kind, duration, amount)])
                    timer = original.read(original.slot + 0x7e, '<h')[0]
                    for elapsed in range(max(timer, 0) + 2):
                        if elapsed:
                            original.g.mem_write(original.slot + 0x88, b'\0')
                        result = original.sample(max(timer - elapsed, 0))
                        lines.append(' '.join(f'{v:08x}' for v in [bits(shield), int(seated), bits(scale),
                            kind, bits(duration), bits(amount), elapsed, *result]))
                    cases.append(dict(shield=shield, seated=seated, scale=scale,
                        response=[kind, duration, amount], timer=timer))
    # Exact request-slot priority and fractional tick conversion, independent
    # of the damage producer's compiled override.
    admission = []
    request = original.alloc(bytes(32))
    for old_priority in (0, 1, 2):
        for priority in (0, 1, 2):
            for timer in (0, 3, 6, 24):
                for duration in (.1, .2, .205, .8):
                    original.g.mem_write(original.slot, bytes(0xa0))
                    original.g.mem_write(original.slot + 0x18, struct.pack('<hhfhhf4f', 1, old_priority, 24., 0, 0, .2, 1, 1, 0, 0))
                    original.g.mem_write(original.slot + 0x7e, struct.pack('<h', timer))
                    original.g.mem_write(request, struct.pack('<hhfhhf4f', 1, priority, duration, 0, 0, .6, 1, 0, .6, .8))
                    original.run(0x1578a0, argv=[bits(.5)], registers=[(UC_X86_REG_EAX, original.slot), (UC_X86_REG_ECX, request)])
                    admission.append(dict(old_priority=old_priority, priority=priority, timer=timer, duration=duration,
                        result=original.read(original.slot + 0x18, '<8I'), remaining=original.read(original.slot + 0x7e, '<h')[0]))
    selection = []
    for shield in (0., 1.):
        for rows in ([(0,.2,1.),(1,.8,1.)], [(2,0.,1.),(2,.8,1.)],
                     [(2,.2,1.),(0,.8,1.)], [(1,.8,1.),(2,.2,1.)]):
            original.select(shield, False, 1., rows)
            selection.append(dict(shield=shield, rows=rows,
                result=original.sample(original.read(original.slot + 0x7e, '<h')[0])))
    a.output.mkdir(parents=True, exist_ok=True)
    tsv = a.output / 'damage-flash.tsv'
    tsv.write_text('\n'.join(lines) + '\n')
    (a.output / 'original.json').write_text(json.dumps(dict(reference='OG Xbox TU1.5', xbe_sha256=NEW_PIN,
        instruction_patches=[], selection_entry='00156840', admission_entry='001578a0', sample_entry='00156e40',
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=cases, admission=admission, selection=selection,
        samples=len(lines)-1, fixture_sha256=hashlib.sha256(tsv.read_bytes()).hexdigest(),
        transition_tables={str(i):list(original.x.read(0x4782a8+i*1024,1024)) for i in (1,2)},
        limits=__doc__), indent=2) + '\n')
    print(f'PASS unchanged TU1.5: {len(cases)} damage responses, {len(lines)-1} samples, {len(admission)} priority cases')


if __name__ == '__main__':
    main()
