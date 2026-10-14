#!/usr/bin/env python3
"""Offline unchanged-original LAN title-address flag decoder.

Execute3DAC60..3DAD28 with explicit scalar state/gateway/DNS inputs. The online
state and troubleshooting bit are zero; kernel IRQ and address acquisition are
outside this bounded proof. No instruction patches or supplied callee results.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    machine=Uc(UC_ARCH_X86,UC_MODE_32);machine.mem_map(0x10000,0x800000)
    for section in image.sections:machine.mem_write(section.va,image.section_bytes(section))
    owner,frame,output=0x50000000,0x51000000,0x52000000
    for p in (owner,frame,output):machine.mem_map(p,4096)
    rows=[]
    for ready,state,assignment in [(False,0,'none'),(True,0,'none'),(True,5,'static'),(True,12,'dhcp'),(True,13,'dhcp'),(True,14,'dhcp')]:
        for gateway in (False,True):
            for dns in (False,True):
                machine.mem_write(owner,bytes(4096));machine.mem_write(frame,bytes(4096));machine.mem_write(output,bytes(4096))
                for offset,value in ((0xa78,int(ready)),(0x8c8,state)):machine.mem_write(owner+offset,bytes([value]))
                for offset,value in ((0x8d0,int(gateway)),(0x8e0,int(dns))):machine.mem_write(owner+offset,struct.pack('<I',value))
                for register in (R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI):machine.reg_write(register,0)
                machine.reg_write(R.UC_X86_REG_EBX,owner);machine.reg_write(R.UC_X86_REG_EDX,output)
                machine.reg_write(R.UC_X86_REG_EBP,frame+0x800);machine.reg_write(R.UC_X86_REG_ESP,frame+0x700);machine.reg_write(R.UC_X86_REG_EFLAGS,2)
                machine.emu_start(0x3dac60,0x3dad29,count=100)
                assert machine.reg_read(R.UC_X86_REG_EIP)==0x3dad29
                flags=int.from_bytes(machine.mem_read(frame+0x808,4),'little')
                rows.append(dict(assignment=assignment,pending=not ready,source_state=state,gateway=gateway,dns=dns,flags=flags))
    report=dict(schema='H2_NATIVE_TU15_TITLE_ADDRESS_FLAGS_ORIGINAL_V1',source_sha256=NEW_PIN,
        region_sha256=hashlib.sha256(image.read(0x3dac60,0xc9)).hexdigest(),
        cases=len(rows),rows=rows,instruction_patches=[],supplied_callee_results=[],xbox_kernel_or_driver_proof=False)
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'cases':len(rows),'output':str(a.output)}))
if __name__=='__main__':main()
