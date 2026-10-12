#!/usr/bin/env python3
"""Whole original16A8E0 terrain-only light spawn correction.

The pinned real Lockout BSP supplies16A440/1DDE10/245AA0's polygon bank.
Every instruction, including2469B0/2466B0 segment clipping, runs unchanged.
The retained contact bank is a separately declared native input: this fixture
proves the correction solver, not native production of terrain contact banks.
No object/instance/sphere/capsule query is substituted or intercepted.
"""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot, u


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--count', type=int, default=512)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    s = Snapshot(args.snapshot)
    digest = hashlib.sha256(s.ram).hexdigest()
    assert digest == '68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    pool = u(s.read(0x4e030c, 4))
    base = u(s.read(pool + 0x44, 4))
    position = struct.unpack('<3f', s.read(base + 21*272 + 0x84, 12))
    rng, calls, counts = random.Random(0x16a8e0), {}, {}
    fixture = bytearray(struct.pack('<II', 0x31504353, args.count))
    corrected, polygons, spheres, capsules, changed = 0, 0, 0, 0, []
    for n in range(args.count):
        g = s.guest()
        g.mem_map(0x70000000, 0x40000)
        center = tuple(x + rng.uniform(-4, 4) for x in position)
        radius = [0., -.25, .01, .25, 1., 2., 4., 8.][n % 8]
        g.mem_write(0x70001000, struct.pack('<4f', *center, radius))
        stack, stop = 0x70030000, 0x7003ff00
        g.mem_write(stack, struct.pack('<8I', stop, 0x0d800001, 0x70001000,
                    u(struct.pack('<f', radius)), 0xe9810015, 0xffffffff,
                    0x70001020, 0x70001030))
        for register, value in ((R.UC_X86_REG_ESP, stack), (R.UC_X86_REG_FPCW, 0x023f),
                                (R.UC_X86_REG_MXCSR, 0x1fa0)):
            g.reg_write(register, value)
        bank = []

        def trace(guest, address, size, data):
            if address in (0x16a8e0, 0x16a440, 0x1dde10, 0x1ddea0, 0x245aa0, 0x2469b0, 0x2461a0, 0x246360, 0x2466b0):
                calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1
            if address == 0x16a994:
                packet = guest.reg_read(R.UC_X86_REG_ESP) + 0x168
                a, b, c = struct.unpack('<3h', guest.mem_read(packet, 6))
                assert all(0 <= n <= 256 for n in (a, b, c)), (a, b, c)
                bank.append((a, b, c, bytes(guest.mem_read(packet + 8, a*32)),
                             bytes(guest.mem_read(packet + 0x2008, b*44)),
                             bytes(guest.mem_read(packet + 0x4c08, c*108))))

        for address in (0x16a8e0, 0x16a440, 0x1dde10, 0x1ddea0, 0x245aa0,
                        0x2469b0, 0x2461a0, 0x246360, 0x2466b0, 0x16a994):
            g.hook_add(UC_HOOK_CODE, trace, begin=address, end=address)
        g.emu_start(0x16a8e0, stop, count=100000000)
        assert g.reg_read(R.UC_X86_REG_EIP) == stop, (n, center, radius, hex(g.reg_read(R.UC_X86_REG_EIP)))
        a, b, count, points, edges, raw = bank[0] if bank else (0, 0, 0, b'', b'', b'')
        result = g.reg_read(R.UC_X86_REG_EAX) & 255
        assert result in (0, 1)
        output = bytes(g.mem_read(0x70001020, 12)) + bytes(g.mem_read(0x70001030, 4))
        fixture += struct.pack('<4f3I', *center, radius, a, b, count) + points + edges + raw + struct.pack('<I', result) + output
        corrected += result
        polygons += count
        spheres += a
        capsules += b
        counts[count] = counts.get(count, 0) + 1
        if result and len(changed) < 16:
            changed.append(dict(center=center, radius=radius, polygons=count,
                                result=struct.unpack('<4f', output)))
        if n % 64 == 63:
            print(n+1, 'original corrections replayed', flush=True)
    (args.output / 'fixture.bin').write_bytes(fixture)
    report = dict(boundary=__doc__, cases=args.count, corrected=corrected, polygons=polygons,
                  spheres=spheres, capsules=capsules,
                  contact_counts=counts, examples=changed, calls=calls,
                  snapshot_sha256=digest, intercepted_instructions=0,
                  fixture_sha256=hashlib.sha256(fixture).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'examples'}, indent=2))


if __name__ == '__main__':
    main()
