#!/usr/bin/env python3
"""Physical unchanged TU1.5 collection rectangle, including aliases/fault prefixes."""
import argparse,copy,gzip,json,os,random,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,CAMERA,DEST,SCENARIO=0x1000000,0x1000100,0x1000300,0x1000500
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(BASE,4096),'modifier':(0x4c6000,4096),'scenario':(0x4ec000,4096),'constants':(0x46a000,4096),'stack':(geometry.STACK,65536)}
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
    for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_view_rectangle_tu15.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(name,camera=CAMERA,destination=DEST,outer=(0,0,480,640),safe=(0,0,480,640),override=0,offset=0,scenario=SCENARIO,kind=0,changes=(),input_changes=(),value_profile=False,**fields):
        n=len(rows);data=bytearray((k*13+n*7+3)&255 for k in range(0x800));banks={BASE:data,0x46a000:bytearray(image.read(0x46a000,0x300)),0x4c6839:bytearray([0x96]*16),0x4ecc20:bytearray(4)}
        def put(at,value,width=4):
            raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
            for p,b in banks.items():
                if p<=at and at+width<=p+len(b):b[at-p:at-p+width]=raw;return
            banks[at]=bytearray(raw)
        for off,values in((0x30,outer),(0x38,safe)):
            for k,value in enumerate(values):put(CAMERA+off+k*2,value,2)
        put(CAMERA+0x58,override,1);put(CAMERA+0x68,offset,1)
        for off,value in((0x5c,.125),(0x60,-.25),(0x64,.7),(0x6c,12.5),(0x70,-18.25)):put(CAMERA+off,bits(value))
        put(0x4ecc20,scenario);put(SCENARIO+0x10,kind,2)
        for address,value,width in changes:put(address,value,width)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=camera;gp[6]=destination;gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=name,entry=0x2f950,gp=gp,matrix=0,source=0,camera=camera,destination=destination,value_profile=value_profile,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=input_changes,**fields))
    for override in(0,1,2,255):
        for offset in(0,1,2,255):
            for kind in(0,2,-1):make(f'flags_{override}_{offset}_{kind}',override=override,offset=offset,kind=kind,value_profile=True)
    for override,offset in((0,0),(1,0),(0,1)):
        make(f'null_scenario_{override}_{offset}',override=override,offset=offset,scenario=0,value_profile=True)
        for outer,safe in[((-7,-9,271,519),(-11,-3,301,607)),((0,0,0,640),(0,0,480,640)),((0,0,480,0),(0,0,480,640)),((0,0,480,640),(0,0,0,640)),((32767,-32768,-32768,32767),(-32768,32767,32767,-32768)),((0,0,-1,-1),(-1,-1,0,0))]:make(f'signed_dimensions_{override}_{offset}_{outer}_{safe}',override=override,offset=offset,outer=outer,safe=safe,value_profile=True)
    for address in(CAMERA+0x5c,CAMERA+0x60,CAMERA+0x64,CAMERA+0x6c,CAMERA+0x70,0x46a11c,0x46a0cc,0x46a0d0,0x46a264,0x46a260):
        for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
            make(f'fp_{address:x}_{value:x}',override=int(address not in(CAMERA+0x6c,CAMERA+0x70,0x46a264,0x46a260)),offset=1,changes=((address,value,4),),value_profile=True)
    for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
        for first,second in((CAMERA+0x5c,0x46a0d0),(CAMERA+0x60,CAMERA+0x64),(0x46a11c,0x46a0cc),(CAMERA+0x6c,0x46a264)):
            make(f'paired_nan_{first:x}_{second:x}_{a:x}_{b:x}',override=int(first!=CAMERA+0x6c),offset=1,changes=((first,a,4),(second,b,4)),value_profile=True)
    for destination in(CAMERA+0x30,CAMERA+0x32,CAMERA+0x34,CAMERA+0x36,CAMERA+0x38,CAMERA+0x3a,CAMERA+0x58,CAMERA+0x5c,CAMERA+0x64,CAMERA+0x68,CAMERA+0x6c,SCENARIO+0x10,0x4c6839,0x46a0cc):
        for override,offset in((0,0),(1,0),(0,1)):make(f'alias_{destination:x}_{override}_{offset}',destination=destination,override=override,offset=offset)
    for k in range(20):
        rng=random.Random(790+k);make(f'rounding_{k}',outer=tuple(rng.randrange(-32768,32768)for _ in range(4)),safe=tuple(rng.randrange(-32768,32768)for _ in range(4)),override=k%2,offset=1,value_profile=True)
    for override,offset in((0,0),(1,0),(0,1)):
        for address,value,width in((CAMERA+0x36,127,2),(CAMERA+0x58,0,1),(CAMERA+0x68,1,1),(CAMERA+0x64,0x3f000001,4),(0x4ecc20,0,4)):
            make(f'changed_after_store_{override}_{offset}_{address:x}',override=override,offset=offset,input_changes=[dict(after_store=0,address=address,bytes=(value&((1<<(width*8))-1)).to_bytes(width,'little').hex())])
    for override,offset in((0,0),(1,0),(0,1)):
        make(f'fault_seed_{override}_{offset}',override=override,offset=offset)
        make(f'fault_seed_alias_{override}_{offset}',override=override,offset=offset,destination=CAMERA+0x34)
    make('actual_missing_camera',camera=0x40000000,actual_read_fault=True)
    make('actual_missing_destination',destination=0x40000000,actual_write_fault=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x2f950,);meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            for n in range(len(events)):
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical rectangle repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),value_profiles=sum(r['value_profile']for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_view_rectangle_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_view_rectangle_tu15.cpp')},scope_limits=['Full2F950 raw alias/fault and disjoint native-value output profiles','Actual2B8B4 caller transport only; no complete2B770/2B9F0 or rendering claim','Masked SIMD exceptions; x87 state unchanged'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical rectangle cases/all{len(visited)} sites')
if __name__=='__main__':main()
