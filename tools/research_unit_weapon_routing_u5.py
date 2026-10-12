#!/usr/bin/env python3
"""Compare complete routing and lookup bodies in the owned TU1.1/TU1.5 images.

Reuses the original byte/branch/table correspondence checker. It does not run
TU1.5 gameplay or establish equivalence of unpaired external callees.
"""
import argparse
import csv
import json
import struct
from pathlib import Path
from research_mp_live_locomotion_owners_u5 import prefix, raw_prefix_candidates
from research_mp_original_data_u5 import norm
from oracle_mp_title_update_display import OLD, NEW, PINS, sha, Xbe, _Code, _trace
from generate_unit_weapon_routing_1_1 import TABLES

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'tests/fixtures/unit_weapon_routing_manifest.json'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_text())
    entries = sorted(set(int(e, 16) for e in manifest['complete_native_entries']) |
                     {0x1cfce0, 0x1deb60, 0x1df670, 0x29c6a0, 0x1e0f50})
    for path, pin in PINS.items():
        assert sha(path) == pin, path
    images = [Xbe(OLD), Xbe(NEW)]
    codes = [_Code(image, '.text') for image in images]
    inventories = [ROOT/'analysis/map-pack-update/functions.tsv',
                   ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv']
    starts = [{int(row['address'], 16) for row in csv.DictReader(p.open(), delimiter='\t')}
              for p in inventories]
    buckets = {}
    for entry in sorted(starts[1]):
        buckets.setdefault(tuple(prefix(codes[1], images[1], entry)), []).append(entry)
    # Only indices selected by the original weapon action descriptor table.
    maps = {0xe8940: (0, 1, 10, 11), 0xe89d8: (0, 1, 10, 11),
            0xe8c96: tuple(range(2, 8))+tuple(range(12, 18)),
            0xe8d1e: tuple(range(2, 8))+tuple(range(12, 18))}
    rows = []
    for entry in entries:
        old = [codes[0].at(p) for p in sorted(_trace(codes[0], entry, starts[0]).code)]
        positions = {i.address: n for n, i in enumerate(old)}
        expected = [norm(i, images[0], positions) for i in old]
        candidates = sorted(set(buckets.get(tuple(prefix(codes[0], images[0], entry)), []) +
                                raw_prefix_candidates(codes[0], images[0], codes[1], entry)))
        matches = []
        for candidate in candidates:
            new = [codes[1].at(p) for p in sorted(_trace(codes[1], candidate, starts[1]).code)]
            np = {i.address: n for n, i in enumerate(new)}
            if [norm(i, images[1], np) for i in new] == expected:
                matches.append((candidate, new))
        row = dict(old_entry=f'{entry:08x}', instructions=len(old),
                   full_body_candidates=[f'{p:08x}' for p, _ in matches])
        if len(matches) == 1:
            candidate, new = matches[0]
            addresses = {a.address: b.address for a, b in zip(old, new)}
            calls, tables, mapped_bytes = [], [], []
            for a, b in zip(old, new):
                assert images[0].read(a.address, a.size) == a.bytes
                assert images[1].read(b.address, b.size) == b.bytes
                if a.mnemonic == 'call' and a.operands[0].type == 2:
                    calls.append(dict(old=f'{a.operands[0].imm:08x}', new=f'{b.operands[0].imm:08x}'))
                for oa, ob in zip(a.operands, b.operands):
                    if oa.type != 3 or ob.type != 3:
                        continue
                    p, q = oa.mem.disp, ob.mem.disp
                    if a.mnemonic == 'jmp' and p in TABLES:
                        count = TABLES[p]
                        before = struct.unpack('<'+'I'*count, images[0].read(p, count*4))
                        after = struct.unpack('<'+'I'*count, images[1].read(q, count*4))
                        assert all(addresses.get(x) == y for x, y in zip(before, after))
                        tables.append(dict(old=f'{p:08x}', new=f'{q:08x}', count=count))
                    if p in maps:
                        indices = maps[p]
                        before = bytes(images[0].read(p+i, 1)[0] for i in indices)
                        after = bytes(images[1].read(q+i, 1)[0] for i in indices)
                        assert before == after
                        mapped_bytes.append(dict(old=f'{p:08x}', new=f'{q:08x}',
                                                 indices=indices, bytes=before.hex()))
            row.update(u5_entry=f'{candidate:08x}', normalized_instructions_and_internal_branches_equal=True,
                       direct_calls=calls, used_jump_tables=tables, declared_action_index_maps=mapped_bytes)
        rows.append(row)
        print(json.dumps({k: row[k] for k in ('old_entry', 'instructions', 'full_body_candidates')}), flush=True)
    mapping = {r['old_entry']: r['u5_entry'] for r in rows if 'u5_entry' in r}
    for row in rows:
        for call in row.get('direct_calls', []):
            call['paired_callee'] = call['old'] in mapping
            if call['paired_callee']:
                assert mapping[call['old']] == call['new']
    report = dict(schema=1, tool_sha256=sha(Path(__file__)), source_pins=PINS,
                  manifest_sha256=sha(MANIFEST), inventory_pins={str(p): sha(p) for p in inventories},
                  owners=rows, paired=len(mapping), requested=len(entries),
                  scope='Static complete normalized body and internal branches; actual used jump tables and declared action indices. Unpaired callees and mutable data bindings remain required. No TU1.5 gameplay execution or live constructor/pose/physics proof.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(dict(paired=len(mapping), requested=len(entries), report_sha256=sha(args.output))))

if __name__ == '__main__':
    main()
