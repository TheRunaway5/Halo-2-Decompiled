#!/usr/bin/env python3
"""Authenticate the actual graphics-cold2F806 immutable scalar owner."""
import argparse
import hashlib
import json
import os
import struct
from pathlib import Path
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    address = 0x45c55c
    value = image.read(address, 4)
    assert value.hex() == 'c3f5483f'
    section = next(s for s in image.sections if s.va <= address < s.va+s.vsize)
    assert section.name == '.rdata' and section.flags == 6
    code = _Code(image, '.text')
    body = _trace(code, 0x2f7e0, {0x2f7e0})
    listing = [dict(pc=hex(pc), bytes=code.at(pc).bytes.hex(),
                    operation=code.at(pc).mnemonic, operands=code.at(pc).op_str)
               for pc in sorted(body.code)]
    assert 0x2f806 in body.code
    read = code.at(0x2f806)
    assert read.mnemonic == 'movss' and read.operands[1].mem.disp == address
    # Decode the entire original text with explicit skip-data handling. This
    # is an xref inventory; it does not execute or qualify another function.
    code.md.skipdata = True
    xrefs = []
    for ins in code.md.disasm(code.bytes, code.lo):
        if not ins.id:
            continue
        operands = [operand for operand in ins.operands
                    if (operand.type == X86_OP_MEM and operand.mem.disp == address)
                    or (operand.type == X86_OP_IMM and operand.imm == address)]
        if operands:
            xrefs.append(dict(pc=hex(ins.address), bytes=ins.bytes.hex(),
                operation=ins.mnemonic, operands=ins.op_str,
                operand_access=[operand.access for operand in operands]))
    pointers = []
    needle = struct.pack('<I', address)
    for candidate in image.sections:
        if candidate.name == '.text':
            continue
        data = image.section_bytes(candidate)
        offset = data.find(needle)
        while offset >= 0:
            pointers.append(dict(section=candidate.name, address=hex(candidate.va+offset)))
            offset = data.find(needle, offset+1)
    source = Path('src/rendering/renderer/native_tu15_renderer_transparent_storage_data.cpp')
    test = Path('tests/native_tu15_cold_data_tests.cpp')
    report = dict(passed=True, original_sha256=NEW_PIN,
        source_address=hex(address), bytes=value.hex(),
        float_value=struct.unpack('<f', value)[0], original_section=vars(section),
        owner_entry='0x2f7e0', failing_read='0x2f806',
        original_owner_sites=len(listing), original_owner_calls=[],
        original_owner_listing=listing, exact_address_text_xrefs=xrefs,
        exact_address_nontext_pointer_refs=pointers,
        adjacent_name=dict(address='0x45c548', bytes=image.read(0x45c548,20).hex(),
                           exclusive_end='0x45c55c'),
        native_owner=str(source), native_owner_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        cold_constructor_test=str(test), cold_constructor_test_sha256=hashlib.sha256(test.read_bytes()).hexdigest(),
        scope='Exact immutable source-data owner only. No initializer, camera semantic kernel, rendered frame or New Game completion claim.',
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(dict(passed=True, bytes=value.hex(), xrefs=xrefs, pointers=pointers)))


if __name__ == '__main__':
    main()
