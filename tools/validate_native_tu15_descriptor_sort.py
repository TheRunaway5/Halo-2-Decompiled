#!/usr/bin/env python3
"""Physical TU1.5 140550 descriptor ordering with both original comparators."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

import oracle_native_tu15_model_mask_order as base

ROOT = Path(__file__).resolve().parents[1]
DATA = 0x1000000
STACK = 0x62000000
BANKS = {'data': (DATA, 0x4000), 'stack': (STACK, 0x20000)}


def cases():
    rng = random.Random(0x3baa0)
    for case, (compare, count, shape, shift) in enumerate(itertools.product(
            (0x3bb70, 0x3bbc0), (0, 1, 2, 7, 8, 9, 16, 17, 32, 65), range(3), (0, 1))):
        offset = 32+shift
        data = bytearray([0xb7])*0x4000
        for i in range(count):
            words = [rng.getrandbits(32) for _ in range(6)]
            if shape == 1:
                # Equal sort keys, with unrelated words to expose swaps.
                if compare == 0x3bb70:
                    words[4] = 0xfeedcafe
                    words[0] = (words[0]&65535)|0x7fff0000
                    words[1] = (words[1]&0xffff00ff)|0xef00
                    words[2] = (words[2]&0xffff0000)|0x8000
                else:
                    words[0] = 0xffff0001
                    words[1] = (words[1]&255)|0x80008000
                    words[3] = 0xfacecafe
                    words[4] = 0x7fffffff
            elif shape == 2:
                # Ties at every comparator depth and values on both sides of
                # the sign bit. The final payload still identifies each row.
                options = (0, 1, 0x7fff, 0x8000, 0xffff)
                key = options[(i//4)%len(options)]
                other = options[(i//9)%len(options)]
                words[0] = key<<16 | other
                words[1] = other<<16 | (key&255)<<8 | (i&255)
                words[2] = (words[2]&0xffff0000)|other
                words[3] = key<<16 | other
                words[4] = other<<16 | key
            struct.pack_into('<6I', data, offset+24*i, *words)
        stack = bytearray(0x20000)
        struct.pack_into('<I', stack, base.SP-STACK+12, 0x4ce268)
        yield dict(id=case, name=f'{compare:x}-{count}-{shape}-{shift}', entry=0x140550,
                   gp=[DATA+offset,count,0x22114455,0x44552211,base.SP,0x21124552,0x45612311,0x43625748],
                   matrix=24, source=compare, before_fp=[0x23f,0x220,0x1fa0], live_x87=[],
                   max_steps=100000, banks=[dict(address=DATA,before=data.hex()),
                                             dict(address=STACK,before=stack.hex())])


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
    meta = base.prepare(Xbe(a.xbe), scratch, (0x140550, 0x1407b0, 0x3bb70, 0x3bbc0))
    rows = list(cases())
    instructions = 0
    digest = hashlib.sha256()
    with (scratch/'native.log').open('w+') as log:
        native = subprocess.Popen([str(a.native.resolve())], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  stderr=log, text=True)
        try:
            for first in range(0, len(rows), 10):
                batch = rows[first:first+10]
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
                    raise ValueError('Original descriptor sort used a substituted result')
                instructions += proof['original_instructions']
                for row, gold in zip(batch, proof['cases'], strict=True):
                    if not gold['completed']:
                        raise ValueError('Incomplete original descriptor sort')
                    original_data = next(b['after'] for b in gold['banks'] if b['address']==DATA)
                    initial_data = row['banks'][0]['before']
                    begin = (row['gp'][0]-DATA)*2
                    end = begin+48*row['gp'][1]
                    if original_data[:begin] != initial_data[:begin] or original_data[end:] != initial_data[end:]:
                        raise ValueError('Original sort changed bytes beyond the descriptor extent')
                    wanted = dict(records=original_data[begin:end], fp=gold['after_fp'])
                    native.stdin.write(json.dumps(row, separators=(',', ':'))+'\n')
                    native.stdin.flush()
                    line = native.stdout.readline()
                    if not line:
                        log.seek(0)
                        raise ValueError(log.read()[-2200:])
                    actual = json.loads(line)
                    if actual != wanted:
                        raise ValueError(f'Original descriptor order differs in {row["name"]}')
                    digest.update(json.dumps(actual, sort_keys=True, separators=(',', ':')).encode())
            native.stdin.close()
            if native.wait(timeout=10):
                raise ValueError('Native descriptor sort runner failed')
        finally:
            if native.poll() is None:
                native.terminate()
                native.wait(timeout=10)
    result = dict(passed=True, cases=len(rows), original_instructions=instructions, owners=meta['owners'],
                  comparison_sha256=digest.hexdigest(), original_xbe_sha256=base.PIN,
                  instruction_patches=[], supplied_original_outcomes=False, retained_large_artifacts=[],
                  owned_scratch=str(scratch), scope='Whole original 140550, 1407B0 and genuine comparators. Compares all descriptor bytes, equal-key permutations and FP. Selection and callback production are separate.')
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(result, indent=2)+'\n')


if __name__ == '__main__':
    main()
