#!/usr/bin/env python3
"""Compare original actor behavior and scalar owners from declared component inputs.

The real tag inheritance, signed selectors, guard/action queries, timing and
random owners execute. Calls outside the generated/retained Frame closure stop
before CALL, with no supplied dependency return. The275FE0 object-update handoff
also remains a declared prefix boundary. Input actor/tag/pool records
are independent component controls, never captured campaign outcomes. Engine
readonly operands are authenticated; mutable descriptor fields stay inputs.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from capstone.x86 import X86_OP_IMM
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

BASE,STACK,STOP=0x2000000,0x202f000,0x3000000
AH,PH,OH,TH,ACTORS,PROPS,OBJECTS,OBJECT,TAGS,CHILD,PARENT,PARAM,BEHAV,OTHERH,OTHER,CLOCK,GLOBALS,OPTIONS,STATE,RNG=(BASE+p for p in
    (0x100,0x200,0x300,0x400,0x1000,0x4000,0x5000,0x6000,0x8000,0x9000,0xa000,0xb000,
     0xc000,0xd000,0xe000,0x10000,0x11000,0x12000,0x13000,0x14000))
ACTOR,PROP=ACTORS+2*0x888,PROPS+0x3c
ACTOR_ID,PROP_ID,OBJECT_ID=0xe3610002,0xf2810001,0xf2700005
CELLS=((0x5020c8,4),(0x50eef0,4),(0x4ecbd0,4),(0x4f0414,4),(0x50eef4,4),
    (0x51df54,4),(0x4ecc1c,4),(0x4f3218,4),(0x5020a8,4),(0x4f3cd8,4),(0x5020bd,1),(0x47c1a4,2))
AUTHORED_INPUTS=((0x473990,20),(0x47d636,2),(0x48d4f4,4))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--generation',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True);ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    generation=json.loads(args.generation.read_text());assert generation['original_xbe_sha256']==NEW_PIN
    entries={int(p,16) for p in generation['entry_instructions']}|set(generation['reused_frame_entries'])
    code=_Code(image,'.text');bodies={p:_trace(code,p,entries) for p in entries}
    sites=set().union(*(set(b.code) for b in bodies.values()));points=set();covered=set()
    specs=[dict(entry=0x1ea360,selector=selector,globals=state,bits=bits,inherited=inherited,poison=0xa5)
        for selector,state,bits,inherited in itertools.product((-32768,-1,0,1,2,5,6,7,8,32767),
            ('absent','empty','negative','present'),(0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0x7fc12345),(False,True))]
    specs += [dict(entry=0x1ed690,selector=selector,count=count,match=match,inherited=inherited,poison=0xa5)
        for selector,count,match,inherited in itertools.product((-1,0,2,7),(-1,0,1,3),(False,True),(False,True))]
    # Each guard is an actual caller/pool/tag field. The deeper cases let the
    # original action/timing/RNG owners run until return or a real service call.
    specs += [dict(entry=0x1b7240,stage=stage,mode=mode,override=override,last=last,seed=seed,poison=0xa5)
        for stage,mode,override,last,seed in itertools.product(range(11),(1,2),(0,1),(-1,0,40),(0x12345678,0xfedcba98))]
    cases=[];prefixes=[]
    for ordinal,spec in enumerate(specs):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x30000);g.mem_write(BASE,bytes([spec['poison']])*0x30000);g.mem_map(STOP,4096)
        patches=[]
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        def fp(at,value):raw(at,struct.pack('<f',value))
        for at,size in AUTHORED_INPUTS:raw(at,image.read(at,size))
        for at,value in ((STACK,STOP),(STACK+4,ACTOR_ID),(STACK+8,ACTOR_ID),
            (0x5020c8,AH),(AH+0x44,ACTORS),(0x50eef0,PH),(PH+0x44,PROPS),(0x4ecbd0,OH),
            (OH+0x44,OBJECTS),(OBJECTS+5*12+8,OBJECT),(0x4f0414,TAGS),(TAGS+3*16+8,CHILD),
            (TAGS+4*16+8,PARENT),(0x50eef4,OTHERH),(OTHERH+0x44,OTHER),(0x51df54,CLOCK),
            (0x4ecc1c,GLOBALS),(0x4f3218,OPTIONS),(0x5020a8,STATE),(0x4f3cd8,RNG),
            (ACTOR+0x18,OBJECT_ID),(ACTOR+0x54,0xabcd0003),(ACTOR+0x338,PROP_ID),
            (CHILD+8,0xabcd0004 if spec.get('inherited',False) else 0xffffffff),(PARENT+8,0xffffffff),
            (CHILD+0x5c,0 if spec.get('inherited',False) else 1),(CHILD+0x60,PARAM),
            (PARENT+0x5c,1),(PARENT+0x60,PARAM),(GLOBALS+0xc8,1),(GLOBALS+0xcc,GLOBALS+0x400),
            (OPTIONS+8,spec.get('mode',1)),(CLOCK+8,42),(RNG,spec.get('seed',0x12345678)),
            (PROP+8,0xabcd0000),(PROP+0x14,0xffffffff),(OTHER+0x58+0x3c,0xffffffff),
            (OBJECT+0x13c,0xffffffff),(OBJECT+0x400+0xa4,0),(OBJECT+0x800+0x7c,0),
            (GLOBALS+0xf8,0),(ACTOR+0x7c,0xffffffff),(ACTOR+0x7c8,spec.get('last',-1))):put(at,value)
        put(OBJECT+0x33e,0x400,2);put(OBJECT+0x12a,0x800,2);put(OBJECT+0x23c,2,1)
        put(PROP+0x18,0,2);put(PROP+0x24,1,2);put(ACTOR+0x225,0,1);put(STATE+0x340,1,1)
        put(0x5020bd,spec.get('override',0),1);put(0x47c1a4,0xffff,2)
        put(OPTIONS+0x132,1,2);fp(CLOCK+4,1/30)
        for tag in (CHILD,PARENT):
            put(tag+0xdc,0 if tag==CHILD and spec.get('inherited',False) else 1);put(tag+0xe0,BEHAV)
        put(BEHAV+4,2,2);put(BEHAV+6,1,2);fp(BEHAV+0x24,0.25);fp(BEHAV+0x28,1.0)
        initial=[ACTOR_ID,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        if spec['entry']==0x1ea360:
            put(PARAM+0x1a,spec['selector'],2)
            put(0x4ecc1c,0 if spec['globals']=='absent' else GLOBALS)
            put(GLOBALS+0xc8,dict(absent=1,empty=0,negative=-1,present=1)[spec['globals']])
            for i in range(6):put(GLOBALS+0x400+0x98+i*4,spec['bits'])
        elif spec['entry']==0x1ed690:
            initial[1]=0xabcd0003;put(STACK+4,spec['selector'],2)
            for tag in (CHILD,PARENT):
                put(tag+0xdc,0 if tag==CHILD and spec['inherited'] else spec['count'])
            for i in range(max(0,spec['count'])):
                put(BEHAV+i*0x3c+4,spec['selector'] if spec['match'] and i==spec['count']-1 else 0x2345,2)
        else:
            stage=spec['stage']
            if stage==0:put(ACTOR+0x338,0xffffffff)
            elif stage==1:put(ACTOR+0x225,1,1)
            elif stage==2:put(PROP+0x24,0,2)
            elif stage==3:put(PROP+0x24,3,2)
            elif stage==4:put(OTHER+0x58+0x3c,0)
            elif stage==5:put(OBJECT+0x800+0x7c,0x80000c4)
            elif stage==6:put(STATE+0x340,0,1)
            elif stage==7:put(OBJECT+0x23c,0xff,1)
            elif stage==8:put(BEHAV+6,0xffff,2)
            elif stage==10:put(ACTOR+0x7c,0xabcd0000)
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];pending=[];prefix_end=None;dependency=None
        def flush():
            for pc,at,prior in pending:stores.append([pc,at,prior,bytes(g.mem_read(at,len(prior)//2)).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            nonlocal prefix_end,dependency
            flush();ins=code.at(pc)
            assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),spec)
            if ins.mnemonic=='call' and (pc==0x1b73a9 or ins.operands[0].type!=X86_OP_IMM or ins.operands[0].imm not in entries):
                prefix_end=pc;dependency=ins.operands[0].imm if ins.operands[0].type==X86_OP_IMM else 'indirect'
                g.emu_stop();return
            if not trace or trace[-1]!=pc:trace.append(pc)
            covered.add(pc)
        def store(_g,_kind,at,size,_value,_u):
            assert BASE<=at<at+size<=BASE+0x30000,(hex(at),spec)
            pending.append((g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size)).hex()))
        def read(_g,_kind,at,size,_value,_u):
            sec=next((s for s in image.sections if s.va<=at<at+size<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':points.update(range(at,at+size))
            elif sec and sec.name=='.data':
                assert any(p<=at<at+size<=p+n for p,n in CELLS+AUTHORED_INPUTS),('Undeclared component field',hex(at),size,spec)
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        try:g.emu_start(spec['entry'],STOP,count=50000);flush()
        except Exception as error:raise RuntimeError(f'Original case{ordinal} {spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}') from error
        assert g.reg_read(R.UC_X86_REG_EIP)==(prefix_end or STOP),(spec,hex(g.reg_read(R.UC_X86_REG_EIP)))
        if prefix_end:prefixes.append([spec['entry'],prefix_end,dependency])
        depth=sum(((g.reg_read(R.UC_X86_REG_FPTAG)>>(i*2))&3)!=3 for i in range(8))
        cases.append(dict(spec,prefix_end=prefix_end,dependency=dependency,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            x87_depth=depth,outputs=[[STACK-0x100,bytes(g.mem_read(STACK-0x100,0x10c)).hex()],
                [ACTOR+0x7c8,bytes(g.mem_read(ACTOR+0x7c8,4)).hex()],[RNG,bytes(g.mem_read(RNG,4)).hex()]]))
    operands=[[p,image.read(p,n).hex()] for p,n in regions(points)]
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x30000],*CELLS,*AUTHORED_INPUTS],immutable_operands=operands,cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original actor-behavior reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases)-len(prefixes),
        dependency_prefixes=len(prefixes),prefix_boundaries=sorted({tuple(p) for p in prefixes},key=str),
        entry_coverage={f'{p:08x}':dict(observed=len(set(bodies[p].code)&covered),static=len(bodies[p].code),
            unexecuted=sorted(set(bodies[p].code)-covered)) for p in sorted(entries)},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        whole_engine_behavior_qualified=False,full_mission_qualified=False,runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='entry_coverage'}))


if __name__=='__main__':main()
