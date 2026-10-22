#!/usr/bin/env python3
"""Execute whole original team/difficulty selection and its real bit/scalar helpers.

All35 remapping rows, allied/enemy teams, invalid teams, modes, difficulty
clamps and absent global data use declared component inputs. Unchanged TU1.5
instructions produce every expected result; no return values are supplied.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

BASE=0x2000000
STACK,OPTIONS,GLOBALS,FACTORS,ALLEGIANCE=(BASE+n for n in (0x1000,0x2000,0x3000,0x4000,0x5000))
STOP=0x3000000
ENTRIES={0x1f19d0,0x1f1950,0xc1e90}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={e:_trace(code,e,ENTRIES) for e in ENTRIES}
    sites=set().union(*(set(b.code) for b in bodies.values()))
    banks=[[BASE,0x10000],[0x4f3218,4],[0x4ecc1c,4],[0x5020c4,4]]
    specs=[dict(row=row,team=team,difficulty=difficulty,mode=1,enemy=enemy,present=3)
        for row,team,difficulty,enemy in itertools.product(range(35),(0,15),(0,3),(False,True))]
    specs += [dict(row=row,team=team,difficulty=difficulty,mode=mode,enemy=False,present=3)
        for row,team,difficulty,mode in itertools.product((0,4,34),(-2,-1,16),(0,3),(1,2,3))]
    specs += [dict(row=row,team=-1,difficulty=difficulty,mode=1,enemy=False,present=3)
        for row,difficulty in itertools.product((0,4,34),(-1,4))]
    specs += [dict(row=2,team=0,difficulty=3,mode=1,enemy=False,present=present) for present in range(3)]
    cases=[];covered=set();points=set(range(0x450210,0x450210+70))
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096)
        for at,size in banks[1:]:g.mem_write(at,b'\xa5'*size)
        patches=[]
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        put(STACK,STOP);put(0x4f3218,OPTIONS);put(OPTIONS+8,spec['mode']);put(OPTIONS+0x132,spec['difficulty'],2)
        put(0x4ecc1c,GLOBALS if spec['present'] else 0);put(GLOBALS+0xf8,int(spec['present']>=2))
        put(GLOBALS+0xfc,FACTORS if spec['present']==3 else 0);put(0x5020c4,ALLEGIANCE)
        put(ALLEGIANCE+0xc4,0 if spec['enemy'] or not 0<=spec['team']<16 else 1<<(spec['team']+16))
        raw(FACTORS,b''.join(struct.pack('<f',float(row*4+column)+0.25) for row in range(35)for column in range(4)))
        initial=[0x12340000|spec['row'],0xabcd0000|(spec['team']&0xffff),0x11223344,0x55667788,STACK,0x99aabbcc,0x13572468,0x24681357]
        xmm=[[0x3f800000+i,0x40000000+i,0x40400000+i,0x40800000+i]for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];pending=[]
        def flush():
            for pc,at,prior in pending:stores.append([pc,at,prior.hex(),bytes(g.mem_read(at,len(prior))).hex()])
            pending.clear()
        def instruction(_g,pc,size,_u):
            flush();assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(spec,hex(pc))
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,at,size,_value,_u):
            assert any(p<=at<at+size<=p+n for p,n in banks),(spec,hex(at))
            pending.append((g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size))))
        def read(_g,_kind,at,size,_value,_u):
            sec=next((s for s in image.sections if s.va<=at<at+size<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':points.update(range(at,at+size))
            elif sec:assert any(p<=at<at+size<=p+n for p,n in banks),('Undeclared field',spec,hex(at))
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        g.emu_start(0x1f19d0,STOP,count=10000);flush();assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        cases.append(dict(spec,entry=0x1f19d0,poison=0xa5,prefix_end=None,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg)for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')))for i in range(8)],
            outputs=[[at,bytes(g.mem_read(at,size)).hex()]for at,size in ((STACK-32,48),(ALLEGIANCE+0xc4,4))]))
    fixture=dict(scope=__doc__,executor='frame',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=banks,immutable_operands=[[at,image.read(at,size).hex()]for at,size in regions(points)],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original difficulty reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases),entry_coverage={f'{e:08x}':dict(
        observed=len(covered&set(b.code)),static=len(b.code),unexecuted=sorted(set(b.code)-covered))for e,b in sorted(bodies.items())},
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
