#!/usr/bin/env python3
"""Check 1FB800 against its unchanged physical TU1.5 owner and real caller."""

import argparse
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess

import validate_tu15_behavior129_direction_publication as fixtures


ENTRY = 0x1FB800
SITE = 0x1FBBE8
ROOT = fixtures.ROOT
HEADER = 'src/campaign/ai/native_campaign_ai_behavior_129_active_direction.hpp'


def input_row(rng, index, mode, name, *, active=1, countdown=17, blocked=0, limit=0x40000000,
              direction=(0x3F800000, 0, 0), origin=(0x3F800000, 0, 0), output=None):
    row = fixtures.input_case(rng, index, mode)
    row['entry'] = ENTRY
    row['name'] = f'{name}-fp{mode:x}'
    row['gp'][1] = row['gp'][0]
    row['gp'][2] = fixtures.DIRECTION if output is None else output
    bank = row['banks'][0]
    raw = bytearray.fromhex(bank['before'])
    actor = fixtures.ACTOR - fixtures.BASE
    struct.pack_into('<3I', raw, actor + 0x238, *origin)
    struct.pack_into('<5IH', raw, actor + 0x608, *direction, 0, limit, countdown)
    raw[actor + 0x605] = active
    raw[actor + 0x5D8] = blocked
    bank['before'] = raw.hex()
    return row


def test_rows():
    rng = random.Random(ENTRY)
    rows = []
    for mode in (0x1F80, 0x3F80, 0x5F80, 0x7F80, 0x9FC0, 0x1F84):
        def add(name, **kwargs):
            rows.append(input_row(rng, len(rows), mode, name, **kwargs))

        add('disabled', active=0, countdown=91)
        add('enabled high bit', active=0x80)
        add('positive countdown')
        add('zero countdown', countdown=0)
        add('expire on success', countdown=1)
        add('decrement auxiliary carry', countdown=16)
        add('negative countdown', countdown=0xFFFF)
        add('minimum signed countdown', countdown=0x8000)
        add('cleared by blocked byte', blocked=0x81)
        add('cleared by projection limit', limit=0x3F000000)
        add('projection equal to limit', limit=0x3F800000)
        add('NaN projection', direction=(0x7FA12345, 0, 0))
        add('NaN limit', limit=0x7FC23456)
        add('denormal dot',
            direction=(0x00000003, 0x80000001, 0x3F800000),
            origin=(0x3F000000, 0x3F800000, 0x80000000), limit=0)
        add('aliased direction', output=fixtures.ACTOR + 0x60C)
        add('aliased countdown', output=fixtures.ACTOR + 0x618)
        add('aliased saved register', output=fixtures.reference.SP - 12)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = fixtures.Xbe(fixtures.NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == fixtures.NEW_PIN
    caller = fixtures._Code(image, '.text').at(SITE)
    assert caller.mnemonic == 'call' and caller.operands[0].imm == ENTRY
    ref = fixtures.reference
    metadata = ref.metadata(image, (ENTRY,))
    for site in metadata['sites']:
        pc = site['instruction']
        if pc in (0x1FB811, 0x1FB88E, 0x1FB88F, 0x1FB8C0, 0x1FB8C1):
            push = pc == 0x1FB811
            site['operands'].append(dict(base=4, index=-1, scale=0, displacement=0xFFFFFFFC if push else 0,
                                         bytes=4, read=not push, write=push))
    owned = ref.prepare(image, metadata, scratch)
    complete = ref.physical(owned, test_rows(), 'complete')
    faults = []
    for label in ('positive countdown-fp1f80', 'cleared by projection limit-fp1f80'):
        anchor = next(row for row in complete if row['name'] == label)
        for index in range(len(anchor['writes'])):
            faults.append(dict(deepcopy(anchor), id=200 + len(faults), name=f'{label}-write-{index}',
                               stop_write=index, fault=dict(phase='before_write', ordinal=index)))
    anchor = next(row for row in complete if row['name'] == 'positive countdown-fp1f80')
    for address in (0x5020C8, fixtures.POOL + 0x44, fixtures.ACTOR + 0x605, fixtures.ACTOR + 0x610,
                    fixtures.ACTOR + 0x238, fixtures.ACTOR + 0x614, fixtures.ACTOR + 0x618, ref.SP):
        ordinal = next(i for i, event in enumerate(anchor['reads']) if event['address'] == address)
        faults.append(dict(deepcopy(anchor), id=300 + len(faults), name=f'read-{address:x}', stop_read=address,
                           fault=dict(phase='before_read', ordinal=ordinal)))
    original = complete + ref.physical(owned, faults, 'failure-prefixes')
    source, binary = scratch / 'active.cpp', scratch / 'active'
    # Reuse the same bank driver as the independently validated publication leaf.
    probe = fixtures.PROBE
    for old, new in (
        ('native_campaign_ai_behavior_129_direction_publication.hpp', Path(HEADER).name),
        ('native_campaign_ai_publish_probe_direction_tu15', 'native_campaign_ai_update_probe_direction_tu15'),
    ):
        assert probe.count(old) == 1
        probe = probe.replace(old, new)
    source.write_text(probe)
    subprocess.run(['c++', '-std=c++20', '-O2', '-Isrc', '-Iinclude', str(source), '-o', str(binary)],
                   cwd=ROOT, check=True)
    inputs, output = scratch / 'native-input.json', scratch / 'native-output.json'
    inputs.write_text(json.dumps(original))
    subprocess.run([binary, inputs, output], check=True)
    for expected, actual in zip(original, json.loads(output.read_text()), strict=True):
        for field in ('completed', 'boundary_gp', 'boundary_xmm', 'boundary_eflags', 'after_fp'):
            assert expected[field] == actual[field], (expected['name'], field, expected[field], actual[field])
        expected_banks = [{'address': b['address'], 'after': b['after']} for b in expected['banks']]
        assert expected_banks == actual['banks'], expected['name']
        stores = [{k: w[k] for k in ('instruction', 'address', 'before', 'written')} for w in expected['writes']]
        assert stores == actual['writes'], (expected['name'], stores, actual['writes'])
    report = dict(passed=True, original_sha256=fixtures.NEW_PIN, source_entry=hex(ENTRY), actual_call_site=hex(SITE),
                  complete_cases=len(complete), fault_prefixes=len(faults),
                  original_instructions=sum(r['original_instructions'] for r in original),
                  ordered_stores=sum(len(r['writes']) for r in original),
                  matching_gp_xmm_eflags_fp=True, matching_retained_and_stack_bytes=True,
                  native_source_sha256=ref.sha(ROOT / HEADER), original_instruction_patches=[],
                  supplied_original_dependencies=[], scratch=str(scratch))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
