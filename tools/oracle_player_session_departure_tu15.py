#!/usr/bin/env python3
"""Execute untouched TU1.5 session-row departure cleanup and its real iterator."""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);args=parser.parse_args()
    assert not args.fixture.exists()
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    regs=[R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,
          R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]
    cases=[]
    for slot,populated in [(0,True),(4,True),(15,True),(7,False)]:
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
        for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
        cpu.mem_map(0x2400000,0x300000);cpu.mem_map(0x2800000,0x1000)
        session,peers,stack=0x2400000,0x2410000,0x2600000
        session_bytes=bytearray([0xa5])*0x1000;peer_bytes=bytearray([0x5a])*0x1000
        for index in range(15):struct.pack_into('<I',session_bytes,0x44+index*4,0)
        if populated:
            # Declared caller rows exercise admitted types, disconnected rows,
            # inactive rows, excluded types and both set/clear membership bits.
            for index,kind,connection,active,mask in [(0,2,1,1,0xffff),(1,4,0xffffffff,1,0xffff),
                    (2,4,1,0,0xffff),(3,1,1,1,0xffff),(4,2,1,1,0),(14,4,1,1,0xffff)]:
                at=index*0x100;struct.pack_into('<I',session_bytes,0x44+index*4,peers+at)
                peer_bytes[at+2]=kind;struct.pack_into('<I',peer_bytes,at+0x3c,connection)
                peer_bytes[at+0x75]=active;struct.pack_into('<I',peer_bytes,at+0x7c,mask)
        stack_bytes=bytearray([0xcd])*0x4000;struct.pack_into('<I',stack_bytes,0x2000,0x2800000)
        initial=[(session,bytes(session_bytes)),(peers,bytes(peer_bytes)),(stack,bytes(stack_bytes))]
        for address,data in initial:cpu.mem_write(address,data)
        gp=[0x72000000+n*0x1000 for n in range(8)];gp[0]=0xec700000|slot;gp[1]=session;gp[4]=stack+0x2000
        for reg,value in zip(regs,gp):cpu.reg_write(reg,value)
        cpu.reg_write(R.UC_X86_REG_EFLAGS,0x202);trace=[]
        def original(cpu,pc,size,_):
            assert bytes(cpu.mem_read(pc,size))==image.read(pc,size)
            trace.append(pc)
        cpu.hook_add(UC_HOOK_CODE,original)
        cpu.emu_start(0x6a9a0,0x2800000,count=100000)
        assert cpu.reg_read(R.UC_X86_REG_EIP)==0x2800000
        cases.append(dict(slot=slot,populated=populated,entry_gp=gp,exit_gp=[cpu.reg_read(r) for r in regs],
            eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),instructions=trace,
            banks=[dict(address=address,initial=data.hex(),final=bytes(cpu.mem_read(address,len(data))).hex())
                   for address,data in initial]))
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text(json.dumps(dict(original_sha256=NEW_PIN,entry=0x6a9a0,cases=cases),indent=2)+'\n')
    print('Executed',len(cases),'whole untouched TU1.5 session-row cleanup cases')
if __name__=='__main__':main()
