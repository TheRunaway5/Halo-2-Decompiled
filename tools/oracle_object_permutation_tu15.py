#!/usr/bin/env python3
"""Capture untouched Xbox TU1.5 permutation lookup routines on an i386 CPU.

The inputs declare a sparse tag/object namespace, not replacement callees. Every
executed instruction belongs to the authenticated original .text section. The
fixture includes all final declared memory, machine state and instruction PCs.
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

STACK_BASE = 0x60000000
STACK = STACK_BASE + 0x1000
RETURN = 0x60300000
TAG_TABLE = 0x62000000
MODEL = 0x62200000
REGIONS = 0x62300000
PERMUTATIONS = 0x62400000
NAMES = [0x01112233, 0x44556677, 0x8899aabb]


def word(value):
    return struct.pack('<I', value)


def lookup_banks(tag, names, count, region=0, permutation=False):
    model = bytearray(0x80)
    struct.pack_into('<II', model, 0x70, 3 if permutation else count, REGIONS)
    regions = bytearray(0x30)
    for n, name in enumerate(NAMES):
        struct.pack_into('<I', regions, 16*n, name)
    if permutation:
        struct.pack_into('<II', regions, 16*region+8, count, PERMUTATIONS)
        permutations = bytearray(8*len(names))
        for n, name in enumerate(names):
            struct.pack_into('<II', permutations, 8*n, name, 0xfed00000+n)
    else:
        for n, name in enumerate(names):
            struct.pack_into('<I', regions, 16*n, name)
    banks = [(0x4f0414, word(TAG_TABLE)),
             (TAG_TABLE+16*(tag & 0xffff)+8, word(MODEL)),
             (MODEL, model), (REGIONS, regions)]
    if permutation:
        banks.append((PERMUTATIONS, permutations))
    return banks


def cases():
    # An exact NONE test precedes low-word tag indexing. A zero string ID is
    # separately rejected, even if an authored record contains that ID.
    region_cases = [
        ('region-none-tag', 0xffffffff, NAMES[0], NAMES, 3, 0xffffffff),
        ('region-zero-name', 0xabcd0002, 0, [0, *NAMES[:2]], 3, 0xffffffff),
        ('region-zero-count', 0xabcd0002, NAMES[0], NAMES, 0, 0xffffffff),
        ('region-negative-count', 0xabcd0002, NAMES[0], NAMES, 0xffffffff, 0xffffffff),
        ('region-min-count', 0xabcd0002, NAMES[0], NAMES, 0x80000000, 0xffffffff),
        ('region-first', 0xabcd0002, NAMES[0], NAMES, 3, 0),
        ('region-middle', 0xabcd0002, NAMES[1], NAMES, 3, 1),
        ('region-last', 0xabcd0002, NAMES[2], NAMES, 3, 2),
        ('region-missing', 0xabcd0002, 0x77777777, NAMES, 3, 0xffffffff),
        ('region-duplicate-first', 0xabcd0002, NAMES[1], [NAMES[1]]*3, 3, 0),
        ('region-full-name-id', 0xabcd0002, NAMES[1], [0x99556677, NAMES[1], 0xaa556677], 3, 1),
        ('region-tag-salt-alias', 0x76540002, NAMES[2], NAMES, 3, 2),
        ('region-zero-tag', 0, NAMES[1], NAMES, 3, 1),
        ('region-max-tag-index', 0x7654ffff, NAMES[2], NAMES, 3, 2),
        ('region-high-name-id', 0xabcd0002, 0xffffffff, [NAMES[0], 0xffffffff, NAMES[2]], 3, 1),
    ]
    for name, tag, target, names, count, expected in region_cases:
        yield name, 0x170030, {1: tag, 7: target}, [], lookup_banks(tag, names, count), expected
    permutation_cases = [
        ('permutation-none-tag', 0xffffffff, 0, NAMES[0], NAMES, 3, 0xffffffff),
        ('permutation-none-region', 0xabcd0002, 0xffffffff, NAMES[0], NAMES, 3, 0xffffffff),
        ('permutation-zero-name', 0xabcd0002, 0, 0, [0, *NAMES[:2]], 3, 0xffffffff),
        ('permutation-zero-count', 0xabcd0002, 0, NAMES[0], NAMES, 0, 0xffffffff),
        ('permutation-negative-count', 0xabcd0002, 1, NAMES[0], NAMES, 0xffffffff, 0xffffffff),
        ('permutation-min-count', 0xabcd0002, 2, NAMES[0], NAMES, 0x80000000, 0xffffffff),
        ('permutation-first', 0xabcd0002, 0, NAMES[0], NAMES, 3, 0),
        ('permutation-middle', 0xabcd0002, 1, NAMES[1], NAMES, 3, 1),
        ('permutation-last', 0xabcd0002, 2, NAMES[2], NAMES, 3, 2),
        ('permutation-missing', 0xabcd0002, 2, 0x77777777, NAMES, 3, 0xffffffff),
        ('permutation-duplicate-first', 0xabcd0002, 2, NAMES[1], [NAMES[1]]*3, 3, 0),
        ('permutation-full-name-id', 0xabcd0002, 1, NAMES[1], [0x99556677, NAMES[1], 0xaa556677], 3, 1),
        ('permutation-tag-salt-alias', 0x76540002, 2, NAMES[2], NAMES, 3, 2),
        ('permutation-zero-tag', 0, 0, NAMES[1], NAMES, 3, 1),
        ('permutation-max-tag-index', 0x7654ffff, 2, NAMES[2], NAMES, 3, 2),
        ('permutation-high-name-id', 0xabcd0002, 1, 0xffffffff, [NAMES[0], 0xffffffff, NAMES[2]], 3, 1),
    ]
    for name, tag, region, target, names, count, expected in permutation_cases:
        # Early NONE paths must not need an invalid region-array element.
        banks = lookup_banks(tag, names, count, region if region != 0xffffffff else 0, True)
        yield name, 0x170080, {1: region, 2: tag, 7: target}, [], banks, expected
    for name, object_datum, definition_datum in [
        ('object-no-model', 0x45670002, 0xabcd0003),
        ('object-no-model-salt-alias', 0x98760002, 0x76540003),
        ('object-no-model-zero-indices', 0, 0),
        ('object-no-model-max-indices', 0x9876ffff, 0x7654ffff),
    ]:
        pool, headers, object_data, definition = 0x61000000, 0x61100000, 0x61200000, 0x62200000
        pool_data = bytearray(0x48)
        struct.pack_into('<I', pool_data, 0x44, headers)
        object_bytes = word(definition_datum)
        definition_bytes = bytearray(0x3c)
        struct.pack_into('<I', definition_bytes, 0x38, 0xffffffff)
        banks = [(0x4ecbd0, word(pool)), (pool, pool_data),
                 (headers+12*(object_datum & 0xffff)+8, word(object_data)),
                 (object_data, object_bytes), (0x4f0414, word(TAG_TABLE)),
                 (TAG_TABLE+16*(definition_datum & 0xffff)+8, word(definition)),
                 (definition, definition_bytes)]
        yield name, 0xbc440, {}, [object_datum, 0x81234567, 0x76543210], banks, definition


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    source = {}
    for entry in (0x170030, 0x170080, 0xbc440):
        body = complete_body(code, entry)
        pcs = sorted(body.code)
        end = max(pc+len(code.at(pc).bytes) for pc in pcs)
        source[f'{entry:x}'] = {'instructions': len(pcs), 'bytes': image.read(entry, end-entry).hex()}
    args.directory.mkdir(parents=True, exist_ok=False)
    args.directory = args.directory.resolve()
    text = image.section('.text')
    text_bytes = image.section_bytes(text)
    (args.directory/'text.bin').write_bytes(text_bytes)
    captured = []
    for index, (name, entry, registers, arguments, banks, expected_eax) in enumerate(cases()):
        folder = args.directory/f'case{index}'
        folder.mkdir()
        gp = [0x12345678, 0x23456789, 0x3456789a, 0x456789ab,
              STACK, 0x56789abc, 0x6789abcd, 0x789abcde]
        for reg, value in registers.items():
            gp[reg] = value
        live = [struct.pack('<QH', 0xa000000000000000, 0x3fff).hex(),
                struct.pack('<QH', 0xa000000000000000, 0xc000).hex()] if index % 2 else []
        before = {'gp': gp, 'xmm': [[0x11223344+n, 0xaabbccdd, 0x80000000, 0x7fc00001] for n in range(8)],
                  'eflags': 0x8d7 if index % 2 else 0x46, 'fs_base': 0,
                  'x87_values': live, 'x87_status': 0x4100 | ((5 if live else 0) << 11),
                  'x87_control_word': 0x37f if index % 2 else 0x67f, 'mxcsr': 0x1f80}
        stack = bytearray([0xa5])*0x2000
        for n, value in enumerate([RETURN, *arguments]):
            struct.pack_into('<I', stack, 0x1000+4*n, value)
        regions = [{'address': text.va, 'size': len(text_bytes), 'before_file': '../text.bin'}]
        for n, (address, data) in enumerate([*banks, (STACK_BASE, stack)]):
            file = f'bank{n}.bin'
            (folder/file).write_bytes(data)
            regions.append({'address': address, 'size': len(data), 'before_file': file})
        record = {'entry': entry, 'reference': 'English TU1.5 only',
                  'boundary': name, 'before': before, 'after': before, 'regions': regions, 'return': RETURN}
        (folder/'capture.json').write_text(json.dumps(record, indent=2)+'\n')
        output = args.directory/f'original{index}'
        result = subprocess.run([sys.executable, str(Path(__file__).with_name('replay_live_phantom_hardware_tu15.py')),
                                 '--capture', str(folder), '--output', str(output)], capture_output=True, text=True)
        assert result.returncode in (0, 1), (name, result.stdout, result.stderr)
        assert (output/'report.json').exists(), (name, result.stdout, result.stderr)
        report = json.loads((output/'report.json').read_text())
        trace = json.loads((output/'trace.json').read_text())
        assert report['original_abi']['gp'][0] == expected_eax, (name, report['original_abi']['gp'])
        assert report['original_abi']['gp'][4] == STACK+4+4*len(arguments), name
        raw = (output/'output.bin').read_bytes()
        offset = 276
        small = []
        for region in sorted(regions, key=lambda r: r['address']):
            after = raw[offset:offset+region['size']]
            offset += region['size']
            if region['address'] != text.va:
                small.append({'address': region['address'],
                              'before': (folder/region['before_file']).read_bytes().hex(), 'after': after.hex()})
        assert offset == len(raw)
        captured.append({'name': name, 'entry': entry, 'expected_eax': expected_eax,
                         'input': before, 'regions': small, 'original': report['original_abi'],
                         'instructions': trace['instructions'], 'eflags_mask': report['eflags_comparison_mask']})
        print(name, 'EAX', hex(expected_eax), len(trace['instructions']), 'original instructions', flush=True)
    fixture = {'reference': 'English TU1.5 only', 'original_sha256': NEW_PIN,
               'source': source, 'cases': captured,
               'boundary': 'Untouched original i386 CPU instructions with declared tag/object namespace; not Xbox hardware or live modeled permutation proof'}
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(fixture, indent=2)+'\n')
    print(f'PASS {len(captured)} original permutation lookup/no-model captures')


if __name__ == '__main__':
    main()
