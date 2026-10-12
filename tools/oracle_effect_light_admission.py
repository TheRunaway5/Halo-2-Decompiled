#!/usr/bin/env python3
"""Whole original 31590 sphere admission, including unchanged local-owner calls.

The retained frag-light snapshot supplies executable code, constants and tag
banks. Explicit light state, rendering/debug fields and a controlled one-owner
local-player bank vary. No instruction or callee is replaced. This does not
establish the live PVS collector, registration order or rendered appearance.
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
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    snap = Snapshot(args.snapshot)
    digest = hashlib.sha256(snap.ram).hexdigest()
    assert digest == '68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    datum = 0xe9810015
    light = u(snap.read(u(snap.read(0x4e030c, 4)) + 0x44, 4)) + (datum & 65535) * 0x110
    definition = u(snap.read(u(snap.read(0x4e3b44, 4)) + (u(snap.read(light + 4, 4)) & 65535) * 16 + 8, 4))
    raw_definition = snap.read(definition, 228)
    raw_record = snap.read(light, 272)
    render_state = u(snap.read(0x5107e8, 4))
    debug_state = u(snap.read(0x4e9ae8, 4))
    debug_index = u(snap.read(debug_state + 0xc14, 4))
    object_pool = u(snap.read(0x4e0300, 4))
    random_source = random.Random(0x31590)
    epsilon = struct.unpack('<f', struct.pack('<I', 0x38d1b717))[0]
    threshold = [0., -1., epsilon, struct.unpack('<f', struct.pack('<I', 0x38d1b716))[0],
                 struct.unpack('<f', struct.pack('<I', 0x38d1b718))[0], .125, 1., 2.]
    rows, calls = [], {}
    fixture = bytearray(struct.pack('<II', 0x31414c48, 4096))
    for case in range(4096):
        guest = snap.guest()
        guest.mem_map(0x70000000, 0x10000)
        stack, stop, shape = 0x7000e000, 0x7000ff00, 0x70001000
        mapped = set()

        def write(address, data):
            for page in range(address & ~4095, (address + len(data) + 4095) & ~4095, 4096):
                if page not in mapped:
                    guest.mem_map(page, 4096)
                    guest.mem_write(page, snap.read(page, 4096))
                    mapped.add(page)
            guest.mem_write(address, data)

        d, record = bytearray(raw_definition), bytearray(raw_record)
        flags = random_source.choice([0, 0x20, 0x40, 0x60, 0x100, 0x400, 0x40020,
                                      0x40420, 0x40100, 0x40400, u(d)])
        struct.pack_into('<I', d, 0, flags)
        struct.pack_into('<2f', d, 0x18, random_source.choice([0., .0001, .125, 2., 8.]),
                         random_source.choice([0., .0001, .125, 1., 4.]))
        identity = 0xffffffff if case % 31 == 0 else datum
        record[2] = random_source.choice([0, 2, 3, 0x1b])
        struct.pack_into('<i', record, 8, -1)
        struct.pack_into('<I', record, 0x4c, 0xe0000001)
        struct.pack_into('<h', record, 0x54, random_source.choice([-1, 0, 3]))
        struct.pack_into('<h', record, 0x48, random_source.choice([-1, 0, 9]))
        struct.pack_into('<f', record, 0x24, random_source.choice(threshold))
        struct.pack_into('<f', record, 0x100, random_source.choice(threshold))
        struct.pack_into('<f', record, 0xc4, random_source.choice(threshold))
        for offset in (0xd4, 0xe0):
            color = random_source.choice([(0., 0., 0.), (.125, .125, .125),
                                          (.22360679, 0., 0.), (.22360681, 0., 0.),
                                          tuple(random_source.uniform(-1, 1) for _ in range(3))])
            struct.pack_into('<3f', record, offset, *color)
        # A positive source control set ensures the full shape/cluster path
        # receives substantial coverage alongside the independent early gates.
        if case % 4 == 0:
            identity = datum
            record[2] = 2
            struct.pack_into('<f', record, 0x24, 2.)
            struct.pack_into('<f', record, 0x100, 1.)
            struct.pack_into('<f', record, 0xc4, 1.)
            struct.pack_into('<3f', record, 0xd4, 1., .5, .25)
        renderer = random_source.choice([0, 1, 2])
        debug_gate = random_source.choice([0, 1, -1])
        debug_flags = random_source.choice([0, 1, 2, 3])
        view_type = random_source.choice([0, 1, 2, 3])
        local = random_source.choice([0, 1])
        write(light, bytes(record))
        write(definition, bytes(d))
        write(render_state + 8, bytes([renderer]))
        write(debug_state, struct.pack('<I', debug_flags))
        write(0x55e4d0 + debug_index * 4, struct.pack('<i', debug_gate))
        write(0x485602, struct.pack('<h', view_type))
        # The original 3E9C0/155760/BAF80 implementations consume this bank.
        # Kind0 root, parentNONE, observer23C110/scalar0, explicit local player.
        write(object_pool + 0x44, struct.pack('<I', 0x70004000))
        guest.mem_write(0x70004000 + 12 + 3, b'\0')
        guest.mem_write(0x70004000 + 12 + 8, struct.pack('<I', 0x70005000))
        guest.mem_write(0x70005000 + 0x14, struct.pack('<I', 0xffffffff))
        guest.mem_write(0x70005000 + 0x13c, struct.pack('<I', 0xe1000001))
        write(0x4b9ed8, struct.pack('<i', 0))
        write(0x4e8c50, struct.pack('<I', 0x23c110))
        write(0x4e8c48, struct.pack('<I', 0))
        write(0x4e8c20, struct.pack('<I', 0x70008000))
        guest.mem_write(0x70008000 + 12, struct.pack('<I', 0xe1000001 if local else 0xe1000002))
        sentinel = bytes([0xa5]) * 124
        guest.mem_write(shape, sentinel)
        guest.mem_write(stack, struct.pack('<II', stop, shape))
        guest.reg_write(R.UC_X86_REG_ESP, stack)
        guest.reg_write(R.UC_X86_REG_EDI, identity)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x023f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)

        def trace(g, address, size, data):
            if address in (0x31590, 0xc3140, 0x31520, 0xc17f0, 0xc19f0,
                           0xc1930, 0xc1a80, 0x3e9c0, 0x155760, 0xbaf80):
                calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1

        guest.hook_add(UC_HOOK_CODE, trace)
        guest.emu_start(0x31590, stop, count=1000000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == stop
        output = bytes(guest.mem_read(shape, 124))
        eligible = guest.reg_read(R.UC_X86_REG_EAX) & 255
        assert eligible in (0, 1)
        produced = output != sentinel
        context = struct.pack('<6I', identity, renderer, debug_gate & 0xffffffff,
                              debug_flags, view_type, local)
        fixture += bytes(d) + bytes(record) + context + struct.pack('<2I', eligible, produced) + output
        rows.append(dict(case=case, eligible=eligible, shape_written=produced))
    (args.output / 'fixture.bin').write_bytes(fixture)
    report = dict(boundary=__doc__, snapshot_sha256=digest, cases=len(rows),
                  admitted=sum(row['eligible'] for row in rows),
                  shapes=sum(row['shape_written'] for row in rows),
                  calls=calls, intercepted_instructions=0,
                  fixture_sha256=hashlib.sha256(fixture).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
