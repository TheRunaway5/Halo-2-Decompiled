#!/usr/bin/env python3
"""Pin the genuine mutable view/reset byte reached by graphics-cold229BB4."""
import argparse
import hashlib
import json
import os
import struct
from pathlib import Path
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    target = 0x4f2968
    section = next(s for s in image.sections if s.va <= target < s.va+s.vsize)
    assert section.name == '.data' and section.flags == 7
    assert target >= section.va+section.rsize
    assert image.read(target, 1) == b'\0'
    code = _Code(image, '.text')
    code.md.skipdata = True
    xrefs, neighbors = [], []
    for ins in code.md.disasm(code.bytes, code.lo):
        if not ins.id:
            continue
        matched = []
        for operand in ins.operands:
            if operand.type == X86_OP_MEM:
                start = operand.mem.disp & 0xffffffff
                if ((start <= target < start+operand.size)
                        or target <= start < target+8):
                    matched.append(dict(kind='memory', address=hex(start),
                        bytes=operand.size, access=operand.access,
                        base=ins.reg_name(operand.mem.base), index=ins.reg_name(operand.mem.index)))
            elif operand.type == X86_OP_IMM and target <= operand.imm < target+8:
                matched.append(dict(kind='immediate', address=hex(operand.imm)))
        if matched:
            row = dict(pc=hex(ins.address), bytes=ins.bytes.hex(),
                operation=ins.mnemonic, operands=ins.op_str, matched=matched)
            (xrefs if any(m['address'] == hex(target) for m in matched) else neighbors).append(row)
    pointers = []
    for candidate in image.sections:
        if candidate.name == '.text':
            continue
        data = image.section_bytes(candidate)
        for address in range(target, target+8):
            needle = struct.pack('<I', address)
            offset = data.find(needle)
            while offset >= 0:
                pointers.append(dict(section=candidate.name, address=hex(candidate.va+offset), target=hex(address)))
                offset = data.find(needle, offset+1)
    assert xrefs and not neighbors and not pointers
    assert all(len(row['matched']) == 1 and row['matched'][0]['kind'] == 'memory'
               and row['matched'][0]['bytes'] == 1
               and not row['matched'][0]['base'] and not row['matched'][0]['index'] for row in xrefs)
    assert any(row['pc'] == '0x229bb4' and row['operation'] == 'mov' for row in xrefs)
    # Retain primary initializer evidence, including the BL=0 definition and
    # its later actual store. Do not execute this broad activation graph.
    contexts = {}
    for start, end in ((0x12675d,0x126774),(0x12681e,0x126828),
                       (0x229ba3,0x229bbb),(0x229bc7,0x229bd0),
                       (0x13081c,0x13082c)):
        contexts[hex(start)] = [dict(pc=hex(i.address), bytes=i.bytes.hex(),
            operation=i.mnemonic, operands=i.op_str)
            for i in code.md.disasm(image.read(start,end-start),start)]
    source = Path('src/physics/native_tu15_arena_data.cpp')
    test = Path('tests/native_tu15_cold_data_tests.cpp')
    report = dict(passed=True, original_sha256=NEW_PIN,
        address=hex(target), bytes='00', whole_field_bytes=1,
        section=vars(section), raw_initialized_end=hex(section.va+section.rsize),
        cold_initializer='XBE loader zero-filled mutable .data tail',
        later_initializer=dict(clear_site='0x126820', zero_definition='0x126766',
                               scope='Primary state-producer evidence; full activation graph remains separately owned.'),
        text_xrefs=xrefs, neighboring_target_xrefs=neighbors,
        nontext_pointer_refs=pointers, primary_contexts=contexts,
        source_owner=str(source), source_owner_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        cold_constructor_test=str(test), cold_constructor_test_sha256=hashlib.sha256(test.read_bytes()).hexdigest(),
        scope='One genuine mutable loader-zeroed field; no guessed surrounding bank, initializer execution, rendered-frame or campaign-completion claim.',
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True, bytes=1, xrefs=xrefs,
                          loader_zeroed=True, pointer_refs=pointers)))


if __name__ == '__main__':
    main()
