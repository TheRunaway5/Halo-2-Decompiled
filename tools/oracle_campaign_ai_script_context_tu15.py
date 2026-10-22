#!/usr/bin/env python3
"""Execute original262590's actor/script context publication without a callback return.

Whole empty/busy guards return normally; admitted requests stop before the real
211840 call. The existing Frame body owns all writes. Two context DWORDs are
loader-zeroed mutable storage, not pointers/results seeded by the native host.
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
AH,SH,ACTORS,ROWS=BASE+0x100,BASE+0x200,BASE+0x1000,BASE+0x4000
ACTOR,ROW=ACTORS+2*0x888,ROWS+3*0xd4
BANKS=((BASE,0x10000),(0x5020c8,4),(0x50eee0,12),(0x50ef00,8))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    section=next(s for s in image.sections if s.name=='.data')
    assert section.va+section.rsize<=0x50eee4<0x50eeec<=section.va+section.vsize
    assert image.read(0x50eee4,8)==bytes(8)
    code=_Code(image,'.text');body=_trace(code,0x262590,{0x262590});cases=[];covered=set()
    assert len(body.code)==123 and set(body.calls)=={0x211840}
    for present,busy,child,prior in itertools.product((False,True),(False,True),(False,True),(0,0xa5a5a5a5,0xffffffff)):
        g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
        for sec in image.sections:g.mem_write(sec.va,image.section_bytes(sec))
        g.mem_map(BASE,0x10000);g.mem_write(BASE,b'\xa5'*0x10000);g.mem_map(STOP,4096);patches=[]
        def put(at,value,size=4):
            raw=(value&((1<<(8*size))-1)).to_bytes(size,'little');g.mem_write(at,raw);patches.append([at,raw.hex()])
        for at,value in ((STACK,STOP),(STACK+4,0xe3610002),(STACK+8,0xf4570003),(0x5020c8,AH),
            (AH+0x44,ACTORS),(0x50eee0,SH),(SH+0x44,ROWS),(ACTOR+0x30,0xef630004 if child else 0xffffffff),
            (ROW+0x38,0xabcd0005 if present else 0xffffffff),(0x50eee4,prior),(0x50eee8,prior),
            (0x50ef00,prior),(0x50ef04,prior)):put(at,value)
        put(ROW+0x44,int(busy),1)
        initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
        xmm=[[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg,value in zip(GP,initial):g.reg_write(reg,value)
        for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR,0x1f80);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
        trace=[];stores=[];prefix=None
        def instruction(_g,pc,size,_u):
            nonlocal prefix
            assert pc in body.code and bytes(g.mem_read(pc,size))==image.read(pc,size),hex(pc)
            if pc==0x262646:prefix=pc;g.emu_stop();return
            trace.append(pc);covered.add(pc)
        def store(_g,_kind,at,size,value,_u):
            assert any(p<=at<at+size<=p+n for p,n in BANKS),hex(at)
            stores.append([g.reg_read(R.UC_X86_REG_EIP),at,bytes(g.mem_read(at,size)).hex(),
                (value&((1<<(8*size))-1)).to_bytes(size,'little').hex()])
        g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
        g.emu_start(0x262590,STOP,count=1000);assert g.reg_read(R.UC_X86_REG_EIP)==(prefix or STOP)
        cases.append(dict(entry=0x262590,poison=0xa5,present=present,busy=busy,child=child,prior=prior,
            prefix_end=prefix,patches=patches,gp=initial,xmm=xmm,trace=trace,stores=stores,
            final_gp=[g.reg_read(r) for r in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
            outputs=[[ROW,bytes(g.mem_read(ROW,0xd4)).hex()],[0x50eee4,bytes(g.mem_read(0x50eee4,8)).hex()],
                [0x50ef00,bytes(g.mem_read(0x50ef00,8)).hex()],[STACK-0x20,bytes(g.mem_read(STACK-0x20,0x2c)).hex()]]))
    fixture=dict(scope=__doc__,executor='frame',original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        native_component_banks=BANKS,immutable_operands=[],cases=cases)
    raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify or args.fixture.exists():assert args.fixture.read_bytes()==raw,'Preserve original script-context reference'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=len(cases),whole_cases=sum(c['prefix_end'] is None for c in cases),
        callback_prefixes=sum(c['prefix_end'] is not None for c in cases),original_instructions=len(body.code),observed_instructions=len(covered),
        unexecuted=sorted(set(body.code)-covered),storage=dict(address=0x50eee4,bytes=8,original_hex=image.read(0x50eee4,8).hex(),
            section=vars(section),source_writers=[0x26262e,0x262640,0x262656,0x26265c],completed_context_seed=False),
        fixture_bytes=len(raw),fixture_sha256=hashlib.sha256(raw).hexdigest(),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        callback_completion_qualified=False,mission_command_execution_qualified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
