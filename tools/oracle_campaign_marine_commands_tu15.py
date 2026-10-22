#!/usr/bin/env python3
"""Execute original Marine behavior and missing command helpers from declared inputs.

Reuse only independently declared caller/pool/tag inputs from the existing
behavior reference. Never use its expected outputs. Execute original inherited
lookups, bit spans, signed selectors and RNG dependencies. Object update,
candidate iteration and unavailable physical callbacks retain actual CALL
boundaries, without supplied returns or captured gameplay state.
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
from oracle_campaign_ai_actor_behavior_tu15 import BASE,STACK,STOP,ACTOR,ACTOR_ID,PROP,OBJECT_ID,OBJECT,OBJECTS,TAGS,CHILD,PARENT,PARAM,BEHAV,OTHER,CLOCK,GLOBALS
from export_final_startup_storage_data_tu15 import regions

SH,ROW,TARGET,BITS,DEF,CATALOG,OUT,TYPE,STYLE,PH2=tuple(BASE+p for p in
    (0x15000,0x16000,0x17000,0x18000,0x19000,0x1a000,0x1b000,0x1c000,0x1d000,0x1e000))
EXTRA_CELLS=((0x4ecc20,4),(0x52d664,4),(0x52be90,4),(0x47d7a8,16),(0x50eef8,4))
NEW={0xcb060,0x116b60,0x1b7000,0x1d2100,0x1d2cc0,0x20b5f0,0x213280,0x213320,
    0x213720,0x213e40,0x216850,0x217870,0x271260,0x275970,0x2814a0}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--controls',type=Path,required=True);ap.add_argument('--generation',type=Path,required=True)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');ap.add_argument('--shared-helper',action='store_true')
    args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    control_bytes=args.controls.read_bytes();controls=json.loads(control_bytes)
    assert controls['original_xbe_sha256']==NEW_PIN and not controls['instruction_patches'] and not controls['supplied_dependency_returns']
    generation=json.loads(args.generation.read_bytes());assert generation['original_xbe_sha256']==NEW_PIN
    entries={int(p,16) for p in generation['entry_instructions']}|set(generation['reused_frame_entries']);assert NEW<=entries
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={p:_trace(code,p,entries) for p in entries}
    sites=set().union(*(set(b.code)for b in bodies.values()));points=set();covered=set();cases=[];prefixes=[]
    base=next(c for c in controls['cases'] if c['entry']==0x1b7240 and c['stage']==9)
    specs=[dict(entry=0x1b7000,control=n) for n,c in enumerate(controls['cases']) if c['entry']==0x1b7240]
    specs += [dict(entry=0x1d2100,index=index,state=state,bits=bits) for index,state,bits in itertools.product(
        (-32768,-1,0,1,2,3,4,5,6,7,8,9,32767),('absent','empty','negative','present'),(0,0x80000000,0x3f000000,0x7fc12345))]
    specs += [dict(entry=e,index=index,value=value)for e,index,value in itertools.product(
        (0x116b60,0x20b5f0,0x213e40,0x2814a0),(-1,0,1,2),(0,6,7))]
    specs += [dict(entry=0x1d2cc0,parent=parent,kind=kind)for parent,kind in itertools.product((False,True),(0,1,2))]
    specs += [dict(entry=0x271260,inherited=inherited,present=present,count=count,value=value)for inherited,present,count,value in itertools.product(
        (False,True),(False,True),(-1,0,1),(0,0x3f000000,0x3f800000,0x7fc12345))]
    specs += [dict(entry=e,present=present,enabled=enabled,count=count,start=start,mask=mask) for e,present,enabled,count,start,mask in itertools.product(
        (0x213280,0x213320),(False,True),(False,True),(-1,0,1,5,32,33),(0,31),(0,0x80000001))]
    specs += [dict(entry=0x213720,present=present,bits=bits,count=count,start=start) for present,bits,count,start in itertools.product(
        (False,True),(0,0x3e800000,0x3f000000,0x3f800000,0xbf800000,0x7fc12345),(5,32),(0,31))]
    specs += [dict(entry=0x217870,offset=offset,enabled=enabled,same=same,priority=priority)for offset,enabled,same,priority in itertools.product(
        (-1,0,2,3,9,10),(False,True),(False,True),(0,10,20))]
    specs += [dict(entry=e,index=index)for e,index in itertools.product((0xcb060,0x216850,0x275970),(0,1,7))]
    if args.shared_helper:
        assert 0x267750 in entries
        specs=[dict(entry=0x267750,owned=owned,actor=actor,kind=kind,a=a,b=b,c=c,d=d)for owned,actor,kind,a,b,c,d in itertools.product(
            (False,True),(False,True),(0,14,15),(0,1),(0,1),(0,1),(0,1))]
    banks=controls['native_component_banks']+[list(v)for v in EXTRA_CELLS]
    for ordinal,spec in enumerate(specs):
        control=controls['cases'][spec['control']] if spec['entry']==0x1b7000 else base
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x30000);g.mem_write(BASE,bytes([control['poison']])*0x30000);g.mem_map(STOP,4096)
        patches=list(control['patches'])
        for at,bits in patches:g.mem_write(at,bytes.fromhex(bits))
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        def fp(at,value):raw(at,struct.pack('<f',value))
        initial=list(control['gp']);xmm=[list(v)for v in control['xmm']]
        for at,value in ((0x4ecc20,SH),(SH+0x244,CATALOG),(0x52d664,SH),(SH+0x24,TARGET),
            (0x52be90,BITS),(TAGS+6*16+8,DEF),(DEF+4,CATALOG),(0x50eef8,PH2),(PH2+0x44,ROW)):
            put(at,value)
        for i in range(-1,32):put(BITS+i*4,0xa5a5a5a5)
        for i in range(4):put(0x47d7a8+i*4,TYPE);put(TYPE+6,spec.get('value',0),2)
        if spec['entry']==0x1b7000:
            put(OTHER+0x58+0x3c,0xffffffff if control['stage']==4 else 0xabcd0000);fp(BEHAV+0x30,0.25)
        elif spec['entry']==0x267750:
            initial[0]=PROP;put(PROP+8,0xabcd0000);put(PROP+0x14,0xabcd0001 if spec['owned'] else 0xffffffff)
            put(OTHER+0x1c,ACTOR_ID if spec['actor'] else 0xffffffff);put(ACTOR+4,spec['kind'],2)
            put(ACTOR+0x267,spec['a'],1);put(ACTOR+0x268,spec['b'],1)
            put(OTHER+0x25,spec['c'],1);put(OTHER+0x23,spec['d'],1)
        elif spec['entry']==0x1d2100:
            put(STACK+4,spec['index'],2);put(0x4ecc1c,0 if spec['state']=='absent' else GLOBALS)
            put(GLOBALS+0xc8,dict(absent=1,empty=0,negative=-1,present=1)[spec['state']])
            for i in range(12):put(GLOBALS+0x400+i*4,spec['bits'])
        elif spec['entry']==0x116b60:
            initial[0]=OBJECT_ID;put(OBJECT+0x342,0x200+spec['index']*4,2);put(OBJECT+0x200+spec['index']*4,spec['value'])
        elif spec['entry']==0x20b5f0:
            initial[0]=ROW;put(ROW+0x2a,spec['index'],2)
        elif spec['entry']==0x213e40:
            initial[1]=OBJECT_ID;put(OBJECT,0xabcd0003);put(CHILD+0x1f0,spec['value'],2)
        elif spec['entry']==0x2814a0:
            initial[1]=0xe3610001;initial[2]=ACTOR_ID
            put(ACTOR+4,max(0,spec['index']),2);put(ACTOR-0x888+4,spec['value'],2)
        elif spec['entry']==0x1d2cc0:
            initial[6]=OBJECT_ID;put(OBJECT+4,1<<26 if spec['parent'] else 0);put(OBJECT+0x14,0xabcd0006)
            put(OBJECTS+6*12+8,TYPE);put(TYPE+4,0);put(OBJECTS+(6 if spec['parent'] else 5)*12+3,spec['kind'],1)
        elif spec['entry']==0x271260:
            initial[0]=ACTOR_ID;put(CHILD+0x34,0 if spec['inherited'] else int(spec['present']));put(CHILD+0x38,PARAM)
            put(PARENT+0x34,int(spec['present']));put(PARENT+0x38,PARAM);put(CHILD+8,0xabcd0004 if spec['inherited'] else 0xffffffff)
            put(GLOBALS+0xc8,spec['count']);put(PARAM+8,spec['value']);fp(GLOBALS+0x400+0x158,0.5)
        elif spec['entry'] in (0x213280,0x213320,0x213720):
            tag=0xabcd0003 if spec['present'] else 0xffffffff
            if spec['entry']==0x213720:initial[0]=tag;xmm[0][0]=spec['bits']
            else:initial[1]=tag;put(STACK+4,OUT if spec['entry']==0x213280 else spec['mask'])
            put(CHILD+8,0,2);put(CHILD+0xa,1 if spec.get('enabled',True) else 0,1);put(CHILD+2,0,1)
            put(GLOBALS+0xc0,1);put(GLOBALS+0xc4,ROW);put(ROW+4,0xabcd0006);put(CATALOG+0xa,0x20,1)
            put(TARGET+6,spec['start'],2);put(TARGET+0xa,spec['count'],2)
            for i in range(-1,8):put(BITS+i*4,spec.get('mask',0xa5a5a5a5))
        elif spec['entry']==0x217870:
            initial[6]=ROW;initial[7]=TARGET;put(GLOBALS+0x400+0x64,0xabcd0006)
            for at,value in ((ROW+0x14,100),(TARGET+0x14,100+spec['offset']),(ROW+4,7),(TARGET+4,7 if spec['same'] else 8),
                (ROW+0x44,0xffffffff),(TARGET+0x44,0xffffffff)):put(at,value)
            put(ROW+0x1c,10,2);put(ROW+0x22,10,2);put(TARGET+0x22,spec['priority'],2);put(TARGET+2,0,2)
            put(ROW+0x40,int(spec['enabled']),1);put(CATALOG+0x1a,spec['priority'],2);put(CLOCK+2,30,2)
        elif spec['entry']==0x275970:
            initial[0]=0xabcd0000|spec['index'];put(ROW+spec['index']*0x50+0x12,spec['index'],2)
        elif spec['entry']==0xcb060:
            initial[0]=ROW;initial[2]=TARGET;put(STACK+4,OUT+spec['index']*4)
        else:
            for i in range(1,8):put(STACK+i*4,OUT+i*8)
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];pending=[];prefix_end=None;dependency=None
        def flush():
            if not pending:return
            pc=pending[0][0];ins=code.at(pc);width=ins.operands[0].size if ins.operands and ins.operands[0].type==3 else 0
            if width==16 and sum(len(v[2])for v in pending)==16:
                assert all(pending[i-1][1]+len(pending[i-1][2])==pending[i][1]for i in range(1,len(pending)))
                start=pending[0][1];stores.append([pc,start,b''.join(v[2]for v in pending).hex(),bytes(g.mem_read(start,16)).hex()])
            else:
                for pc,at,before in pending:stores.append([pc,at,before.hex(),bytes(g.mem_read(at,len(before))).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            nonlocal prefix_end,dependency
            flush();assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),spec)
            ins=code.at(pc)
            if ins.mnemonic=='call' and (pc in (0x1b716a,0x216878,0x2759a0) or ins.operands[0].type!=X86_OP_IMM or ins.operands[0].imm not in entries):
                prefix_end=pc;dependency=ins.operands[0].imm if ins.operands[0].type==X86_OP_IMM else 'indirect';g.emu_stop();return
            if not trace or trace[-1]!=pc:trace.append(pc)
            covered.add(pc)
        def store(_g,_kind,at,size,_value,_u):
            assert any(p<=at<at+size<=p+n for p,n in banks),(hex(at),spec)
            pending.append((g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size))))
        def read(_g,_kind,at,size,_value,_u):
            sec=next((s for s in image.sections if s.va<=at<at+size<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':points.update(range(at,at+size))
            elif sec and sec.name=='.data':assert any(p<=at<at+size<=p+n for p,n in banks),('Undeclared field',hex(at),spec)
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        try:g.emu_start(spec['entry'],STOP,count=50000);flush()
        except Exception as error:raise RuntimeError(f'Original case{ordinal} {spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}') from error
        assert g.reg_read(R.UC_X86_REG_EIP)==(prefix_end or STOP),(spec,hex(g.reg_read(R.UC_X86_REG_EIP)))
        if prefix_end:prefixes.append([spec['entry'],prefix_end,dependency])
        cases.append(dict(spec,poison=control['poison'],prefix_end=prefix_end,dependency=dependency,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg)for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')))for i in range(8)],
            x87_depth=sum(((g.reg_read(R.UC_X86_REG_FPTAG)>>(i*2))&3)!=3 for i in range(8)),
            outputs=[[at,bytes(g.mem_read(at,size)).hex()]for at,size in ((STACK-0x100,0x120),(ROW,0x80),(TARGET,0x80),(BITS-4,0x84),(OUT,0x80),(ACTOR+0x7c8,4))]))
    points.update(range(0x1d216c,0x1d2190));operands=[[p,image.read(p,n).hex()]for p,n in regions(points)]
    scope=__doc__ if not args.shared_helper else ('Execute whole original267750, including every property/actor guard.\n'+__doc__)
    checked_entries={0x267750} if args.shared_helper else NEW
    fixture=dict(scope=scope,original_xbe_sha256=NEW_PIN,component_controls_sha256=hashlib.sha256(control_bytes).hexdigest(),
        instruction_patches=[],supplied_dependency_returns=[],native_component_banks=banks,immutable_operands=operands,cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original Marine/command reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=scope,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases)-len(prefixes),dependency_prefixes=len(prefixes),
        prefix_boundaries=sorted({tuple(p)for p in prefixes},key=str),entry_coverage={f'{p:08x}':dict(observed=len(set(bodies[p].code)&covered),
            static=len(bodies[p].code),unexecuted=sorted(set(bodies[p].code)-covered))for p in sorted(checked_entries)},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        component_controls_sha256=fixture['component_controls_sha256'],full_mission_commands_qualified=False,full_actor_behavior_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k!='entry_coverage'}))


if __name__=='__main__':main()
