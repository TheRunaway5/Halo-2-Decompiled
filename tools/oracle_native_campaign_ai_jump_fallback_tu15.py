#!/usr/bin/env python3
"""Unchanged physicalFF580 trajectory fallback, including owed native pending quantities."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,FIRST,SECOND,DIRECTION,DISTANCE,SPEED,DURATION=0x1000000,0x1000100,0x1000140,0x1000200,0x1000240,0x1000244,0x1000248
ENTRIES=(0xff580,)
sha=common.sha
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();sec=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(sec));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(BASE,0x4000),'constants':(0x46a000,0x1000),'default':(0x44a000,0x1000),'pointer':(0x474000,0x1000),'stack':(geometry.STACK,65536)}
 a=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
 for n,(_,size)in regions.items():a+=f'.section .{n},"aw"\n.space {size}\n'
 a+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
 (p/'reference.s').write_text(a);(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{sec.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{x:x}:{{*(.{n})}} 'for n,(x,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_jump_fallback_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert r.returncode==0,r.stderr
 return p

def inputs(image):
 rows=[]
 def make(name,a=(2.,1.,3.),b=(0.,0.,0.),speed=bits(2.),direction=DIRECTION,distance=DISTANCE,speed_out=SPEED,duration=DURATION,changes=(),first=FIRST,second=SECOND,rounding=None,denorm=None,precision=None,epsilon=None,**fields):
  n=len(rows);raw=bytearray(0x400)
  for p,values in((FIRST,a),(SECOND,b)):
   for k,v in enumerate(values):struct.pack_into('<I',raw,p-BASE+k*4,bits(v))
  for p,value in changes:struct.pack_into('<I',raw,p-BASE,value)
  gp=[0x5a000000+n*29+k*11 for k in range(8)];gp[0]=first;gp[2]=direction;gp[3]=speed_out;gp[7]=distance;gp[4]=geometry.SP+n%4*4
  r=n%4 if rounding is None else rounding;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  banks={BASE:raw,0x46a0d0:bytearray(image.read(0x46a0d0,4)),0x46a0ec:bytearray(image.read(0x46a0ec,4)if epsilon is None else int(epsilon).to_bytes(4,'little')),0x474d10:bytearray(image.read(0x474d10,4)),0x44ac40:bytearray(image.read(0x44ac40,12))}
  rows.append(dict(id=n,name=name,entry=0xff580,gp=gp,first=first,second=second,direction=direction,distance=distance,speed_out=speed_out,duration=duration,speed_bits=speed,banks=[dict(address=p,before=data.hex())for p,data in sorted(banks.items())],before_fp=[0x7f|(((0,2,3)[n%3]if precision is None else precision)<<8)|(r<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,6)[n%4])],**fields))
 for r in range(4):
  for d in(0,0x40,0x8000,0x8040):make(f'finite_{r}_{d:x}',rounding=r,denorm=d)
 for point in(FIRST,SECOND):
  for lane in range(3):
   for value in(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'exception_{point:x}_{lane}_{value:x}',changes=((point+lane*4,value),))
 for speed in(0,0x80000000,1,0x80000001,bits(-2.),bits(.0001),bits(.00010000001),bits(.00009999999),0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'speed_{speed:x}',speed=speed)
 for a in((0.,0.,0.),(.0001,0.,0.),(.00009999999,0.,0.),(.00010000001,0.,0.),(1e-38,1e-38,1e-38),(4096.,1.,1.)):
  for r in range(4):make(f'epsilon_{a}_{r}',a=a,rounding=r)
 for optional in range(8):make(f'optional_{optional}',distance=DISTANCE if optional&1 else 0,speed_out=SPEED if optional&2 else 0,duration=DURATION if optional&4 else 0)
 for p in(FIRST,FIRST+4,SECOND,SECOND+4,DISTANCE,DURATION):make(f'alias_direction_{p:x}',direction=p)
 for p in(FIRST,SECOND,DIRECTION,DIRECTION+4,DIRECTION+8):make(f'alias_all_optional_{p:x}',distance=p,speed_out=p,duration=p)
 for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xff854321)):
  make(f'paired_nan_{a:x}_{b:x}',changes=((FIRST,a),(SECOND,b),(FIRST+8,b),(SECOND+8,a)))
 for pc in(0,2,3):
  for r in range(4):make(f'norm_association_{pc}_{r}',a=(4096.,1.,1.),precision=pc,rounding=r)
 make('adversarial_zero_epsilon',a=(0.,0.,0.),epsilon=0)
 make('fault_seed_fractional_z',a=(2.,1.,4096.),b=(0.,0.,.0001),precision=3)
 make('fault_seed_nonzero');make('fault_seed_zero',a=(0.,0.,0.));make('fault_seed_nan',changes=((FIRST,0x7f812345),));make('fault_seed_alias',direction=FIRST+4)
 make('actual_second_read_fault',second=0x40000000,actual_read_fault=True)
 make('actual_direction_write_fault',direction=0x40000000,actual_write_fault=True)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original fallback repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=2,adversarial_constant_cases=1,pending_quantities=sum(bool(r['pending_extended'])for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_jump_fallback_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_jump_fallback_tu15.cpp')},scope_limits=['ActualFF580 mathematical outputs only; no numeric ABI or full jump feasibility promoted','Masked original FP; at least two temporary x87 slots available','Pending source quantities are explicit mathematical values at true input failure boundaries','FullFF1A0/278E60/27B340/200D30 closure remains open'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} original jump fallback comparisons/all{len(visited)} sites; fixture{a.output.stat().st_size} bytes')
if __name__=='__main__':main()
