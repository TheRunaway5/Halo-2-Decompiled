#!/usr/bin/env python3
"""Execute the original kind-guarded actor-frame callback selectors.

All115 nonnull descriptor identities traverse whole1B0EC0/1B0F10. A null or
non-kind2 callback completes normally. A nonnull callback stops before its
CALL instruction executes, retaining a declared prefix rather than supplying
the callback's behavior or return. No original instructions are patched.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_ai_behavior_descriptor_data_tu15 import descriptor_inputs,descriptor_callback_tail_inputs

BASE,HEADER,ACTORS,STACK,STOP=0x2000000,0x2000100,0x2001000,0x200f000,0x3000000


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');records,_,_=descriptor_inputs(image)
    tails,_=descriptor_callback_tail_inputs(image,records)
    tailmap={r['descriptor']:r for r in tails};cases=[];covered=set();bodies={}
    for entry,call,field in ((0x1b0ec0,0x1b0f06,0x44),(0x1b0f10,0x1b0f56,0x48)):
        body=_trace(code,entry,{entry});bodies[entry]=body
        for record in records:
            index=record['directory_indices'][0];actor_index=index%4;lane=index%3
            frame=ACTORS+actor_index*0x888+0x90+lane*0x40
            target=tailmap[record['address']]['callbacks'][(field-0x44)//4] if record['kind']==2 else 0
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            g.mem_map(BASE,0x10000);g.mem_write(BASE,bytes([0xa5])*0x10000);g.mem_map(STOP,4096)
            patches=[]
            def put(at,value,size=4):
                raw=value.to_bytes(size,'little');g.mem_write(at,raw);patches.append([at,raw.hex()])
            for at,value in ((0x5020c8,HEADER),(HEADER+0x44,ACTORS),(STACK,STOP)):put(at,value)
            put(ACTORS+actor_index*0x888+0x190,lane,2);put(frame,index,2)
            initial=[0x11223344,0x22334455,0xe3610000|actor_index,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
            xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
            for reg,value in zip(GP,initial):g.reg_write(reg,value)
            for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
            g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            trace=[];stores=[]
            def instruction(_g,pc,size,_u):
                assert pc in body.code and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
                trace.append(pc);covered.add(pc)
                if pc==call:g.emu_stop()
            def store(_g,_kind,address,size,value,_u):
                assert BASE<=address<address+size<=BASE+0x10000
                stores.append([g.reg_read(R.UC_X86_REG_EIP),address,bytes(g.mem_read(address,size)).hex(),
                    (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
            g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
            g.emu_start(entry,STOP,count=100)
            assert g.reg_read(R.UC_X86_REG_EIP)==(call if target else STOP)
            cases.append(dict(entry=entry,index=index,descriptor=record['address'],kind=record['kind'],
                callback=target,prefix_end=call if target else None,patches=patches,gp=initial,xmm=xmm,
                final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                trace=trace,stores=stores))
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_bank=[BASE,0x10000],native_component_poison=0xa5,callback_fields=tails,cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original callback-selector evidence'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),
        whole_cases=sum(r['prefix_end'] is None for r in cases),callback_prefix_cases=sum(r['prefix_end'] is not None for r in cases),
        static_parent_instructions=sum(len(b.code) for b in bodies.values()),observed_parent_instruction_sites=len(covered),
        completed_parent_instructions=len(covered-{0x1b0f06,0x1b0f56}),call_instructions_stopped_before_execution=True,
        descriptor_tails=len(tails),immutable_bytes=8*len(tails),fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),full_callback_behavior_qualified=False,
        runtime_reference_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
