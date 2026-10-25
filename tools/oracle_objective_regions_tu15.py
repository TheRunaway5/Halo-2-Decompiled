#!/usr/bin/env python3
"""Whole Xbox region constructors: palette choice, capacity and timer progress."""
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
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_MEM_WRITE,UC_HOOK_CODE
from unicorn import x86_const as x86

BANK,OPTIONS,POOL,ROWS,BODY,ROSTER,PLAYERS,PLAYER_ROWS,RECORD,POINT,STACK=(
    0x4000000,0x4001000,0x4002000,0x4003000,0x4004000,0x4005000,
    0x4006000,0x4007000,0x4008000,0x4009000,0x400f000)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    spec=SOURCE['u5'];path=Path(spec['path']);assert hashlib.sha256(path.read_bytes()).hexdigest()==spec['pin']
    image=Xbe(path);code=_Code(image,'.text');cpu=Uc(UC_ARCH_X86,UC_MODE_32)
    cpu.mem_map(0x10000,0x600000);cpu.mem_write(code.lo,code.bytes);cpu.mem_map(BANK,0x10000)
    writes=[];cases=[];xmm=[[0x3f000000+n*0x100+j for j in range(4)] for n in range(8)]
    cpu.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,user:
        writes.append([uc.reg_read(x86.UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)]))
    required_boundary=[0]
    cpu.hook_add(UC_HOOK_CODE,lambda uc,address,size,user:
        uc.emu_stop() if required_boundary[0] and address==required_boundary[0] else None)
    constants=[(address,4,int.from_bytes(image.read(address,4),'little')) for address in
               (0x46a0d0,0x46a178,*range(0x4751e4,0x4751f0,4),*range(0x4751f4,0x475200,4))]
    def bits(value):return int.from_bytes(struct.pack('<f',value),'little')
    def add(entry,name,patches,registers,*,boundary=0):
        required_boundary[0]=boundary
        cpu.mem_write(BANK,bytes(0x10000));gp=[0x12345678,0x24681357,0x13572468,0x10203040,STACK,0x87654321,0x76543210,0xabcdef01]
        for n,value in registers.items():gp[n]=value&0xffffffff
        patches=[(STACK,4,0x10000),*constants,*patches]
        for at,width,value in patches:cpu.mem_write(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        for reg,value in zip(REGS,gp):cpu.reg_write(reg,value)
        for n,values in enumerate(xmm):cpu.reg_write(getattr(x86,f'UC_X86_REG_XMM{n}'),sum(v<<(32*j) for j,v in enumerate(values)))
        cpu.reg_write(x86.UC_X86_REG_FPCW,0x27f);cpu.reg_write(x86.UC_X86_REG_FPSW,0);cpu.reg_write(x86.UC_X86_REG_FPTAG,0xffff)
        cpu.reg_write(x86.UC_X86_REG_MXCSR,0x1f80);cpu.reg_write(x86.UC_X86_REG_EFLAGS,0xad7);writes.clear()
        cpu.emu_start(entry,0x10000,count=10000);assert cpu.reg_read(x86.UC_X86_REG_EIP)==(boundary or 0x10000),name
        actual_xmm=[]
        for n in range(8):
            value=cpu.reg_read(getattr(x86,f'UC_X86_REG_XMM{n}'));actual_xmm.append([(value>>(j*32))&0xffffffff for j in range(4)])
        mask=0xc5 if boundary else 0x8d5
        cases.append({'entry':entry,'name':name,'inputs':patches,'gp':gp,'eflags':0xad7,'xmm':xmm,'boundary':boundary,'eflags_mask':mask,
            'expected_gp':[cpu.reg_read(reg) for reg in REGS],'expected_flags':cpu.reg_read(x86.UC_X86_REG_EFLAGS)&mask,
            'writes':list(writes),'expected_xmm':actual_xmm,'expected_x87_status':cpu.reg_read(x86.UC_X86_REG_FPSW)&0xfdc0,
            'expected_mxcsr':cpu.reg_read(x86.UC_X86_REG_MXCSR)})
    for count,team,remaining,engine,flags,seconds in itertools.product((0,1,2),(-1,0,8),(-1,0,1),(1,9),(0,2),(0,5)):
        patches=[(0x4ecbd0,4,POOL),(POOL+0x44,4,ROWS),(ROWS+3*12+8,4,BODY),(BODY+0x17e,2,team),
            (RECORD+0x44,4,count),(0x4f3218,4,OPTIONS),(OPTIONS+0x180,4,engine),
            (OPTIONS+0x230,4,seconds),(OPTIONS+0x246,2,seconds),(0x52d710,4,BANK),
            *((POINT+n*4,4,bits(value)) for n,value in enumerate((1.25,-2.5,3.75)))]
        if team>=0:patches.extend([(BANK+0x114+team*2,2,remaining),(BANK+0x170+team,1,flags)])
        add(0x24d390,f'region-{count}-{team}-{remaining}-{engine}-{flags}-{seconds}',patches,
            {0:RECORD,1:POINT,2:0x87650003})
    for local,species,point in [(local,species,point) for local,species in [(-1,0),*( (0,n) for n in (0,1,2,3,255))] for point in (0,1)]:
        patches=[(0x4f54f0,4,ROSTER),(0x4f54f4,4,PLAYERS),(PLAYERS+0x44,4,PLAYER_ROWS),
            (0x4c66f8,4,local),(ROSTER+0xc,4,0x87650002),(PLAYER_ROWS+2*0x21c+0x88,1,species),
            *((POINT+n*4,4,bits(value)) for n,value in enumerate((1.25,-2.5,3.75) if point else (0,0,0)))]
        add(0x24d1c0,f'palette-{local}-{species}-{point}',patches,{1:RECORD,2:POINT})
    # The actual renderer remains a required owner. Stop before its first
    # instruction and compare the exact argument/stack publication contract;
    # no renderer return value or successful draw is supplied.
    for index,color in itertools.product((0,1),(-1,0,1,7,8)):
        for selected in (False,True):
            patches=[(0x4ecc1c,4,OPTIONS),(OPTIONS+0x16c,4,0x87650000|index),
                (0x4f0414,4,POOL),(POOL+index*16+8,4,BODY),(BODY+0xc,4,ROWS),
                (ROWS+0x534,4,BANK+0xa000),(BANK+0xa000+0xc4,4,0x12340000|int(selected))]
            add(0x24d920,f'draw-owner-{index}-{color}-{int(selected)}',patches,
                {0:POINT,1:color},boundary=0x41480)
    engine=BANK+0xb000;globals_tag=BANK+0xa000;render_data=BANK+0xd000
    registered=int.from_bytes(image.read(0x47d174,4),'little')
    for kind,local_team in itertools.product((0,1,2),(0,1)):
        patches=[(STACK+4,4,0x1010000),(0x4f3218,4,OPTIONS),(OPTIONS+0x180,4,9),
            (OPTIONS+0x184,1,1),(OPTIONS+0x240,4,kind),(0x4f54f4,4,PLAYERS),
            (PLAYERS+0x44,4,PLAYER_ROWS),(PLAYER_ROWS+0xc0,1,local_team),
            (0x4f63b8,4,engine),(engine+0xc14,4,9),(0x56dfc4,4,registered),(engine+0xc,2,3),
            (engine+0xc18,4,2),(engine+0xc1c,4,0x1030002),(engine+0xc20,2,0),
            (engine+0xc2c,4,0x1040003),(engine+0xc30,2,1),(0x52d710,4,BANK),
            (0x4ecc20,4,PLAYERS),(PLAYERS+0x118,4,1),(PLAYERS+0x11c,4,POINT),
            (0x4ecc1c,4,globals_tag),(globals_tag+0x16c,4,0x87650000),
            (0x4f0414,4,POOL),(POOL+8,4,BODY),(BODY+0xc,4,ROWS),
            (ROWS+0x534,4,render_data),(render_data+0xc4,4,0x12340000)]
        patches.extend((engine+0x10+n*2,2,n if n<2 else -1) for n in range(9))
        patches.extend((BANK+0x26+n*2,2,-1) for n in range(9))
        patches.extend((POINT+n*4,4,bits(value)) for n,value in enumerate((1.25,-2.5,3.75)))
        add(0x24aa90,f'assault-goal-owner-{kind}-{local_team}',patches,{},boundary=0x41480)
    for engine_kind in (1,9):
        patches=[(STACK+4,4,0x1010000),(0x4f3218,4,OPTIONS),(OPTIONS+0x180,4,engine_kind),
            (OPTIONS+0x240,4,2),(0x4f54f4,4,PLAYERS),(PLAYERS+0x44,4,PLAYER_ROWS),(0x4f63b8,4,engine)]
        patches.extend((engine+0x10+n*2,2,-1) for n in range(9))
        add(0x24aa90,f'inactive-goals-{engine_kind}',patches,{})
    assert len(cases)==256
    data=json.dumps({'schema':'H2_NATIVE_OBJECTIVE_OWNERS_V1','original_sha256':spec['pin'],'supplied_service_results':[],
        'expected_case_count':256,'required_owner_boundaries':26,'kind':'objective-regions','cases':cases},separators=(',',':'))+'\n'
    assert len(data.encode())<=1024*1024;args.fixture.write_text(data)
    print(json.dumps({'cases':len(cases),'bytes':len(data.encode()),'sha256':hashlib.sha256(data.encode()).hexdigest()}))


if __name__=='__main__':main()
