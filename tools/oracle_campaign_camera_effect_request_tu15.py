#!/usr/bin/env python3
"""Execute the whole original camera-effect request owner with declared inputs.

All seven authored types, disabled/remapped mutable inputs, priority/time guards,
overlapping request/slot copies and raw float conversions run unchanged1578A0.
Expected requests, timers, flags and ordered writes come only from TU1.5.
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
STACK,SLOT,REQUEST,CLOCK=(BASE+n for n in (0xf000,0x1000,0x2000,0x8000))
STOP=0x3000000
PROFILES=((30,30,0x3f800000,0x3f000000),(30,1,0x3d000000,0xbf800000),
    (30,-1,0x80000000,0x3f800000),(60,120,0x3f000000,0x00000000),
    (60,32767,0x7fc12345,0x3f800000),(30,2,0x7f800000,0x3f800000),
    (30,0,0xbf800000,0x3f800000),(60,30,0x4f000000,0x3f000000))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0x1578a0,{0x1578a0});assert not body.calls
    banks=[[BASE,0x10000],[0x51df54,4],[0x47524c,14]]
    specs=[dict(kind=kind,priority=priority,profile=profile,alias=0,override=None)
        for kind,priority,profile in itertools.product(range(7),range(3),range(len(PROFILES)))]
    specs += [dict(kind=kind,priority=1,profile=profile,alias=alias,override=None)
        for kind,profile,alias in itertools.product((0,1,6),(0,4),(0x14,0x18,0x1c))]
    specs += [dict(kind=kind,priority=1,profile=profile,alias=0,override=override)
        for kind,profile,override in itertools.product((0,1,6),(0,4),(0,6,-1))]
    cases=[];covered=set();points=set()
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096)
        patches=[]
        def raw(at,data):g.mem_write(at,data);patches.append([at,data.hex()])
        def put(at,value,size=4):raw(at,(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        rate,timer,duration,scale=PROFILES[spec['profile']];request=SLOT+spec['alias'] if spec['alias'] else REQUEST
        put(STACK,STOP);put(STACK+4,scale);put(0x51df54,CLOCK)
        put(CLOCK+2,rate,2);raw(CLOCK+4,struct.pack('<f',1.0/rate))
        prior_priority,new_priority=((3,1),(1,1),(1,3))[spec['priority']]
        put(SLOT+0x1a,prior_priority,2);put(SLOT+0x7e,timer,2);put(SLOT+0x88,0xa0,1)
        put(request,spec['kind'],2);put(request+2,new_priority,2);put(request+4,duration);put(request+0xc,0x3fc00000)
        if spec['override'] is not None:put(0x47524c+spec['kind']*2,spec['override'],2)
        initial=[SLOT,request,0x11223344,0x55667788,STACK,0x99aabbcc,0x13572468,0x24681357]
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
            flush();assert pc in body.code and bytes(g.mem_read(pc,size))==image.read(pc,size),(spec,hex(pc))
            if not trace or trace[-1]!=pc:trace.append(pc)
            covered.add(pc)
        def store(_g,_kind,at,size,_value,_u):
            assert any(p<=at<at+size<=p+n for p,n in banks),(spec,hex(at))
            pending.append((g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size))))
        def read(_g,_kind,at,size,_value,_u):
            sec=next((s for s in image.sections if s.va<=at<at+size<=s.va+s.vsize),None)
            if sec and sec.name=='.rdata':points.update(range(at,at+size))
            elif sec:assert any(p<=at<at+size<=p+n for p,n in banks),('Undeclared field',spec,hex(at))
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_READ,read)
        g.emu_start(0x1578a0,STOP,count=1000);flush();assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        cases.append(dict(spec,entry=0x1578a0,poison=0xa5,prefix_end=None,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg)for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')))for i in range(8)],
            final_mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
            outputs=[[at,bytes(g.mem_read(at,size)).hex()]for at,size in ((STACK-32,48),(SLOT,0xa0),(0x47524c,14))]))
    fixture=dict(scope=__doc__,executor='camera_effect_request',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=banks,authored_operands=[[0x47524c,image.read(0x47524c,14).hex()]],
        immutable_operands=[[at,image.read(at,size).hex()]for at,size in regions(points)],cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original camera request reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases),observed_instructions=len(covered),static_instructions=len(body.code),
        unexecuted=sorted(set(body.code)-covered),alias_cases=sum(bool(c['alias'])for c in cases),mutable_type_cases=sum(c['override']is not None for c in cases),
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
