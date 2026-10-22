#!/usr/bin/env python3
"""Execute whole original267A30, paired with267A40 by1E7E00 registration.

Every low-byte key and additional upper-word/signed controls execute all four
original instructions and real RET4. No supplied bucket or dependency return.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,STACK,STOP=0x2000000,0x200f000,0x3000000


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    assert image.read(0x1e7e00,10).hex()=='68407a260068307a2600'
    code=_Code(image,'.text');body=_trace(code,0x267a30,{0x267a30});assert len(body.code)==4
    keys=set(range(256))|{hi|lo for hi in (0x10000,0xffff0000,0x80000000,0x7fffff00,0xffffff00)
        for lo in (0,1,0x7f,0x80,0xfe,0xff)}
    cases=[];covered=set();buckets=set()
    for key in sorted(keys):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096)
        raw=struct.pack('<2I',STOP,key);g.mem_write(STACK,raw)
        initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        trace=[]
        def instruction(_g,pc,size,_u):
            assert pc in body.code and bytes(g.mem_read(pc,size))==image.read(pc,size)
            trace.append(pc);covered.add(pc)
        g.hook_add(UC_HOOK_CODE,instruction);g.emu_start(0x267a30,STOP,count=10)
        assert g.reg_read(R.UC_X86_REG_EIP)==STOP;buckets.add(g.reg_read(R.UC_X86_REG_EAX))
        cases.append(dict(entry=0x267a30,poison=0xa5,prefix_end=None,key=key,patches=[[STACK,raw.hex()]],
            gp=initial,xmm=xmm,trace=trace,stores=[],outputs=[[STACK,raw.hex()]],
            final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)]))
    assert covered==set(body.code) and buckets==set(range(0,1024,4))
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=[[BASE,0x10000]],immutable_operands=[],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original bucket query reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_instructions=4,
        observed_instructions=len(covered),observed_buckets=len(buckets),fixture_bytes=len(raw),
        fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
