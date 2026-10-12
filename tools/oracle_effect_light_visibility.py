#!/usr/bin/env python3
"""Whole unchanged 132A30 light visibility visit and all its original callees.

Explicit current light, visited epoch, context and retained queue columns vary.
The two controlled portal frustums are literal source inputs. No instruction,
allocator, frustum or queue callee is replaced. Spatial-list registration/order
and the separate C3950 scan are not supplied by this single-visit fixture.
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


def cube(center, half):
    x, y, z = center
    return [x-half, x+half, y-half, y+half, z-half, z+half,
            1, 0, 0, x+half, -1, 0, 0, half-x, 0, 1, 0, y+half,
            0, -1, 0, half-y, 0, 0, 1, z+half, 0, 0, -1, half-z]


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
    datum = 0xe9810015
    light = u(snap.read(u(snap.read(0x4e030c, 4)) + 0x44, 4)) + (datum & 65535) * 272
    entries = [[i, 1 if i % 3 else 0, 1 if i % 2 else 0, 0, 0, 0, 0, 0, 1, 0xffff, 0xffff, 0xffff, 0xffff] for i in range(8)]
    frustums = [cube((0., 0., 0.), 8.), cube((16., -4., 0.), 4.)]
    fixture = bytearray(struct.pack('<5I', 0x31564c48, 4096, 2, len(entries), len(frustums)))
    for entry in entries:
        fixture += struct.pack('<13I', *entry)
    for frustum in frustums:
        fixture += struct.pack('<30f', *frustum)
    rng = random.Random(0x132a30)
    calls, appended, stamped = {}, 0, 0
    for case in range(4096):
        guest = snap.guest()
        guest.mem_map(0x70000000, 0x40000)
        stack, stop, context, bank = 0x7003e000, 0x7003ff00, 0x70001000, 0x70005000
        queue, columns = 0x70010000, [0x70011000, 0x70012000, 0x70013000, 0x70014000]
        mapped = set()

        def write(address, data):
            for page in range(address & ~4095, (address + len(data) + 4095) & ~4095, 4096):
                if page not in mapped:
                    guest.mem_map(page, 4096)
                    guest.mem_write(page, snap.read(page, 4096))
                    mapped.add(page)
            guest.mem_write(address, data)

        at = 8 + rng.randrange(4096) * 656
        record = bytearray(retained[at + 228:at + 500])
        if case % 3 == 0:
            record[2] = 2
            struct.pack_into('<f', record, 0x24, rng.choice([.125, 1., 2., 4.]))
            struct.pack_into('<f', record, 0x100, 1.)
            struct.pack_into('<3f', record, 0xd4, 1., .5, .25)
        position = tuple(rng.uniform(-20, 20) for _ in range(3))
        struct.pack_into('<3f', record, 0x18, *position)
        epoch = rng.choice([0, 1, 0x7fffffff, 0xffffffff])
        last_seen = epoch if case % 5 == 0 else (epoch - 1) & 0xffffffff
        struct.pack_into('<I', record, 12, last_seen)
        mode, override, ordinal, force = rng.randrange(4), rng.choice([0, 1, 0x80, 0xff]), rng.randrange(8), rng.choice([0, 0, 0, 1, 255])
        query = [rng.uniform(-20, 20) for _ in range(3)] + [rng.choice([0., .125, 2., 8., 32.])]
        capacity = rng.choice([1, 2, 8, 16])
        count = rng.randrange(capacity)
        storage = [(rng.getrandbits(32), rng.randrange(65536), rng.getrandbits(32), rng.randrange(65536)) for _ in range(capacity)]
        write(light, bytes(record))
        write(0x4e0308, struct.pack('<I', epoch))
        guest.mem_write(context, struct.pack('<I', bank))
        guest.mem_write(context + 0x10, struct.pack('<I', queue))
        guest.mem_write(context + 0x2a7c, struct.pack('<I', mode))
        guest.mem_write(context + 0x2a60, bytes([override]))
        guest.mem_write(context + 0x2a68, struct.pack('<4f', *query))
        guest.mem_write(bank, struct.pack('<h', 2))
        for i, entry in enumerate(entries):
            guest.mem_write(bank + 0xa6e + i * 26, struct.pack('<13H', *entry))
        for i, frustum in enumerate(frustums):
            guest.mem_write(bank + 0x1974 + i * 264 + 96, struct.pack('<30f', *frustum))
        guest.mem_write(queue, struct.pack('<6I', capacity, count, columns[1], columns[0], columns[2], columns[3]))
        for i, (identity, flags, other, association) in enumerate(storage):
            for address, fmt, value in zip(columns, ['I', 'H', 'I', 'H'], [identity, flags, other, association]):
                guest.mem_write(address + i * struct.calcsize(fmt), struct.pack('<' + fmt, value))
        guest.mem_write(stack, struct.pack('<3I', stop, ordinal, force))
        guest.reg_write(R.UC_X86_REG_ESP, stack)
        guest.reg_write(R.UC_X86_REG_EBX, datum)
        guest.reg_write(R.UC_X86_REG_ESI, context)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x023f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)

        def trace(g, address, size, data):
            if address in (0x132a30, 0xc3140, 0x165010, 0x181800, 0x461c0, 0x163c80, 0xc3110):
                calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1

        guest.hook_add(UC_HOOK_CODE, trace)
        guest.emu_start(0x132a30, stop, count=1000000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == stop
        output_count = struct.unpack('<H', guest.mem_read(queue + 4, 2))[0]
        output_epoch = u(guest.mem_read(light + 12, 4))
        fixture += bytes(record) + struct.pack('<7I4f2I', datum, last_seen, epoch, mode, override, ordinal, force, *query, capacity, count)
        for row in storage:
            fixture += struct.pack('<4I', *row)
        fixture += struct.pack('<2I', output_count, output_epoch)
        for i in range(capacity):
            for address, fmt in zip(columns, ['I', 'H', 'I', 'H']):
                fixture += struct.pack('<I', struct.unpack('<' + fmt, guest.mem_read(address + i * struct.calcsize(fmt), struct.calcsize(fmt)))[0])
        appended += output_count != count
        stamped += output_epoch != last_seen
    (args.output / 'fixture.bin').write_bytes(fixture)
    report = dict(boundary=__doc__, snapshot_sha256=digest, cases=4096,
                  appended=appended, stamped=stamped, calls=calls, intercepted_instructions=0,
                  admission_fixture_sha256=hashlib.sha256(retained).hexdigest(),
                  fixture_sha256=hashlib.sha256(fixture).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
