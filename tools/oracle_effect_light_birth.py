#!/usr/bin/env python3
"""Whole unchangedC0350 standalone light creation over retained real map state.

The actual350-record lights pool, spatial reference pools, BSP, clock and code
come from the pinned snapshot. Only the declared tag/control/world frame vary.
Original16B840/C0C80/C2D00/1CAC60 and their consumed callees execute unchanged;
there is no replacement allocator, location query or registration callback.
This supplies original ownership and birth evidence, not native allocation or
spatial-registration parity. The effect-part caller's A/B control producer is
outside this boundary.
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
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    s = Snapshot(args.snapshot)
    digest = hashlib.sha256(s.ram).hexdigest()
    assert digest == '68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    pool = u(s.read(0x4e030c, 4))
    base = u(s.read(pool + 0x44, 4))
    assert u(s.read(pool + 0x20, 4)) == 350 and u(s.read(pool + 0x24, 4)) == 272
    source = s.read(base + 21*272, 272)
    source_position = struct.unpack_from('<3f', source, 0x84)
    forward, up = struct.unpack_from('<3f', source, 0xac), struct.unpack_from('<3f', source, 0xb8)
    rng, rows, calls = random.Random(0xc0350), [], {}
    for tag in (0xe63a04c5, 0xe694051f):
        for control in (0., .5, 1., 2.):
            for case in range(16):
                g = s.guest()
                g.mem_map(0x70000000, 0x20000)
                position = tuple(x + rng.uniform(-1, 1) for x in source_position)
                g.mem_write(0x70001000, struct.pack('<9f', *position, *forward, *up))
                g.mem_write(0x70010000, struct.pack('<5I', 0x7001ff00, tag, 0xffffffff, 0xffffffff, 0x70001018))
                for reg, value in ((R.UC_X86_REG_ESP, 0x70010000), (R.UC_X86_REG_EBX, 0x70001000),
                                   (R.UC_X86_REG_EDI, 0x7000100c), (R.UC_X86_REG_FPCW, 0x023f),
                                   (R.UC_X86_REG_MXCSR, 0x1fa0),
                                   (R.UC_X86_REG_XMM0, u(struct.pack('<f', control)))):
                    g.reg_write(reg, value)
                instructions = [0]

                def trace(guest, address, size, data):
                    instructions[0] += 1
                    if address in (0xc0350, 0x16b840, 0xc0c80, 0xc2d00, 0xc28b0, 0x11bed0, 0x1cac60, 0x14a5b0):
                        calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1

                g.hook_add(UC_HOOK_CODE, trace)
                g.emu_start(0xc0350, 0x7001ff00, count=5000000)
                assert g.reg_read(R.UC_X86_REG_EIP) == 0x7001ff00
                identity = g.reg_read(R.UC_X86_REG_EAX)
                assert identity != 0xffffffff
                record = bytes(g.mem_read(base + (identity & 65535)*272, 272))
                assert u(record, 0x4c) == 0xffffffff and u(record, 0x58) == 0xffffffff
                assert record[0x54:0x56] == b'\xff\xff' and record[0x5c:0x5e] == b'\xff\xff'
                assert record[0x84:0x90] == struct.pack('<3f', *position)
                assert record[0xac:0xb8] == struct.pack('<3f', *forward)
                assert record[0xb8:0xc4] == struct.pack('<3f', *up)
                assert u(record, 0xcc) == u(struct.pack('<f', control))
                rows.append(dict(tag=f'{tag:08x}', control=control, position=position,
                                 forward=forward, up=up, identity=f'{identity:08x}',
                                 record=record.hex(), instructions=instructions[0]))
    report = dict(boundary=__doc__, snapshot_sha256=digest, cases=rows, calls=calls,
                  intercepted_instructions=0, pool_capacity=350, pool_stride=272,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', len(rows), 'whole original standalone light births; owner/object/node NONE; no native registration claim')
    print(json.dumps(calls, indent=2))


if __name__ == '__main__':
    main()
