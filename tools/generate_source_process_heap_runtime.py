#!/usr/bin/env python3
"""Compile complete paired original heap/SEH/copy bodies with required owners.

Consumes independently matched original listings, including the explicit signed
indices in the copy body's dispatch closure. No production decoder is emitted.
"""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from generate_physics_world_bootstrap_runtime_20261011 import generate, sha
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE = ROOT / 'analysis/damage-shields-goal-20261011/overlay-pass/registration'

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--manifest', type=Path, required=True)
    ap.add_argument('--startup', action='store_true', help='Include complete installation/DPC/timer setup owners')
    a = ap.parse_args()
    reports = [BASE / 'process-heap-reserve-commit/paired-complete/report.json',
               BASE / 'process-heap-seh-copy/complete-report.json']
    pins = ['c5d4935da54f4e053868608d46ea2588248189f9952d6aebe9e75abaa9ca1e63',
            'fb79138067717c85ae7013fe48b69f84f2a36efaae717be2feea5537a71ad136']
    if a.startup:
        reports.append(BASE / 'process-heap-ready/paired/report.json')
        pins.append('57d430a60d02fc41268779946603186fdbd19828af4872b8e394888b0dbe0521')
    for path, pin in zip(reports, pins): assert sha(path) == pin
    original = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(original) == PIN and sha(NEW_DEFAULT) == NEW_PIN
    old, new = Xbe(original), Xbe(NEW_DEFAULT)
    codes = [_Code(old, '.text'), _Code(new, '.text')]
    first, supplement = [json.loads(p.read_text()) for p in reports[:2]]
    entries, mapping, instructions, bodies, routines = {}, {}, {}, {}, []
    def bind(p, q):
        p &= 0xffffffff; q &= 0xffffffff
        assert p not in mapping or mapping[p] == q, (hex(p), hex(q))
        mapping[p] = q
    def listing(path, pin):
        assert sha(path) == pin
        return [(int(row.split()[0], 16), bytes.fromhex(row.split()[1]))
                for row in path.read_text().splitlines()]
    def body(entry, later, listings, tables):
        rows = [listing(path, pin) for path, pin in listings]
        assert len(rows[0]) == len(rows[1])
        entries[f'{entry:08x}'] = later
        bodies[f'{entry:08x}'] = [p for p, raw in rows[0]]
        bind(entry, later)
        for (p, raw), (q, later_raw) in zip(*rows):
            assert old.read(p, len(raw)) == raw and new.read(q, len(later_raw)) == later_raw
            x, y = codes[0].at(p), codes[1].at(q)
            assert bytes(x.bytes) == raw and bytes(y.bytes) == later_raw
            assert x.mnemonic == y.mnemonic and len(x.operands) == len(y.operands)
            instructions[f'{p:08x}'] = q; bind(p, q)
            for u, v in zip(x.operands, y.operands):
                assert u.type == v.type and u.size == v.size
                if u.type == 2 and u.imm != v.imm: bind(u.imm, v.imm)
                if u.type == 3 and u.mem.disp != v.mem.disp: bind(u.mem.disp, v.mem.disp)
        routines.append(dict(entry=entry, tables=tables))
    for report, parent in [(first, reports[0].parent),
                           (dict(owners=supplement['supplemental_pairs']), reports[1].parent / 'paired')]:
        for row in report['owners']:
            assert row['full_normalized_instruction_internal_branch_topology_equal']
            assert row['full_body_matches'] == [row['u5_entry']]
            assert not row['jump_tables']
            body(int(row['old_entry'], 16), int(row['u5_entry'], 16),
                 [(parent / x['file'], x['sha256']) for x in row['listings']], [])
    copy = supplement['copy_entry']
    assert copy['full_normalized_instruction_internal_branch_topology_equal']
    tables = []
    for table in copy['table_dispatches']:
        p, q = [int(table[k + '_operand_base'], 16) for k in ('TU11', 'u5')]
        bind(p, q)
        tables.append(dict(disc=p, indices=table['effective_indices']))
    body(0x326e20, 0x32a990,
         [(reports[1].parent / x['path'], x['sha256']) for x in copy['listings']], tables)
    if a.startup:
        for row in json.loads(reports[2].read_text())['owners']:
            if int(row['old_entry'],16) not in (0x2d7f8e,0x2dc5ff,0x2dc5d2): continue
            assert row['full_normalized_instruction_internal_branch_topology_equal']
            assert row['full_body_matches'] == [row['u5_entry']] and not row['jump_tables']
            body(int(row['old_entry'],16),int(row['u5_entry'],16),
                 [(reports[2].parent / x['file'],x['sha256']) for x in row['listings']],[])
    paired = dict(complete_bodies=len(entries), entries=entries,
        disc_1_1_sha256=PIN, later_owned_xbe_sha256=NEW_PIN,
        address_map={f'{p:08x}': q for p, q in sorted(mapping.items())},
        instruction_map=instructions, body_sites=bodies, routines=routines,
        input_reports={str(p.relative_to(ROOT)): sha(p) for p in reports},
        generator_sha256=sha(Path(__file__)),complete_startup_owners=a.startup)
    normalized = a.manifest.with_name('normalized-correspondence.json')
    normalized.parent.mkdir(parents=True, exist_ok=True)
    normalized.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(correspondence=normalized, output=a.output,
                              manifest=a.manifest), paired, process_heap=True)

if __name__ == '__main__': main()
