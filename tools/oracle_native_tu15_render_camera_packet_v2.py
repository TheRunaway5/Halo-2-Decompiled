#!/usr/bin/env python3
"""Full unchanged physical2F7E0 null/default x87 and380450 initialization reference."""
import argparse,copy,gzip,json,os,struct,subprocess,sys
from pathlib import Path
sys.dont_write_bytecode=True
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,DEST,GRID,SPANS=0x1000000,0x1000200,0x1000300,0x1000310
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(BASE,4096),'factor':(0x45c000,4096),'ratio':(0x456000,4096),'constants':(0x46a000,4096),'ranges':(0x492000,4096),'vectors':(0x44a000,4096),'pointers':(0x474000,4096),'fov':(0x532000,4096),'stack':(geometry.STACK,65536)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
    for name,(_,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
    (p/'reference.s').write_text(assembly);(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_render_camera_packet_v2.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p

def inputs(image):
    rows=[]
    ordinary=struct.pack('<QH',0xc000000000000000,0x3fff).hex()
    zero=struct.pack('<QH',0,0).hex()
    def make(name,entry=0x2f7e0,destination=DEST,fov=None,depth=None,occupied=None,older=None,changes=(),input_changes=(),**fields):
        n=len(rows);depth=(0,1,3,6,7,8)[n%6]if depth is None else depth
        mask=(1<<depth)-1 if occupied is None else occupied
        values=[ordinary]*8
        if older is not None:values[0]=older
        banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x1000)),0x44ab00:bytearray(image.read(0x44ab00,0x200)),0x474ce0:bytearray(image.read(0x474ce0,0x80)),0x5320ac:bytearray(0x80),0x492200:bytearray(image.read(0x492200,0x200))}
        def put(address,value,width=4):
            raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for first,bank in banks.items():
                if first<=address and address+width<=first+len(bank):bank[address-first:address-first+width]=raw;return
            banks[address]=bytearray(raw)
        for p,width in((0x46a0cc,4),(0x46a0d0,4),(0x46a13c,4),(0x46a1d0,8),(0x46ab88,4)):banks[p]=bytearray(image.read(p,width))
        put(0x5320ac,int.from_bytes(image.read(0x46ab88,4),'little')if fov is None else fov)
        for p,value in((BASE+0x600,0x40800000),(BASE+0x604,0x40a00000),(BASE+0x608,0x40c00000),(BASE+0x640,0x3f000000),(BASE+0x644,0x3f800000),(BASE+0x648,0x40000000),(BASE+0x680,0xbf800000),(BASE+0x684,0x3e800000),(BASE+0x688,0x3f000000)):put(p,value)
        put(GRID,1,2);put(GRID+2,1,2);put(SPANS,1,2);put(SPANS+2,1,2)
        for p,value,width in changes:put(p,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=0;gp[1]=destination;gp[2]=SPANS;gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=GRID,source=0,observer=0,destination=destination,spans=SPANS,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=values,x87_occupied=mask,input_changes=input_changes,**fields))
    for depth in range(9):
        for fov in(0,0x80000000,0x3f9c61aa,0x7f7fffff,0xff7fffff):make(f'depth_{depth}_fov_{fov:x}',depth=depth,fov=fov)
    special=[0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345]
    for address in(0x5320ac,0x46a0cc,0x46a13c,0x46a0d0):
        for value in special:make(f'scalar_{address:x}_{value:x}',changes=((address,value,4),))
    for value in(0,0x8000000000000000,1,0x0010000000000000,0x7fefffffffffffff,0x7ff0000000000000,0xfff0000000000000,0x7ff8123456789abc,0x7ff0123456789abc):make(f'double_{value:x}',changes=((0x46a1d0,value,8),))
    for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
        make(f'paired_fov_half_{a:x}_{b:x}',changes=((0x5320ac,a,4),(0x46a0cc,b,4)),depth=3)
    extended=[(0,0),(0,0x8000),(1,0),(0x8000000000000000,0),(0x8000000000000000,0x7fff),(0xc123456789abcdef,0x7fff),(0x8123456789abcdef,0x7fff),(0x123456789abcdef,0x3fff)]
    for significand,exponent in extended:
        for depth in(1,6,7,8):make(f'older_tag_{depth}_{significand:x}_{exponent:x}',fov=0x7f7fffff,depth=depth,older=struct.pack('<QH',significand,exponent).hex())
    for mask in(2,0x55,0xaa,0x80,0xfe):make(f'sparse_{mask:x}',occupied=mask,depth=0)
    for destination in(0x44abec,0x44ac28,0x44ac40,0x474ce8,0x474d08,0x474d10,0x5320ac,0x4922d0,GRID,DEST+1):make(f'destination_alias_{destination:x}',destination=destination,depth=3,actual_read_fault=destination in(0x474d08,0x474d10))
    for pointer,vector in((0x474ce8,BASE+0x600),(0x474d08,BASE+0x640),(0x474d10,BASE+0x680)):
        make(f'actual_dynamic_vector_{pointer:x}',changes=((pointer,vector,4),),depth=3)
        make(f'changed_vector_after_store_{pointer:x}',input_changes=[dict(after_store=0,address=pointer,bytes=vector.to_bytes(4,'little').hex())],depth=3)
    make('changed_aspect_after_vectors',input_changes=[dict(after_store=9,address=0x46a13c,bytes='0100403f')],depth=3)
    for depth in(0,1,3,6,7,8):make(f'fault_seed_default_{depth}',depth=depth)
    make('fault_seed_large_empty',fov=0x7f7fffff,depth=0)
    make('fault_seed_large_older',fov=0x7f7fffff,depth=3)
    make('fault_seed_sparse',occupied=0x55,depth=0)
    make('fault_seed_alias',destination=0x44abec,depth=3)
    make('actual_missing_default_position',changes=((0x474ce8,0x40000000,4),),actual_read_fault=True)
    make('actual_missing_default_destination',destination=0x40000000,actual_write_fault=True)
    for value in special:make(f'initialize_{value:x}',entry=0x380450,changes=((0x46ab88,value,4),))
    make('fault_seed_initialize',entry=0x380450)
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x2f7e0,0x380450);meta=common.metadata(image)
    # Capstone labels x87 FSTP's memory operand as a read. Authenticate the
    # original opcode and correct the observer descriptor to its real store;
    # no original instruction or execution outcome is changed.
    store=next(site for site in meta['sites']if site['instruction']==0x2f919)
    assert store['bytes']=='d95928'and len(store['operands'])==1
    assert store['operands'][0]['bytes']==4 and store['operands'][0]['base']==1 and store['operands'][0]['displacement']==0x28
    store['operands'][0].update(read=False,write=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));missing=[site['instruction']for site in meta['sites']if site['instruction']not in visited];assert all(p<0x2f8a5 for p in missing),missing
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical full default repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_sites=len(visited),null_branch_sites=sum(0x2f8a5<=p<0x2f91c for p in visited),initializer_sites=sum(p>=0x380450 for p in visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_render_camera_packet_v2.cpp':sha(ROOT/'tools/observe_native_tu15_render_camera_packet_v2.cpp')},scope_limits=['New null/default and initializer reference; immutable v1 nonnull reference remains separate','Actual masked hardwarex87/SIMD including older/sparse/full-stack contexts and committed fault prefixes','No cold data mount/bootstrap activation or whole camera parent claim'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical default cases/{report["null_branch_sites"]} null sites/{report["initializer_sites"]} initializer sites')
if __name__=='__main__':main()
