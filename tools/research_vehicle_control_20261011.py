#!/usr/bin/env python3
"""Authenticate complete vehicle control callers and two scalar rate owners.

Only raw owned executable bytes establish correspondence. The retained vehicle
update anchors the two driving controllers and their actual scalar callees.
Caller correspondence does not claim that those controllers are ported.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens

PARENT = ROOT / 'analysis/multiplayer/integration/vehicle-object-coverage-20261010/paired-final/vehicle-report.json'
PARENT_PIN = '1f17cd004b5ac30a47cef9d3ea822356c1cc4d125e1b0ee6a1a5a2ea1310bf9d'
ENTRIES = (0xf1420, 0xf0b30, 0xf02d0, 0xf06b0, 0x208d80, 0x208e70)
NATIVE = (0x208d80, 0x208e70)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(PARENT) == PARENT_PIN
    parent = json.loads(PARENT.read_text())
    anchor = next(row for row in parent['owners'] if row['old_entry'] == '000f1420')
    assert anchor['full_normalized_instruction_internal_branch_topology_equal']
    paths = (ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03', NEW_DEFAULT)
    assert tuple(map(sha, paths)) == (PIN, NEW_PIN)
    images = tuple(map(Xbe, paths))
    codes = tuple(_Code(image, '.text') for image in images)
    inventories = (ROOT / 'analysis/map-pack-update/functions.tsv',
                   ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv')
    starts = [{int(row['address'], 16) for row in csv.DictReader(path.open(), delimiter='\t')}
              for path in inventories]
    mapping = {0xf1420: int(anchor['u5_entry'], 16)}
    anchors = []
    while set(mapping) != set(ENTRIES):
        previous = len(mapping)
        for entry, target in list(mapping.items()):
            left = [codes[0].at(p) for p in sorted(_trace(codes[0], entry, starts[0]).code)]
            right = [codes[1].at(p) for p in sorted(_trace(codes[1], target, starts[1]).code)]
            assert full_tokens(left, images[0]) == full_tokens(right, images[1]), hex(entry)
            for old, new in zip(left, right):
                if old.mnemonic != 'call' or old.operands[0].type != 2:
                    continue
                callee = old.operands[0].imm
                if callee not in ENTRIES:
                    continue
                assert new.operands[0].type == 2
                later = new.operands[0].imm
                assert callee not in mapping or mapping[callee] == later
                if callee not in mapping:
                    anchors.append(dict(caller=entry, old_site=old.address, later_site=new.address,
                                        old_entry=callee, later_entry=later))
                    mapping[callee] = later
        assert len(mapping) > previous, 'Actual paired caller missing'
    args.output.mkdir(parents=True, exist_ok=False)
    directory = args.output / 'instructions'
    directory.mkdir()
    rows = []
    for entry, target in sorted(mapping.items()):
        assert entry in starts[0] and target in starts[1]
        left = [codes[0].at(p) for p in sorted(_trace(codes[0], entry, starts[0]).code)]
        right = [codes[1].at(p) for p in sorted(_trace(codes[1], target, starts[1]).code)]
        assert full_tokens(left, images[0]) == full_tokens(right, images[1])
        row = dict(entry=entry, later_entry=target, instructions=len(left),
                   native_implemented=entry in NATIVE, instruction_map={}, image_operands=[])
        for label, instructions in (('old', left), ('later', right)):
            path = directory / f'{entry:08x}-{label}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'
                                    for i in instructions))
            row[label + '_listing'] = dict(file=path.name, sha256=sha(path))
        for old, new in zip(left, right):
            if entry in NATIVE:
                assert new.address == old.address + 0x4900
            row['instruction_map'][f'{old.address:08x}'] = new.address
            for a, b in zip(old.operands, new.operands):
                if a.type == b.type == 3 and not a.mem.base and not a.mem.index:
                    assert a.size == b.size
                    if not any(s.va <= a.mem.disp < s.va + s.vsize for s in images[0].sections):
                        continue
                    before, after = images[0].read(a.mem.disp, a.size), images[1].read(b.mem.disp, b.size)
                    row['image_operands'].append(dict(old=a.mem.disp, later=b.mem.disp,
                        width=a.size, old_hex=before.hex(), later_hex=after.hex()))
                    if entry in NATIVE:
                        assert before == after == bytes.fromhex('0000803f')
        rows.append(row)
    report = dict(schema=1, tool_sha256=sha(__file__), parent_sha256=PARENT_PIN,
        original_sha256=PIN, later_original_sha256=NEW_PIN, anchors=anchors, routines=rows,
        native_entries=list(NATIVE), complete_paired_bodies=len(rows),
        native_instruction_count=sum(row['instructions'] for row in rows if row['native_implemented']),
        instruction_patches=[], live_vehicle_physics_integrated=False)
    path = args.output / 'report.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(complete_paired_bodies=len(rows), native_bodies=len(NATIVE),
                          native_instructions=report['native_instruction_count'], report_sha256=sha(path))))


if __name__ == '__main__':
    main()
