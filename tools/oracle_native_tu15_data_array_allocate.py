#!/usr/bin/env python3
"""Physical unchanged16E6A0 allocation reference; no supplied outcomes."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,POOL,BITS,ROWS=0x1000000,0x1000100,0x1000500,0x1001000
sha=common.sha

def prepare(image,meta,scratch):
    owned=scratch/'physical-original';owned.mkdir();section=image.section('.text')
    (owned/'text.bin').write_bytes(image.section_bytes(section))
    (owned/'metadata.json').write_text(json.dumps(meta))
    (owned/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{owned/'text.bin'}"
.section .bank,"aw"
.space 65536
.section .stack,"aw"
.space 65536
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''')
    (owned/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .bank 0x{BASE:x}:{{*(.bank)}} .stack 0x{geometry.STACK:x}:{{*(.stack)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_data_array_allocate.cpp'),'-o',str(owned/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True)
        assert result.returncode==0,result.stderr
    return owned

def inputs():
    rows=[]
    def make(cursor=0,extent=0,capacity=8,stride=12,used=0,salt=0x8000,occupied=(),bits=BITS,record=ROWS,name='ordinary',pool=POOL):
        n=len(rows);raw=bytearray((k*13+n*7+3)&255 for k in range(0x3000))
        def put(at,value,width=4):
            raw[at-BASE:at-BASE+width]=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
        for off in range(-4,20,4):put(bits+off,0)
        for i in occupied:
            at=bits+(i>>5)*4;value=int.from_bytes(raw[at-BASE:at-BASE+4],'little');put(at,value|(1<<(i&31)))
        for offset,value in ((0x20,capacity),(0x24,stride),(0x34,cursor),(0x38,extent),(0x3c,used),(0x44,record),(0x48,bits)):put(pool+offset,value)
        put(pool+0x40,salt,2)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[2]=pool;gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=f'{name}_{n}',entry=0x16e6a0,matrix=0,source=0,gp=gp,banks=[dict(address=BASE,before=raw.hex())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
    for stride in range(0,18):
        for salt in (0,0x7fff,0x8000,0xfffd,0xfffe,0xffff):make(stride=stride,salt=salt,name='append_clear_salt')
    for cursor in (0,1,30,31,32,33,63):
        for free in (cursor,cursor+1,cursor+3):
            make(cursor=cursor,extent=free+2,capacity=free+5,occupied=range(cursor,free),stride=4,used=0xffffffff,name='scan_word_boundary')
    for cursor,extent,capacity in ((0,8,8),(8,8,8),(9,8,8),(0,0,0),(0,0,-1),(0,-1,8),(-1,0,8),(-2,0,8),(-1,-1,8),(-2,-1,8),(0,8,4)):
        make(cursor=cursor,extent=extent,capacity=capacity,occupied=range(max(cursor,0),max(extent,0)),name='capacity_signed')
    for p in (POOL+0x20,POOL+0x24,POOL+0x34,POOL+0x38,POOL+0x3c,POOL+0x40,POOL+0x44,POOL+0x48,BITS,BITS+1):
        make(record=p,stride=5,name='record_alias')
    for offset in (0x20,0x24,0x34,0x38,0x3c,0x40,0x44):
        make(bits=POOL+offset,extent=0,name='bitmap_header_alias')
    for alignment in (1,3):make(pool=POOL+alignment,bits=BITS+alignment,record=ROWS+alignment,name='unaligned')
    make(cursor=30,extent=35,occupied=(30,31,32),stride=13,salt=0xfffe,name='fault_seed_scan')
    make(record=POOL+0x3c,stride=9,salt=0xfffe,name='fault_seed_alias')
    make(extent=8,capacity=8,cursor=8,name='fault_seed_refuse')
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x16e6a0,);meta=common.metadata(image)
    # REP with count zero executes no operand access. The dedicated physical
    # observer checks real ECX; it does not infer an allocation result.
    for site in meta['sites']:
        if site['bytes'].startswith('f3'):site['repeat_count_register']=1
        # Capstone reports no operand access for85/TEST despite its real
        # memory read. Authenticate the unchanged instruction before repairing
        # observer metadata; original executable bytes stay untouched.
        if site['instruction']==0x16e6c5:
            assert site['bytes']=='852c8b' and len(site['operands'])==1
            site['operands'][0]['read']=True
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));assert len(visited)==78,len(visited)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_reads=sum(len(r['reads'])for r in rows),original_stores=sum(len(r['writes'])for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),fixture=str(args.output),profile='ordinary DF-clear ABI; signed indices/header aliases and byte/word clears retained; no callbacks',owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_data_array_allocate.cpp':sha(ROOT/'tools/observe_native_tu15_data_array_allocate.cpp')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical original allocation comparisons,78sites; fixture{args.output.stat().st_size}bytes')
if __name__=='__main__':main()
