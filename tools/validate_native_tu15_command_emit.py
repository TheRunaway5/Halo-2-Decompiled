#!/usr/bin/env python3
"""Compare ordered non-stack stores and FP with physical TU1.5 41030."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

import oracle_native_tu15_model_mask_order as base

ROOT = Path(__file__).resolve().parents[1]
SP = base.SP
NONE = 0xffffffff
BANKS = {'constants': (0x46a000, 4096), 'player': (0x473000, 4096),
         'view': (0x491000, 8192), 'pool': (0x4c2000, 4096), 'units': (0x4c6000, 8192),
         'bsp': (0x4ec000, 4096), 'command': (0x4ce000, 4096), 'tags': (0x4f0000, 4096),
         'data': (0x1000000, 0x9000), 'stack': (0x62000000, 0x20000)}
TAGS, MODEL, GEO, PARTS, PACKET, RECORDS, POOL, BSP = range(0x1000000, 0x1008000, 4096)
SHADERS = 0x1008000


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def cases(image, single=False):
    rows = []
    constants = image.read(0x46a000, 4096)

    def add(*, mode=0, flags=6, grouped=0, layers=0x8084, original_layers=None,
            override=0, plane=1, shader_flags=3, model='model', part_type=1,
            coverage=0xabcdef12, part=1, unit=0xace00003, scale=0x3f800000,
            coord=0x3f800000, bias=0, modifier=0, capacity=2, full_pool=False, pool_notice=1,
            fp=(0x27f, 0x220, 0x1f80), count=3, group=0x73686164,
            mask=0xf00fffff, add_layers=0x40000000, stop=None):
        banks = {at: bytearray([0xa7]) * size for at, size in BANKS.values()}
        banks[0x46a000][:] = constants

        def put(at, value, width=4):
            for start, data in banks.items():
                if start <= at and at + width <= start + len(data):
                    data[at-start:at-start+width] = (value & ((1 << (8*width))-1)).to_bytes(width, 'little')
                    return
            raise ValueError(f'Undeclared test address {at:#x}')

        put(0x4f0414, TAGS)
        put(0x4ecc18, BSP)
        low = unit & 65535
        signed_unit = low if low < 32768 else low-65536
        u = 0x4c6958 + 32*signed_unit
        put(u, flags | (0x1000 if model == 'instance' else 0))
        put(u+4, 0xaaaa0000 if model == 'model' else NONE)
        put(u+0xc, (3 << 9) | (2 << 18))
        put(TAGS+8, MODEL)
        put(MODEL+0x28, MODEL+0x100)
        put(MODEL+0x100+3*0x5c+0x34, GEO)
        put(BSP+0xa0, BSP+0x100)
        put(BSP+0x100+3*0xb0+0x50, GEO)
        put(BSP+0x144, BSP+0x400)
        put(BSP+0x400+2*0x58+0x34, -1, 2)
        put(BSP+0x13c, BSP+0x800)
        put(BSP+0x800-0xc8+0x50, GEO)
        put(GEO+4, PARTS if part < 10 else PARTS-72*part+0x100)
        put(PARTS+72*part if part < 10 else PARTS+0x100, part_type, 2)
        source = 0xbbbb0001
        effective = source if override == NONE else 0xdddd0002
        shader_arg = NONE if override == NONE else effective
        if override == 2:
            shader_arg = effective = source
        if override == 3:
            source = NONE
        for i, datum in enumerate((source, effective)):
            if datum == NONE:
                continue
            ref = SHADERS + 0x200*i
            j = datum & 65535
            put(TAGS+16*j, group if i == 1 else 0x73686164)
            put(TAGS+16*j+8, ref)
            put(ref+0x16, shader_flags, 1)
            put(ref+0x24, ref+0x80)
            put(ref+0x3c, bias, 2)
            put(ref+0x80, 0xabab0000 | (8+i))
            put(TAGS+16*(8+i)+8, ref+0x90)
            put(ref+0x90+0x5c, ref+0x100)
            put(ref+0x100, count)
            put(ref+0x104, ref+0x120)
            for lod, threshold in enumerate((40., 10., 0.)):
                row_layers = original_layers if i == 0 and original_layers is not None else layers
                put(ref+0x120+lod*10+2, row_layers | (0x20000*lod))
                put(ref+0x120+lod*10+6, bits(threshold))
        put(PACKET, source)
        for i, value in enumerate((coord, bits(-2), bits(3), bits(.25)), 1):
            put(PACKET+4*i, value)
        put(PACKET+0x14, plane, 1)
        for i, value in enumerate((.1, .2, .3, .4)):
            put(PACKET+0x18+4*i, bits(value))
        put(PACKET+0x28, scale)
        put(0x4c6839, modifier, 1)
        put(0x492348+4*bias, bits(.75))
        put(0x473644, bits(.5))
        for i, value in enumerate((.1, -.3, .25, 1., -2., .5)):
            put(0x491e38+4*i, bits(value))
        put(0x4c2aa0, 1024 if full_pool else 1)
        put(0x4c2aa8, POOL)
        put(0x4c2ab1, pool_notice, 1)
        for category in range(3):
            hdr = 0x4ce268+24*category
            put(hdr, capacity)
            put(hdr+4, 1)
            put(hdr+8, 0x08000400)
            put(hdr+0x14, RECORDS+0x100*category)
        for i, value in enumerate((PACKET, shader_arg, grouped, mask, add_layers)):
            put(SP+12+4*i, value)
        row = dict(id=len(rows), name=f'41030-{len(rows)}', entry=0x41030,
                   gp=[0, part, unit, 0, SP, 0, 0, 0], matrix=mode, source=coverage,
                   args=[unit, part, mode, coverage, PACKET, shader_arg, grouped, mask, add_layers],
                   before_fp=list(fp), live_x87=[], max_steps=2000,
                   banks=[dict(address=at, before=data.hex()) for at, data in banks.items()])
        if stop is not None:
            row['stop_before_entries'] = [stop]
        rows.append(row)

    if single:
        add()
        return rows
    for mode, flags, grouped, layers in itertools.product(range(4), (0, 2, 4, 6), (0, 1), (0, 0x8000, 0x8084)):
        add(mode=mode, flags=flags, grouped=grouped, layers=layers)
    for model, shader_flags, plane, full in itertools.product(('model', 'cluster', 'instance'), range(4), (0, 0x81), (False, True)):
        add(model=model, shader_flags=shader_flags, plane=plane, full_pool=full)
    for original, effective, arg, cap in itertools.product((0, 0x8000, 0x8080, 0x8082), (0x8000, 0x208084), (0, NONE, 2, 3), (1, 2)):
        add(original_layers=original, layers=effective, override=arg, capacity=cap)
    for raw, rounding, part_type in itertools.product((0, 0x80000000, 1, 0x7fc12345, 0x7f812345, 0x7f800000),
                                                     (0x1f80, 0x3f80, 0x5f80, 0x7f80, 0x9fc0), (0, 1, 5)):
        add(scale=raw, coord=raw, part_type=part_type, fp=(0x23f, 0, rounding), modifier=1, bias=5 if rounding == 0x5f80 else 0)
    for unit, part in itertools.product((0x01000000, 0xade00003, 0x1234ffff), (0, 5, 0x8000, 0x10005)):
        add(unit=unit, part=part, grouped=0x100)
    for group in (0x5052544d, 0x70727433):
        add(group=group, stop=0x13a6b0)
    for count in (0, -1):
        add(count=count, stop=0x411ae)
    add(full_pool=True, pool_notice=0)
    return rows


def expected(row, gold):
    if not gold['completed']:
        raise ValueError('Incomplete original case')
    banks = {b['address']: bytearray.fromhex(b['before']) for b in row['banks'] if b['address'] != 0x62000000}
    stores = []
    for s in gold['writes']:
        at = s['address']
        if 0x62000000 <= at < 0x62020000:
            continue
        raw = bytes.fromhex(s['written'])
        start = next(k for k, v in banks.items() if k <= at and at+len(raw) <= k+len(v))
        old = banks[start][at-start:at-start+len(raw)]
        if old.hex() != s['before']:
            raise ValueError('Original store chronology has an unobserved write')
        banks[start][at-start:at-start+len(raw)] = raw
        stores.append([s['instruction'], at, int.from_bytes(raw, 'little'), len(raw)])
    for b in gold['banks']:
        if b['address'] in banks and b['after'] != banks[b['address']].hex():
            raise ValueError('Original final bank has an unobserved write')
    rejected = 'stop_before_entries' in row
    if rejected and (gold['stopped_at'] not in row['stop_before_entries'] or stores):
        raise ValueError('Residual case missed its pristine original boundary')
    return dict(stores=stores, rejected=rejected, fp=row['before_fp'] if rejected else gold['after_fp'])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    a = p.parse_args()
    if hashlib.sha256(a.xbe.read_bytes()).hexdigest() != base.PIN:
        raise ValueError('Wrong TU1.5 executable')
    sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(a.xbe)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    rows = cases(image)
    witness = None
    layout = dict(BANKS)
    if root := os.environ.get('HALO2_COMMAND_EMIT_GENUINE_ROOT'):
        from native_tu15_command_emit_genuine_inputs import collect
        genuine, pages, witness = collect(image, Path(root), SP, base.STOP)
        for page in sorted(pages):
            if not any(at <= page and page+4096 <= at+n for at, n in layout.values()):
                layout['retained_'+hex(page)] = (page, 4096)
        for row in genuine:
            row['id'] = len(rows)
            rows.append(row)
    base.SECTIONS_MAP = layout
    meta = base.prepare(image, scratch, (0x41030, 0x1bf30, 0x40dd0, 0x40e00, 0x40e20, 0x3b9c0, 0x1e2b0))
    digest = hashlib.sha256()
    instructions = banks = pool = genuine_appends = 0
    with (scratch/'native.log').open('w+') as errors:
        native = subprocess.Popen([str(a.native.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=errors, text=True)
        try:
            for start in range(0, len(rows), 16):
                batch = rows[start:start+16]
                before, after = scratch/'before.json', scratch/'after.json'
                before.write_text(json.dumps(batch, separators=(',', ':')))
                run = subprocess.run([str(scratch/'observer'), str(scratch/'reference'), str(scratch/'metadata.json'),
                                      str(before), str(after)], capture_output=True, text=True, timeout=240)
                if run.returncode:
                    raise ValueError((run.stdout+run.stderr)[-2000:])
                proof = json.loads(after.read_text())
                before.unlink()
                after.unlink()
                if proof['instruction_patches'] or proof['supplied_original_outcomes']:
                    raise ValueError('Original instructions or outcomes were substituted')
                instructions += proof['original_instructions']
                for row, gold in zip(batch, proof['cases'], strict=True):
                    wanted = expected(row, gold)
                    native.stdin.write(json.dumps(row, separators=(',', ':'))+'\n')
                    native.stdin.flush()
                    line = native.stdout.readline()
                    if not line:
                        errors.seek(0)
                        raise ValueError(errors.read()[-2000:])
                    actual = json.loads(line)
                    if actual != wanted:
                        raise ValueError(f'{row["name"]} {row["args"]} differs: {actual} vs {wanted}')
                    banks += sum(s[0] == 0x3ba38 for s in actual['stores'])
                    pool += sum(s[0] == 0x1e2d0 for s in actual['stores'])
                    if 'kind' in row:
                        genuine_appends += sum(s[0] == 0x3ba38 for s in actual['stores'])
                    digest.update(json.dumps(actual, sort_keys=True, separators=(',', ':')).encode())
                print('whole original 41030 cases', min(start+16, len(rows)), '/', len(rows), flush=True)
            native.stdin.close()
            if native.wait(timeout=10):
                raise ValueError('Native 41030 runner failed')
        finally:
            if native.poll() is None:
                native.terminate()
                native.wait(timeout=10)
    report = dict(passed=True, whole_cases=sum('stop_before_entries' not in r for r in rows),
                  original_prefixes=sum('stop_before_entries' in r for r in rows), bank_appends=banks,
                  transparent_pool_appends=pool, original_instructions=instructions, comparison_sha256=digest.hexdigest(),
                  original_xbe_sha256=base.PIN, owners=meta['owners'], instruction_patches=[],
                  supplied_original_outcomes=False, retained_large_artifacts=[], owned_scratch=str(scratch),
                  genuine_secondary_witness=witness, genuine_secondary_bank_appends=genuine_appends,
                  scope='Physical i386 declared and optional Lockout caller histories; complete ordered non-stack stores, full final banks, and FP. No live World or GPU claim.')
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
