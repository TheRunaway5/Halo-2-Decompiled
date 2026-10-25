#!/usr/bin/env python3
"""Retain authenticated TU1.5 references to the lens caller's view count."""
import hashlib,json,os,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=Cs(CS_ARCH_X86,CS_MODE_32);code.detail=True
    target=0x4c686c;needle=struct.pack('<I',target);rows=[]
    for section in image.sections:
        if not section.flags&4:continue
        data=image.section_bytes(section);cursor=0
        while (at:=data.find(needle,cursor))>=0:
            cursor=at+1
            for start in range(max(0,at-12),at):
                instruction=next(code.disasm(data[start:at+16],section.va+start,count=1),None)
                if instruction is None:continue
                if not instruction.address<=section.va+at<instruction.address+instruction.size:continue
                operands=[o for o in instruction.operands if o.type==X86_OP_MEM and not o.mem.base and not o.mem.index and o.mem.disp==target]
                if not operands:continue
                row=dict(address=instruction.address,bytes=bytes(instruction.bytes).hex(),mnemonic=instruction.mnemonic,operands=instruction.op_str,writes=any(o.access&CS_AC_WRITE for o in operands))
                if row not in rows:rows.append(row)
    out=Path(__file__).resolve().parents[1]/'analysis/combat-presentation-audit-20261022/repair-client-lens/caller-input-xrefs.json'
    caller=[dict(address=i.address,bytes=bytes(i.bytes).hex(),mnemonic=i.mnemonic,operands=i.op_str) for i in code.disasm(image.read(0x2b5b0,0x80),0x2b5b0)]
    callsites=[]
    for section in image.sections:
        if not section.flags&4:continue
        data=image.section_bytes(section)
        for at in range(len(data)-4):
            if data[at]!=0xe8:continue
            if (section.va+at+5+struct.unpack_from('<i',data,at+1)[0])&0xffffffff!=0x2b5b0:continue
            callsites.append(dict(address=section.va+at,preceding_bytes=data[max(0,at-64):at+5].hex()))
    out.write_text(json.dumps(dict(xbe_sha256=NEW_PIN,target=target,scope=__doc__,references=rows,render_caller=caller,callsites=callsites,instruction_or_result_replaced=False),indent=2)+'\n')
    print(json.dumps(callsites),flush=True)
if __name__=='__main__':main()
