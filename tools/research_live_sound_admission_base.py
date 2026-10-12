#!/usr/bin/env python3
"""Pair complete base-disc sound admission owners with the native TU1.1 authority.

Candidate signatures are navigation only. Every complete instruction stream,
internal branch, direct callee mapping and literal operand is checked. This is
static correspondence, separate from the captured original game's execution.
"""
import argparse
import csv
import json
from pathlib import Path

from oracle_mp_title_update_display import ROOT, OLD, Xbe, _Code, _trace, sha, save, encoded
from research_mp_inventory_switch_u5 import prefix, raw_prefix_candidates, norm

BASE = ROOT / 'assets/disc/default.xbe'
PINS = {BASE: '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
        OLD: 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'}
ENTRIES = (0x126c30, 0x124fc0, 0x125010, 0x1251e0, 0x126bd0,
           0x127010, 0x127d00, 0x127e20, 0x12ac20, 0x12aff0,
           0x218d70, 0x218de0, 0x2195f0, 0x2197f0, 0x221810, 0x221da0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve previous research output'
    for path, pin in PINS.items():
        assert sha(path) == pin
    paths = [ROOT / 'analysis/retail/functions.tsv', ROOT / 'analysis/map-pack-update/functions.tsv']
    images = [Xbe(BASE), Xbe(OLD)]
    codes = [_Code(x, '.text') for x in images]
    starts = [{int(q['address'], 16) for q in csv.DictReader(p.open(), delimiter='\t')} for p in paths]
    buckets = {}
    for entry in sorted(starts[1]):
        buckets.setdefault(tuple(prefix(codes[1], images[1], entry)), []).append(entry)
    rows = []
    for entry in ENTRIES:
        body = _trace(codes[0], entry, starts[0])
        before = [codes[0].at(a) for a in sorted(body.code)]
        positions = {i.address: n for n, i in enumerate(before)}
        normalized = [norm(i, images[0], positions) for i in before]
        candidates = set(buckets.get(tuple(prefix(codes[0], images[0], entry)), []))
        candidates.update(raw_prefix_candidates(codes[0], images[0], codes[1], entry))
        matches = []
        for candidate in sorted(candidates):
            other = _trace(codes[1], candidate, starts[1])
            after = [codes[1].at(a) for a in sorted(other.code)]
            pos = {i.address: n for n, i in enumerate(after)}
            if [norm(i, images[1], pos) for i in after] == normalized:
                matches.append((candidate, after))
        assert len(matches) == 1, (hex(entry), [hex(q[0]) for q in matches])
        target, after = matches[0]
        calls, memory = [], []
        for a, b in zip(before, after):
            if a.mnemonic == 'call' and a.operands[0].type == 2:
                calls.append(dict(base_site=f'{a.address:08x}', old_site=f'{b.address:08x}',
                                  base_target=f'{a.operands[0].imm:08x}', old_target=f'{b.operands[0].imm:08x}'))
            for left, right in zip(a.operands, b.operands):
                # Non-control immediates must be literal equal. Do not mask a
                # scalar just because its number falls inside the image range.
                if left.type == right.type == 2 and not (a.mnemonic.startswith('j') or a.mnemonic == 'call'):
                    assert left.imm == right.imm, (hex(a.address), a.op_str, b.op_str)
                if left.type == right.type == 3 and left.mem.base == left.mem.index == right.mem.base == right.mem.index == 0:
                    assert left.size == right.size
                    memory.append(dict(base_va=f'{left.mem.disp:08x}', old_va=f'{right.mem.disp:08x}',
                                       bytes=left.size, base_hex=images[0].read(left.mem.disp, left.size).hex(),
                                       old_hex=images[1].read(right.mem.disp, right.size).hex()))
        for label, image, seq in [('base', images[0], before), ('old', images[1], after)]:
            for i in seq:
                assert image.read(i.address, i.size) == i.bytes
            save(args.output / 'instructions' / f'{entry:08x}-{label}.asm',
                 ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in seq).encode())
        rows.append(dict(base=f'{entry:08x}', old=f'{target:08x}', rows=len(before),
                         complete_body_and_internal_branches_equal=True, calls=calls, memory=memory))
        print(f'{entry:08x} -> {target:08x}: {len(before)} instructions', flush=True)
    mapping = {r['base']: r['old'] for r in rows}
    for row in rows:
        for call in row['calls']:
            call['callee_paired_here'] = call['base_target'] in mapping
            if call['callee_paired_here']:
                assert mapping[call['base_target']] == call['old_target']
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), owners=rows,
                  image_sha256={str(p): h for p, h in PINS.items()},
                  inventory_sha256={str(p): sha(p) for p in paths},
                  instruction_patches=[], intercepted_callees=[])
    save(args.output / 'report.json', encoded(report))


if __name__ == '__main__':
    main()
