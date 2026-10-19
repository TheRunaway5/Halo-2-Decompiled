#!/usr/bin/env python3
"""Export the original AI descriptor interface without constructor results.

Use tools/run_test.py. The original131-slot directory identifies each record.
Kind0 owns its20-byte base; kinds1/2 expose callbacks through+40, and kind1
also exposes the child count/pointer at+44/+48 (original1AF9EA..1AF9F6).
The71 real CRT mask destinations are excluded and stay constructor-owned.
The cache words at+8/+C remain writable, with their literal initial -2/0.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM, X86_REG_EAX
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from export_final_startup_storage_data_tu15 import regions


def descriptor_inputs(image):
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    roots = struct.unpack('<131I', image.read(0x47b478, 131 * 4))
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    constructors = []
    for index in range(71):
        entry = 0x381a60 + index * 16
        assert struct.unpack('<I', image.read(0x46dad8 + index * 4, 4))[0] == entry
        ins = list(md.disasm(image.read(entry, 11), entry))
        assert len(ins) == 3 and [i.mnemonic for i in ins] == ['mov', 'mov', 'ret']
        load, store, ret = ins
        assert load.operands[0].reg == X86_REG_EAX and load.operands[1].type == X86_OP_MEM
        assert store.operands[0].type == X86_OP_MEM and store.operands[1].reg == X86_REG_EAX
        assert not load.operands[1].mem.base and not load.operands[1].mem.index
        assert not store.operands[0].mem.base and not store.operands[0].mem.index
        source = load.operands[1].mem.disp & 0xffffffff
        destination = store.operands[0].mem.disp & 0xffffffff
        assert destination - 4 in roots and image.read(destination, 4) == bytes(4)
        constructors.append(dict(entry=entry, source=source, destination=destination,
            operand_hex=image.read(source, 4).hex(), instruction_hex=b''.join(i.bytes for i in ins).hex()))
    mutable = {p for row in constructors for p in range(row['destination'], row['destination'] + 4)}
    points = set()
    records = []
    unique = sorted(set(roots) - {0})
    for ordinal, root in enumerate(unique):
        kind = struct.unpack('<H', image.read(root + 2, 2))[0]
        assert kind in (0, 1, 2)
        size = 20 if kind == 0 else 76 if kind == 1 else 68
        if ordinal + 1 < len(unique):
            assert root + size <= unique[ordinal + 1]
        raw = image.read(root, size)
        assert raw[8:16] == bytes.fromhex('feffffff00000000')
        points.update(range(root, root + size))
        mutable.update(range(root + 8, root + 16))
        records.append(dict(address=root, kind=kind, bytes=size,
            directory_indices=[i for i, p in enumerate(roots) if p == root],
            original_hex=raw.hex(), sha256=hashlib.sha256(raw).hexdigest()))
    points.difference_update(mutable)
    return records, constructors, regions(points)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    parser.add_argument('--fixture', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.output.resolve().is_relative_to(scratch)
    image = Xbe(NEW_DEFAULT)
    records, constructors, ranges = descriptor_inputs(image)
    lines = ['// Authenticated TU1.5 AI descriptor interfaces; mutable CRT masks are excluded.']
    for address, count in ranges:
        raw = image.read(address, count)
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{count}>{{' +
            ','.join(f'0x{value:02x}' for value in raw) + '});')
    for row in records:
        address = row['address'] + 8
        raw = image.read(address, 8)
        lines.append(f'descriptor_cache(memory,0x{address:x}u,std::array<std::uint8_t,8>{{' +
            ','.join(f'0x{value:02x}' for value in raw) + '});')
    args.output.write_text('\n'.join(lines) + '\n')
    report = dict(original_xbe_sha256=NEW_PIN, records=records, mask_constructors=constructors,
        immutable_ranges=ranges, immutable_bytes=sum(n for _, n in ranges),
        mutable_descriptor_caches=[dict(address=r['address'] + 8, bytes=8,
            initial_hex='feffffff00000000') for r in records],
        mask_results_imported=False, supplied_game_results=False,
        source_instruction_changes=False, data_inc_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scratch=str(scratch), large_artifacts_retained=[])
    args.manifest.write_text(json.dumps(report, indent=2) + '\n')
    if args.fixture:
        assert args.fixture.resolve().is_relative_to(scratch)
        fixture = {key: report[key] for key in ('original_xbe_sha256', 'records',
            'mask_constructors', 'mutable_descriptor_caches')}
        fixture['boundary'] = 'Original metadata/constructor inputs, not completed runtime masks or MCC gameplay results'
        args.fixture.write_text(json.dumps(fixture, indent=2) + '\n')
    print(json.dumps(dict(records=len(records), immutable_bytes=report['immutable_bytes'],
        excluded_real_constructor_masks=len(constructors))), flush=True)


if __name__ == '__main__':
    main()
