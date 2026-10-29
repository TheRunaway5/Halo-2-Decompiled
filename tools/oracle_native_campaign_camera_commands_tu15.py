#!/usr/bin/env python3
"""Offline unchanged physical TU1.5 cinematic reset/stop/FOV reference.

The FOV command's overwritten argument word is retired private scratch. The
two public argument reads remain lazy/faultable; no instruction is patched,
dependency outcome supplied, or stopped instruction resumed.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
ENTRIES=(0x13ea40,0x13f560,0x16f5a0)
DIRECTOR,CINEMATIC,GAME,SIMULATION,USERS,RECORDING,ENABLED,SCREEN=0x1001000,0x1002000,0x1003000,0x1005000,0x1006000,0x1007000,0x1007100,0x1009000
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(0x1000000,0x10000),'globals':(0x4e9000,0x36000),'constants':(0x46a000,0x1000),'fov_defaults':(0x55d000,0x1000),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_camera_commands_tu15.cpp'),'-o',str(p/'observer')]):
        result=subprocess.run(command,text=True,capture_output=True);assert not result.returncode,result.stderr
    return p
def inputs(image):
    rows=[]
    def make(entry,name,screen=SCREEN,cinematic=CINEMATIC,director=DIRECTOR,game_mode=1,active=1,current=0.,target=1.2,elapsed=.5,duration=1.,degrees=60.,ticks=30,changes=(),input_changes=(),**fields):
        n=len(rows);banks={}
        def mount(at,size):
            begin,end=at,at+size;overlap=[]
            for p,b in banks.items():
                if p<end and begin<p+len(b):begin=min(begin,p);end=max(end,p+len(b));overlap.append(p)
            raw=bytearray(((i+begin)*13+n*7+3)&255 for i in range(end-begin))
            for p in overlap:
                b=banks.pop(p);raw[p-begin:p-begin+len(b)]=b
            banks[begin]=raw
        def put(at,data):
            for p,b in banks.items():
                if p<=at and at+len(data)<=p+len(b):b[at-p:at-p+len(data)]=data;return
            mount(at,len(data));put(at,data)
        def word(at,value,width=4):put(at,int(value&((1<<(width*8))-1)).to_bytes(width,'little'))
        for p,size in ((DIRECTOR,0x80),(CINEMATIC,0x24),(GAME,0x1200),(SIMULATION,0x100),(USERS,0x40),(RECORDING,0x40),(ENABLED,4)):
            mount(p,size)
        if screen:mount(screen,0x3fc)
        if cinematic not in (CINEMATIC,0,0x40000000):mount(cinematic,0x24)
        if director not in (DIRECTOR,0,0x40000000):mount(director,0x80)
        for p,value in ((0x4eabc4,SIMULATION),(0x4f3218,GAME),(0x4f54f0,USERS),(0x5020a8,RECORDING),(0x51df50,cinematic),(0x51df6c,director),(0x515ec0,screen),(0x515e2c,ENABLED)):
            word(p,value)
        word(0x515ef5,1,1);word(GAME+8,game_mode)
        if cinematic not in (0,0x40000000):word(cinematic+5,active,1)
        for p in(0x46a0d0,0x46a164,0x46a17c):put(p,image.read(p,4))
        # Actual381710/381730/381750 copy these original operands to BSS.
        for p,source in((0x55d6e4,0x46ab88),(0x55d6e8,0x46a164),(0x55d6ec,0x46a5f8)):put(p,image.read(source,4))
        if director not in (0,0x40000000):
            for off,f in((4,elapsed),(8,duration),(12,current),(16,target)):word(director+off,bits(f))
        for p,value in changes:word(p,value)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        rounding=fields.pop('rounding',n%4);denorm=fields.pop('denorm',(0,0x40,0x8000,0x8040)[n%4])
        rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=bits(degrees),source=ticks&0xffffffff,
            banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(rounding<<13)|denorm|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=list(input_changes),**fields))
    for mode in(0,1,2,0xffffffff):make(0x13ea40,f'reset_mode_{mode}',game_mode=mode)
    for cine in(CINEMATIC,GAME,0x46a0d0):make(0x13ea40,f'reset_alias_{cine:x}',cinematic=cine)
    for screen in(0,SCREEN,SCREEN+1,SCREEN+3,0x515ec0,0x46a000,CINEMATIC):
        for active in(0,1,255):make(0x13f560,f'stop_{screen:x}_{active}',screen=screen,active=active)
    for state in(0,CINEMATIC):
        for active in(0,1,255):
            for current in(0.,-0.,1.):make(0x16f5a0,f'fov_admission_{state:x}_{active}_{current}',cinematic=state,active=active,current=current)
    for elapsed,duration in((-.5,1.),(0.,1.),(.5,1.),(1.,1.),(2.,1.),(-2.,-1.),(0.,0.),(-1.,0.)):
        for r in range(4):make(0x16f5a0,f'fov_fraction_{elapsed}_{duration}_{r}',elapsed=elapsed,duration=duration,rounding=r,current=.8)
    for ticks in(-32768,-1,0,1,32767,0x7fff8000,0xffff7fff):make(0x16f5a0,f'ticks_{ticks}',ticks=ticks)
    for degrees in(-100.,0.,.5,1.,90.,179.,180.,181.,400.):make(0x16f5a0,f'degrees_{degrees}',degrees=degrees)
    exceptions=(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345)
    for address in(DIRECTOR+4,DIRECTOR+8,DIRECTOR+12,DIRECTOR+16,0x55d6e4,0x55d6e8,0x55d6ec,0x46a0d0,0x46a17c,0x46a164):
        for value in exceptions:make(0x16f5a0,f'exception_{address:x}_{value:x}',changes=((address,value),),current=.8)
    for value in exceptions:
        for r in range(4):
            make(0x16f5a0,f'degrees_exception_{value:x}_{r}',rounding=r);rows[-1]['matrix']=value
    for d in(0,0x40,0x8000,0x8040):
        for r in range(4):make(0x16f5a0,f'controls_{d}_{r}',denorm=d,rounding=r,current=.7,target=1.8,elapsed=.3,duration=.7)
    for first,second in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
        for a,b in((DIRECTOR+12,DIRECTOR+16),(DIRECTOR+4,DIRECTOR+8),(DIRECTOR+16,0x46a0d0)):
            make(0x16f5a0,f'paired_nan_{a:x}_{b:x}_{first:x}_{second:x}',changes=((a,first),(b,second)),current=.8)
    for director in(0x55d6e0,0x46a160,CINEMATIC):make(0x16f5a0,f'fov_alias_{director:x}',director=director,current=.8)
    make(0x13ea40,'dynamic_game_root',input_changes=[dict(after_store=12,address=0x4f3218,bytes=(GAME+0x100).to_bytes(4,'little').hex())],changes=((GAME+0x108,2),))
    make(0x13f560,'dynamic_cached_users',input_changes=[dict(after_store=0,address=0x4f54f0,bytes=(USERS+0x10).to_bytes(4,'little').hex())])
    make(0x16f5a0,'dynamic_cached_director',input_changes=[dict(after_store=0,address=0x51df6c,bytes=(DIRECTOR+0x40).to_bytes(4,'little').hex())])
    for entry in ENTRIES:make(entry,f'fault_seed_{entry:x}',current=.8)
    make(0x16f5a0,'fault_seed_fov_invalid',degrees=400.,current=.8)
    make(0x16f5a0,'fault_seed_fov_default',current=0.)
    make(0x13ea40,'actual_reset_null_read',cinematic=0,actual_write_fault=True)
    make(0x16f5a0,'actual_fov_missing_director',director=0x40000000,actual_read_fault=True)
    make(0x13f560,'actual_stop_missing_screen',screen=0,changes=((0x515ec0,0x40000000),),actual_write_fault=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    for seed in(r for r in rows if r['name'].startswith('fault_seed')):
        for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
            indices=range(len(events))if phase=='before_read' or len(events)<30 else sorted({0,1,6,7,8,134,260,len(events)-1}&set(range(len(events))))
            for n in indices:
                row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,retired_private_sites=[0x16f65d,0x16f663],cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical camera command repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=3,original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_camera_commands_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_camera_commands_tu15.cpp')},scope_limits=['Masked SIMD exception profile; no unmasked trap platform owner','Ordinary void caller; overwritten private argument-slot scratch and dead volatile registers/flags retired','Full cinematic startup/director/animation/observer/New Game native closure remains open'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical camera command cases/all{len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
