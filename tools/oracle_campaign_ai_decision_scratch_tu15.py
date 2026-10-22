#!/usr/bin/env python3
"""Execute the original per-actor decision gathering phase without substitutions.

Whole1B08B0 reaches1B09E0 after its actual mask/context producers and exactly
three pending entries. Compare the source prefix only, never supply later
behavior callbacks. The original131-kind, three-lane,24-byte matrix, counts,
five-word bitmap and context scalar retain their loader-zero provenance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE,UC_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_campaign_ai_working_mask_tu15 import BASE,STOP,DEST,ACTORS,ROWS,TAGS,CHAR,SQUADS,SQUAD_ROWS,STACK,GP

PREFIX_END=0x1b09e0
REGIONS=((0x4fafc0,4),(0x4fafc8,3*131*24),(0x4fd4a0,131),(0x4fd524,5*4))


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    ap.add_argument('--verify',action='store_true');args=ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:ap.error('Run through tools/run_test.py')
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0x1b08b0,{0x1b08b0})
    assert image.read(0x1b092b,5).hex()=='bf03000000'
    assert image.read(0x1b0990,6).hex()=='69c983000000'
    for address,size in REGIONS:
        section=next(s for s in image.sections if s.va<=address<address+size<=s.va+s.vsize)
        assert address>=section.va+section.rsize and image.read(address,size)==bytes(size)
    cases=[];covered=set()
    kinds=((0,31,130),(31,32,65),(130,130,130),(-1,-1,-1),(0,0,0),(129,-1,128))
    for selection in kinds:
        for poison in (0,0xa5,255):
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
            for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
            g.mem_map(BASE,0x10000);g.mem_map(STOP,4096);patches=[]
            def put(p,v,n=4):
                data=(v&((1<<(8*n))-1)).to_bytes(n,'little');g.mem_write(p,data);patches.append([p,data.hex()])
            for p,v in ((0x5020c8,ACTORS),(ACTORS+0x44,ROWS),(0x4f0414,TAGS),
                        (TAGS+8,CHAR),(0x52be7c,SQUADS),(SQUADS+0x44,SQUAD_ROWS)):
                put(p,v)
            put(ROWS+0x858,0xffffffff);put(ROWS+0x30,0);put(ROWS+0x54,0)
            put(ROWS+0x26c,0xffffffff);put(CHAR+0x20,0xffffffff)
            put(0x47b908,0x12345678)
            for i in range(5):put(SQUAD_ROWS+0x84+i*4,0xffffffff-i)
            for lane,kind in enumerate(selection):
                put(ROWS+0x1c4+lane*12,0xffffffff if kind<0 else 0xabcd0000+lane)
                put(ROWS+0x1c8+lane*12,kind,2)
            put(STACK,STOP);put(STACK+4,0xa1b20000)
            g.mem_write(DEST,bytes([poison])*20)
            for address,size in REGIONS:g.mem_write(address,bytes([poison])*size)
            initial=[0x11223344,0x22334455,0x33445566,0x44556677,STACK,0x55667788,0x66778899,0x778899aa]
            for reg,value in zip(GP,initial):g.reg_write(reg,value)
            xmm=[[0x3f000000+i*4+lane for lane in range(4)] for i in range(8)]
            for i,lanes in enumerate(xmm):g.reg_write(R.UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*lanes),'little'))
            g.reg_write(R.UC_X86_REG_EFLAGS,0x202);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            trace=[];stores=[];written=set();bad_reads=[]
            watch=lambda at:any(p<=at<p+n for p,n in (*REGIONS,(DEST,20)))
            def instruction(_g,pc,size,_u):
                assert bytes(g.mem_read(pc,size))==image.read(pc,size)
                trace.append(pc);covered.add(pc)
            def access(_g,kind,address,size,value,_u):
                pc=g.reg_read(R.UC_X86_REG_EIP);watched={p for p in range(address,address+size) if watch(p)}
                if kind==UC_MEM_WRITE:
                    stores.append([pc,address,bytes(g.mem_read(address,size)).hex(),
                        (value&((1<<(size*8))-1)).to_bytes(size,'little').hex()]);written.update(watched)
                elif not watched<=written:bad_reads.append(pc)
            g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,access);g.hook_add(UC_HOOK_MEM_READ,access)
            g.emu_start(0x1b08b0,PREFIX_END,count=10000)
            assert g.reg_read(R.UC_X86_REG_EIP)==PREFIX_END and not bad_reads
            assert len(written&set(range(0x4fd524,0x4fd538)))==20
            assert len(written&set(range(0x4fafc0,0x4fafc4)))==4
            touched={kind for kind in selection if kind>=0}
            for kind in touched:assert int.from_bytes(g.mem_read(0x4fd4a0+kind,1),'little')==selection.count(kind)
            matrix_written={p for p in written if 0x4fafc8<=p<0x4fd4a0}
            # Original packed stores leave byte7,10/11 and tail20..23 intact.
            # Preserve that prior padding; only17 bytes of each24-byte record
            # are written, with the last DWORD ending at byte20.
            assert len(matrix_written)==sum(k>=0 for k in selection)*17
            cases.append(dict(selection=selection,poison=poison,patches=patches,gp=initial,xmm=xmm,
                final_gp=[g.reg_read(reg) for reg in GP],final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                final_xmm=[list(struct.unpack('<4I',g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                trace=trace,stores=stores,outputs=[[p,bytes(g.mem_read(p,n)).hex()] for p,n in (*REGIONS,(DEST,20))]))
    fixture=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,instruction_patches=[],supplied_dependency_returns=[],
        entry=0x1b08b0,prefix_end=PREFIX_END,regions=REGIONS,mask_destination=DEST,
        native_component_banks=[[BASE,0x10000],[0x4f0414,4],[0x47b908,4]],cases=cases)
    data=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
    if args.verify:assert args.fixture.read_bytes()==data
    elif args.fixture.exists():assert args.fixture.read_bytes()==data,'Preserve prior original evidence'
    else:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(data)
    report=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        cases=len(cases),complete_parent_instructions=len(body.code),executed_prefix_instructions=len(covered),
        regions=REGIONS,loader_zero_tail=True,watched_read_before_source_write=False,
        maximum_records_per_kind=3,record_stride=24,record_written_bytes=17,
        record_write_extent=20,preserved_record_padding=[7,10,11,20,21,22,23],kinds=131,
        fixture_bytes=len(data),fixture_sha256=hashlib.sha256(data).hexdigest(),complete_decision_owner_verified=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
