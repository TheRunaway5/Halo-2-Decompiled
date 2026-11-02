#!/usr/bin/env python3
"""Execute unchanged TU1.5 17DCE0, including its recursive calls and read stops.

Only caller-owned pool/cursor/stack inputs are supplied. No instructions or
callees are patched. Expected data covers full GP/XMM/flags, ordered stores,
dead recursive stack bytes and cursor aliases. Not a rendering/audio oracle.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_XMM0
from oracle_weapon_effect_dispatch_tu15 import GP,STACK,STOP,NONE,pack
from weapon_reference_versions import images

POOL,ROWS,CURSOR,BAD=0x1000000,0x1004000,0x1002000,0x4000000


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args();Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _,image,pin=next(x for x in images() if x[0]=='TU1.5')
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
    for section in image.sections:u.mem_write(section.va,image.section_bytes(section))
    u.mem_map(0x1000000,0x500000);u.mem_map(0x2000000,0x20000);u.mem_map(STOP,4096)
    instructions={};stores=[];calls=[];fault=[];trace=bytearray()
    def code(vm,at,size,_):
        raw=bytes(vm.mem_read(at,size));assert raw==image.read(at,size),(hex(at),'changed original')
        assert 0x17dce0<=at<=0x17dd44,hex(at)
        instructions[f'{at:08x}']=raw.hex()
        trace.append(at-0x17dce0)
        if at==0x17dd3c:calls.append(dict(gp=[vm.reg_read(r) for r in GP],stack=bytes(vm.mem_read(STACK-512,544)).hex()))
    def write(vm,access,at,size,value,_):stores.append([vm.reg_read(UC_X86_REG_EIP),at,size,value&((1<<(size*8))-1)])
    def invalid(vm,access,at,size,value,_):
        fault.append(dict(instruction=vm.reg_read(UC_X86_REG_EIP),address=at,size=size,access=access));return False
    code_hook=u.hook_add(UC_HOOK_CODE,code);write_hook=u.hook_add(UC_HOOK_MEM_WRITE,write);u.hook_add(UC_HOOK_MEM_INVALID,invalid)
    rng=random.Random(0x17dce0);cases=[];recipes=[]
    modes=[0,1,3,2,0xffffffff,0xffff,0x8000,0x7fff,0xdead0001,0xbeef0003,0x80010000,0x80010002,0x1234fffe,4,0x80000001,0xffff0003]
    chains=[[],[0],[0xffff],[0x8000],[0x8001,0],[0,0x8001],[0xffff,0x8000,0x7fff],[0x8000,0x8001,0xffff,0x7fff]]
    for mode in modes:
        for nodes in chains:recipes.append(dict(mode=mode,nodes=nodes,kind='chain'))
    for mode in modes[:12]:recipes.append(dict(mode=mode,kind='node-cursor-alias'))
    for mode in [0,1,3,0xffff]:recipes.append(dict(mode=mode,kind='stack-cursor-alias'))
    for node in [0,0x8000,0xffff,0x8001]:recipes.append(dict(mode=0x80010003,node=node,kind='mode-cursor-alias'))
    for n in range(8):recipes.append(dict(mode=modes[n],kind=['cursor-read-stop','pool-read-stop','rows-read-stop'][n%3],index=n))
    for recipe in recipes:
        kind=recipe['kind'];mode=recipe['mode'];memory=[[0x4f7208,POOL,4],[POOL+0x44,ROWS,4]];cursor=CURSOR
        if kind=='chain':
            nodes=recipe['nodes'];head=NONE if not nodes else 0x80010000
            for i,node in enumerate(nodes):memory.extend([[ROWS+i*60+2,node,2],[ROWS+i*60+4,NONE if i+1==len(nodes) else (rng.randrange(1,65536)<<16)|(i+1),4]])
            memory.append([cursor,head,4])
        elif kind=='node-cursor-alias':
            cursor=ROWS+2;memory.extend([[ROWS+4,0xffff0001,4],[ROWS+32769*60+2,0,2],[ROWS+32769*60+4,NONE,4],[cursor,0x80010000,4]])
        elif kind=='stack-cursor-alias':
            cursor=STACK+8;memory.extend([[ROWS+8*60+2,0x8000 if mode==1 else 0,2],[ROWS+8*60+4,NONE,4]])
        elif kind=='mode-cursor-alias':
            cursor=STACK+12;memory.extend([[ROWS+3*60+2,recipe['node'],2],[ROWS+3*60+4,NONE,4]])
        else:
            index=recipe['index'];head=(0x8001<<16)|index;memory.append([cursor,head,4])
            if kind=='cursor-read-stop':cursor=BAD
            elif kind=='pool-read-stop':memory.append([0x4f7208,BAD,4])
            else:memory.append([POOL+0x44,BAD,4])
        gp=[rng.getrandbits(32) for _ in GP];gp[4]=STACK;xmm=[rng.randbytes(16).hex() for _ in range(8)]
        flags=0x202|(rng.getrandbits(12)&0xcd5);arguments=[rng.getrandbits(32),cursor,mode]
        u.mem_write(STACK-512,rng.randbytes(544));u.mem_write(STACK,pack([STOP,*arguments]))
        for at,value,width in memory:u.mem_write(at,value.to_bytes(width,'little'))
        initial=bytes(u.mem_read(STACK-512,544)).hex()
        for r,value in zip(GP,gp):u.reg_write(r,value)
        for i,raw in enumerate(xmm):u.reg_write(UC_X86_REG_XMM0+i,int.from_bytes(bytes.fromhex(raw),'little'))
        u.reg_write(UC_X86_REG_EFLAGS,flags);stores.clear();calls.clear();fault.clear();trace.clear()
        try:u.emu_start(0x17dce0,STOP,count=20000)
        except UcError:
            if not fault:raise
        assert bool(fault)==kind.endswith('read-stop'),recipe
        if not fault:assert u.reg_read(UC_X86_REG_EIP)==STOP,recipe
        # IMUL leaves PF/AF/ZF/SF architecturally undefined. Only its CF/OF
        # plus untouched control flags are owed at the following read stop.
        mask=0xffffff2b if fault and fault[0]['instruction']==0x17dcfe else 0xffffffff
        cases.append(dict(recipe=recipe,gp=gp,xmm=xmm,eflags=flags,memory=memory,arguments=arguments,initial_stack=initial,
            stores=list(stores),recursive_calls=list(calls),fault=list(fault),eflags_mask=mask,trace_offsets=trace.hex(),
            expected_gp=[u.reg_read(r) for r in GP],expected_eflags=u.reg_read(UC_X86_REG_EFLAGS),
            expected_xmm=[u.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little').hex() for i in range(8)],
            expected_stack=bytes(u.mem_read(STACK-512,544)).hex()))
    assert len(cases)==156
    # Exhaust the original16-bit node word in the three actual caller modes.
    # The complete unchanged body has already been checked at every executed
    # PC above. Remove observation callbacks, not instructions or callees.
    u.hook_del(code_hook);u.hook_del(write_hook);node_modes={};counts={}
    for at,value in [(0x4f7208,POOL),(POOL+0x44,ROWS),(ROWS+4,NONE)]:u.mem_write(at,pack([value]))
    for mode in [0,1,3]:
        accepted=bytearray();u.reg_write(UC_X86_REG_EFLAGS,0x202)
        for node in range(65536):
            u.mem_write(ROWS+2,struct.pack('<H',node));u.mem_write(CURSOR,pack([0x80010000]));u.mem_write(STACK,pack([STOP,0x12345678,CURSOR,mode]));u.reg_write(GP[4],STACK)
            fault.clear();u.emu_start(0x17dce0,STOP,count=1000)
            assert not fault and u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(GP[4])==STACK+16
            result=u.reg_read(GP[0]);assert result in [0,ROWS]
            assert bytes(u.mem_read(CURSOR,4))==pack([NONE]);accepted.append(int(result==ROWS))
        node_modes[str(mode)]=accepted.hex();counts[str(mode)]=sum(accepted)
    data=(json.dumps(dict(schema='H2_TU15_WEAPON_EFFECT_ITERATOR_REFERENCE_V1',original_sha256=pin,scope=__doc__,rows=cases,node_modes=node_modes),separators=(',',':'))+'\n').encode()
    assert len(data)<=1024*1024,len(data);args.fixture.write_bytes(data)
    report=dict(passed=True,original_sha256=pin,cases=len(cases),read_stops=sum(bool(c['fault']) for c in cases),
        recursive_calls=sum(len(c['recursive_calls']) for c in cases),aliased_cursor_cases=sum('alias' in c['recipe']['kind'] for c in cases),
        instruction_map=instructions,fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),patched_instructions=0,substituted_callees=0,
        exhaustive_node_mode_cases=196608,accepted_node_counts=counts,scratch_owner=str(Path(os.environ['HALO2_TEST_SCRATCH']).resolve()))
    args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='instruction_map'}))


if __name__=='__main__':main()
