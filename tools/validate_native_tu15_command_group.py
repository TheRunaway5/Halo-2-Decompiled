#!/usr/bin/env python3
"""Compare the entire native 44C80 walk and its leaves with physical TU1.5."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys

import oracle_native_tu15_model_mask_order as base
from validate_native_tu15_command_emit import BANKS as EMIT_BANKS, GEO, MODEL, NONE, PARTS, TAGS
from validate_native_tu15_command_emit import bits, cases as emit_cases, expected

ROOT = Path(__file__).resolve().parents[1]
SP, PACKET = base.SP, base.SP-44
BANKS = {**EMIT_BANKS, 'group': (0x4cd000, 4096), 'palette': (0x494000, 4096)}
OWNERS = (0x44c80, 0x45cd0, 0x1d690, 0x186e70, 0x1e260, 0x140db0,
          0x1e8a0, 0x1e8d0, 0x41030, 0x1bf30, 0x40dd0, 0x40e00, 0x40e20, 0x3b9c0, 0x1e2b0)


def cases(image):
    seed = {b['address']: bytes.fromhex(b['before']) for b in emit_cases(image, single=True)[0]['banks']}
    rows = []

    def add(mode=0, section=0, opaque=-1, use_override=1, slot=255, override=0xdddd0002,
            part_counts=(1, 2, 1), flags=7, mask=NONE, prefix=3, draw_count=3,
            capacity=16, fp=(0x27f, 0x220, 0x1f80), special=''):
        banks = {at: bytearray(seed.get(at, bytes(size))) for at, size in BANKS.values()}

        def put(at, value, width=4):
            for start, data in banks.items():
                if start <= at and at+width <= start+len(data):
                    data[at-start:at-start+width] = (value & ((1 << (width*8))-1)).to_bytes(width, 'little')
                    return
            raise ValueError(f'Unmapped 44C80 declared address {at:#x}')

        put(0x4cd39c, prefix)
        put(0x4cd398, draw_count)
        put(0x4cda48+4*254, override)
        material = MODEL+0x800
        put(MODEL+0x64, material)
        put(material+0xc, 0xbbbb0001)
        for i, coordinate in enumerate((1.2, -2.3, 3.4)):
            put(0x49433c+0x44+12+16*i, bits(coordinate))
        for i, count in enumerate(part_counts):
            unit = 0x4c6958+32*i
            model_section = 255 if special == 'unlit' and i == 1 else i
            model_row = MODEL+0x100+0x5c*model_section
            geometry = GEO+0x100*i
            part_base = PARTS+0x100*i
            context = MODEL+0xa00+0x100*i
            row_flags = 0 if special == 'flag' and i == 1 else flags
            row_group = section & 31
            if special == 'wrong-section' and i == 1:
                row_group = (row_group+1) & 31
            put(unit, row_flags)
            put(unit+4, 0xaaaa0000)
            put(unit+8, 0)
            put(unit+0xc, (model_section << 9) | (row_group << 4) | i)
            put(unit+0x10, context)
            put(unit+0x1a, 0 if special == 'coverage' and i == 1 else 1, 1)
            put(unit+0x1b, 0, 1)
            put(context+0x76, 255 if i == opaque else 128, 1)
            put(0x4cd3a4+i, 32+60*i, 1)
            put(0x4cd6f6+i, slot, 1)
            put(model_row+0x2c, -1, 2)
            put(model_row+0x34, 0 if special == 'null' and i == 1 else geometry)
            put(geometry, count)
            put(geometry+4, part_base)
            for part in range(max(0, count)):
                at = part_base+72*part
                put(at, 1, 2)
                put(at+4, 0, 2)
                put(at+0x2c, bits(1.5+part))
        for bank in range(3):
            put(0x4ce268+24*bank, capacity)
        args = (mode, mask, section, 0x0011818e, 0x40000, 0x8000, 0xc0000, use_override)
        if special == 'ordinary-caller':
            args = (mode, mask, section, NONE, 0, NONE, 0, use_override)
        for i, value in enumerate(args):
            put(SP+4+4*i, value)
        for i in range(44):
            put(PACKET+i, (i*29+11)&255, 1)
        rows.append(dict(id=len(rows), name=f'44C80-{len(rows)}-{special}', entry=0x44c80,
            gp=[0xabcdeffe, 0x12345678, 0xaabbccdd, 0x61254533, SP, 0x6201c000, 0xfe3c229f, 0xc7def02a],
            matrix=mode, source=mask, args=args, before_fp=list(fp), live_x87=[], max_steps=15000,
            banks=[dict(address=at, before=data.hex()) for at, data in banks.items()]))

    for section, opaque, use_override, slot in itertools.product((0, 30, 31), (-1, 0, 1), (0, 1, 0x100), (254, 255)):
        add(section=section, opaque=opaque, use_override=use_override, slot=slot)
    for mode, flags, capacity, fp in itertools.product((0, 1), (1, 7, 0x8007), (1, 16),
                                                       ((0x27f, 0x220, 0x1f80), (0x23f, 0, 0x9fc0))):
        add(mode=mode, prefix=1 if mode else 3, flags=flags, slot=254, use_override=0,
            capacity=capacity, fp=fp, opaque=1)
    for special in ('null', 'flag', 'wrong-section', 'coverage', 'unlit', 'ordinary-caller'):
        add(special=special, opaque=1, slot=254)
    for counts in ((1, 0, 1), (1, -1, 1), (0, 0, 0)):
        add(part_counts=counts, opaque=1, slot=254)
    add(mode=1, prefix=0xbeef0002, draw_count=0xcafe0001)
    add(prefix=0x12340000)
    add(section=32)
    add(mask=0)
    add(slot=254, override=NONE)
    return rows


def section_cases(image, camouflage):
    rows = []
    entry = 0x42090 if camouflage else 0x42750
    constants = image.read(0x46a000, 4096)

    def add(kinds, count, depth=0x3f800000, notice=0x7d, highwater=None, fp=(0x27f, 0x220, 0x1f80),
            rectangles=(), points=(), depths=()):
        banks = {at: bytearray([0xb3])*size for at, size in BANKS.values()}
        banks[0x46a000][:] = constants

        def put(at, value, width=4):
            for start, data in banks.items():
                if start <= at and at+width <= start+len(data):
                    data[at-start:at-start+width] = (value & ((1 << (width*8))-1)).to_bytes(width, 'little')
                    return
            raise ValueError(f'Unmapped {entry:x} declared address {at:#x}')

        put(0x4ce1cc, len(kinds) if highwater is None else highwater)
        put(0x4c2aa0, count)
        put(0x4c2aa8, 0x1004000-0x70*count)
        put(0x4c2ab1, notice, 1)
        for index, kind in enumerate(kinds):
            section = 0x4cdd4c+36*index
            put(section, depths[index] if depths else depth)
            point = points[index] if points else (bits(-3.5), bits(.5+index), 0x80000000)
            for j, coord in enumerate(point, 1):
                put(section+4*j, coord)
            if camouflage:
                bounds = rectangles[index] if rectangles else tuple(map(bits, (index, index+1, 0, 1)))
                for j, coord in enumerate(bounds, 4):
                    put(section+4*j, coord)
            put(section+32, kind)
        rows.append(dict(id=len(rows), name=f'{entry:x}-{len(rows)}', entry=entry,
            gp=[0xabcd1234, 0xaadd1122, 0x12345678, 0x224488aa, SP, 0x6201c000, 0x4a2323be, 0xabbbdddf],
            matrix=0xabcdef12, source=0, before_fp=list(fp), live_x87=[], max_steps=60000 if camouflage else 12000,
            banks=[dict(address=at, before=data.hex()) for at, data in banks.items()]))

    add((), 0, highwater=-1)
    add((), 0)
    if camouflage:
        add((0, 3, 2, 0x10001), 0)
        overlap = tuple(map(bits, (0, 1, 0, 1)))
        for count, notice in itertools.product((-1, 0, 14, 1022, 1023, 1024), (0, 0x81)):
            add((1, 0, 1, 3, 1), count, notice=notice, rectangles=(overlap,)*5)
        add((1,), 1025)
        add((1, 0, 0, 0, 0), 0, rectangles=tuple(tuple(map(bits, rectangle)) for rectangle in (
            (0, 1, 0, 1), (-1, 0, 0, 1), (0, 1, -1, 0), (1, 2, 0, 1), (0, 1, 1, 2))))
        add((3,)*31+(1,), 0)
        add((1,)*32, 1, rectangles=(overlap,)*32)
        add((1, 3, 2, 1, 4, 0, 1), 0, rectangles=(overlap, overlap, (0x7fc12345, 0, 0, 0),
            overlap, overlap, tuple(map(bits, (-1, 0, -1, 0))), overlap))
        special = (0, 0x80000000, 1, 0x80000001, 0x7fc12345, 0x7f812345, 0x7f800000, 0xff800000, 0x7f7fffff)
        for value, rounding in itertools.product(special, (0x1f80, 0x3f80, 0x5f80, 0x7f80, 0x9fc0)):
            add((1, 0, 1, 1), 11, rectangles=(overlap, (0, value, 0, 0x3f800000), overlap, overlap),
                points=((value, value, value), (value, 1, 0x80000000), (0x7f7fffff, value, 0), (0, value, 1)),
                depths=(value, bits(-1), bits(4), 0x80000000), fp=(0x23f, 0x220, rounding))
        return rows
    add((1, 0, 2, 0x10003), 0)
    for count, notice in itertools.product((-1, 0, 14, 1022, 1023, 1024), (0, 0x81)):
        add((3, 0, 1, 3, 0, 3), count, notice=notice)
    add(tuple(3 if i % 3 else 0 for i in range(32)), 1)
    for depth, rounding in itertools.product((0, 0x80000000, 1, 0x80000001, 0x7fc12345, 0x7f812345,
                                               0x7f800000, 0xff800000, 0x7f7fffff),
                                              (0x1f80, 0x3f80, 0x5f80, 0x7f80, 0x9fc0)):
        add((3, 0, 3), 11, depth=depth, fp=(0x23f, 0x220, rounding))
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    producer = p.add_mutually_exclusive_group()
    producer.add_argument('--section-queue', action='store_true')
    producer.add_argument('--camouflage-queue', action='store_true')
    a = p.parse_args()
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest() != base.PIN:
        raise ValueError('Wrong original TU1.5 executable')
    sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(a.xbe)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    base.SECTIONS_MAP = BANKS
    queue_entry = 0x42090 if a.camouflage_queue else 0x42750 if a.section_queue else None
    queue_append = 0x42174 if a.camouflage_queue else 0x427ba
    meta = base.prepare(image, scratch, (queue_entry,) if queue_entry else OWNERS)
    if a.camouflage_queue:
        sites = {site['instruction']: site for site in meta['sites']}
        for pc, encoding in ((0x42345, 'f3a5'), (0x42350, 'f3a4')):
            if sites[pc]['bytes'] != encoding:
                raise ValueError('Original camouflage REP encoding changed')
            sites[pc]['repeat_ecx'] = True
        (scratch/'metadata.json').write_text(json.dumps(meta))
    rows = section_cases(image, a.camouflage_queue) if queue_entry else cases(image)
    instructions = bank_appends = transparent_appends = 0
    comparison = hashlib.sha256()
    with (scratch/'native.log').open('w+') as errors:
        native = subprocess.Popen([str(a.native.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=errors, text=True)
        try:
            for first in range(0, len(rows), 12):
                batch = rows[first:first+12]
                before, after = scratch/'before.json', scratch/'after.json'
                before.write_text(json.dumps(batch, separators=(',', ':')))
                r = subprocess.run([str(scratch/'observer'), str(scratch/'reference'), str(scratch/'metadata.json'),
                                    str(before), str(after)], capture_output=True, text=True, timeout=240)
                if r.returncode:
                    raise ValueError((r.stdout+r.stderr)[-2200:])
                proof = json.loads(after.read_text())
                before.unlink()
                after.unlink()
                if proof['instruction_patches'] or proof['supplied_original_outcomes']:
                    raise ValueError('Original group instructions or outcomes were substituted')
                instructions += proof['original_instructions']
                for row, gold in zip(batch, proof['cases'], strict=True):
                    gold_gp = gold['boundary_gp']
                    if any(gold_gp[i] != row['gp'][i] for i in (3, 5, 6, 7)):
                        raise ValueError('Original group did not preserve nonvolatile registers')
                    wanted = expected(row, gold)
                    original_stores = wanted['stores']
                    if queue_entry:
                        wanted = dict(stores=[s[1:] for s in original_stores], fp=wanted['fp'],
                                      appends=sum(s[0] == queue_append for s in original_stores))
                    else:
                        stack = next(b['after'] for b in gold['banks'] if b['address'] == 0x62000000)

                        def stack_bytes(at, length):
                            first_byte = (at-0x62000000)*2
                            return stack[first_byte:first_byte+length*2]

                        wanted.update(eax=gold_gp[0], esp=gold_gp[4], packet=stack_bytes(PACKET, 44),
                                      controls=stack_bytes(SP+0x10, 8))
                    native.stdin.write(json.dumps(row, separators=(',', ':'))+'\n')
                    native.stdin.flush()
                    line = native.stdout.readline()
                    if not line:
                        errors.seek(0)
                        raise ValueError(errors.read()[-2200:])
                    actual = json.loads(line)
                    if actual != wanted:
                        changed = {k: [actual.get(k), v] for k, v in wanted.items() if actual.get(k) != v}
                        raise ValueError(f'{row["name"]} differs: {str(changed)[:3000]}')
                    bank_appends += sum(s[0] == 0x3ba38 for s in original_stores)
                    transparent_appends += sum(s[0] in (0x1e2d0, queue_append) for s in original_stores)
                    comparison.update(json.dumps(actual, sort_keys=True, separators=(',', ':')).encode())
            native.stdin.close()
            if native.wait(timeout=10):
                raise ValueError('Native group runner failed')
        finally:
            if native.poll() is None:
                native.terminate()
                native.wait(timeout=10)
    result = dict(passed=True, cases=len(rows), original_instructions=instructions, owners=meta['owners'],
                  bank_appends=bank_appends, transparent_pool_appends=transparent_appends,
                  original_xbe_sha256=base.PIN, comparison_sha256=comparison.hexdigest(),
                  instruction_patches=[], supplied_original_outcomes=False, owned_scratch=str(scratch),
                  retained_large_artifacts=[], scope=(
                      f'Whole {queue_entry:X} transparent queue including full-pool notification tail. Compares ordered stores, complete non-stack banks and FP.'
                      if queue_entry else
                      'Whole declared model group and original leaves. Compares ordered stores, final non-stack banks, caller packet, layer controls, EAX/ESP and complete FP.'))
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
