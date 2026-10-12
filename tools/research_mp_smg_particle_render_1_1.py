#!/usr/bin/env python3
"""Complete original disc/TU1.1 particle render ownership correspondence.

Static navigation only. No graphics/device/view admission success is supplied.
"""
import argparse
import csv
from pathlib import Path
from research_mp_inventory_switch_u5 import prefix, raw_prefix_candidates, norm
from oracle_mp_title_update_display import ROOT, OLD, Xbe, _Code, _trace
from oracle_smg_effect_owner_1_1 import sha, save, encoded

BASE = ROOT / 'assets/disc/default.xbe'
PINS = {str(BASE): '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
        str(OLD): 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'}
ENTRIES = (0x174220, 0x174420, 0x173cb0, 0x173c90, 0x173c30,
           0x175610, 0x1751d0, 0x248450, 0x2242b0, 0x2243c0,
           0x224550, 0x23ea80, 0x23e460, 0x23ed30, 0x23ecd0, 0x174f80)
TU_ONLY = (0x3eb10, 0x2b9f0, 0x2b970, 0x3f2b0, 0x441a0,
           0x133760, 0x1360f0, 0x133860, 0x1342f0, 0x136050,
           0x174bb0, 0x135b00, 0x2c390, 0x3fde0, 0x40e80)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    for path, pin in PINS.items():
        assert sha(path) == pin, path
    images = [Xbe(BASE), Xbe(OLD)]
    codes = [_Code(image, '.text') for image in images]
    indices = [ROOT / 'analysis/retail/functions.tsv', ROOT / 'analysis/map-pack-update/functions.tsv']
    starts = []
    for path in indices:
        with path.open() as stream:
            starts.append({int(r['address'], 16) for r in csv.DictReader(stream, delimiter='\t')})
    starts[1].update(TU_ONLY)
    buckets = {}
    for entry in sorted(starts[1]):
        buckets.setdefault(tuple(prefix(codes[1], images[1], entry)), []).append(entry)
    rows = []
    for entry in ENTRIES:
        old = _trace(codes[0], entry, starts[0])
        ins = [codes[0].at(a) for a in sorted(old.code)]
        form = [norm(i, images[0], {q.address: n for n, q in enumerate(ins)}) for i in ins]
        candidates = sorted(set(buckets.get(tuple(prefix(codes[0], images[0], entry)), []) +
                                raw_prefix_candidates(codes[0], images[0], codes[1], entry)))
        matches = []
        for candidate in candidates:
            routine = _trace(codes[1], candidate, starts[1])
            current = [codes[1].at(a) for a in sorted(routine.code)]
            actual = [norm(i, images[1], {q.address: n for n, q in enumerate(current)}) for i in current]
            if actual == form:
                matches.append((candidate, routine, current))
        row = dict(base_entry=f'{entry:08x}', base_instructions=len(ins),
                   candidates=[f'{a:08x}' for a in candidates],
                   full_body_matches=[f'{a:08x}' for a, _, _ in matches])
        if len(matches) == 1:
            candidate, routine, current = matches[0]
            row.update(tu_entry=f'{candidate:08x}', complete_instruction_and_internal_branch_equal=True,
                       paired_calls=[], paired_absolute_operands=[], listings=[])
            for label, body, image in zip(('base', 'tu1_1'), (ins, current), images):
                path = args.output / 'instructions' / f'{entry:08x}-{label}.asm'
                for i in body:
                    assert image.read(i.address, i.size) == i.bytes
                save(path, ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in body).encode())
                row['listings'].append(dict(file=str(path.relative_to(args.output)), sha256=sha(path)))
            for x, y in zip(ins, current):
                if x.mnemonic == 'call' and x.operands[0].type == y.operands[0].type == 2:
                    row['paired_calls'].append(dict(base_site=f'{x.address:08x}', tu_site=f'{y.address:08x}',
                        base_target=f'{x.operands[0].imm:08x}', tu_target=f'{y.operands[0].imm:08x}'))
                for a, b in zip(x.operands, y.operands):
                    if a.type == b.type == 3 and not a.mem.base and a.mem.disp:
                        row['paired_absolute_operands'].append(dict(base_site=f'{x.address:08x}', tu_site=f'{y.address:08x}',
                            base_va=f'{a.mem.disp:08x}', tu_va=f'{b.mem.disp:08x}', bytes=a.size,
                            indexed=bool(a.mem.index), scale=a.mem.scale,
                            base_bytes=images[0].read(a.mem.disp, a.size).hex(), tu_bytes=images[1].read(b.mem.disp, b.size).hex()))
        rows.append(row)
        print(f"{entry:08x} -> {row.get('tu_entry', 'UNRESOLVED')} ({len(ins)} instructions)", flush=True)
    supplemental = []
    for entry in TU_ONLY:
        routine = _trace(codes[1], entry, starts[1])
        ins = [codes[1].at(a) for a in sorted(routine.code)]
        path = args.output / 'instructions' / f'{entry:08x}-tu1_1-owner.asm'
        for i in ins:
            assert i.bytes == images[1].read(i.address, i.size)
        save(path, ''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins).encode())
        supplemental.append(dict(tu_entry=f'{entry:08x}', instructions=len(ins),
            file=str(path.relative_to(args.output)), sha256=sha(path),
            calls=[dict(site=f'{i.address:08x}', target=f'{i.operands[0].imm:08x}')
                   for i in ins if i.mnemonic == 'call' and i.operands[0].type == 2]))
    callback = images[1].read(0x447620, 24)
    assert int.from_bytes(callback[:4], 'little') == 0x133760
    save(args.output / 'report.json', encoded(dict(tool_sha256=sha(__file__), images=PINS,
        index_pins={str(p.relative_to(ROOT)): sha(p) for p in indices}, owners=rows,
        tu_only_owning_context=supplemental,
        renderer_constructor_callback=dict(address='00447620', raw24_hex=callback.hex(),
            table='00447428', stride=24, zero_based_index=21),
        scope='Static complete original body/branch correspondence only; no per-view admission, material or GPU result.')))


if __name__ == '__main__':
    main()
