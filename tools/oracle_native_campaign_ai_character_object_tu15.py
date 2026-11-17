#!/usr/bin/env python3
"""Unchanged-i386 1ED700 character/object-property lookup reference.

ECX is the actor datum; [ESP+4] is the exact object definition to match.
The original walks character +E4/+E8 records (stride B4, key +4), following
the character's +8 parent. No callees, outcomes or instruction patches.
All executable, object and raw-bank output is owned by HALO2_TEST_SCRATCH.
"""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_ai_area_perform_tu15 as reference


def inputs():
    base = reference.areas.BASE
    pool, actors, tags, child, parent, records, inherited = (base + n for n in
        (0x100, 0x1000, 0x4000, 0x4200, 0x4400, 0x5000, 0x6000))
    rows = []
    cases = (
        ('none', 0xffffffff, 0xffffffff, 0, (), 0, (), 17),
        ('empty', 2, 0xffffffff, 0, (), 0, (), 17),
        ('negative', 2, 0xffffffff, -1, (), 0, (), 17),
        ('minimum', 2, 0xffffffff, -0x80000000, (), 0, (), 17),
        ('first', 2, 0xffffffff, 3, (17, 18, 19), 0, (), 17),
        ('middle', 2, 0xffffffff, 3, (17, 18, 19), 0, (), 18),
        ('last', 2, 0xffffffff, 3, (17, 18, 19), 0, (), 19),
        ('missing', 2, 0xffffffff, 3, (17, 18, 19), 0, (), 20),
        ('parent_first', 2, 3, 0, (), 2, (20, 21), 20),
        ('parent_last', 2, 3, 3, (17, 18, 19), 2, (20, 21), 21),
        ('parent_negative', 2, 3, -1, (), -1, (), 20),
        ('parent_missing', 2, 3, 2, (17, 18), 2, (20, 21), 22),
        ('child_precedence', 2, 3, 1, (17,), 1, (17,), 17),
        ('full_key_match', 2, 3, 2, (0x12340011, 0x23450011), 1, (17,), 0x23450011),
        ('full_key_miss', 2, 3, 1, (0x12340011,), 1, (0x23450011,), 17),
        ('none_key', 2, 0xffffffff, 2, (0, 0xffffffff), 0, (), 0xffffffff),
    )
    for actor in (0, 0xe3610001):
        for label, character, parent_datum, count, keys, parent_count, parent_keys, wanted in cases:
            raw = bytearray(0x8000)
            def put(address, value):
                struct.pack_into('<I', raw, address - base, value & 0xffffffff)
            put(pool + 0x44, actors)
            put(actors + (actor & 0xffff) * 0x888 + 0x54,
                character if character == 0xffffffff else 0xabcd0000 | character)
            put(tags + 2 * 16 + 8, child)
            put(tags + 3 * 16 + 8, parent)
            for tag, next_datum, total, entries, table in (
                    (child, parent_datum, count, keys, records),
                    (parent, 0xffffffff, parent_count, parent_keys, inherited)):
                put(tag + 8, next_datum if next_datum == 0xffffffff else 0x43210000 | next_datum)
                put(tag + 0xe4, total)
                put(tag + 0xe8, table)
                for index, key in enumerate(entries):
                    put(table + index * 0xb4 + 4, key)
                    put(table + index * 0xb4 + 8, 0xa0000000 | index)
            n = len(rows)
            gp = [0x5a100000 + n * 31 + k * 7 for k in range(8)]
            gp[0], gp[1], gp[4] = 0x1ed700, actor, reference.SP
            stack = bytearray(struct.pack('<I', 0xc0de0000 + n) * 8)
            struct.pack_into('<III', stack, 16, reference.STOP, wanted, 0xabcddcba)
            banks = [dict(address=base, before=raw.hex()),
                     dict(address=0x5020c8, before=struct.pack('<I', pool).hex()),
                     dict(address=0x4f0414, before=struct.pack('<I', tags).hex()),
                     dict(address=reference.SP - 16, before=stack.hex())]
            rows.append(dict(id=n, name=f'object_property_{label}_{actor:x}', entry=0x1ed700,
                gp=gp, matrix=wanted, source=0xabcddcba, banks=sorted(banks, key=lambda b:b['address']),
                before_fp=[0x27f, 0, 0x1f80], live_x87=[],
                xmm=[[0x6b000000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)],
                stop_before_entries=[], max_steps=1000, strict_writes=True, xmm_exact=True))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--verify', type=Path)
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, (0x1ed700,))
    owned = reference.prepare(image, meta, scratch)
    rows = reference.physical(owned, inputs(), 'complete')
    prefixes = []
    for seed in rows:
        for ordinal in range(len(seed['reads'])):
            row = copy.deepcopy(seed)
            for key in ('reads', 'writes', 'executed_sites', 'calls', 'boundary', 'completed'):
                row.pop(key, None)
            for bank in row['banks']:
                bank.pop('after', None)
            row.update(id=len(rows) + len(prefixes), name=f"{seed['name']}_before_read_{ordinal}",
                       fault=dict(phase='before_read', ordinal=ordinal))
            prefixes.append(row)
    rows += reference.physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    sites = {row['instruction'] for row in meta['sites']}
    assert reached == sites, sorted(sites - reached)
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False, stop_entries=[],
                   bank_encoding='sparse-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original 1ED700 repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(passed=True, original_sha256=NEW_PIN, cases=len(rows), completed=len(rows)-len(prefixes),
                  prefixes=len(prefixes), reachable_sites=len(reached), total_sites=len(sites),
                  original_instructions=sum(row['original_instructions'] for row in rows),
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                    (Path(__file__), Path(reference.__file__), reference.ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
