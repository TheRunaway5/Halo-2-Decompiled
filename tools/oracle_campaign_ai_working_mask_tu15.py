#!/usr/bin/env python3
"""Authenticate the original AI working-mask destination and complete consumer.

Actual1B08D9 selects56AE84 for whole1B0720. All five words are written before
any destination read. Declared raw actor/tag/squad records and poisoned prior
contents test the contract; no code patch, dependency return or mask outcome
is supplied. The source destination resides in the original loader-zero tail.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE,STOP,DEST,ENTRY=0x2000000,0x3000000,0x56ae84,0x1b0720
ACTORS,ROWS,TAGS,CHAR,MASK,SQUADS,SQUAD_ROWS,PROPS,PROP_ROWS,STACK=[BASE+p for p in
    (0x1000,0x2000,0x5000,0x6000,0x7000,0x8000,0x9000,0xa000,0xb000,0xf000)]


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True)
    ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true')
    args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:ap.error('Run through tools/run_test.py')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,ENTRY,{ENTRY})
    assert image.read(0x1b08d9,5).hex()=='bf84ae5600'
    assert image.read(0x1b08f1,5).hex()=='e82afeffff'
    section=next(s for s in image.sections if s.va<=DEST<DEST+20<=s.va+s.vsize)
    assert DEST>=section.va+section.rsize and image.read(DEST,20)==bytes(20)
    cases=[];covered=set()
    for profile in range(5):
        for poison in (0,0xa5,255,0x73):
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
            for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
            g.mem_map(BASE,0x10000);g.mem_map(STOP,4096);patches=[]
            def put(p,v,n=4):
                data=(v&((1<<(8*n))-1)).to_bytes(n,'little');g.mem_write(p,data);patches.append([p,data.hex()])
            for p,v in ((0x5020c8,ACTORS),(ACTORS+0x44,ROWS),(0x4f0414,TAGS),
                        (0x52be7c,SQUADS),(SQUADS+0x44,SQUAD_ROWS),(0x50eee0,PROPS),(PROPS+0x44,PROP_ROWS),
                        (TAGS+8,CHAR),(TAGS+16+8,MASK),(TAGS+32+8,CHAR+0x100)):
                put(p,v)
            put(ROWS+0x858,0x8a000000 if profile>=2 else 0xffffffff)
            put(ROWS+0x30,0xffffffff if profile==1 else 0x8b000000)
            put(ROWS+0x54,0x8c000000)
            put(ROWS+0x26c,0x8d000000 if profile==4 else 0xffffffff)
            put(ROWS+0x269,int(profile==4),1)
            put(PROP_ROWS+0x86,int(profile>=3),1);put(PROP_ROWS+0x88,0x8e000001)
            put(CHAR+0x20,0x8f000002 if profile%2 else 0xffffffff)
            for i in range(5):
                put(SQUAD_ROWS+0x84+i*4,0xf0f00f0f^(i*0x11111111))
                put(MASK+0x38+i*4,0x87654321+i*0x11111111)
                put(0x56aeb8+i*4,0x13579bdf^(i*0x1020304))
                put(CHAR+0x100+0x38+i*4,0xfff0ff0f^(i*0x01010101))
            put(STACK,STOP)
            initial=[0x11223344,0xa1b20000,0x33445566,0x55667788,STACK,0x778899aa,0x99aabbcc,DEST]
            for reg,value in zip(GP,initial):g.reg_write(reg,value)
            xmm=[[0x3f000000+i*4+lane for lane in range(4)] for i in range(8)]
            for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
            g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            g.mem_write(DEST,bytes([poison])*20)
            initialized=set();read_before_write=[];trace=[];stores=[];accesses=[]
            def instruction(_g,pc,_size,_user):
                assert pc in body.code,hex(pc)
                trace.append(pc);covered.add(pc)
            def memory_access(_g,access,address,size,value,_user):
                from unicorn import UC_MEM_WRITE
                pc=g.reg_read(R.UC_X86_REG_EIP)
                if access==UC_MEM_WRITE:
                    stores.append([pc,address,bytes(g.mem_read(address,size)).hex(),
                        (value&((1<<(size*8))-1)).to_bytes(size,'little').hex()])
                if address<DEST+20 and DEST<address+size:
                    assert DEST<=address and address+size<=DEST+20
                    offsets=set(range(address-DEST,address-DEST+size))
                    if access==UC_MEM_WRITE:initialized.update(offsets)
                    elif not offsets<=initialized:read_before_write.append(pc)
                    accesses.append([pc,access,address,size])
            g.hook_add(UC_HOOK_CODE,instruction)
            g.hook_add(UC_HOOK_MEM_WRITE,memory_access);g.hook_add(UC_HOOK_MEM_READ,memory_access)
            g.emu_start(ENTRY,STOP,count=1000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP and initialized==set(range(20)) and not read_before_write
            cases.append(dict(profile=profile,poison=poison,patches=patches,gp=initial,xmm=xmm,
                final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                trace=trace,stores=stores,accesses=accesses,output=bytes(g.mem_read(DEST,20)).hex()))
    for profile in range(5):assert len({row['output'] for row in cases if row['profile']==profile})==1
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        caller_entry=0x1b08b0,consumer_entry=ENTRY,destination=DEST,bytes=20,
        loader_zero_tail=True,complete_consumer_instructions=len(body.code),covered_consumer_instructions=len(covered),
        native_component_banks=[[BASE,0x10000],[0x4f0414,4]],cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify:assert args.fixture.read_bytes()==data,'Original working-mask reference changed'
    elif args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve prior evidence; choose a new fixture path'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        destination=DEST,bytes=20,loader_zero_tail=True,caller_site=0x1b08d9,consumer_entry=ENTRY,
        destination_read_before_full_source_write=False,poison_does_not_change_source_result=True,
        complete_consumer_instructions=len(body.code),covered_consumer_instructions=len(covered),cases=len(cases),
        fixture_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(data),runtime_reference_file_required=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
