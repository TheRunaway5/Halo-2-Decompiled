#!/usr/bin/env python3
"""Read unchanged original matchmaking screen update/sound caller operands."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    old=Xbe(Path('assets/disc/default.xbe'));image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True
    section=image.section('.text');raw=image.section_bytes(section)
    def locate(entry,size):
        pattern=bytearray(old.read(entry,size));masked=set()
        for instruction in md.disasm(bytes(pattern),entry):
            start=instruction.address-entry
            if (instruction.mnemonic=='call' or instruction.mnemonic.startswith('j')) and instruction.imm_size:
                masked.update(range(start+instruction.imm_offset,start+instruction.imm_offset+instruction.imm_size))
            if instruction.disp_size and any(o.type==X86_OP_MEM and 0x10000<=o.mem.disp<0x800000 for o in instruction.operands):
                masked.update(range(start+instruction.disp_offset,start+instruction.disp_offset+instruction.disp_size))
        expression=b''.join(b'.'if index in masked else re.escape(bytes([value]))for index,value in enumerate(pattern))
        matches=list(re.finditer(expression,raw,re.DOTALL));assert len(matches)==1,(hex(entry),len(matches))
        return section.va+matches[0].start()
    functions=[]
    for name,entry,size in [('screen_update',0x251B39,0xC6),('lobby_countdown_update',0x250A8B,0xE1),('ui_sound',0x236299,0x50)]:
        target=locate(entry,min(size,0x40))
        original=list(md.disasm(old.read(entry,size),entry))
        updated=list(md.disasm(image.read(target,size),target))
        functions.append(dict(name=name,retail_entry=hex(entry),tu15_entry=hex(target),bytes=size,
            fixed_extent_mnemonics_equal=[i.mnemonic for i in original]==[i.mnemonic for i in updated],
            tu15_code_sha256=hashlib.sha256(image.read(target,size)).hexdigest(),
            instructions=[dict(address=hex(i.address),mnemonic=i.mnemonic,operands=i.op_str)for i in updated]))
    for name,target,size in [('matched_update_tail',0x257F1B,0x12),('matched_status',0x257FBF,0xF3),('matched_pane3',0x2580B2,0x209),('matched_countdown',0x25842D,0x7C),('timer_query',0x195041,0x82),('countdown_seconds_query',0x1950C3,0x32),('countdown_active_query',0x1950F5,0x40)]:
        functions.append(dict(name=name,tu15_entry=hex(target),bytes=size,
            tu15_code_sha256=hashlib.sha256(image.read(target,size)).hexdigest(),
            instructions=[dict(address=hex(i.address),mnemonic=i.mnemonic,operands=i.op_str)
                          for i in md.disasm(image.read(target,size),target)]))
    report=dict(original_sha256=NEW_PIN,read_only=True,no_instruction_patches=True,no_replaced_callees=True,functions=functions)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    for function in functions:
        print(function['name'],function.get('retail_entry','actual TU1.5 caller target'),'->',function['tu15_entry'])
        for instruction in function['instructions']:
            print(instruction['address'],instruction['mnemonic'],instruction['operands'])


if __name__=='__main__':main()
