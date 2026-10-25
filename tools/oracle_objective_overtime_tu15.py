#!/usr/bin/env python3
"""Unmodified original24A620 overtime calls, with integer and FP boundaries."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from oracle_objective_native_tu15 import REGS
from oracle_ctf_activation_tu15 import SOURCE
from oracle_smg_firing_1_1 import Xbe,_Code
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_WRITE
from unicorn import x86_const as x86

BANK,CLOCK,OPTIONS,STACK=0x4000000,0x4001000,0x4002000,0x400f000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    spec=SOURCE['u5'];path=Path(spec['path']);assert hashlib.sha256(path.read_bytes()).hexdigest()==spec['pin']
    image=Xbe(path);code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x10000,0x600000);cpu.mem_write(code.lo,code.bytes);cpu.mem_map(BANK,0x10000)
    original_window=int.from_bytes(image.read(0x46a100,4),'little')
    writes=[];cases=[]
    cpu.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,user:
        writes.append([uc.reg_read(x86.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)]))
    initial_xmm=[[0x3f000000+n*0x100+j for j in range(4)] for n in range(8)]
    for remaining,advance,eligible,enabled,latched,previous in itertools.product(
            (-1,0,149,150,151),(False,True),(False,True),(False,True),(False,True),(-2147483648,-1,0,1,150,2147483647)):
        cpu.mem_write(BANK,bytes(0x10000));gp=[0x12345678,0x24681357,0x13572468,0x10203040,STACK,0x87654321,0x76543210,0xabcdef01]
        patches=[(STACK,4,0x10000),(STACK+4,4,remaining),(STACK+8,4,int(advance)),(STACK+12,4,int(eligible)),
            (0x51df54,4,CLOCK),(0x52d710,4,BANK),(0x4f3218,4,OPTIONS),(0x46a100,4,original_window),
            (CLOCK+2,2,30),(CLOCK+4,4,int.from_bytes(struct.pack('<f',1/30),'little')),
            (OPTIONS+0x22c,1,2 if enabled else 0),(BANK+0x1fc,1,int(latched)),(BANK+0x200,4,previous)]
        for at,width,value in patches:cpu.mem_write(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        for reg,value in zip(REGS,gp):cpu.reg_write(reg,value)
        for n,values in enumerate(initial_xmm):cpu.reg_write(getattr(x86,f'UC_X86_REG_XMM{n}'),sum(v<<(32*j) for j,v in enumerate(values)))
        cpu.reg_write(x86.UC_X86_REG_FPCW,0x27f);cpu.reg_write(x86.UC_X86_REG_FPSW,0);cpu.reg_write(x86.UC_X86_REG_FPTAG,0xffff)
        cpu.reg_write(x86.UC_X86_REG_MXCSR,0x1f80);cpu.reg_write(x86.UC_X86_REG_EFLAGS,0xad7);writes.clear()
        cpu.emu_start(0x24a620,0x10000,count=1000);assert cpu.reg_read(x86.UC_X86_REG_EIP)==0x10000
        xmm=[]
        for n in range(8):
            value=cpu.reg_read(getattr(x86,f'UC_X86_REG_XMM{n}'));xmm.append([(value>>(j*32))&0xffffffff for j in range(4)])
        cases.append({'entry':0x24a620,'name':f'overtime-{remaining}-{int(advance)}-{int(eligible)}-{int(enabled)}-{int(latched)}-{previous}',
            'inputs':patches,'gp':gp,'eflags':0xad7,'xmm':initial_xmm,'expected_gp':[cpu.reg_read(reg) for reg in REGS],
            'expected_flags':cpu.reg_read(x86.UC_X86_REG_EFLAGS)&0x8d5,'writes':list(writes),'expected_xmm':xmm,
            'expected_x87_status':cpu.reg_read(x86.UC_X86_REG_FPSW)&0xfdc0,'expected_mxcsr':cpu.reg_read(x86.UC_X86_REG_MXCSR)})
    assert len(cases)==480
    data=json.dumps({'schema':'H2_NATIVE_OBJECTIVE_OWNERS_V1','original_sha256':spec['pin'],'supplied_service_results':[],
        'original_window_constant':{'address':0x46a100,'bits':original_window},'cases':cases},separators=(',',':'))+'\n'
    assert len(data.encode())<=1024*1024;args.fixture.write_text(data)
    print(json.dumps({'cases':len(cases),'bytes':len(data.encode()),'sha256':hashlib.sha256(data.encode()).hexdigest()}))


if __name__=='__main__':main()
