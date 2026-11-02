#!/usr/bin/env python3
"""Authenticate only the original timer state/timestamp storage operands."""
import argparse
import hashlib
import json
import os
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    listings = {}
    for entry in (0x777e0, 0x77ab0):
        body = complete_body(code, entry)
        listings[hex(entry)] = [dict(pc=hex(pc), bytes=code.at(pc).bytes.hex(),
            operation=code.at(pc).mnemonic, operands=code.at(pc).op_str)
            for pc in sorted(body.code)]
    for pc, operation, operands in (
        (0x777ff, 'imul', 'eax, eax, 0x98'),
        (0x77808, 'cmp', 'dword ptr [esi + 0x515f70], 3'),
        (0x77818, 'mov', 'dword ptr [esi + 0x515f70], 4'),
        (0x77830, 'mov', 'dword ptr [esi + 0x515f74], eax'),
        (0x77836, 'add', 'edi, 0x99c'),
        (0x7783d, 'cmp', 'edi, 0x300c'),
        (0x77abe, 'mov', 'byte ptr [0x51d01c], 1'),
        (0x77ac5, 'mov', 'byte ptr [0x51d01d], al'),
        (0x77ad3, 'call', '0x777e0'),
        (0x77ad9, 'cmp', 'esi, 0x10')):
        instruction = code.at(pc)
        assert (instruction.mnemonic, instruction.op_str) == (operation, operands), hex(pc)
    section = image.section('.data')
    fields = []
    for group in range(5):
        for slot in range(16):
            address = 0x515f70 + group * 0x99c + slot * 0x98
            assert section.va + section.rsize <= address
            assert address + 8 <= section.va + section.vsize
            assert image.read(address, 8) == bytes(8)
            fields.append(dict(group=group, slot=slot, address=hex(address), bytes=8,
                loader_bytes='0000000000000000'))
    assert section.va + section.rsize <= 0x51d01c
    assert 0x51d01e <= section.va + section.vsize
    assert image.read(0x51d01c, 2) == bytes(2)
    fields.append(dict(role='parent policy fields', address='0x51d01c', bytes=2,
                       loader_bytes='0000'))
    report = dict(passed=True, original_sha256=NEW_PIN, original_roots=listings,
        section=dict(name=section.name, va=hex(section.va), file_bytes=section.rsize,
                     virtual_bytes=section.vsize),
        fields=fields, total_declared_bytes=sum(row['bytes'] for row in fields),
        source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        limits=['Only the two consumed DWORDs of each of 80 original records are declared',
                'Record payloads, group headers and intervening gaps remain undeclared',
                'Loader zeros do not replace original initialization or live timer state'],
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('Authenticated 80 original loader-zeroed state/timestamp pairs and two parent policy bytes (642 bytes)')


if __name__ == '__main__':
    main()
