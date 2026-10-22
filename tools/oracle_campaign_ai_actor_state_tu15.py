#!/usr/bin/env python3
"""Execute original1FB3C0 actor-state updates with real nested Frame bodies.

Declared actor/waypoint/unit component records exercise state/active guards,
signed byte count/index comparisons, original waypoint advancement and fallback
transition.1FB2A0 produces vectors and200A40 produces state; their returns are
never supplied. All ordered writes and full actor bytes are compared.
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

BASE,STACK,STOP=0x2000000,0x200f000,0x3000000
HEADER,ACTOR,UNITS,OBJECTS=BASE+0x1000,BASE+0x3000,BASE+0x2000,BASE+0x5000
ENTRIES={0x1fb3c0,0x1fb2a0,0x200a40}


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={p:_trace(code,p,ENTRIES) for p in ENTRIES}
    assert len(bodies[0x1fb3c0].code)==43
    sites=set().union(*(set(b.code) for b in bodies.values()));covered=set();cases=[]
    # Advancing controls remain inside the declared source waypoint slots.
    # Other signed controls test the parent's original comparison/guards.
    counts=((2,0),(3,1),(1,0),(0,0),(-128,127),(-1,-1),(127,126))
    for state,active,(count,index),fallback,poison in itertools.product((0,1,4),(0,1),counts,(0,1),(0,0xa5)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([poison])*0x10000);g.mem_map(STOP,4096)
        patches=[]
        def raw(address,data):patches.append([address,data.hex()]);g.mem_write(address,data)
        def u32(address,value):raw(address,struct.pack('<I',value))
        u32(0x5020c8,HEADER);u32(HEADER+0x44,ACTOR)
        u32(0x4ecbd0,UNITS);u32(UNITS+0x44,OBJECTS);raw(OBJECTS+3,b'\x01')
        u32(ACTOR+0x18,0xc1230000);raw(ACTOR+0x227,b'\x01');raw(ACTOR+0x229,b'\x00')
        raw(ACTOR+0x238,struct.pack('<3f',1.,2.,3.));raw(ACTOR+0x504,struct.pack('<H',state))
        raw(ACTOR+0x50c,bytes([active]));raw(ACTOR+0x538,struct.pack('<Bbb',fallback,count,index))
        #127/126 is a comparison boundary whose next index is127; allocate
        #that complete referenced record within the explicit component bank.
        next_index=index+1
        if -128<=next_index<=127:
            raw(ACTOR+0x548+next_index*0x1c,struct.pack('<3fh',4.,6.,8.,-1))
        raw(STACK,struct.pack('<I',STOP))
        initial=[0xe3610000,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[]
        def instruction(_g,pc,size,_u):
            assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),(hex(pc),state,active,count,index,fallback)
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,address,size,value,_u):
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        g.emu_start(0x1fb3c0,STOP,count=1000);assert g.reg_read(R.UC_X86_REG_EIP)==STOP
        cases.append(dict(entry=0x1fb3c0,prefix_end=None,state=state,active=active,count=count,index=index,
            fallback=fallback,poison=poison,patches=patches,gp=initial,xmm=xmm,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            trace=trace,stores=stores,outputs=[[ACTOR,bytes(g.mem_read(ACTOR,0x888)).hex()],
                [STACK-0x80,bytes(g.mem_read(STACK-0x80,0x84)).hex()]]))
    assert set(bodies[0x1fb3c0].code)<=covered
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000],[0x5020c8,4],[0x4ecbd0,4]],immutable_operands=[],cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve original actor-state reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=len(cases),
        new_body_instructions=43,new_body_observed=43,
        original_coverage={f'{entry:08x}':dict(total=len(body.code),observed=len(set(body.code)&covered),
            unexecuted=sorted(set(body.code)-covered)) for entry,body in bodies.items()},
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
