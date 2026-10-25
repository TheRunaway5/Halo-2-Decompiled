#!/usr/bin/env python3
"""Offline unchanged Xbox references for whole native objective query owners.

Only caller inputs, register results and ordered stores enter the bounded
fixture. No original executable payload is retained or used by the game.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from oracle_objective_native_tu15 import REGS
from oracle_ctf_activation_tu15 import SOURCE
from oracle_smg_firing_1_1 import Xbe, _Code
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_EFLAGS

BANK,ENGINE,POOL,ROWS,BODY,STACK=0x4000000,0x4001000,0x4002000,0x4003000,0x4004000,0x400f000
END=0x10000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    spec=SOURCE['u5'];path=Path(spec['path'])
    assert hashlib.sha256(path.read_bytes()).hexdigest()==spec['pin']
    image=Xbe(path);code=_Code(image,'.text')
    cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x600000);cpu.mem_write(code.lo,code.bytes)
    cpu.mem_map(BANK,0x10000);writes=[]
    cpu.hook_add(UC_HOOK_MEM_WRITE,lambda uc,access,address,size,value,user:
        writes.append([uc.reg_read(UC_X86_REG_EIP),address,size,value&((1<<(size*8))-1)]))
    cases=[]
    def add(entry,label,patches,registers=None):
        cpu.mem_write(BANK,bytes(0x10000))
        gp=[0x12345678,0x24681357,0x13572468,0x10203040,STACK,0x87654321,0x76543210,0xabcdef01]
        for index,value in (registers or {}).items():gp[index]=value&0xffffffff
        patches=[(STACK,4,END),*patches]
        for at,width,value in patches:cpu.mem_write(at,(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        for reg,value in zip(REGS,gp):cpu.reg_write(reg,value)
        cpu.reg_write(UC_X86_REG_EFLAGS,0xad7);writes.clear()
        cpu.emu_start(entry,END,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==END,label
        cases.append({'entry':entry,'name':label,'inputs':patches,'gp':gp,'eflags':0xad7,
            'expected_gp':[cpu.reg_read(reg) for reg in REGS],
            'expected_flags':cpu.reg_read(UC_X86_REG_EFLAGS)&0x8d5,'writes':list(writes)})
    for relationship in (-2147483648,-1,0,1,2,3,2147483647):
        add(0x24b2f0,f'relationship-{relationship}',[(STACK+4,4,relationship)])
    for object_class,team,index,salt in itertools.product(
            (0,1,2,3,31,32,34,63,127,128,130,255),(-32768,-1,0,1,7,8,9,32767),(0,3),(0,0x87650000)):
        patches=[(STACK+4,4,salt|index),(0x4ecbd0,4,POOL),(POOL+0x44,4,ROWS),
            (ROWS+index*12+8,4,BODY),(BODY+0xaa,1,object_class),(BODY+0x17e,2,team),(0x52d710,4,BANK)]
        patches.extend((BANK+0x14c+n*4,4,0xf1230000+n) for n in range(9))
        add(0x24a180,f'carrier-{object_class}-{team}-{index}-{salt:x}',patches)
    for team,secondary,marker in itertools.product(range(9),(False,True),(-1,0,16,32767)):
        patches=[(0x52d710,4,BANK),(0x4f63b8,4,ENGINE)]
        for n in range(9):
            patches.extend([(BANK+0x14+n*2,2,marker if n==team and not secondary else 201+n*2),
                (BANK+0x26+n*2,2,marker if n==team and secondary else 202+n*2),
                (ENGINE+0x10+n*2,2,7-n)])
        add(0x24be50,f'marker-owner-{team}-{int(secondary)}-{marker}',patches,{2:marker})
    for marker in (-32768,-1,0,32767):
        patches=[(0x52d710,4,BANK),(0x4f63b8,4,ENGINE)]
        patches.extend((BANK+0x14+n*2,2,100+n) for n in range(18))
        add(0x24be50,f'marker-owner-absent-{marker}',patches,{2:marker})
    for team,mask,reverse in itertools.product((-2147483648,-2,-1,*range(10),32767),(0,1,3,0x100,0x1ff),(False,True)):
        patches=[(0x52d710,4,BANK),(0x4f63b8,4,ENGINE),(ENGINE+8,2,mask),(BANK+0x38,4,0x87654321)]
        for n in range(9):patches.extend([(ENGINE+0x10+n*2,2,8-n if reverse else n),(BANK+2+n*2,2,100+n)])
        add(0x24b4c0,f'home-{team}-{mask:x}-{int(reverse)}',patches,{0:team})
    def pickup_inputs(engine,classification,owner,team,flags,*,selected=True,phase=1,mode=1,round_phase=1,player=True,item=True):
        options,players,player_rows,tags,tag=0x400b000,0x4005000,0x4006000,0x4008000,0x400a000
        return [(STACK+4,4,0x87650000 if player else -1),(STACK+8,4,0xf1230003 if item else -1),
            (0x4f63b8,4,ENGINE),(ENGINE+0xc14,4,1),(0x56dfa4,4,0x400c000 if selected else 0),
            (ENGINE+0x6c,2,phase),(ENGINE+0xc04,4,round_phase),(0x4f3218,4,options),
            (options+0xc,1,mode),(options+0x180,4,engine),(0x4f54f4,4,players),
            (players+0x44,4,player_rows),(player_rows+0xc0,1,team),(0x4ecbd0,4,POOL),
            (POOL+0x44,4,ROWS),(ROWS+3*12+8,4,BODY),(BODY+0x17e,2,owner),
            (BODY,4,0x87650001),(0x4f0414,4,tags),(tags+16+8,4,tag),(tag+0x290,2,classification),
            (0x52d710,4,BANK),*(([(BANK+0x170+owner,1,flags)]) if owner>=0 else [])]
    for engine,classification,owner,team,flags in itertools.product((1,9),(0,1,2),(-1,0,1,7,8,9),(-1,0,1,7,8),(0,2)):
        add(0x24a810,f'pickup-{engine}-{classification}-{owner}-{team}-{flags}',
            pickup_inputs(engine,classification,owner,team,flags))
    for selected,phase,mode,round_phase,missing in itertools.product((False,True),(0,1),(1,4),(0,1),('none','player','item')):
        add(0x24a810,f'pickup-gate-{int(selected)}-{phase}-{mode}-{round_phase}-{missing}',
            pickup_inputs(1,1,1,0,0,selected=selected,phase=phase,mode=mode,round_phase=round_phase,
                player=missing!='player',item=missing!='item'))
    data=json.dumps({'schema':'H2_NATIVE_OBJECTIVE_OWNERS_V1','original_sha256':spec['pin'],
        'supplied_service_results':[],'cases':cases},separators=(',',':'))+'\n'
    assert len(data.encode())<=1024*1024
    args.fixture.write_text(data)
    print(json.dumps({'cases':len(cases),'bytes':len(data.encode()),'sha256':hashlib.sha256(data.encode()).hexdigest()}))


if __name__=='__main__':main()
