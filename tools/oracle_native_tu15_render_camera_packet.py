#!/usr/bin/env python3
"""Unchanged physical2F7E0 observer packet profile and actual divide/default boundaries."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,OBSERVER,DEST,GRID,SPANS=0x1000000,0x1000100,0x1000200,0x1000300,0x1000310
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(BASE,4096),'factor':(0x45c000,4096),'ratio':(0x456000,4096),'constants':(0x46a000,4096),'ranges':(0x492000,4096),'stack':(geometry.STACK,65536)}
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
    for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_render_camera_packet.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(name,observer=OBSERVER,destination=DEST,grid=GRID,spans=SPANS,grid_xy=(1,1),span_xy=(1,1),changes=(),**fields):
        n=len(rows);data=bytearray((k*13+n*7+3)&255 for k in range(0x800));banks={BASE:data}
        def put(at,value,width=4):
            raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
            if BASE<=at and at+width<=BASE+len(data):data[at-BASE:at-BASE+width]=raw
            else:banks[at]=bytearray(raw)
        for off,value in[(0,1.),(4,2.),(8,3.),(0x20,1.),(0x24,0.),(0x28,0.),(0x2c,0.),(0x30,0.),(0x34,1.),(0x4c,.7),(0x50,1.2)]:put(OBSERVER+off,bits(value))
        for address,values in((GRID,grid_xy),(SPANS,span_xy)):
            for k,value in enumerate(values):put(address+k*2,value,2)
        for p in(0x45c55c,0x46a0cc,0x456910,0x4922f4,0x4922f8):banks[p]=bytearray(image.read(p,4))
        for address,value in changes:put(address,value)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=observer;gp[1]=destination;gp[2]=spans;gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=name,entry=0x2f7e0,gp=gp,matrix=grid,source=0,observer=observer,destination=destination,spans=spans,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=[],**fields))
    for span in(0,SPANS):
        for grid in(0,GRID):make(f'nullable_split_{span:x}_{grid:x}',spans=span,grid=grid)
    for g,s in[((1,1),(1,1)),((2,-1),(1,-1)),((1,-2),(1,-1)),((1,2),(1,1)),((2,1),(1,1)),((1,1),(2,1)),((1,1),(1,2)),((-1,-1),(1,1)),((3,1),(1,1)),((1,0),(1,1)),((32767,32767),(-32768,32767)),((1,-16384),(-32768,-1)),((0,1),(1,1)),((1,1),(1,0))]:
        make(f'ratio_{g}_{s}',grid_xy=g,span_xy=s)
    for address in(OBSERVER+0x4c,OBSERVER+0x50,0x45c55c,0x46a0cc,0x456910):
        for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            make(f'fp_{address:x}_{value:x}',changes=((address,value),),grid_xy=(1,2)if address==0x46a0cc else(2,1)if address==0x456910 else(1,1))
    for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
        make(f'paired_nan_{a:x}_{b:x}',changes=((OBSERVER+0x4c,a),(0x45c55c,b)))
    for p in(OBSERVER,OBSERVER+4,OBSERVER+0x20,OBSERVER+0x2c,OBSERVER+0x4c,GRID,SPANS):make(f'destination_alias_{p:x}',destination=p)
    for p in(OBSERVER,OBSERVER+4,OBSERVER+0x20,DEST):make(f'input_split_alias_{p:x}',grid=p,spans=p+4)
    for k in range(16):make(f'rounding_denorm_{k}',changes=((OBSERVER+0x4c,0x3f123457+k),(OBSERVER+0x50,0x00800000+k)))
    make('default_camera_required',observer=0)
    make('fault_seed_packet');make('fault_seed_ratio200',grid_xy=(1,2));make('fault_seed_ratio50',grid_xy=(2,1))
    make('actual_missing_observer',observer=0x40000000,actual_read_fault=True)
    make('actual_missing_destination',destination=0x40000000,actual_write_fault=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x2f7e0,);meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert all(0x2f8a5<=s<0x2f91c for s in uncovered),uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical observer packet repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=2,actual_integer_traps=sum(r['boundary']is not None and r['boundary']['phase']=='integer_divide'for r in rows),default_projection_boundaries=1,original_sites=len(visited),unqualified_default_projection_sites=uncovered,original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_render_camera_packet.cpp':sha(ROOT/'tools/observe_native_tu15_render_camera_packet.cpp')},scope_limits=['Nonnull observer packet complete, null default remains explicit genuine2F8A5 capability boundary','Masked SIMD exceptions; signed IDIV zero/overflow is a real typed arithmetic trap','Actual229A90 caller binding only; local generateddispatch must be retired before liveuse','No full graphics/cinematic/New Game native-only closure claimed'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical packet cases/{len(visited)} observer sites')
if __name__=='__main__':main()
