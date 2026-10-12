#!/usr/bin/env python3
"""Unchanged retail16B7A0/16B840/1CAC60/14A550/1CAE40/16BA40 light pools.

Cold real named pool headers use declared backing buffers, initial bytes and
independent allocation pressure. Whole1CAC60 consumes explicit forced cluster
masks over a declared16-cluster count; original membership expansion and all
allocators execute. Geometry-derived membership/C2D00 registration ownership
remain separate. Complete pool bytes/bitmaps and chronology outputs are kept.
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
    headers = [s.read(u(s.read(a, 4)), 76) for a in (0x4e0314, 0x4e0318)]
    assert [h[:32].split(b'\0')[0] for h in headers] == [b'cluster light reference', b'light cluster reference']
    assert all(u(h, 0x20) == 2048 and u(h, 0x24) == 12 for h in headers)
    cases, calls, rng = 16, {}, random.Random(0x1cac60)
    fixture = bytearray(struct.pack('<II', 0x31534c48, cases))
    for case in range(cases):
        g = s.guest()
        g.mem_map(0x70000000, 0x100000)
        pools, records, bitmaps = [0x70010000, 0x70011000], [0x70020000, 0x70030000], [0x70012000, 0x70012100]
        heads, owners, registry, mask, overflow = 0x70013000, 0x70014000, 0x70015000, 0x70016000, 0x70017000
        stack, stop = 0x700fe000, 0x700fff00
        fill, debug = (0x12345678 ^ case), case & 1
        free_entries, free_references = [0, 1, 8, 2048][case // 4], [0, 1, 8, 2048][case % 4]
        g.mem_write(heads, b'\xff' * (16*4))
        g.mem_write(owners, b'\xff' * (32*4))
        g.mem_write(registry, struct.pack('<3I', heads, *pools))
        scenario = u(s.read(0x4e0348, 4))
        page=(scenario+0x9c)&~4095
        g.mem_map(page,4096)
        g.mem_write(page,s.read(page,4096))
        g.mem_write(scenario + 0x9c, struct.pack('<I', 16))
        g.reg_write(R.UC_X86_REG_FPCW, 0x023f)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)

        def trace(guest, address, size, data):
            if address in (0x16b7a0, 0x16b840, 0x1cac60, 0x14a550, 0x1cae40, 0x16ba40):
                calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1

        g.hook_add(UC_HOOK_CODE, trace)

        def call(address, args=(), registers=()):
            g.mem_write(stack, struct.pack('<' + 'I' * (1+len(args)), stop, *args))
            g.reg_write(R.UC_X86_REG_ESP, stack)
            for register, value in registers:
                g.reg_write(register, value)
            g.emu_start(address, stop, count=1000000)
            assert g.reg_read(R.UC_X86_REG_EIP) == stop
            return g.reg_read(R.UC_X86_REG_EAX)

        def summary():
            result = []
            for address in pools:
                result.extend([u(g.mem_read(address + x, 4)) if x != 0x40 else struct.unpack('<H', g.mem_read(address+x, 2))[0]
                               for x in (0x34, 0x38, 0x3c, 0x40)])
            return result

        for i in range(2):
            h = bytearray(headers[i])
            h[0x2a] = (h[0x2a] & ~8) | (debug << 3)
            struct.pack_into('<II', h, 0x44, records[i], bitmaps[i])
            g.mem_write(pools[i], bytes(h))
            g.mem_write(records[i], struct.pack('<I', fill) * 6144)
            call(0x16b7a0, registers=((R.UC_X86_REG_ESI, pools[i]),))
            for _ in range(2048 - [free_entries, free_references][i]):
                assert call(0x16b840, registers=((R.UC_X86_REG_EDX, pools[i]),)) != 0xffffffff
        operations = 96
        fixture += struct.pack('<5I', fill, debug, free_entries, free_references, operations)
        fixture += struct.pack('<8I', *summary())
        for operation in range(operations):
            owner = rng.randrange(32)
            identity = 0xe9810000 | owner
            deleting = rng.randrange(4) == 0
            selected_mask = rng.randrange(65536) if not deleting else 0
            if deleting:
                call(0x1cae40, (identity, owners + owner*4), ((R.UC_X86_REG_EDI, registry),))
            else:
                g.mem_write(mask, struct.pack('<I', selected_mask))
                call(0x1cac60, (registry, identity, owners+owner*4, 0, 0, 0, 0, 0, overflow), ((R.UC_X86_REG_EAX, mask),))
                assert bytes(g.mem_read(overflow, 1)) == b'\0'
            fixture += struct.pack('<3I', int(deleting), owner, selected_mask)
            fixture += struct.pack('<8I', *summary())
            fixture += bytes(g.mem_read(heads, 16*4)) + bytes(g.mem_read(owners, 32*4))
        for i in range(2):
            fixture += bytes(g.mem_read(bitmaps[i], 64*4)) + bytes(g.mem_read(records[i], 2048*12))
    (args.output / 'fixture.bin').write_bytes(fixture)
    report = dict(boundary=__doc__, snapshot_sha256=digest, cases=cases, ordered_operations=cases*96,
                  independent_pressure_cases=16, complete_final_pool_words=cases*2*(64+6144),
                  calls=calls, intercepted_instructions=0,
                  fixture_sha256=hashlib.sha256(fixture).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
