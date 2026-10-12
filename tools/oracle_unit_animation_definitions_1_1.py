#!/usr/bin/env python3
"""Read-only graph/event ownership from the original Lockout resident metadata.

The original 1CA5A0 executes for each direct/inherited graph query. Animation
and reference records are read from that actual returned graph. No animation
resource loader, gameplay request, sound/effect success or clock is supplied.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from inspect_multiplayer import Cache
from xbe import Xbe
from functions import _Code, _trace
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EAX, UC_X86_REG_EIP

XBE = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
MAP = ROOT / 'assets/disc/maps/lockout.map'
TABLE, CHANNEL, STACK, STOP = 0x1000000, 0x1100000, 0x120F000, 0x1300000


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def save(p, b):
    p.parent.mkdir(parents=True, exist_ok=True)
    if p.exists():
        assert p.read_bytes() == b, ('Immutable output differs', str(p))
    else:
        p.write_bytes(b)


def run(out):
    assert sha(XBE) == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    assert sha(MAP) == '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
    image, c = Xbe(XBE), Cache(MAP)
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for s in image.sections:
        g.mem_write(s.va, image.section_bytes(s))
    g.mem_map(TABLE, 0x400000)
    page = c.magic & ~4095
    g.mem_map(page, (len(c.data) + c.magic - page + 4095) & ~4095)
    g.mem_write(c.magic, bytes(c.data))
    for datum, t in c.tags.items():
        if t['offset'] is not None:
            g.mem_write(TABLE + (datum & 0xffff) * 16 + 8,
                        struct.pack('<I', t['offset'] + c.magic))
    g.mem_write(0x4EB114, struct.pack('<I', TABLE))
    starts = {int(r['address'], 16) for r in csv.DictReader(
        (ROOT / 'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')}
    code = _Code(image, '.text')
    routines, executed, query_instructions = {}, set(), []
    for owner in (0x1CA5A0, 0x1DE890, 0x1CAE40):
        routine = _trace(code, owner, starts)
        lines = []
        for at in sorted(routine.code):
            i = code.at(at)
            lines.append(f'{at:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
        save(out / 'instructions' / f'{owner:08x}.asm', ('\n'.join(lines) + '\n').encode())
        routines[owner] = routine

    def hook(guest, at, size, user):
        if at != STOP:
            assert at in routines[0x1CA5A0].code, f'Unowned execution {at:08x}'
            executed.add(at)
            query_instructions.append(f'{at:08x}')
    g.hook_add(UC_HOOK_CODE, hook)

    def word(at):
        return int.from_bytes(g.mem_read(at, 4), 'little')

    def block(at, stride):
        count, pointer = word(at), word(at + 4)
        assert count <= 65536
        return bytes(g.mem_read(pointer, count * stride)).hex() if count else '-'

    rows, queries, counts = [], [], dict(direct=0, inherited=0, timing=0, sounds=0, effects=0)
    for graph, t in sorted(c.tags.items()):
        if t['group'] != 'jmad' or t['offset'] is None:
            continue
        targets = [(0xffff, t)]
        for index, p in enumerate(c.block(t['offset'] + 0x4c, 32)):
            target = c.tags.get(c.u(p + 4))
            if target and target['group'] == 'jmad' and target['offset'] is not None:
                assert index < 32768
                targets.append((index, target))
        for low, target in targets:
            clips = c.block(target['offset'] + 0x2c, 0x6c)
            assert len(clips) < 32768
            for index in range(len(clips)):
                packed = index << 16 | low
                g.mem_write(CHANNEL, struct.pack('<II', graph, packed))
                g.mem_write(STACK, struct.pack('<I', STOP))
                g.reg_write(UC_X86_REG_ESP, STACK)
                g.reg_write(UC_X86_REG_ECX, CHANNEL)
                query_instructions.clear()
                g.emu_start(0x1CA5A0, STOP, count=100)
                assert g.reg_read(UC_X86_REG_EIP) == STOP
                assert g.reg_read(UC_X86_REG_ESP) == STACK + 4
                resolved = g.reg_read(UC_X86_REG_EAX)
                assert resolved == target['offset'] + c.magic
                at = word(resolved + 0x30) + index * 0x6c
                raw = bytes(g.mem_read(at, 0x6c)).hex()
                records = [block(at + 0x4c, 4), block(at + 0x54, 8), block(at + 0x5c, 4),
                           block(resolved + 0x14, 12), block(resolved + 0x1c, 12)]
                rows.append(f'D {graph:08x} {packed:08x} {raw} ' + ' '.join(records))
                queries.append(dict(graph=f'{graph:08x}', packed=f'{packed:08x}',
                                    resolved_graph=target['datum'], original_return=f'{resolved:08x}',
                                    definition_pointer=f'{at:08x}', instructions=list(query_instructions)))
                counts['direct' if low == 0xffff else 'inherited'] += 1
                for name, stride, record in zip(('timing', 'sounds', 'effects'), (4, 8, 4), records):
                    counts[name] += 0 if record == '-' else len(record) // (2 * stride)
    save(out / 'fixtures.txt', ('\n'.join(rows) + '\n').encode())
    report = dict(schema='source-unit-animation-cache-definitions-v1', tool_sha256=sha(__file__),
                  source_pins={str(p.relative_to(ROOT)): sha(p) for p in
                               (XBE, MAP, ROOT / 'tools/inspect_multiplayer.py',
                                ROOT / 'tools/reference/halo2-decompiled/tools/functions.py',
                                ROOT / 'tools/reference/halo2-decompiled/tools/xbe.py')},
                  fixture_sha256=sha(out / 'fixtures.txt'), definitions=len(rows), counts=counts,
                  queries=queries, executed_owner='001ca5a0',
                  executed_instructions=[f'{x:08x}' for x in sorted(executed)],
                  supplied_inputs='Literal resident map metadata and tag-table pointer; exhaustive direct/inherited metadata queries. No gameplay/animation/action admission is represented.',
                  scope='Original read-only graph resolution plus literal definition/event/reference bytes. No 1E13C0 resource notification, codec, selection, pose, audio or effect consumer is executed.')
    save(out / 'report.json', (json.dumps(report, sort_keys=True, indent=2) + '\n').encode())
    c.data.close()
    c.stream.close()
    print(json.dumps(dict(definitions=len(rows), counts=counts, report_sha256=sha(out / 'report.json'))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    run(p.parse_args().output)
