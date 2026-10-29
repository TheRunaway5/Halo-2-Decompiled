#!/usr/bin/env python3
"""Independent unchanged physical TU1.5 campaign point geometry reference.

Declared data/register inputs precede each original call. No instruction,
dependency result or resumed fault outcome is supplied. Private original
argument/temporary stack is excluded from persistent-storage observations.
"""
import argparse,copy,gzip,json,os,random,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,MATRIX,SOURCE,SECOND,DEST,FRAMES,POOL,OBJECTS,BODY=0x1000000,0x1000100,0x1000200,0x1000240,0x1000300,0x1000400,0x1000500,0x1000600,0x1001000
ENTRIES=(0x145070,0x219030,0x218780,0x218b20,0x218d00)
sha=common.sha
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(BASE,0x4000),'axis':(0x44a000,0x1000),'constants':(0x46a000,0x1000),'objects':(0x4ec000,0x1000),'frames':(0x505000,0x1000),'stack':(geometry.STACK,65536)}
 assembly=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
 for name,(address,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
 assembly+='''.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
 (p/'reference.s').write_text(assembly)
 (p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_point_geometry_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p

def inputs(image):
 rows=[]
 def make(entry,name,seed=0,axis=0,variant=0,frame=0,state='valid',destination=DEST,source=SOURCE,matrix=MATRIX,second=SECOND,first_frame=0,second_frame=1,changes=(),rounding=None,denorm=None):
  n=len(rows);rng=random.Random(seed);raw=bytearray(0x4000)
  # Only authored matrices need entropy. The rest is declared zero padding,
  # retained for alias/fault checks without duplicating megabytes of random
  # bytes that no original instruction reads or writes.
  for start,size in((MATRIX,0x34),(BODY+0x200,3*52),(BODY-0x80,0x34)):
   for address in range(start,start+size,4):struct.pack_into('<I',raw,address-BASE,bits(rng.uniform(-8,8)))
  def put(p,v,width=4):raw[p-BASE:p-BASE+width]=int(v&((1<<(width*8))-1)).to_bytes(width,'little')
  put(POOL+0x44,OBJECTS);put(OBJECTS+8,BODY)
  for index in range(-2,4):
   row=FRAMES+index*16;put(row,0x198b0000);put(row+6,abs(index)%3,2);put(row+0xa,axis,1);put(row+0xb,variant,1)
  put(BODY+0x114,3*52,2);put(BODY+0x116,0x200,2)
  for index in range(3):put(BODY+0x200+index*52,bits(1. if index%2==0 else .75))
  put(MATRIX,bits(1. if seed%2 else .75))
  for p in(source,second):
   for k in range(3):put(p+k*4,bits(rng.uniform(-6,6)))
  put(source+0xc,first_frame,2);put(second+0xc,second_frame,2)
  row=FRAMES+struct.unpack('<h',struct.pack('<H',frame))[0]*16
  if frame!=0xffff:
   if state=='missing_object':put(row,0xffffffff)
   elif state=='negative_node':put(row+6,-1,2)
   elif state=='last_node':put(row+6,2,2)
   elif state=='equal_bound':put(row+6,3,2)
   elif state=='above_bound':put(row+6,4,2)
   elif state=='zero_bytes':put(BODY+0x114,0,2)
   elif state=='negative_bytes':put(BODY+0x114,-1,2)
   elif state=='negative_offset':put(BODY+0x116,-0x80,2);put(BODY-0x80,bits(1.))
  for p,value in changes:put(p,value)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
  arg=0
  if entry==0x145070:gp[0]=source;gp[2]=destination;arg=matrix
  elif entry==0x219030:gp[0]=destination;gp[1]=(gp[1]&0xffff0000)|(axis&0xffff);gp[2]=source;arg=variant
  elif entry==0x218780:gp[0]=(gp[0]&0xffff0000)|frame;gp[7]=destination;arg=source
  elif entry==0x218b20:gp[0]=destination;gp[6]=source
  elif entry==0x218d00:gp[0]=source;gp[7]=second
  else:raise ValueError(entry)
  r=n%4 if rounding is None else rounding;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  ranges=((MATRIX,0x80),(SOURCE-0x10,0xa0),(DEST,0x10),(FRAMES-0x20,0x70),(POOL,0x50),(OBJECTS,0x10),(BODY-0x80,0x400))
  banks=[dict(address=p,before=raw[p-BASE:p-BASE+size].hex())for p,size in ranges]
  banks +=[dict(address=0x44acb0,before=image.read(0x44acb0,36).hex()),dict(address=0x46a0d0,before=image.read(0x46a0d0,4).hex()),dict(address=0x4ecbd0,before=struct.pack('<I',POOL).hex()),dict(address=0x505e78,before=struct.pack('<I',FRAMES).hex())]
  rows.append(dict(id=n,name=name,entry=entry,matrix=arg,source=0,gp=gp,banks=banks,point_source=source,destination=destination,point_matrix=matrix,second=second,axis=axis,variant=variant,frame=frame,
   before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
 for e in ENTRIES:
  for r in range(4):
   for d in(0,0x40,0x8000,0x8040):make(e,f'finite_{e:x}_{r}_{d:x}',seed=len(rows),rounding=r,denorm=d)
 for axis in range(3):
  for variant in range(2):
   for e in(0x219030,0x218780,0x218b20,0x218d00):make(e,f'permutation_{e:x}_{axis}_{variant}',axis=axis,variant=variant,seed=51)
 for e in(0x218780,0x218b20,0x218d00):
  for frame in(0,1,0xfffe,0xffff):
   for state in('valid','missing_object','negative_node','last_node','equal_bound','above_bound','zero_bytes','negative_bytes','negative_offset'):
    make(e,f'frame_{e:x}_{frame:x}_{state}',frame=frame,first_frame=frame,state=state,seed=27)
 for e in ENTRIES:
  for dest in(SOURCE,SOURCE+4,SOURCE-4,MATRIX,MATRIX+4,MATRIX+0x14,MATRIX+0x28,BODY+0x200,BODY+0x204):
   if e!=0x218d00:make(e,f'alias_{e:x}_{dest:x}',destination=dest,seed=92)
 for scale in(0,0x80000000,1,0x3f800000,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  make(0x145070,f'scale_{scale:x}',changes=((MATRIX,scale),),seed=73)
 fields=(MATRIX,MATRIX+4,MATRIX+0x10,MATRIX+0x1c,MATRIX+0x28,SOURCE,SOURCE+4,SOURCE+8)
 for e in(0x145070,0x218d00):
  for field in(fields if e==0x145070 else(SOURCE,SOURCE+4,SOURCE+8,SECOND,SECOND+4,SECOND+8)):
   for value in(0,0x80000000,1,0x80000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(e,f'exception_{e:x}_{field:x}_{value:x}',changes=((field,value),),seed=98)
  for first,second in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
   make(e,f'paired_nan_{e:x}_{first:x}_{second:x}',changes=((SOURCE,first),(SOURCE+4,second),(MATRIX+4,second),(MATRIX+0x10,first)),seed=62)
 # Equal invalid frames bypass all object/frame lookup; unequal frames have a
 # distinct summation association visible at this single-precision boundary.
 for a,b in((0xffff,0xffff),(0xfffe,0xfffe),(0,0),(0xffff,0)):
  make(0x218d00,f'distance_association_{a:x}_{b:x}',first_frame=a,second_frame=b,changes=((SOURCE,bits(0.)),(SOURCE+4,bits(0.)),(SOURCE+8,bits(0.)),(SECOND,bits(4096.)),(SECOND+4,bits(1.)),(SECOND+8,bits(1.))),seed=13,rounding=0,denorm=0)
 for e in ENTRIES:
  make(e,f'fault_seed_{e:x}',seed=41)
  if e!=0x218d00:make(e,f'fault_seed_alias_{e:x}',destination=SOURCE+4,seed=42)
 for e in(0x218780,0x218b20,0x218d00):make(e,f'fault_seed_fallback_{e:x}',state='negative_node',seed=43)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows if r['completed']));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);raw=json.dumps(fixture,separators=(',',':')).encode();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(raw,mtime=0))
 repeated=None
 if a.verify:repeated=a.verify.read_bytes()==a.output.read_bytes();assert repeated,'Independent original repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),owners=meta['owners'],original_sha256=NEW_PIN,fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),fixture=str(a.output),repeat_identical=repeated,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_point_geometry_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_point_geometry_tu15.cpp')},scope_limits=['Masked SIMD exceptions; no unmasked trap owner','Internal semantic APIs only, no arbitrary numeric-call ABI qualification','Private callee temporary stack omitted; persistent operand/fault/store order checked','No navigation pathability/search/update closure or live activation claimed'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical original point comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
