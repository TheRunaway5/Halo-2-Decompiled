#!/usr/bin/env python3
"""Execute original284300 and its complete four-variant selection/query paths.

Explicit component context controls cover empty/nonempty counts, mask-byte and
DWORD boundaries, signed indices/remapping and both selectors. Actual original
instructions produce counters, vectors, request pointers and stack writes.
Indirect calls stop before execution; no callback identity/result is supplied.
The .data sign mask retains its actual writable loader bytes and ownership.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_INTR
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,STACK,STOP=0x2000000,0x200f000,0x3000000
HEADER,INDICES,BITMAP,REMAP,RECORDS=BASE+0x1000,BASE+0x2000,BASE+0x3000,BASE+0x4000,BASE+0x6000
MASKS=(BASE+0x7000,BASE+0x7100,BASE+0x7200)
ENTRIES={0x284300,0x285cf0,0x2861a0,0x286460,0x2868a0}
CONTEXT,CONTEXT_BYTES=0x510ef0,0xb4


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');bodies={p:_trace(code,p,ENTRIES) for p in ENTRIES}
    sites=set().union(*(set(b.code) for b in bodies.values()));covered=set();cases=[];breakpoints=[]
    for mapped,sse,count,masks,profile,poison in itertools.product((0,1),(0,1),(-1,0,1,7,8,9,31,32,33),
        ((0,0,0),(255,0,0),(0,255,0),(0,0,255),(85,128,170),(255,255,255)),
        ('valid','negative','past_capacity','remap_negative'),(0,0xa5)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([poison])*0x10000);g.mem_map(STOP,4096)
        patches=[]
        def raw(address,data):patches.append([address,data.hex()]);g.mem_write(address,data)
        def u32(address,value):raw(address,struct.pack('<I',value&0xffffffff))
        raw(CONTEXT,bytes([poison])*CONTEXT_BYTES);raw(0x48c7e0,image.read(0x48c7e0,16))
        for cell,value in ((0x510f3c,count),(0x510f40,4),(0x510f58,HEADER),(0x510f5c,REMAP if mapped else 0),
            (0x510f70,MASKS[0]),(0x510f74,MASKS[1]),(0x510f78,MASKS[2]),(0x510f7c,RECORDS)):
            u32(cell,value)
        raw(0x510f80,bytes([sse]));u32(HEADER+0xc,INDICES);u32(HEADER+0x14,BITMAP)
        raw(BITMAP,b'\xff'*16)
        index=-1 if profile=='negative' else 4 if profile=='past_capacity' else 0
        raw(INDICES,struct.pack('<64h',*([index]*64)))
        raw(REMAP,struct.pack('<64h',*([-1 if profile=='remap_negative' else 0]*64)))
        for address,value in zip(MASKS,masks):raw(address,bytes([value])*8)
        raw(RECORDS,struct.pack('<32f',*([1.,2.,3.,4.,5.,6.,7.,8.]*4)))
        for offset in (0x510f00,0x510f20):raw(offset,struct.pack('<4f',1.,2.,3.,4.))
        raw(0x510f84,struct.pack('<4f',2.,3.,4.,.5))
        raw(STACK,struct.pack('<I',STOP))
        initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[];stores=[];stop={}
        def instruction(_g,pc,size,_u):
            assert pc in sites and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            ins=code.at(pc)
            if ins.mnemonic=='call':stop['prefix_end']=pc;g.emu_stop();return
            trace.append(pc);covered.add(pc)
        def interrupt(_g,number,_u):
            assert number==3 and trace[-1]==0x28619c
            stop['original_breakpoint']=trace[-1];g.emu_stop()
        def store(_g,_kind,address,size,value,_u):
            stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_INTR,interrupt)
        try:g.emu_start(0x284300,STOP,count=100000)
        except Exception as error:
            raise RuntimeError(f'Original mapped={mapped},sse={sse},count={count},masks={masks},profile={profile},'
                f'pc={g.reg_read(R.UC_X86_REG_EIP):x}') from error
        assert stop or g.reg_read(R.UC_X86_REG_EIP)==STOP
        case=dict(entry=0x284300,prefix_end=stop.get('prefix_end'),mapped=mapped,sse=sse,count=count,masks=masks,
            profile=profile,poison=poison,patches=patches,gp=initial,xmm=xmm,trace=trace,stores=stores,
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            outputs=[[CONTEXT,bytes(g.mem_read(CONTEXT,CONTEXT_BYTES)).hex()],
                [STACK-0x80,bytes(g.mem_read(STACK-0x80,0x84)).hex()]])
        if 'original_breakpoint' in stop:case['original_breakpoint']=stop['original_breakpoint'];breakpoints.append(len(cases))
        cases.append(case)
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000],[CONTEXT,CONTEXT_BYTES],[0x48c7e0,16]],immutable_operands=[],cases=cases)
    raw_data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw_data,'Preserve original birth query reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw_data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),
        whole_cases=sum(c['prefix_end'] is None and 'original_breakpoint' not in c for c in cases),
        callback_prefixes=sum(c['prefix_end'] is not None for c in cases),original_breakpoints=breakpoints,
        original_coverage={f'{p:08x}':dict(total=len(b.code),observed=len(set(b.code)&covered),
            unexecuted=sorted(set(b.code)-covered)) for p,b in bodies.items()},
        fixture_bytes=len(raw_data),fixture_sha256=hashlib.sha256(raw_data).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
