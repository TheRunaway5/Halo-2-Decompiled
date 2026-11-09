#!/usr/bin/env python3
"""Physical TU1.5 4CBE0 material callback, including its real integer children."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

import oracle_native_tu15_model_mask_order as base

ROOT = Path(__file__).resolve().parents[1]
DATA, STACK = 0x1000000, 0x62000000
DESCRIPTOR, UNIT = DATA+0xf000, 0x4c6958+32*5
BANKS = {'lighting': (0x492000, 4096), 'units': (0x4c6000, 4096), 'bsp': (0x4ec000, 4096),
         'tags': (0x4f0000, 4096), 'data': (DATA, 0x10000), 'stack': (STACK, 0x20000)}
OWNERS = (0x4cbe0, 0x15d50, 0x226b0, 0x30cb0, 0x30cf0, 0x30d80, 0x30de0, 0x44930, 0x1cb00, 0x4cba0)


def cases():
    rows = []

    def add(layer, *, mode='model', features=0, root_flags=0, flags=0x77, group=0, part_type=1,
            vertex_class=0, selector=(0, 1), vertex_count=4, vertex_flags=0x10, vertex_tag=4,
            pass_count=1, specular=True, fp_gate=(0, 0, 0), lod=0, pass_index=0, lightmap=False, virtual=False):
        banks = {at: bytearray(size) for at, size in BANKS.values()}

        def put(at, value, width=4):
            start = next(a for a, raw in banks.items() if a <= at and at+width <= a+len(raw))
            banks[start][at-start:at-start+width] = (value & ((1 << (width*8))-1)).to_bytes(width, 'little')

        table, shader, template, post, pass_root, vertex, implementations = (
            DATA+off for off in (0x1000, 0x2000, 0x3000, 0x3800, 0x5000, 0x6100, 0x7000))
        model, sections, geometry, parts, bsp, clusters, definitions, instances = (
            DATA+off for off in (0x8800, 0x9000, 0xa000, 0xa100, 0xb000, 0xb200, 0xb800, 0xbe00))
        put(0x4f0414, table)
        for i, (kind, address) in enumerate(((0x5052544d if virtual else 0x73686164, shader),
                (0x7374656d, template), (0x73706173, pass_root), (0x76727478, vertex), (0x6d6f6465, model)), 1):
            put(table+16*i, kind)
            put(table+16*i+8, address)
        put(shader+0x24, shader+0x80)
        put(shader+0x80, 2)
        put(shader+0x3e, int(specular), 2)
        put(template+0x5c, post)
        put(post+4, post+0x40)
        put(post+0xc, post+0x80)
        put(post+0x14, post+0x180)
        put(post+0x40+10*lod, 32 if lod else 0, 2)
        put(post+0x80+2*((32 if lod else 0)+layer), (pass_count<<9)|2, 2)
        put(post+0x180+10*(2+pass_index)+4, 3)
        put(pass_root+0x20, pass_root+0x40)
        put(pass_root+0x44, implementations)
        for classification in range(7):
            put(implementations+0x132*classification+0x100, vertex_tag)
        put(vertex+4, vertex_count)
        put(vertex+8, vertex+0x80)
        for i in range(12):
            put(vertex+0x80+0x1c*i+0xc, vertex_flags)

        put(0x4c6834, features)
        put(0x4920e4, DATA+0xca00)
        put(DATA+0xca00, root_flags, 1)
        put(0x4c6824, fp_gate[0], 1)
        put(0x492295, fp_gate[1], 1)
        put(0x492296, fp_gate[2], 1)
        put(0x4ecc18, bsp)
        put(bsp+8, 0x666)
        if lightmap:
            put(0x4ecc14, DATA+0xcb00)
            put(DATA+0xcb80, 1)
            put(DATA+0xcb84, DATA+0xcc00)
            put(DATA+0xcc04, 0x666)
            put(DATA+0xcc1c, 1)

        put(UNIT, flags|(0x1000 if mode == 'instance' else 0))
        put(UNIT+4, 5 if mode == 'model' else 0xffffffff)
        put(UNIT+0xc, (group<<29)|(1<<9)|((2<<18) if mode == 'instance' else 0))
        put(UNIT+0x10, 0xced0ab51)
        put(model+0x28, sections)
        put(bsp+0xa0, clusters)
        put(bsp+0x13c, definitions)
        put(bsp+0x144, instances)
        put(instances+0x58*2+0x34, 1, 2)
        metadata, part_owner = {'model': (sections+0x5c+4, sections+0x5c+0x34),
                                'cluster': (clusters+0xb0, clusters+0xb0+0x50),
                                'instance': (definitions+0xc8, definitions+0xc8+0x50)}[mode]
        put(metadata+0x10, selector[0], 1)
        put(metadata+0x11, selector[1], 1)
        put(metadata+0x14, vertex_class, 2)
        put(part_owner, geometry)
        put(geometry+4, parts)
        put(parts+72, part_type, 2)
        for i in range(6):
            put(DESCRIPTOR+4*i, 0xc7701000+i)

        token = (5<<8)|1
        args = (1, 2, lod, layer, pass_index, token, DESCRIPTOR)
        for n, value in enumerate(args, 1):
            put(base.SP+n*4, value)
        row = dict(id=len(rows), name=f'4cbe0-{len(rows)}-layer-{layer}-{mode}', entry=0x4cbe0,
                   gp=[0x77770000, 0xcccc0000, 0xaaaa0000, 0x22223333, base.SP, 0x87654321, 0x11223344, 0xabcdef12],
                   matrix=1, source=2, before_fp=[0x23f, 0x220, 0x5fa0], live_x87=[], max_steps=2500,
                   args=(1,lod,layer,pass_index,token,DESCRIPTOR),
                   banks=[dict(address=at, before=raw.hex()) for at, raw in banks.items()])
        if virtual:
            row['stop_before_entries'] = [0x13a6b0]
        rows.append(row)

    for layer in (1, 2, 4, 7, 8, 9, 14, 15, 17, 20, 21, 22):
        add(layer, group=layer%3, part_type=1+(layer%5), lod=2 if layer%2 else 0, pass_index=layer%2)
        add(layer, pass_count=0)
    for flags in (0, 0x2000):
        for vertex_tag in (4, 0xffffffff):
            add(5, flags=flags, vertex_tag=vertex_tag, group=2)
    for mode in ('model', 'cluster', 'instance'):
        for flags, features in ((0x77, 0x14), (0x477, 0x14), (0x77, 0x30), (0x77, 0)):
            add(10, mode=mode, flags=flags, features=features, group=2, vertex_class=1)
        add(11, mode=mode, features=0x14, group=1, vertex_class=2)
    for features in (0, 1, 2, 4, 8|4, 0x14, 0x34, 0x44):
        for flags in (0x77, 0x477):
            add(11, flags=flags, features=features)
    add(11, root_flags=0x20)
    add(11, features=2, specular=False)
    for features in (0, 1, 2, 4):
        add(12, features=features, vertex_flags=0 if features==0 else 0x10)
    add(12, root_flags=0x20)
    add(12, features=2, specular=False)
    add(13, features=3)
    add(13, features=1)
    for upper in range(8):
        add(3, flags=upper<<29, lightmap=bool(upper%2), pass_count=upper%2, group=upper%3)
    for g in ((0,0,0),(1,0,0),(1,1,0),(1,0,1)):
        for flags in (0, 0x80, 0x180):
            add(16, flags=flags, fp_gate=g)
    for part_type in (1, 4, 5):
        for selector in ((-3,-2),(-1,0),(1,3),(126,127)):
            add(1, part_type=part_type, vertex_class=3, selector=selector)
    add(10, virtual=True)
    return rows


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
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    base.SECTIONS_MAP = BANKS
    meta = base.prepare(Xbe(a.xbe), scratch, OWNERS)
    rows = cases()
    comparison = hashlib.sha256()
    instructions = accepted = class_rejected = post_rejected = residual = 0
    with (scratch/'native.log').open('w+') as errors:
        native = subprocess.Popen([str(a.native.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=errors, text=True)
        try:
            for first in range(0, len(rows), 16):
                batch = rows[first:first+16]
                before, after = scratch/'before.json', scratch/'after.json'
                before.write_text(json.dumps(batch, separators=(',', ':')))
                subprocess.run([str(scratch/'observer'), str(scratch/'reference'), str(scratch/'metadata.json'),
                                str(before), str(after)], check=True, timeout=240)
                proof = json.loads(after.read_text())
                before.unlink()
                after.unlink()
                if proof['instruction_patches'] or proof['supplied_original_outcomes']:
                    raise ValueError('Original material selector used a substituted result')
                instructions += proof['original_instructions']
                for row, gold in zip(batch, proof['cases'], strict=True):
                    native.stdin.write(json.dumps(row, separators=(',', ':'))+'\n')
                    native.stdin.flush()
                    line = native.stdout.readline()
                    if not line:
                        errors.seek(0)
                        raise ValueError(errors.read()[-2200:])
                    actual = json.loads(line)
                    if 'stop_before_entries' in row:
                        if actual != {'residual': True} or gold['stopped_at'] != 0x13a6b0:
                            raise ValueError('Virtual shader did not retain its original dispatch')
                        residual += 1
                        continue
                    gp = gold['boundary_gp']
                    if (not gold['completed'] or gp[4] != base.SP+0x20 or gold['after_fp'] != row['before_fp'] or
                            any(gp[i] != row['gp'][i] for i in (3,5,6,7))):
                        raise ValueError(f'{row["name"]}: original integer ABI contract changed')
                    original = []
                    tracked = {b['address']: bytearray.fromhex(b['before']) for b in row['banks']}
                    for store in gold['writes']:
                        at = store['address']
                        written = bytes.fromhex(store['written'])
                        if (DESCRIPTOR <= at < DESCRIPTOR+24 or base.SP+0x18 <= at < base.SP+0x20):
                            original.append([store['instruction'], at, int.from_bytes(written, 'little'), len(written)])
                            start = STACK if at >= STACK else DATA
                            offset = at-start
                            if tracked[start][offset:offset+len(written)].hex() != store['before']:
                                raise ValueError('Original material store missed an earlier write')
                            tracked[start][offset:offset+len(written)] = written
                        elif at < STACK or at >= base.SP:
                            raise ValueError(f'{row["name"]}: original selector escaped its descriptor or caller stack')
                    for bank in gold['banks']:
                        start = bank['address']
                        observed = bytes.fromhex(bank['after'])
                        if start == STACK:
                            offset = base.SP-STACK+0x18
                            if observed[offset:offset+8] != tracked[start][offset:offset+8]:
                                raise ValueError('Original material caller arguments lost a store')
                        elif observed != tracked[start]:
                            raise ValueError('Original material output bank lost a store')
                    wanted = dict(residual=False, accepted=bool(gp[0]&255), stores=original)
                    if actual != wanted:
                        raise ValueError(f'{row["name"]} differs: {actual} versus {wanted}')
                    accepted += wanted['accepted']
                    class_rejected += not wanted['accepted'] and len(original)==2
                    post_rejected += not wanted['accepted'] and len(original)>2
                    comparison.update(json.dumps(actual, sort_keys=True, separators=(',', ':')).encode())
            native.stdin.close()
            if native.wait(timeout=10):
                raise ValueError('Native material runner failed')
        finally:
            if native.poll() is None:
                native.terminate()
                native.wait(timeout=10)
    if not all((accepted,class_rejected,post_rejected,residual)):
        raise ValueError('Material selection proof lost an outcome class')
    result = dict(passed=True, complete_cases=len(rows)-residual, residual_prefixes=residual,
                  accepted=accepted, class_rejected=class_rejected, post_rejected=post_rejected,
                  original_instructions=instructions, owners=meta['owners'], original_xbe_sha256=base.PIN,
                  comparison_sha256=comparison.hexdigest(), instruction_patches=[], supplied_original_outcomes=False,
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    a.report.write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
