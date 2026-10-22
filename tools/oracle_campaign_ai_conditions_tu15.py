#!/usr/bin/env python3
"""Execute unchanged original AI conditions and their real nested owners.

Inputs are declared raw component records. No instruction patch, callee stub or
dependency return is supplied. The unavailable22CB90 safety owner is an explicit
stop at its call boundary; those cases compare only the prefix and call ABI.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE, STOP = 0x2000000, 0x3000000
SCNR,CLOCK,AI = BASE+0x1000,BASE+0x2000,BASE+0x3000
SQUADS,SQUAD_ROWS,ORDERS,ORDER_ROWS = BASE+0x4000,BASE+0x5000,BASE+0x6000,BASE+0x7000
ACTORS,ACTOR_ROWS = BASE+0x8000,BASE+0x10000
PLAYERS,PLAYER_ROWS,BITS,OBJECTS,OBJECT_ROWS,UNIT = [BASE+p for p in (0xa000,0xb000,0xc000,0xd000,0xe000,0xf000)]
CONDITION,OUT,STACK = BASE+0x25000,BASE+0x26000,BASE+0x3f000
ENTRIES = (0x29afd0,0x20b500,0x20b570,0x2802f0,0x203b60)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--verify',action='store_true')
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:ap.error('Run through tools/run_test.py')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    tables=sorted({(p,w*n) for entry in (0x29afd0,0x29ac70) for p,w,n in _trace(code,entry,{entry}).tables})
    common=[]

    def put(p,v,n=4,patches=common):patches.append([p,(v&((1<<(8*n))-1)).to_bytes(n,'little').hex()])

    for p,v in ((0x4ecc20,SCNR),(0x51df54,CLOCK),(0x5020a8,AI),(0x52be7c,SQUADS),
                (0x52be80,ORDERS),(0x5020c8,ACTORS),(0x4f54f4,PLAYERS),(0x4ecbd0,OBJECTS)):
        put(p,v)
    for pool,rows,stride in ((SQUADS,SQUAD_ROWS,0x98),(ORDERS,ORDER_ROWS,0x38),
                             (ACTORS,ACTOR_ROWS,0x888),(PLAYERS,PLAYER_ROWS,0x21c),(OBJECTS,OBJECT_ROWS,12)):
        put(pool+0x44,rows);put(pool+0x24,stride);put(pool+0x38,1)
    put(PLAYERS+0x48,BITS);put(BITS,1);put(CLOCK+2,30,2)
    put(SCNR+0x1bc,BASE+0x27000);put(BASE+0x27000+0x24,0xffffffff)
    put(ORDER_ROWS+4,0xffffffff);put(ORDER_ROWS+8,0x80000000)
    put(SQUAD_ROWS+0x74,0xffff,2);put(SQUAD_ROWS+0x68,0x81000000)
    put(ACTORS+0x44,ACTOR_ROWS);put(AI+0x14,0x81000000)
    for i in range(2):
        put(ACTOR_ROWS+i*0x888+0x20,0x81000001 if i==0 else 0xffffffff)
        put(ACTOR_ROWS+i*0x888+0x2,0,2)
    put(OBJECT_ROWS+8,UNIT);put(UNIT+0x30,0x3f800000)
    put(CONDITION+2,0xffff,2);put(STACK,STOP);put(STACK+4,CONDITION)
    put(STACK+8,0);put(STACK+12,OUT);put(OUT,0x88776655)
    specs=[(0x29afd0,kind,variant,negate,False) for kind in range(-1,23)
           for variant in range(4) for negate in (0,1)]
    specs += [(0x29afd0,kind,variant,0,True) for kind in (0,6,11,19) for variant in (0,1)]
    specs += [(entry,0,variant,0,False) for entry in (0x20b500,0x20b570) for variant in range(4)]
    specs += [(0x29afd0,kind,4,0,False) for kind in (14,18)]
    specs += [(0x203b60,kind,variant,mode,False) for kind in (0x70,0x77,0x78,0xffff,0x10077)
              for variant in range(6) for mode in (0,1,2,3)]
    cases,covered=[],set()
    for entry,kind,variant,negate,recursive in specs:
        name=f'{entry:x}-{kind}-{variant}-{negate}-{int(recursive)}'
        request=entry==0x203b60
        request_variant=variant
        extra=variant==4 and not request
        if extra:variant=2
        if request:variant%=4
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_write(0x4e0000,bytes(0x60000));g.mem_map(BASE,0x40000);g.mem_map(STOP,4096)
        patches=[]
        def patch(p,v,n=4):put(p,v,n,patches)
        patch(CONDITION,kind,2);patch(CONDITION+2,0 if recursive else 0xffff,2)
        patch(CONDITION+4,0xffff if variant==3 else 0,2)
        patch(CONDITION+6,(1,2,0xffff,0x8000)[variant],2)
        patch(CONDITION+8,(0x3f000000,0x40000000,0x7fc12345,0)[variant])
        patch(CONDITION+0xc,0xffff if variant%2 else 0,2)
        patch(CONDITION+0x30,0xffff if variant%2 else 0,2)
        patch(CONDITION+0x34,negate,1)
        patch(ORDER_ROWS+0x23,int(variant!=2 or extra),1)
        patch(ORDER_ROWS+0x24,(1,2,0xffff,0x8000)[variant],2)
        patch(ORDER_ROWS+0x34,(2,1,0x8000,0xffff)[variant],2)
        patch(ORDER_ROWS+0x2c,(1,3,0xffff,0x8000)[variant],2)
        patch(ORDER_ROWS+0x30,(0x3f800000,0,0x7fc54321,0x80000000)[variant])
        patch(ORDER_ROWS+0x28,int(variant%2),1);patch(ORDER_ROWS+0x18,10)
        patch(SQUAD_ROWS+2,4,2);patch(SQUAD_ROWS+0x227,1,1)
        patch(AI+1,int(variant!=3),1);patch(AI+0x36c,0xffffffff if variant==3 else 5)
        patch(CLOCK+8,(11,12,60,0x80000000)[variant])
        patch(PLAYER_ROWS+0x2c,0x82000000 if variant%2 else 0xffffffff)
        if extra and kind==18:
            patch(PLAYERS+0x38,2);patch(BITS,2);patch(PLAYER_ROWS+0x21c+0x2c,0xffffffff)
        for i in range(2):
            patch(ACTOR_ROWS+i*0x888+0x227,int(variant!=1 or i==0),1)
            patch(ACTOR_ROWS+i*0x888+0x26c,0xffffffff if variant==2 else 0x81000000)
            patch(ACTOR_ROWS+i*0x888+0x2,8 if variant==1 else 0,2)
        if request:
            patch(0x4f3218,BASE+0x28000);patch(BASE+0x28008,negate)
            patch(0x5020c4,BASE+0x2a000)
            patch(SQUAD_ROWS+0x76,(-1,0,1,15,16,-128)[request_variant],1)
            patch(BASE+0x2a000+0xc4,0x2 if request_variant%2 else 0)
        for p,data in common+patches:g.mem_write(p,bytes.fromhex(data))
        initial=[0x11112222,0xffffffff if variant==2 else 0x83000000,0x33334444,
                 0x55556666,STACK,0x77778888,0x9999aaaa,0xbbbbcccc]
        if request:initial[0]=0x83000000;initial[7]=0x55aa0000|(kind&0xffff)
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        xmm=[[0x3f000000+i*4+lane for lane in range(4)] for i in range(8)]
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace,stores=[],[];boundary=[]
        def instruction(_g,pc,size,_user):
            assert bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            if pc in (0x22cb90,0x160a60):boundary.append(pc);g.emu_stop();return
            trace.append(pc);covered.add(pc)
        def store(_g,_access,address,size,value,_user):
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(size*8))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        try:g.emu_start(entry,STOP,count=10000)
        except Exception as error:raise RuntimeError(name+f' at {g.reg_read(R.UC_X86_REG_EIP):x}: '+str(error)) from error
        assert g.reg_read(R.UC_X86_REG_EIP) in (STOP,0x22cb90,0x160a60),name
        row=dict(name=name,entry=entry,patches=patches,gp=initial,xmm=xmm,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            trace=trace,stores=stores,output=bytes(g.mem_read(OUT,16)).hex())
        if boundary:row['stop_target']=boundary[0]
        cases.append(row)
    body_sites={p for entry in ENTRIES for p in _trace(code,entry,{entry}).code}
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        banks=[[BASE,0x40000],[0x4e0000,0x60000]],tables=[[p,image.read(p,n).hex()] for p,n in tables],
        initial_patches=common,cases=cases,complete_new_instructions=len(body_sites),
        covered_new_instructions=len(covered&body_sites),
        uncovered_new_instructions=[f'{p:08x}' for p in sorted(body_sites-covered)])
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify:assert args.fixture.read_bytes()==data,'Original AI condition fixture changed'
    else:
        args.fixture.parent.mkdir(parents=True,exist_ok=True)
        if args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve the existing reference; use a reviewed new fixture path'
        else:args.fixture.write_bytes(data)
    print(json.dumps(dict(cases=len(cases),complete_new_instructions=len(body_sites),covered_new_instructions=len(covered&body_sites),
        boundaries=sum('stop_target' in row for row in cases),fixture_sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),
        uncovered=fixture['uncovered_new_instructions'])))


if __name__=='__main__':main()
