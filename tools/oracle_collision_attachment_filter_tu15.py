#!/usr/bin/env python3
"""Execute the unchanged TU1.5 collision attachment filter over declared ABI inputs."""
import argparse, hashlib, itertools, json, struct
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from pathlib import Path

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for section in image.sections:machine.mem_write(section.va,image.section_bytes(section))
    machine.mem_map(0x1000000,0x10000)
    regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
          UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
    pool,headers,body,stack=0x1001000,0x1002000,0x1003000,0x100f000
    spans=[(0x4ecbd0,4),(0x46a0d0,4),(pool,0x48),(headers,24),(body,0x2c0),(stack-32,64)]
    trace=[];machine.hook_add(UC_HOOK_CODE,lambda _,pc,size,user:trace.append(pc))
    def word(p,v):machine.mem_write(p,struct.pack('<I',v&0xffffffff))
    def banks():return [dict(address=p,hex=bytes(machine.mem_read(p,n)).hex())for p,n in spans]
    cases=[]
    for kind,header_flag,runtime_flag,camouflage in itertools.product(range(8),(0,0x10),(0,1),
            (0,0x3f000000,0x3f800000,0x7fc01234)):
        machine.mem_write(0x1000000,bytes(0x10000));word(0x4ecbd0,pool);word(pool+0x44,headers)
        machine.mem_write(headers+14,bytes((header_flag,kind)));word(headers+20,body)
        word(body+4,runtime_flag);word(body+0x2b0,camouflage);word(stack,0x11000000)
        gp=[0x11110001,0x12345678,0x13572468,0x89abcdef,stack,0x55667788,0x66778899,0x778899aa]
        for r,v in zip(regs,gp):machine.reg_write(r,v)
        for n in range(8):machine.reg_write(UC_X86_REG_XMM0+n,0)
        machine.reg_write(UC_X86_REG_EFLAGS,0x202);machine.reg_write(UC_X86_REG_MXCSR,0x1f80)
        initial=banks();trace.clear();machine.emu_start(0x1ae4a0,0x11000000,count=100)
        assert machine.reg_read(UC_X86_REG_EIP)==0x11000000
        cases.append(dict(entry=0x1ae4a0,controls=dict(kind=kind,header_flag=header_flag,
            runtime_flag=runtime_flag,camouflage_bits=camouflage),entry_gp=gp,initial_banks=initial,
            exit_gp=[machine.reg_read(r)for r in regs],eflags=machine.reg_read(UC_X86_REG_EFLAGS),
            exit_xmm=[f'{machine.reg_read(UC_X86_REG_XMM0+n):032x}'for n in range(8)],
            instructions=trace.copy(),final_banks=banks()))
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text(json.dumps(dict(original_sha256=NEW_PIN,
        scope=__doc__,boundaries=['Declared raw ABI inputs; original instructions execute without supplied returns.'],
        cases=cases),separators=(',',':'))+'\n')
    print(f'PASS original TU1.5 attachment filter: {len(cases)} cases')

if __name__=='__main__':main()
