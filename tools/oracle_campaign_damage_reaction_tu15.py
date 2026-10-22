#!/usr/bin/env python3
"""Execute original reaction selection and indexed animation preparation.

WholeCBC50/CBB90 run with declared unit/definition/option inputs. Actor-limit
and animation services stop before their real CALLs, never supplied returns.
All eight table rows are read by112620; row4 is a declared ABI control, not
claimed as a naturally emitted selector result. Full GP/SIMD/flags, original
instruction/store order and preserved outputs come only from TU1.5 execution.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

BASE=0x2000000
STACK,HEADER,RECORDS,UNIT,TAGS,DEFINITION,OPTIONS,OUTPUT,VECTOR=(BASE+p for p in (0x1f000,0x1000,0x2000,0x4000,0x7000,0x8000,0xa000,0xc000,0xd000))
STOP=0x3000000
UNIT_ID=0xabcd0003
ENTRIES={0xcbc50,0xcbb90,0x112620}
PROFILES=((0,0),(0x3fc00000,0),(0,0x3fc00000),(0x40400000,0x40400000),(0x7fc12345,0x3fc00000),(0xbf800000,0xbf800000))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text')
    bodies={e:_trace(code,e,ENTRIES)for e in ENTRIES};sites=set().union(*(set(b.code)for b in bodies.values()))
    clamp_address=code.at(0xcbd4b).operands[1].mem.disp
    actor_pool=code.at(0xcbbb7).operands[1].mem.disp
    sid_types=code.at(0x1126a6).operands[1].mem.disp
    fixed={0x4ecbd0:4,0x4f0414:4,0x4f3218:4,clamp_address:1,actor_pool:4,sid_types:16}
    banks=[[BASE,0x30000]]+[[at,n]for at,n in sorted(fixed.items())]
    specs=[dict(entry=0xcbc50,fatal=0,strong=0,knocked=knocked,force=force,frozen=frozen,clamp=clamp,mode=mode,profile=profile,actor=False)
        for knocked,force,frozen,clamp,mode,profile in itertools.product((0,1),(0,1),(0,1),range(4),(1,2),range(len(PROFILES)))]
    specs += [dict(entry=0xcbc50,fatal=1,strong=strong,knocked=knocked,force=0,frozen=0,clamp=0,mode=1,profile=profile,actor=False)
        for strong,knocked,profile in itertools.product((0,1),(0,1),range(len(PROFILES)))]
    specs += [dict(entry=0xcbc50,fatal=0,strong=0,knocked=0,force=0,frozen=0,clamp=0,mode=1,profile=profile,actor=True)
        for profile in range(len(PROFILES))]
    specs += [dict(entry=0x112620,kind=kind,direction=direction)for kind,direction in itertools.product(range(8),range(4))]
    cases=[];covered=set();points=set(range(0x44a9e0,0x44aa40));results=set();prefixes=[]
    for ordinal,spec in enumerate(specs):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x30000);g.mem_write(BASE,b'\xa5'*0x30000);g.mem_map(STOP,4096)
        for at,n in banks[1:]:g.mem_write(at,b'\xa5'*n)
        g.mem_write(sid_types,image.read(sid_types,16))
        patches=[]
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,n=4):raw(at,(value&((1<<(8*n))-1)).to_bytes(n,'little'))
        for at,value in ((0x4ecbd0,HEADER),(HEADER+0x44,RECORDS),(RECORDS+3*12+8,UNIT),(0x4f0414,TAGS),
            (TAGS+8,DEFINITION),(UNIT,0xabcd0000),(UNIT+0x12c,0xffffffff),(UNIT+0x13c,0xffffffff),
            (UNIT+0x134,0),(0x4f3218,OPTIONS),(OPTIONS+8,1),(clamp_address,0),(actor_pool,HEADER),(UNIT+0xf8,0),
            (UNIT+0x10a,0),(UNIT+0x33e,0x600),(UNIT+0x12a,0x800),(DEFINITION+0x104,0x3f800000),
            (DEFINITION+0x10c,0x40000000),(DEFINITION+0x114,0x40000000),(STACK,STOP)):
            put(at,value,fixed.get(at,4) if at in fixed else (2 if at in (UNIT+0x10a,UNIT+0x33e,UNIT+0x12a)else 4))
        initial=[UNIT_ID,0x11223344,0x55667788,0,STACK,0x99aabbcc,0x13572468,0x24681357]
        xmm=[[0x3f800000+i,0x40000000+i,0x40400000+i,0x40800000+i]for i in range(8)]
        if spec['entry']==0xcbc50:
            a,b=PROFILES[spec['profile']];initial[3]=0x12345600|spec['fatal']
            for at,value in ((STACK+4,spec['strong']),(STACK+8,spec['knocked']),(STACK+12,0),(STACK+16,spec['force']),
                (STACK+20,a),(STACK+24,b),(UNIT+0x10a,spec['frozen']*4),(UNIT+0x134,32 if spec['clamp']==1 else 0),
                (UNIT+0x13c,0xabcd0000 if spec['clamp']==2 else 0xffffffff),(OPTIONS+8,spec['mode']),
                (clamp_address,int(spec['clamp']==3)),(UNIT+0x12c,0xabcd0000 if spec['actor']else 0xffffffff)):
                put(at,value,1 if at==clamp_address else (2 if at==UNIT+0x10a else 4))
        else:
            initial[0]=0x12345678;initial[1]=spec['kind']
            for at,value in ((STACK+4,UNIT_ID),(STACK+8,spec['direction']),(STACK+12,0),(STACK+16,OUTPUT+4),
                (STACK+20,VECTOR),(STACK+24,OUTPUT),(VECTOR,0x3f800000),(VECTOR+4,0),(VECTOR+8,0)):
                put(at,value)
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];pending=[];prefix=None;dependency=None
        def flush():
            for pc,at,prior in pending:stores.append([pc,at,prior.hex(),bytes(g.mem_read(at,len(prior))).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            nonlocal prefix,dependency
            flush();assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(spec,hex(pc))
            ins=code.at(pc)
            if ins.mnemonic=='call' and (ins.operands[0].type!=X86_OP_IMM or ins.operands[0].imm not in ENTRIES):
                prefix=pc;dependency=ins.operands[0].imm if ins.operands[0].type==X86_OP_IMM else 'indirect';g.emu_stop();return
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,at,n,_value,_u):
            assert any(p<=at<at+n<=p+size for p,size in banks),(spec,hex(at))
            pending.append((g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,n))))
        def read(_g,_kind,at,n,_value,_u):
            sec=next((s for s in image.sections if s.va<=at<at+n<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':points.update(range(at,at+n))
            elif sec:assert any(p<=at<at+n<=p+size for p,size in banks),('Undeclared field',spec,hex(at))
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        try:g.emu_start(spec['entry'],STOP,count=2000);flush()
        except Exception as error:raise RuntimeError(f'Original reaction case{ordinal} {spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}')from error
        assert g.reg_read(R.UC_X86_REG_EIP)==(prefix or STOP),(spec,hex(g.reg_read(R.UC_X86_REG_EIP)))
        if prefix:prefixes.append((spec['entry'],prefix,dependency))
        if spec['entry']==0xcbc50 and not prefix:results.add(g.reg_read(R.UC_X86_REG_EAX))
        cases.append(dict(spec,canonical_entry=0xcaad0 if spec['entry']==0xcbc50 else 0x111700,poison=0xa5,prefix_end=prefix,dependency=dependency,
            patches=patches,gp=initial,xmm=xmm,trace=trace,stores=stores,final_gp=[g.reg_read(reg)for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')))for i in range(8)],
            outputs=[[at,bytes(g.mem_read(at,n)).hex()]for at,n in ((STACK-0x100,0x120),(UNIT+0x600,0x100),(OUTPUT,0x10))]))
    assert results=={0,1,2,3,5,6,7},results
    fixture=dict(scope=__doc__,executor='damage',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=banks,authored_operands=[[sid_types,image.read(sid_types,16).hex()]],
        immutable_operands=[[at,image.read(at,n).hex()]for at,n in regions(points)],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original damage reaction reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases)-len(prefixes),dependency_prefixes=len(prefixes),
        natural_selector_results=sorted(results),maximum_selector_result=max(results),row4_natural_reachability_not_claimed=True,
        prefix_boundaries=sorted(set(prefixes),key=str),entry_coverage={f'{e:08x}':dict(observed=len(covered&set(b.code)),static=len(b.code),unexecuted=sorted(set(b.code)-covered))for e,b in sorted(bodies.items())},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),full_damage_animation_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
