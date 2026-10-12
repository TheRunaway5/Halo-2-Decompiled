#!/usr/bin/env python3
"""Whole unchanged132220 with its light-list walk and C3950 fallback.

Controlled current light/tag, occupied-pool order, spatial heads/link records,
constructed cluster entries, queues and owner services are explicit inputs.
Object branches are disabled by context bit6; the independent sky object scan
is disabled. All light code/callees execute unchanged, including31520/3E9C0,
132A30/C3140/165010/461C0/163C80/C3110. No instruction is replaced. This is
not spatial membership production or a live gameplay frame comparison.
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
from oracle_effect_light_visibility import cube


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot', type=Path, required=True)
    p.add_argument('--admission-fixture', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    snap = Snapshot(args.snapshot)
    digest = hashlib.sha256(snap.ram).hexdigest()
    assert digest == '68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
    retained = args.admission_fixture.read_bytes()
    assert hashlib.sha256(retained).hexdigest() == 'fe1416940704c73e743757fcea32cbd4e580baf95603a904576f979a7f3c04a8'
    pool, objects = [u(snap.read(a, 4)) for a in (0x4e030c, 0x4e0300)]
    entries = [[c, 1 if i % 3 else 0, 1 if i % 2 else 0, 0, 0, 0, 0, 0, 1, 0xffff, 0xffff, 0xffff, 0xffff]
               for i, c in enumerate([3, 1, 7, 0, 6, 2, 5, 4])]
    frustums = [cube((0., 0., 0.), 8.), cube((16., -4., 0.), 4.)]
    cases, rng = 256, random.Random(0x132220)
    fixture = bytearray(struct.pack('<5I', 0x314c4c48, cases, 2, len(entries), len(frustums)))
    for row in entries:
        fixture += struct.pack('<13I', *row)
    for f in frustums:
        fixture += struct.pack('<30f', *f)
    calls, appends, stamps, fallback_cases, output_words = {}, 0, 0, 0, 0
    for case in range(cases):
        g = snap.guest()
        g.mem_map(0x70000000, 0x100000)
        mapped = set()

        def write(a, data):
            for page in range(a & ~4095, (a + len(data) + 4095) & ~4095, 4096):
                if page not in mapped:
                    g.mem_map(page, 4096)
                    g.mem_write(page, snap.read(page, 4096))
                    mapped.add(page)
            g.mem_write(a, data)

        def put(a, *v):
            write(a, struct.pack('<' + 'I' * len(v), *v))

        context, bank, queue = 0x70001000, 0x70005000, 0x70010000
        columns = [0x70011000, 0x70012000, 0x70013000, 0x70014000]
        records, definitions, table, bitmap = 0x70020000, 0x70030000, 0x70040000, 0x70041000
        heads, links, link_pool = 0x70042000, 0x70043000, 0x70045000
        headers, obj, players = 0x70060000, 0x70061000, 0x70062000
        stack, stop = 0x700fe000, 0x700fff00
        put(pool + 0x44, records, bitmap)
        put(0x4e3b44, table)
        put(objects + 0x44, headers)
        g.mem_write(headers + 20, struct.pack('<I', obj))
        g.mem_write(headers + 15, bytes([0]))
        g.mem_write(obj + 0x14, struct.pack('<I', 0xffffffff))
        g.mem_write(obj + 0x13c, struct.pack('<I', 0xe1000001))
        g.mem_write(headers + 32, struct.pack('<I', obj + 0x400))
        g.mem_write(headers + 27, bytes([2]))
        g.mem_write(obj + 0x400 + 0x14, struct.pack('<I', 0xffffffff))
        local = 1 if case >= 192 else rng.randrange(2)
        put(0x4b9ed8, 0)
        put(0x4e8c50, 0x23c110)
        put(0x4e8c48, 0)
        put(0x4e8c20, players)
        g.mem_write(players + 12, struct.pack('<I', 0xe1000001 if local else 0xe1000002))
        put(0x4b9f8c, bitmap + 0x100)
        write(0x4b9ee9, b'\0')
        epoch = rng.choice([0, 1, 0x7fffffff, 0xffffffff])
        mode = rng.randrange(4)
        flags = 0x40 | rng.choice([0, 0, 0x10, 0x80])
        pass_mode = 0 if case >= 192 else rng.choice([0, 0, 1, 2])
        local_view = 0 if case >= 192 else rng.choice([-1, 0, 3])
        local_owner = 0xe0000001 if case >= 192 else rng.choice([0xffffffff, 0xe0000001])
        if case >= 192:
            flags = 0x40
        query = [rng.uniform(-20, 20) for _ in range(3)] + [rng.choice([.125, 2., 8., 32.])]
        put(0x4b9ed4, local_owner)
        put(0x4e0308, epoch)
        g.mem_write(context, struct.pack('<I', bank))
        g.mem_write(context + 8, struct.pack('<h', local_view))
        g.mem_write(context + 0x10, struct.pack('<I', queue))
        g.mem_write(context + 0x2a60, struct.pack('<I', flags))
        g.mem_write(context + 0x2a78, struct.pack('<II', pass_mode, mode))
        g.mem_write(context + 0x2a68, struct.pack('<4f', *query))
        g.mem_write(bank, struct.pack('<h', 2))
        g.mem_write(bank + 0xa6c, struct.pack('<h', len(entries)))
        for i, row in enumerate(entries):
            g.mem_write(bank + 0xa6e + i * 26, struct.pack('<13H', *row))
        for i, f in enumerate(frustums):
            g.mem_write(bank + 0x1974 + i * 264 + 96, struct.pack('<30f', *f))
        occupied = [i for i in range(34) if rng.randrange(5)]
        if case >= 192:
            occupied = list(range(24))
        put(pool + 0x38, 34)
        bits = sum(1 << i for i in occupied)
        g.mem_write(bitmap, struct.pack('<2I', bits & 0xffffffff, bits >> 32))
        source_bank, identities = bytearray(), []
        for ordinal, index in enumerate(occupied):
            at = 8 + rng.randrange(4096) * 656
            d, r = bytearray(retained[at:at + 228]), bytearray(retained[at + 228:at + 500])
            identity = 0xe9810000 | index
            identities.append(identity)
            tag = 0xffffffff if rng.randrange(12) == 0 else 0xe63a0000 | index
            struct.pack_into('<H', r, 0, identity >> 16)
            struct.pack_into('<I', r, 4, tag)
            struct.pack_into('<I', r, 12, epoch if rng.randrange(5) == 0 else (epoch-1) & 0xffffffff)
            struct.pack_into('<I', r, 0x4c, rng.choice([0xffffffff, 0xe0000001, 0xe0000002]))
            struct.pack_into('<3f', r, 0x18, *(rng.uniform(-20, 20) for _ in range(3)))
            struct.pack_into('<h', r, 0x48, rng.choice([-1, -1, 1, 3]))
            if case >= 192:
                struct.pack_into('<I', d, 0, rng.choice([0x40020, 0x40020, 0x40060, 0]))
                struct.pack_into('<h', r, 0x54, rng.choice([-1, 0, 1]))
                struct.pack_into('<I', r, 0x4c, 0xe0000001)
                r[2] = rng.choice([0, 2, 2])
                struct.pack_into('<2f', r, 0x24, 2., 0.)
                struct.pack_into('<f', r, 0x100, 1.)
                struct.pack_into('<3f', r, 0xd4, 1., .5, .25)
            g.mem_write(records + index * 272, bytes(r))
            g.mem_write(definitions + index * 256, bytes(d))
            g.mem_write(table + index * 16 + 8, struct.pack('<I', definitions + index * 256))
            source_bank += struct.pack('<I', identity) + bytes(d) + bytes(r)
        lists = [[rng.choice(identities) for _ in range(rng.randrange(12))] for _ in range(8)]
        if case >= 224:
            lists = [[] for _ in range(8)]
        put(0x4e0310, heads)
        put(0x4e0314, link_pool)
        g.mem_write(link_pool + 0x24, struct.pack('<I', 12))
        g.mem_write(link_pool + 0x44, struct.pack('<I', links))
        link_count = 0
        for i, ids in enumerate(lists):
            first = 0xffffffff if not ids else 0xc0000000 | link_count
            g.mem_write(heads + i * 4, struct.pack('<I', first))
            for n, identity in enumerate(ids):
                tail = 0xffffffff if n+1 == len(ids) else 0xc0000000 | (link_count+1)
                g.mem_write(links + link_count * 12, struct.pack('<III', 0xc000, identity, tail))
                link_count += 1
        capacity = rng.choice([1, 2, 8, 16, 32, 64])
        count = rng.randrange(capacity)
        storage = [(rng.getrandbits(32), rng.randrange(65536), rng.getrandbits(32), rng.randrange(65536)) for _ in range(capacity)]
        g.mem_write(queue, struct.pack('<6I', capacity, count, columns[1], columns[0], columns[2], columns[3]))
        for i, row in enumerate(storage):
            for address, fmt, value in zip(columns, ['I', 'H', 'I', 'H'], row):
                g.mem_write(address + i * struct.calcsize(fmt), struct.pack('<' + fmt, value))
        g.mem_write(stack, struct.pack('<II', stop, context))
        g.reg_write(R.UC_X86_REG_ESP, stack)
        g.reg_write(R.UC_X86_REG_FPCW, 0x023f)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1fa0)
        priority_calls = [0]

        def trace(guest, address, size, data):
            if address in (0x132220, 0xc3950, 0x31520, 0x3e9c0, 0x132a30, 0xc3140, 0x165010, 0x461c0, 0x163c80, 0xc3110):
                calls[f'{address:08x}'] = calls.get(f'{address:08x}', 0) + 1
            if address == 0x3e9c0:
                priority_calls[0] += 1

        g.hook_add(UC_HOOK_CODE, trace)
        g.emu_start(0x132220, stop, count=2000000)
        assert g.reg_read(R.UC_X86_REG_EIP) == stop, hex(g.reg_read(R.UC_X86_REG_EIP))
        output_count = struct.unpack('<H', g.mem_read(queue + 4, 2))[0]
        fixture += struct.pack('<8I4f3I', epoch, mode, flags, pass_mode, local_view & 0xffffffff,
                               local_owner, local, len(occupied), *query, capacity, count, priority_calls[0])
        fixture += source_bank
        for ids in lists:
            fixture += struct.pack('<I', len(ids)) + struct.pack('<' + 'I' * len(ids), *ids)
        for row in storage:
            fixture += struct.pack('<4I', *row)
        fixture += struct.pack('<I', output_count)
        for index in occupied:
            after = u(g.mem_read(records + index * 272 + 12, 4))
            before = u(source_bank, occupied.index(index) * 504 + 4 + 228 + 12)
            fixture += struct.pack('<I', after)
            stamps += after != before
        for i in range(capacity):
            for address, fmt in zip(columns, ['I', 'H', 'I', 'H']):
                fixture += struct.pack('<I', struct.unpack('<' + fmt, g.mem_read(address + i * struct.calcsize(fmt), struct.calcsize(fmt)))[0])
        output_words += capacity * 4
        appends += output_count-count
        fallback_cases += case >= 224 and output_count > count
    (args.output / 'fixture.bin').write_bytes(fixture)
    report = dict(boundary=__doc__, snapshot_sha256=digest, cases=cases,
                  appends=appends, stamps=stamps, fallback_only_nonempty_cases=fallback_cases,
                  retained_queue_words=output_words, calls=calls, intercepted_instructions=0,
                  fixture_sha256=hashlib.sha256(fixture).hexdigest(),
                  admission_fixture_sha256=hashlib.sha256(retained).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
