#!/usr/bin/env python3
"""Compare original object callback bodies without supplied dependencies.

Use the real three/four-argument callback layouts from269D70/26A380. Original
leaf arithmetic/filtering executes completely; a call outside the new21-body
closure stops before CALL, without a callback or service return substitution.
Pools, candidate records, actor settings and prior bytes are declared component
inputs. All code/data evidence comes from the pinned original executable.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from capstone.x86 import X86_OP_IMM
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,STACK,STOP=0x2000000,0x202f000,0x3000000
ACTOR,ROWS,OTHER,AH,ACTORS,PH,PROPS,OH,OBJECTS,CLOCK,OUT=(BASE+p for p in
    (0x1888,0x4000,0x6000,0x7000,0x1000,0x7100,0x8000,0x7200,0x9000,0xa000,0xb000))
GLOBALS=(0x5020c8,0x50eef4,0x4ecbd0,0x51df54)
FIRST=(0x268140,0x268490,0x268510,0x268810,0x268880,0x268a70,0x268ae0,0x268bb0,0x268fa0)
SECOND=(0x269940,0x269a60,0x269b60,0x269bf0,0x269ca0)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--generation',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    im=Xbe(NEW_DEFAULT);assert hashlib.sha256(im.data).hexdigest()==NEW_PIN
    generation=json.loads(args.generation.read_text());assert generation['original_xbe_sha256']==NEW_PIN
    entries={int(p,16) for p in generation['entry_instructions']};code=_Code(im,'.text')
    bodies={e:_trace(code,e,entries) for e in entries};allowed=set().union(*(set(b.code) for b in bodies.values()))
    specs=[dict(entry=e,count=count,enabled=enabled,profile=profile,present=True,kind=profile,poison=0xa5)
        for e,count,enabled,profile in itertools.product(FIRST,(-1,0,2),(0,1),range(3))]
    specs += [dict(entry=e,count=1,enabled=enabled,profile=profile,present=present,kind=kind,poison=0xa5)
        for e,kind,enabled,present,profile in itertools.product(SECOND,(-1,0,1,2,3,4,5),(0,1),(False,True),range(3))]
    specs += [dict(entry=0x1bec60,count=count,found=found,pointer=pointer,enabled=0,profile=0,present=True,kind=0,poison=0xa5)
        for count,found,pointer in itertools.product((0,1,3),(False,True),(False,True))]
    specs += [dict(entry=0x1b92d0,attached=attached,actor_prop=prop,pointer=pointer,second_pointer=second,
        count=0,enabled=0,profile=0,present=True,kind=0,poison=0xa5)
        for attached,prop,pointer,second in itertools.product((False,True),repeat=4)]
    cases=[];covered=set();prefixes=[]
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in im.sections:g.mem_write(sec.va,im.section_bytes(sec))
        g.mem_map(BASE,0x30000);g.mem_write(BASE,bytes([spec['poison']])*0x30000);g.mem_map(STOP,4096)
        patches=[]
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        def fp(at,value):raw(at,struct.pack('<f',value))
        raw(ACTOR,bytes(0x888));raw(OTHER,bytes(0x800));raw(ROWS,bytes([spec['poison']])*240)
        for at,value in ((STACK,STOP),(STACK+4,0xe3610001),(STACK+8,ACTOR),
                (STACK+12,spec['count'] if spec['entry'] in FIRST else ROWS if spec['present'] else 0),(STACK+16,ROWS),
                (0x5020c8,AH),(AH+0x44,ACTORS),(0x50eef4,PH),(PH+0x44,PROPS),
                (0x4ecbd0,OH),(OH+0x44,OBJECTS),(OBJECTS+8,OTHER),(0x51df54,CLOCK),(CLOCK+8,42),
                (ACTOR+0x18,0xffffffff),(ACTOR+0x30,0xffffffff),(ACTOR+0x26c,0xffffffff)):
            put(at,value)
        fp(CLOCK+4,30.0)
        for offset in (0x54,0x55,0x5b,0x618,0x644,0x668):put(ACTOR+offset,spec['enabled'],1)
        for offset,value in ((8,2.0),(0x18,5.0),(0x61c,10.0),(0x66c,1.0),(0x670,-2.0),(0x674,0.5),(0x680,0.25)):
            fp(ACTOR+offset,value)
        put(ACTOR+0x684,5);put(ACTOR+0x688,2)
        for lane in range(2):
            at=ROWS+lane*0x78;put(at,OTHER);put(at+4,0xabcd0000);put(at+8,spec['kind'],2)
            for offset in range(0xc,0x78,4):fp(at+offset,(-1.0,0.5,20.0)[spec['profile']]+lane)
            for offset in (0x10,0x11,0x14,0x15,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x58,0x59,0x5a,0x5b):
                put(at+offset,int(spec['profile']!=0),1)
            fp(at+0x54,0.25)
        initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        if spec['entry']==0x1bec60:
            initial[1]=0xabcd0001;put(STACK+4,3,2);put(STACK+8,OUT,2)
            put(PROPS+0xc4+0x53,spec['count'],1)
            for i in range(spec['count']):put(PROPS+0xc4+0x3e+i*4,OUT if spec['found'] and i==spec['count']-1 else OUT+4)
        elif spec['entry']==0x1b92d0:
            initial[0]=0xabcd0000;initial[1]=0xe3610001
            initial[2]=OUT if spec['pointer'] else 0;initial[6]=OUT+4 if spec['second_pointer'] else 0
            put(OTHER+0x1fc,0 if spec['attached'] else 0xffff,2)
            put(ACTOR+0x26c,0 if spec['actor_prop'] else 0xffffffff)
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[];prefix_end=None;dependency=None
        def instruction(_g,pc,size,_u):
            nonlocal prefix_end,dependency
            ins=code.at(pc)
            assert pc in allowed and bytes(g.mem_read(pc,size))==im.read(pc,size),hex(pc)
            if ins.mnemonic=='call' and ins.operands[0].type==X86_OP_IMM and ins.operands[0].imm not in entries:
                prefix_end=pc;dependency=ins.operands[0].imm;g.emu_stop();return
            if not trace or trace[-1]!=pc:trace.append(pc)
            covered.add(pc)
        def store(_g,_kind,address,size,value,_u):
            assert BASE<=address<address+size<=BASE+0x30000,hex(address)
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        try:g.emu_start(spec['entry'],STOP,count=20000)
        except Exception as error:raise RuntimeError(f'{spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}: {error}') from error
        assert g.reg_read(R.UC_X86_REG_EIP)==(prefix_end or STOP),(spec,hex(g.reg_read(R.UC_X86_REG_EIP)))
        if prefix_end:prefixes.append([spec['entry'],prefix_end,dependency])
        cases.append(dict(spec,prefix_end=prefix_end,dependency=dependency,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            outputs=[[ACTOR+0x610,bytes(g.mem_read(ACTOR+0x610,0x80)).hex()],
                [ROWS,bytes(g.mem_read(ROWS,240)).hex()],[OUT,bytes(g.mem_read(OUT,8)).hex()]]))
    operands=[[p,im.read(p,n).hex()] for p,n in generation['immutable_ranges']]
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x30000],*[[p,4] for p in GLOBALS]],immutable_operands=operands,cases=cases,
        context_storage=[[0x52d874,2],[0x52d878,4]])
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original callback-body reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases)-len(prefixes),
        dependency_prefixes=len(prefixes),prefix_boundaries=sorted({tuple(p) for p in prefixes}),
        entry_coverage={f'{p:08x}':dict(observed=len(covered&set(b.code)),static=len(b.code)) for p,b in sorted(bodies.items())},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        runtime_reference_required=False,full_callbacks_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
