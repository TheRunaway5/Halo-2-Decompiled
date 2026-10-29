#!/usr/bin/env python3
"""Physical unchanged TU1.5 object-to-actor reference."""
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
.section .object_binding,"aw"
.space 4096
.section .high,"aw"
.space 8192
.section .policy,"aw"
.space 4096
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
    (owned/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .bank 0x{BASE:x}:{{*(.bank)}} .actor_binding 0x502000:{{*(.actor_binding)}} .query_binding 0x50e000:{{*(.query_binding)}} .object_binding 0x4ec000:{{*(.object_binding)}} .high 0xffffb000:{{*(.high)}} .policy 0x47d000:{{*(.policy)}} .clock_binding 0x51d000:{{*(.clock_binding)}} .stack 0x{geometry.STACK:x}:{{*(.stack)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in (['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')],['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_data_array_allocate.cpp'),'-o',str(owned/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True)
        assert result.returncode==0,result.stderr
    return owned

def inputs(image):
    rows=[];objects=BASE+0x1000;record_base=BASE+0x5000
    def make(kind=0,datum=0xe3610001,record=record_base,actor=0xe3620002,extension_actor=0xe3630003,extension_gate=0,offset=0x200,name='ordinary'):
        n=len(rows);raw=bytearray((k*13+n*7+3)&255 for k in range(0x9000));regions={BASE:raw}
        if record>=0xffffb000:regions[0xffffb000]=bytearray([0x67]*8192)
        def put(at,value,width=4):
            for first,region in regions.items():
                off=(at&0xffffffff)-first
                if 0<=off and off+width<=len(region):region[off:off+width]=int(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError((hex(at),width))
        put(POOL+0x44,objects);put(objects+(datum&0xffff)*12+8,record)
        put(record+0xaa,kind,1);put(record+0x12c,actor);put(record+0x134,extension_gate);put(record+0x13a,offset,2)
        extension=(record+offset)&0xffffffff
        if extension:put(extension+4,extension_actor)
        binding=bytearray([0x57]*4096);struct.pack_into('<I',binding,0xbd0,POOL)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=datum;gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=f'{name}_{n}',entry=0x1eb730,object=datum,matrix=0,source=0,gp=gp,banks=sorted([dict(address=0x4ec000,before=binding.hex()),*[dict(address=at,before=data.hex())for at,data in regions.items()]],key=lambda b:b['address']),before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
    for kind in range(256):make(kind=kind,name='all_object_kinds')
    for gate in (0,1,0xffffffff):
        for offset in (-0x100,0,1,0x200,0x1000):make(kind=12,offset=offset,extension_gate=gate,name='extension_gate_offset')
    for datum in (0,1,0xffff0002,0xe3610003):
        for kind in (0,1,12,32,33):make(datum=datum,kind=kind,name='salted_objects')
    for actor in (0,0xe3620002,0xffffffff):make(actor=actor,name='actor_values')
    make(kind=12,record=0xffffc000,offset=0x4000,name='actual_wrapped_null_extension')
    make(name='fault_seed_direct');make(kind=12,name='fault_seed_extension');make(kind=12,extension_gate=1,name='fault_seed_extension_disabled')
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args();assert not args.output.exists()
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x1eb730,);meta=common.metadata(image)
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
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in (r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));assert len(visited)==sum(v["sites"]for v in meta["owners"].values()),len(visited)
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_reads=sum(len(r['reads'])for r in rows),original_stores=sum(len(r['writes'])for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture_bytes=args.output.stat().st_size,fixture_sha256=sha(args.output),fixture=str(args.output),profile='ordinary DF-clear ABI; read-only typed object-actor dependency; no numeric transport or full behavior initializer claim',owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_data_array_allocate.cpp':sha(ROOT/'tools/observe_native_tu15_data_array_allocate.cpp')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical original allocation object-actor comparisons; fixture{args.output.stat().st_size}bytes')
if __name__=='__main__':main()
