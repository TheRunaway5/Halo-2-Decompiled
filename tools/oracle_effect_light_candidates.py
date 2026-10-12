#!/usr/bin/env python3
"""Original 31910 candidate collection up to, but excluding, 4A7C0 selection.

Runs unchanged 31590/C3140/31520/C17F0 and owner helpers over explicit ordered
visible queues and controlled light/tag/owner banks. Code and constants come
from the retained retail frag-light snapshot. No callee is replaced; stopping
at the selection call preserves the preselection boundary. The live PVS and
successful light registration producer are not established by this fixture.
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
    parser.add_argument('--admission-fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    snap = Snapshot(args.snapshot)
    digest = hashlib.sha256(snap.ram).hexdigest()
    assert digest == '68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    retained = args.admission_fixture.read_bytes()
    assert hashlib.sha256(retained).hexdigest() == 'fe1416940704c73e743757fcea32cbd4e580baf95603a904576f979a7f3c04a8'
    assert struct.unpack_from('<II', retained) == (0x31414c48, 4096)
    light_pool = u(snap.read(0x4e030c, 4))
    object_pool = u(snap.read(0x4e0300, 4))
    render_state = u(snap.read(0x5107e8, 4))
    debug_state = u(snap.read(0x4e9ae8, 4))
    debug_index = u(snap.read(debug_state + 0xc14, 4))
    rng = random.Random(0x31910)
    cases, calls, counts, fixture = 192, {}, [], bytearray(struct.pack('<II', 0x31434c48, 192))
    for case in range(cases):
        guest = snap.guest()
        guest.mem_map(0x70000000, 0x100000)
        mapped = set()

        def write(address, data):
            for page in range(address & ~4095, (address + len(data) + 4095) & ~4095, 4096):
                if page not in mapped:
                    guest.mem_map(page, 4096)
                    guest.mem_write(page, snap.read(page, 4096))
                    mapped.add(page)
            guest.mem_write(address, data)

        def put(address, *words):
            write(address, struct.pack('<' + 'I' * len(words), *words))

        stack, out = 0x700ff000, 0x700c0000
        table, records, definitions = 0x70010000, 0x70020000, 0x70040000
        headers, obj, players = 0x70060000, 0x70061000, 0x70062000
        collector, queue_address = 0x70070000, 0x70071000
        put(light_pool + 0x44, records)
        put(0x4e3b44, table)
        put(object_pool + 0x44, headers)
        guest.mem_write(headers + 12 + 8, struct.pack('<I', obj))
        kind = rng.choice([0, 2])
        local = rng.choice([0, 1]) if kind == 0 else 0
        guest.mem_write(headers + 12 + 3, bytes([kind]))
        guest.mem_write(obj + 0x14, struct.pack('<I', 0xffffffff))
        guest.mem_write(obj + 0x13c, struct.pack('<I', 0xe1000001))
        guest.mem_write(headers + 24 + 8, struct.pack('<I', obj + 0x400))
        guest.mem_write(headers + 24 + 3, bytes([2]))
        guest.mem_write(obj + 0x400 + 0x14, struct.pack('<I', 0xe0000001))
        put(0x4b9ed8, 0)
        put(0x4e8c50, 0x23c110)
        put(0x4e8c48, 0)
        put(0x4e8c20, players)
        guest.mem_write(players + 12, struct.pack('<I', 0xe1000001 if local else 0xe1000002))
        renderer, debug_gate, debug_flags, view_type = (1, 0, 0, 2) if case >= 160 else (
            rng.choice([0, 1, 2]), rng.choice([0, 1]), rng.choice([0, 2]), rng.choice([0, 1, 2]))
        write(render_state + 8, bytes([renderer]))
        put(debug_state, debug_flags)
        put(0x55e4d0 + debug_index * 4, debug_gate)
        write(0x485602, struct.pack('<h', view_type))
        plane = [rng.uniform(-1, 1) for _ in range(4)]
        pixel_scale = rng.choice([120., 240., 360., 540.])
        for address, value in zip((0x4b9e20, 0x4b9e2c, 0x4b9e38, 0x4b9e44, 0x4b9ed0), plane + [pixel_scale]):
            write(address, struct.pack('<f', value))
        count = 1 + case % 190
        bank, identities = bytearray(), []
        for index in range(count):
            at = 8 + rng.randrange(4096) * 656
            definition, record = bytearray(retained[at:at + 228]), bytearray(retained[at + 228:at + 500])
            identity = 0xe9810000 | index
            identities.append(identity)
            struct.pack_into('<I', record, 4, 0xe63a0000 | index)
            struct.pack_into('<I', record, 0x4c, rng.choice([0xffffffff, 0xe0000001, 0xe0000002]))
            struct.pack_into('<3f', record, 0x84, *(rng.uniform(-16, 16) for _ in range(3)))
            struct.pack_into('<3f', record, 0x18, *(rng.uniform(-16, 16) for _ in range(3)))
            if case >= 160:
                struct.pack_into('<I', definition, 0, rng.choice([0x100, 0x100100, 0x400100]))
                struct.pack_into('<2f', definition, 0x18, 2., 1.)
                record[2] = 2
                struct.pack_into('<h', record, 0x48, 9)
                struct.pack_into('<f', record, 0x24, 2.)
                struct.pack_into('<f', record, 0x100, rng.choice([.125, .5, 1.]))
                struct.pack_into('<f', record, 0xc4, 1.)
                struct.pack_into('<3f', record, 0xd4, 1., .5, .25)
            guest.mem_write(records + index * 272, bytes(record))
            guest.mem_write(definitions + index * 256, bytes(definition))
            guest.mem_write(table + index * 16 + 8, struct.pack('<I', definitions + index * 256))
            bank += struct.pack('<I', identity) + bytes(definition) + bytes(record)
        queue_count = 300 if case >= 160 else case % 160 * 2
        queue = [rng.choice(identities + [0xffffffff]) for _ in range(queue_count)]
        if case == 191:
            queue = identities * 2
        write(0x547f98, struct.pack('<I', collector))
        guest.mem_write(collector + 4, struct.pack('<h', len(queue)))
        guest.mem_write(collector + 12, struct.pack('<I', queue_address))
        if queue:
            guest.mem_write(queue_address, struct.pack('<' + 'I' * len(queue), *queue))
        guest.mem_write(stack, struct.pack('<II', 0x700ffff0, out))
        guest.reg_write(R.UC_X86_REG_ESP, stack)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x023f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)

        def trace(g, address, size, data):
            if address in (0x31910, 0x31590, 0xc3140, 0x31520, 0xc17f0, 0x3e9c0, 0xbaf80):
                calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1

        guest.hook_add(UC_HOOK_CODE, trace)
        guest.emu_start(0x31910, 0x4a7c0, count=10000000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == 0x4a7c0
        sp = guest.reg_read(R.UC_X86_REG_ESP)
        output_count = u(guest.mem_read(sp + 8, 4))
        assert u(guest.mem_read(sp + 4, 4)) == out and output_count <= 128
        completed = bytes(guest.mem_read(out, output_count * 20))
        fixture += struct.pack('<6I5f2I', renderer, debug_gate, debug_flags, view_type, local, kind,
                               *plane, pixel_scale, count, len(queue))
        fixture += bank + struct.pack('<' + 'I' * len(queue), *queue)
        fixture += struct.pack('<I', output_count) + completed
        counts.append(output_count)
    (args.output / 'fixture.bin').write_bytes(fixture)
    report = dict(boundary=__doc__, snapshot_sha256=digest, cases=cases,
                  candidate_words=sum(counts) * 5, maximum_candidates=max(counts),
                  full_capacity_cases=counts.count(128), empty_cases=counts.count(0),
                  calls=calls, intercepted_instructions=0,
                  admission_fixture_sha256=hashlib.sha256(retained).hexdigest(),
                  fixture_sha256=hashlib.sha256(fixture).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
