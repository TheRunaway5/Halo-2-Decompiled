#!/usr/bin/env python3
"""Export bounded original code-reference data for compiled physics and live constructors.

This is an offline export, not a runtime executable/tag import. Published
operands and every exported byte come from the pinned English TU1.5 image.
"""
from source_layout import source_file
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

REFERENCE_SHA256 = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BASE_CALLABLE_SHA256 = 'e97d4e0f22bd7c3dd1bfdf6f9c1e93fc9189874a125911a2def82d94f16e63dc'
SOURCES = {
    'physics_live_support_tu15_generated.cpp': '4f2cbf886af68d453aaf22d59ac946434919dfbbb0aaca06bba4ebf8ac66ded4',
    'physics_world_runtime_generated.cpp': 'dd3a1d5632cd191308447872a65a706ec159cf1522ac34c81aa0fc2ed7ed30f5',
    'physics_contact_runtime_generated.cpp': '9d11e39baf70c46bc26dc72e26111a4d948a54997cf99121bffe85d1921cc3e2',
    'physics_vehicle_motion_math_generated.cpp': '99f38774f71c2b604bf9d6488e22b7ebda7ba94dda9e60a69b0f1c25201471b9',
    'source_map_world_support_tu15_generated.cpp': '62d87656f5546eb39d8c15d02fd2f2341210b9bb9e851ab2ebad78a0baa6e5f4',
    'death_cycle_runtime_tu15_generated.cpp': '71c31f38121cc8553f246019bd504fd508938ef643d5b6d87bc38b6abd4ec3bd',
    'source_combat_action_tu15_generated.cpp': 'f636ef97771fd1fb02ffea229f1d732f68d00da2539845303714d8a3ab3ac947',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--base-data', type=Path,
        help='Extend the pinned146-table snapshot after rechecking every byte against TU1.5')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    sites = {}
    for name, expected in ({} if args.base_data else SOURCES).items():
        raw = (source_file(ROOT, name)).read_bytes()
        assert hashlib.sha256(raw).hexdigest() == expected
        text = raw.decode()
        if name.endswith('_tu15_generated.cpp'):
            for pc in re.findall(r'services\.instruction\(0x([0-9a-f]+)u?\)', text):
                sites.setdefault(int(pc, 16), set()).add(name)
        for _, pc in re.findall(r'\b\w*observation_pc\(\s*0x([0-9a-f]+)u?,\s*0x([0-9a-f]+)u?', text):
            sites.setdefault(int(pc, 16), set()).add(name)
    image = Xbe(args.reference)
    assert hashlib.sha256(image.data).hexdigest() == REFERENCE_SHA256
    code = _Code(image, '.text')
    # Actual imported-MCC conveyor solves reach this constructor's interface
    # through326491/+24 and320201/+28. It is a distinct source owner between
    # the existing41ED80 and41EDBC interfaces (original audit v1401).
    constructor = code.at(0x31f728)
    assert (constructor.mnemonic == 'mov' and constructor.operands[0].type == X86_OP_MEM
            and constructor.operands[0].size == 4 and constructor.operands[1].type == X86_OP_IMM
            and constructor.operands[1].imm == 0x41ed90)
    sites.setdefault(0x31f728, set()).add('audited live TU1.5 conveyor constraint constructor')
    readonly = image.section('.rdata')
    assert not readonly.flags & 1
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    decoder.skipdata = True
    section = image.section('.text')
    candidates = {}
    for instruction in decoder.disasm(image.section_bytes(section), section.va):
        if instruction.mnemonic != 'mov' or len(instruction.operands) != 2:
            continue
        destination, operand = instruction.operands
        if destination.type != X86_OP_MEM or operand.type != X86_OP_IMM or destination.size != 4:
            continue
        address = operand.imm
        if address % 4 or not readonly.va <= address < readonly.va + readonly.vsize - 3:
            continue
        if not code.inside(int.from_bytes(image.read(address, 4), 'little')):
            continue
        candidates.setdefault(address, []).append(instruction.address)
    ordered, records = sorted(candidates), []
    for ordinal, address in enumerate(ordered):
        matched = [pc for pc in candidates[address] if pc in sites]
        if not matched:
            continue
        limit = ordered[ordinal+1] if ordinal+1 < len(ordered) else readonly.va + readonly.vsize
        end = address
        while end + 4 <= limit and code.inside(int.from_bytes(image.read(end, 4), 'little')):
            end += 4
        assert end > address
        records.append(dict(address=address, data=image.read(address, end-address),
                            published_by=matched, boundary=end,
                            sources=sorted({name for pc in matched for name in sites[pc]})))
    if args.base_data:
        base = args.base_data.read_bytes()
        assert hashlib.sha256(base).hexdigest() == BASE_CALLABLE_SHA256
        text = base.decode()
        arrays = {}
        for size, name, body in re.findall(
                r'constexpr std::array<std::byte,(\d+)> (callable_\d+)\{(.*?)\};', text, re.S):
            data = bytes(int(value, 16) for value in re.findall(r'std::byte\{0x([0-9a-f]+)\}', body))
            assert len(data) == int(size) and len(data) % 4 == 0
            arrays[name] = data
        rows = re.findall(r'BranchTable\{0x([0-9a-f]+)u,(callable_\d+)\}', text)
        assert len(rows) == len(arrays) == 146 and sum(map(len, arrays.values())) == 4496
        for address, name in rows:
            address = int(address, 16);data = arrays[name]
            assert readonly.va <= address < address + len(data) <= readonly.va + readonly.vsize
            assert image.read(address, len(data)) == data
            assert all(code.inside(int.from_bytes(data[n:n+4], 'little')) for n in range(0,len(data),4))
            records.append(dict(address=address, data=data, published_by=[],
                boundary=address+len(data), sources=['byte-verified pinned callable-data snapshot']))
        records.sort(key=lambda row: row['address'])
    assert len(records) == 147 and sum(len(row['data']) for row in records) == 4540
    conveyor = next(row for row in records if row['address'] == 0x41ed90)
    assert len(conveyor['data']) == 44 and conveyor['boundary'] == 0x41edbc
    lines = ['// Generated by tools/export_native_tu15_physics_callable_data.py.',
             '// English TU1.5 reference SHA256: ' + REFERENCE_SHA256,
             '// Exact code-reference prefixes, not inferred complete C++ class layouts.']
    for ordinal, row in enumerate(records):
        raw = row['data']
        lines.append(f'constexpr std::array<std::byte,{len(raw)}> callable_{ordinal}{{')
        for i in range(0, len(raw), 12):
            lines.append('    ' + ','.join(f'std::byte{{0x{value:02x}}}' for value in raw[i:i+12]) + ',')
        lines.append('};')
    lines.append('constexpr std::array callable_tables{')
    for ordinal, row in enumerate(records):
        lines.append(f'    BranchTable{{0x{row["address"]:x}u,callable_{ordinal}}},')
    lines.append('};')
    output = ('\n'.join(lines) + '\n').encode()
    temporary = scratch / 'callable-data.inc'
    temporary.write_bytes(output)
    args.output.write_bytes(temporary.read_bytes())
    report = dict(passed=True, scratch=str(scratch), reference_sha256=REFERENCE_SHA256,
                  source_sha256={} if args.base_data else SOURCES,
                  base_data_sha256=BASE_CALLABLE_SHA256 if args.base_data else None,
                  records=len(records), literal_bytes=4540,
                  output_sha256=hashlib.sha256(output).hexdigest(),
                  scope='Readonly original code-reference prefixes and the audited live31F728 constructor; no complete class-layout, implemented-target or live-acceptance claim.',
                  rows=[dict(**{key:value for key,value in row.items() if key != 'data'},
                             data_hex=row['data'].hex()) for row in records],
                  runtime_reference_file_required=False, full_physics_parity=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key:value for key,value in report.items() if key != 'rows'}))


if __name__ == '__main__':
    main()
