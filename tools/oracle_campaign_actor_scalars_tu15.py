#!/usr/bin/env python3
"""Execute original actor/object scalar and action guards without supplied outcomes.

Declare pool/tag/clock fields, then execute105F70's actual FF7B0/difficulty
dependencies, byte-counter aliasing and action guards. Physical/action handoffs
stop before real CALL instructions. Whole32ECD2/32ECDD check original status
capture and FNCLEX across exception, condition, TOP and busy inputs. Expected
states come only from unchanged original instructions.
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
import oracle_campaign_ai_actor_behavior_tu15 as P
from export_final_startup_storage_data_tu15 import regions

HEADER,RECORDS,MOD,BLOCK,FACTORS=tuple(P.BASE+p for p in (0x15000,0x16000,0x17000,0x18000,0x1a000))
CHECKED={0x105f70,0xff7b0,0xcf1b0,0x1ec5c0,0x205eb0,0x205fa0,0x25f9b0,0x275480,0x32ecd2,0x32ecdd}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--controls',type=Path,required=True);ap.add_argument('--generation',type=Path,required=True)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    control_bytes=args.controls.read_bytes();controls=json.loads(control_bytes)
    assert controls['original_xbe_sha256']==NEW_PIN and not controls['instruction_patches'] and not controls['supplied_dependency_returns']
    base=next(c for c in controls['cases']if c['entry']==0x1b7240 and c['stage']==9)
    generation=json.loads(args.generation.read_bytes());assert generation['original_xbe_sha256']==NEW_PIN
    entries={int(p,16)for p in generation['entry_instructions']}|set(generation['reused_frame_entries']);assert CHECKED<=entries
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={e:_trace(code,e,entries)for e in entries}
    sites=set().union(*(set(b.code)for b in bodies.values()));covered=set();points=set();cases=[];prefixes=[]
    profiles=((0,0),(0x3f800000,0),(0x3f800000,0x3f800000),(0x3f800000,0xbf800000),
        (0x3f800000,0x7fc12345),(0x7fc12345,0x3f800000),(0x7f800000,0x40000000),(0xbf800000,0x3f000000))
    specs=[dict(entry=0x105f70,index=index,count=count,reference=reference,scaled=scaled,difficulty=difficulty,mode=mode,profile=profile)
        for index,count,reference,scaled,difficulty,mode,profile in itertools.product((-1,0,1,2),(0,2),(False,True),
            (False,True),(0,3),(1,2),range(len(profiles)))]
    specs += [dict(entry=0xcf1b0,index=index,prior=prior,increment=increment)for index,prior,increment in itertools.product(
        (-1,0,1,2),(0,0x7f,0x80,0xff),(-1,0,1,128))]
    specs += [dict(entry=0x275480,index=index,tick=tick)for index,tick in itertools.product((-1,0,3),(0,0x7fffffff))]
    specs += [dict(entry=0x205eb0,request=request,state=state,present=present,status=status,elapsed=elapsed)for request,state,present,status,elapsed in itertools.product(
        (0,1,2,3,4),(0,1,2),(False,True),(3,4,6),(0,1,300))]
    specs += [dict(entry=0x205fa0,request=request,target=target,blocked=blocked)for request,target,blocked in itertools.product(
        (0,2,3,4),(False,True),(False,True))]
    specs += [dict(entry=0x25f9b0,bits=bits,unsafe=unsafe)for bits,unsafe in itertools.product(
        (0,0x80000000,0x3f800000,0xbf800000,0x7fc12345),(False,True))]
    specs += [dict(entry=0x1ec5c0,index=i)for i in (0,1)]
    specs += [dict(entry=0x32ecdd,fp_status=flags|condition|(top<<11)|busy)for flags,condition,top,busy in itertools.product(
        (0,1,8,0x10,0x20,0x3f,0x41,0xff),(0,0x100,0x200,0x400,0x4000,0x4700),range(8),(0,0x8000))]
    specs += [dict(entry=0x32ecd2,fp_status=flags|condition|(top<<11))for flags,condition,top in itertools.product(
        (0,1,8,0x10,0x20,0x3f),(0,0x100,0x200,0x400,0x4000,0x4700),(0,7))]
    banks=controls['native_component_banks']+[[0x50eeec,4],[0x50eef8,4]]
    for ordinal,spec in enumerate(specs):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(P.BASE,0x30000);g.mem_write(P.BASE,bytes([base['poison']])*0x30000);g.mem_map(P.STOP,4096)
        patches=list(base['patches'])
        for at,bits in patches:g.mem_write(at,bytes.fromhex(bits))
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        def fp(at,value):raw(at,struct.pack('<f',value))
        initial=list(base['gp']);xmm=[list(v)for v in base['xmm']]
        for at,value in ((0x50eeec,HEADER),(0x50eef8,HEADER),(HEADER+0x44,RECORDS),(P.OBJECT,0xabcd0003),
            (P.TAGS+7*16+8,MOD),(P.GLOBALS+0xf8,1),(P.GLOBALS+0xfc,FACTORS)):
            put(at,value)
        for i,value in enumerate((0.5,1.0,2.0,4.0)):fp(FACTORS+(10*4+i)*4,value)
        if spec['entry']==0x105f70:
            initial[0]=P.OBJECT_ID;initial[2]=0xabcd0000|(spec['index']&0xffff)
            put(P.CHILD+0x2d0,spec['count']);put(P.CHILD+0x2d4,BLOCK)
            for i in range(2):put(BLOCK+i*0xec+0x90,0xabcd0007 if spec['reference'] else 0xffffffff)
            numerator,denominator=profiles[spec['profile']];put(P.STACK+4,numerator);put(MOD+0x17c,denominator)
            put(MOD+0xbc,0x400 if spec['scaled'] else 0);put(P.OPTIONS+8,spec['mode']);put(P.OPTIONS+0x132,spec['difficulty'],2)
        elif spec['entry']==0xcf1b0:
            initial[0]=P.OBJECT_ID;initial[2]=0xabcd0000|(spec['index']&0xffff)
            put(P.STACK+4,spec['increment'],1);put(P.OBJECT+0x23e+spec['index'],spec['prior'],1)
        elif spec['entry']==0x275480:
            initial[0]=0xffffffff if spec['index']<0 else 0xabcd0000|spec['index'];put(P.CLOCK+8,spec['tick'])
        elif spec['entry']==0x205eb0:
            initial[0]=P.ACTOR_ID;put(P.STACK+4,spec['request'],2);put(P.ACTOR+0x722,spec['state'],2)
            put(P.ACTOR+0x338,P.PROP_ID);put(P.PROP+0x14,0xabcd0000 if spec['present'] else 0xffffffff)
            put(RECORDS+0x70,spec['status'],2);put(P.ACTOR+0x350,spec['elapsed']);put(P.ACTOR+0x268,1,1)
        elif spec['entry']==0x205fa0:
            initial[7]=P.ACTOR_ID;put(P.STACK+4,spec['request'],2);put(P.ACTOR+0x724,P.PROP_ID)
            put(P.OTHER+0x58+0x3c,0xabcd0000 if spec['target'] else 0xffffffff);put(P.OTHER+0x25,int(spec['blocked']),1)
        elif spec['entry']==0x25f9b0:
            initial[7]=P.ACTOR_ID;put(P.OBJECT+0xf8,spec['bits']);put(P.OBJECT+0x800+0x7c,0x80000c4 if spec['unsafe'] else 0)
        elif spec['entry']==0x1ec5c0:initial[0]=P.ACTOR_ID
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        if 'fp_status'in spec:g.reg_write(R.UC_X86_REG_FPSW,spec['fp_status'])
        initial_status=g.reg_read(R.UC_X86_REG_FPSW);trace=[];stores=[];pending=[];prefix=None;dependency=None
        def flush():
            for pc,at,prior in pending:stores.append([pc,at,prior.hex(),bytes(g.mem_read(at,len(prior))).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            nonlocal prefix,dependency
            flush();assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),spec)
            ins=code.at(pc)
            if ins.mnemonic=='call' and (pc in (0x206035,0x25fa1e,0x1ec610) or ins.operands[0].type!=X86_OP_IMM or ins.operands[0].imm not in entries):
                prefix=pc;dependency=ins.operands[0].imm if ins.operands[0].type==X86_OP_IMM else 'indirect';g.emu_stop();return
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
        try:g.emu_start(spec['entry'],P.STOP,count=50000);flush()
        except Exception as error:raise RuntimeError(f'Original scalar case{ordinal} {spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}')from error
        assert g.reg_read(R.UC_X86_REG_EIP)==(prefix or P.STOP),(spec,hex(g.reg_read(R.UC_X86_REG_EIP)))
        if prefix:prefixes.append([spec['entry'],prefix,dependency])
        case=dict(spec,poison=base['poison'],prefix_end=prefix,dependency=dependency,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg)for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')))for i in range(8)],
            x87_depth=sum(((g.reg_read(R.UC_X86_REG_FPTAG)>>(i*2))&3)!=3 for i in range(8)),
            outputs=[[at,bytes(g.mem_read(at,size)).hex()]for at,size in ((P.STACK-0x100,0x110),(P.OBJECT+0x230,0x20),
                (RECORDS,0x150),(P.ACTOR+0x7c8,4))])
        if 'fp_status'in spec:case.update(initial_x87_status=initial_status,final_x87_status=g.reg_read(R.UC_X86_REG_FPSW))
        cases.append(case)
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,component_controls_sha256=hashlib.sha256(control_bytes).hexdigest(),
        instruction_patches=[],supplied_dependency_returns=[],native_component_banks=banks,
        immutable_operands=[[p,image.read(p,n).hex()]for p,n in regions(points)],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original actor scalar reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases)-len(prefixes),dependency_prefixes=len(prefixes),
        prefix_boundaries=sorted({tuple(p)for p in prefixes},key=str),entry_coverage={f'{e:08x}':dict(observed=len(covered&set(bodies[e].code)),
            static=len(bodies[e].code),unexecuted=sorted(set(bodies[e].code)-covered))for e in sorted(CHECKED)},
        exception_status_cases=sum('fp_status'in c for c in cases),fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),full_actor_or_crt_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items()if k!='entry_coverage'}))


if __name__=='__main__':main()
