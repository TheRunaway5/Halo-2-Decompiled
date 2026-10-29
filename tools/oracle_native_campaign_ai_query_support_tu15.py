#!/usr/bin/env python3
"""Physical unchanged TU1.5 query create/attach/iterator reference."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,POOL,QUERIES,BITS,ROWS,QROWS,CLOCK,STATE,ITER=0x1000000,0x1000100,0x1000200,0x1000400,0x1001000,0x1006000,0x1000300,0x1008000,0x1008100
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
.section .actor_binding,"aw"
.space 4096
.section .query_binding,"aw"
.space 8192
.section .clock_binding,"aw"
.space 4096
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
    (owned/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .bank 0x{BASE:x}:{{*(.bank)}} .actor_binding 0x502000:{{*(.actor_binding)}} .query_binding 0x50e000:{{*(.query_binding)}} .clock_binding 0x51d000:{{*(.clock_binding)}} .stack 0x{geometry.STACK:x}:{{*(.stack)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_data_array_allocate.cpp'),'-o',str(owned/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True)
        assert result.returncode==0,result.stderr
    return owned

def inputs():
    rows=[]
    def make(entry,actor=0xe3610000,slot=0,member=1,datum=0xe3620001,state=STATE,iterator=ITER,index=-1,kind=3,expiries=(11,12,13,14),kinds=(3,3,3,3),clock=10,capacity=8,extent=0,cursor=0,owner=0xe3600000,refs=1,salt=0x8000,occupied=(),name='ordinary'):
        n=len(rows);raw=bytearray((k*13+n*7+3)&255 for k in range(0x9000))
        def put(at,value,width=4):
            offset=at-BASE
            assert 0<=offset and offset+width<=len(raw),(hex(at),width)
            raw[offset:offset+width]=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
        actor_record=ROWS+(actor&0xffff)*0x888
        put(POOL+0x44,ROWS);put(actor_record+0x7c,owner)
        for offset,value in ((0x20,capacity),(0x24,0xbc),(0x34,cursor),(0x38,extent),(0x3c,0),(0x40,salt),(0x44,QROWS),(0x48,BITS)):put(QUERIES+offset,value,2 if offset==0x40 else 4)
        put(BITS,sum(1<<i for i in occupied));put(CLOCK+8,clock)
        for k in range(-1,4):
            record=actor_record+0x194+k*12
            put(record,kinds[k%4],2);put(record+2,member,2);put(record+4,datum);put(record+8,expiries[k%4])
        put(QROWS+(datum&0xffff)*0xbc+0x7c,refs,2)
        put(iterator,actor);put(iterator+4,index,2);put(iterator+6,kind,2)
        bindings=[]
        for address,size,pointer,target in ((0x502000,4096,0x5020c8,POOL),(0x50e000,8192,0x50eefc,QUERIES),(0x51d000,4096,0x51df54,CLOCK)):
            bank=bytearray([0x57]*size);struct.pack_into('<I',bank,pointer-address,target);bindings.append(dict(address=address,before=bank.hex()))
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        if entry==0x278650:gp[3]=actor
        elif entry==0x2789d0:gp[0]=actor;gp[2]=state
        else:gp[1]=iterator
        rows.append(dict(id=n,name=f'{name}_{n}',entry=entry,actor=actor,slot=slot,state=state,iterator=iterator,matrix=slot&0xffffffff,source=0,gp=gp,banks=sorted([dict(address=BASE,before=raw.hex()),*bindings],key=lambda b:b['address']),before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
    for actor in (0,1,0xe3610000,0xffff0002):
        for owner in (0,0xe3600000,0xffffffff):
            for capacity in (0,8):make(0x278650,actor=actor,owner=owner,capacity=capacity,name='create_admission')
    for salt in (0,0x8000,0xfffe,0xffff):make(0x278650,salt=salt,name='create_salt')
    make(0x278650,cursor=0,extent=2,name='create_reuse');make(0x278650,cursor=0,extent=2,occupied=(0,1),name='create_scan_append');make(0x278650,cursor=8,extent=8,name='create_exhausted')
    for slot in (-1,0,1,2,3):
        for member in (-1,0,1,9):
            for refs in (0,0xffff):make(0x2789d0,slot=slot,member=member,refs=refs,name='attach_slot_member')
    for state in (ROWS+0x194-0x10,ROWS+0x194-0x18,ROWS+0x194+4-0x10,QUERIES+0x30,QROWS+0xbc+0x7c-0x18):make(0x2789d0,state=state,name='attach_alias')
    for index in (-2,-1,0,1,2,3,4,5,32767):
        for kind in (0,3,0xffff):make(0x278dd0,index=index,kind=kind,name='iterator_bound')
    for expiry in (0,9,10,11,0xffffffff,0x7fffffff,0x80000000):
        for clock in (0,10,0xffffffff,0x7fffffff,0x80000000):make(0x278dd0,expiries=(expiry,)*4,clock=clock,name='iterator_signed_expiry')
    for candidate in range(4):
        make(0x278dd0,kinds=tuple(3 if k==candidate else 1 for k in range(4)),name='iterator_match_position')
    for iterator in (ROWS+0x194,ROWS+0x194-4,CLOCK+4):make(0x278dd0,iterator=iterator,name='iterator_alias')
    make(0x278650,name='fault_seed_create');make(0x2789d0,name='fault_seed_attach');make(0x2789d0,state=ROWS+0x194-0x10,name='fault_seed_attach_alias');make(0x278dd0,kinds=(1,3,3,3),expiries=(10,10,11,14),name='fault_seed_iterator')
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x278650,0x16e6a0,0x2789d0,0x278dd0);meta=common.metadata(image)
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
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));assert len(visited)==190,len(visited)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_reads=sum(len(r['reads'])for r in rows),original_stores=sum(len(r['writes'])for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),fixture=str(args.output),profile='ordinary DF-clear ABI; typed argument values; no numeric transport or full behavior initializer claim',owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_data_array_allocate.cpp':sha(ROOT/'tools/observe_native_tu15_data_array_allocate.cpp')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical original allocation comparisons,190sites; fixture{args.output.stat().st_size}bytes')
if __name__=='__main__':main()
