#!/usr/bin/env python3
"""Unchanged physical157EB0 ordinary reset and genuine246190 required boundary."""
import argparse,copy,gzip,json,os,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
DIRECTOR=0x1001000
def prepare(image,meta,scratch):
    p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
    regions={'data':(0x1000000,0x10000),'globals':(0x4e9000,0x89000),'constants':(0x46a000,0x1000),'selectors':(0x48c000,4096),'stack':(geometry.STACK,65536)}
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
    for command in (['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_camera_reset_tu15.cpp'),'-o',str(p/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
    return p
def inputs(image):
    rows=[]
    def make(name,mode=0,skip=(),changes=(),director=DIRECTOR,**fields):
        n=len(rows);banks={0x4f5504:bytearray((k*13+n*7+3)&255 for k in range(0x510)),0x51df6c:bytearray(director.to_bytes(4,'little')),DIRECTOR:bytearray((k*7+n*3+1)&255 for k in range(0x80)),0x46a0d0:bytearray(image.read(0x46a0d0,4))}
        for k in range(4):banks[0x48c4b8+k*0x20]=bytearray(((n+k*5)*17).to_bytes(4,'little'))
        external=[]
        if mode:external.append(dict(after_store=324,address=0x4f550c,bytes=(mode&0xffffffff).to_bytes(4,'little').hex()))
        for view in skip:external.append(dict(after_store=324,address=0x4f5514+view*0x140,bytes='01'))
        external+=list(changes)
        gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
        rows.append(dict(id=n,name=name,entry=0x157eb0,gp=gp,matrix=0,source=0,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|(n*3%64),0x1f80|((n%4)<<13)|(n*7%64)],live_x87=[] if n%2 else ['0000000000000080ff3f'],input_changes=external,**fields))
    for k in range(16):make(f'ordinary_cold_{k}')
    for mode in(0,1,3,4,5,0xffffffff,0x80000000):
        for skipped in((),(0,),(1,3),(0,1,2,3)):make(f'mode_{mode:x}_skip_{skipped}',mode=mode,skip=skipped)
    for skip in((),(0,),(0,1),(0,1,2)):
        make(f'genuine_mode2_required_{len(skip)}',mode=2,skip=skip,required_owner=0x246190)
    make('mode2_all_already_active',mode=2,skip=(0,1,2,3))
    make('dynamic_selector_refresh',changes=[dict(after_store=333,address=0x48c4b8,bytes='78563412')])
    make('dynamic_director_refresh',changes=[dict(after_store=346,address=0x51df6c,bytes=(DIRECTOR+0x20).to_bytes(4,'little').hex())])
    make('fault_seed_reset')
    make('actual_missing_director',director=0x40000000,actual_write_fault=True)
    return rows
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x157eb0,);meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
    seed=next(r for r in rows if r['name']=='fault_seed_reset')
    for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
        indices=range(len(events))if phase=='before_read' else sorted({0,1,127,321,322,323,324,325,326,327,328,329,330,331,332,333,334,345,346,len(events)-2,len(events)-1}&set(range(len(events))))
        for n in indices:
            row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
    rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited]
    assert set(uncovered)=={0x157f10,0x157f18,0x157f20,0x157f23,0x157f2a},uncovered
    fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
    if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical ordinary reset repeat differs'
    report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=1,required_owner_cases=4,original_sites=len(visited),unqualified_mode2_post_call_sites=uncovered,original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_camera_reset_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_camera_reset_tu15.cpp')},scope_limits=['Ordinary DF=0 and disjoint private loop/caller scratch','Mode2 retains genuine native246190 boundary; no dependency outcome is supplied and fivepost-call sites are unqualified','Full controller/debugcallback/camera/observer/New Game native closure remains open'])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical reset cases/{len(visited)} ordinary sites +4 genuine required-owner entries')
if __name__=='__main__':main()
