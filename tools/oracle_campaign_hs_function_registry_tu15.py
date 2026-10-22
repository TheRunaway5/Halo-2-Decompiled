#!/usr/bin/env python3
"""Compare the original HS dispatcher against every authored function definition.

Run211B10 from declared thread/frame/syntax records. All948 function lookups
stop before their real evaluator CALL; disabled-thread guards return normally.
Preserve the original null evaluator and script-call branch. Registry data are
authored engine inputs, never executable replacements or supplied call returns.
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
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_campaign_hs_function_registry_tu15 import registry,TABLE
from export_final_startup_storage_data_tu15 import regions

BASE,STACK,STOP=0x2000000,0x200f000,0x3000000
TH,NH,THREADS,FRAME,NODES=BASE+0x100,BASE+0x200,BASE+0x1000,BASE+0x5000,BASE+0x6000
THREAD,NODE=THREADS+2*0x418,NODES+3*20
CELLS=((0x505e5c,4),(0x505e60,1),(0x505e64,4),(0x505e6c,4),(0x4f3218,4))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    rows,points,checked=registry(image);code=_Code(image,'.text');body=_trace(code,0x211b10,{0x211b10})
    assert len(body.code)==133;cases=[];covered=set();prefixes=0
    specs=[dict(opcode=i,enabled=1,script=0,initialize=1)for i in range(len(rows))]
    specs += [dict(opcode=0,enabled=enabled,script=script,initialize=initialize)for enabled,script,initialize in itertools.product((0,1),(0,1),(0,1))]
    for spec in specs:
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096);patches=[]
        def put(at,value,size=4):
            raw=(value&((1<<(8*size))-1)).to_bytes(size,'little');g.mem_write(at,raw);patches.append([at,raw.hex()])
        for at,value in ((STACK,STOP),(0x505e5c,TH),(TH+0x44,THREADS),(0x505e6c,NH),(NH+0x44,NODES),
            (0x505e64,0xabcdef12),(0x4f3218,0),(THREAD+0x10,FRAME),(THREAD+8,0x12345678),(FRAME+4,0x1180003)):
            put(at,value)
        put(THREAD+2,1,1);put(THREAD+3,spec['initialize'],1);put(0x505e60,spec['enabled'],1)
        put(NODE+2,spec['opcode'],2);put(NODE+6,2 if spec['script'] else 0,1)
        initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0xf4570002]
        xmm=[[0x3f000000+i*4+j for j in range(4)]for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];prefix=None
        def instruction(_g,pc,size,_u):
            nonlocal prefix
            assert pc in body.code and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            if pc in (0x211c1b,0x211c2b):prefix=pc;g.emu_stop();return
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,at,size,value,_u):
            assert BASE<=at<at+size<=BASE+0x10000 or at==0x505e64,hex(at)
            stores.append([g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        g.emu_start(0x211b10,STOP,count=1000);assert g.reg_read(R.UC_X86_REG_EIP)==(prefix or STOP)
        prefixes+=prefix is not None
        cases.append(dict(spec,entry=0x211b10,poison=0xa5,prefix_end=prefix,patches=patches,gp=initial,xmm=xmm,
            trace=trace,stores=stores,final_gp=[g.reg_read(reg)for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little')))for i in range(8)],
            outputs=[[at,bytes(g.mem_read(at,size)).hex()]for at,size in ((STACK-0x40,0x44),(THREAD,0x20),(FRAME,0x20),(0x505e64,4))]))
    operands=[[p,image.read(p,n).hex()]for p,n in regions(points)]
    fixture=dict(scope=__doc__,executor='frame',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000],*CELLS,[TABLE,len(rows)*4]],immutable_operands=operands,
        authored_operands=[[TABLE,image.read(TABLE,len(rows)*4).hex()]],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original HS registry reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_guards=len(cases)-prefixes,
        evaluator_call_prefixes=prefixes,source_body_instructions=len(body.code),observed_instructions=len(covered),
        unexecuted=sorted(set(body.code)-covered),original_functions=len(rows),profile_signatures_checked=checked,
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        evaluator_bodies_qualified=False,complete_hs_execution_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
