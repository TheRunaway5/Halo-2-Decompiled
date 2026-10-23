#!/usr/bin/env python3
"""Execute unchanged TU1.5 17AAB0 for the named turret effect flag tuples.

Explicit component tag/stack/RNG bindings; no gameplay or rendering is claimed.
Use run_test.py. Only the bounded receipt is retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--xbe', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use tools/run_test.py')
    if digest(args.xbe) != PIN:
        raise ValueError('Require the authenticated English TU1.5 executable')
    manifest_path = ROOT / 'tools/mcc-turret-effect-scalar-profiles.json'
    manifest = json.loads(manifest_path.read_text())
    census_path = ROOT / manifest['source_census']
    if digest(census_path) != manifest['source_census_sha256']:
        raise ValueError('Turret metadata census changed')
    census = json.loads(census_path.read_text())
    for profile in manifest['profiles']:
        observations = [(row['map'], change) for row in census['differences']
            if row.get('effect_node') == 'effe:' + profile['name']
            for change in row['differences'] if change['field'] == '/flags/00']
        if (set(profile['observed_maps']) != {name for name, _ in observations} or
            not observations or any((change['xbox'], change['mcc']) !=
                (profile['target_bits'], profile['donor_bits']) for _, change in observations)):
            raise ValueError('Turret flag manifest differs from its original/MCC map observations')
    xbe = Xbe(args.xbe)
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in xbe.sections:
        machine.mem_write(section.va, xbe.section_bytes(section))
    definition, table, rng, stack, stop = (0x61000000, 0x62000000, 0x63000000, 0x64000000, 0x65000000)
    for address in (definition, table, rng, stack, stop):
        machine.mem_map(address, 4096)
    def word(address, value):
        machine.mem_write(address, struct.pack('<I', value))
    word(0x4f0414, table);word(table + 16 + 8, definition);word(0x4f3cd8, rng)
    trace = []
    machine.hook_add(UC_HOOK_CODE, lambda uc, pc, size, data: trace.append((pc, size)))
    cases = []
    for row in manifest['profiles']:
        result = {'name': row['name']}
        for label, key, expected in (('xbox', 'target_bits', rng), ('mcc', 'donor_bits', rng + 4)):
            trace.clear();word(definition, int(row[key], 16));word(stack + 2048, stop)
            machine.reg_write(UC_X86_REG_EAX, 0x80000001)
            machine.reg_write(UC_X86_REG_ESP, stack + 2048)
            machine.emu_start(0x17aab0, stop, count=32)
            value = machine.reg_read(UC_X86_REG_EAX)
            if machine.reg_read(UC_X86_REG_EIP) != stop or value != expected:
                raise ValueError('Unchanged original selector differs from the named effect expectation')
            result[label] = {'flags': int(row[key], 16), 'selected_word': (value - rng) // 4,
                             'instructions': len(trace)}
        cases.append(result)
    receipt = {'passed': True, 'scope': __doc__, 'xbe_sha256': PIN,
               'manifest_sha256': digest(manifest_path), 'cases': cases,
               'original_selector_sha256': hashlib.sha256(xbe.read(0x17aab0, 0x24)).hexdigest(),
               'tool_sha256': digest(Path(__file__)), 'full_tu15_parity': False,
               'large_artifacts_retained': []}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS 10 unchanged TU1.5 selector executions')


if __name__ == '__main__':
    main()
