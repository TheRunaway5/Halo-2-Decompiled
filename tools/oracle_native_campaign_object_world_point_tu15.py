#!/usr/bin/env python3
"""Independent unchanged physical TU1.5 object world-point reference.

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
ENTRIES=(0xbbe00,)
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
 rows=[];pool=BASE+0x100;headers=BASE+0x200;child=BASE+0x1000;parent_body=BASE+0x2000;private=geometry.STACK+0x6000
 def make(name='ordinary',seed=0,object=0xe3600000,parent=0xe3610001,node=0,offset=0x200,destination=None,scale=0x3f800000,changes=(),rounding=None,denorm=None):
  n=len(rows);rng=random.Random(seed);raw=bytearray(0x4000)
  def put(at,value,width=4):
   off=at-BASE;assert 0<=off and off+width<=len(raw),(hex(at),width)
   raw[off:off+width]=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
  actual_parent=child if (parent&0xffff)==(object&0xffff) else parent_body
  matrix=actual_parent+node*52+offset
  for k in range(13):put(matrix+k*4,bits(rng.uniform(-5,5)))
  put(matrix,scale)
  for k in range(3):put(child+0x64+k*4,bits(rng.uniform(-5,5)))
  put(pool+0x44,headers);put(headers+(object&0xffff)*12+8,child)
  if parent!=0xffffffff and (parent&0xffff)!=(object&0xffff):put(headers+(parent&0xffff)*12+8,parent_body)
  put(child+0x14,parent);put(child+0x18,node,1);put(actual_parent+0x116,offset,2)
  for address,value in changes:put(address,value)
  dest=private if destination is None else destination
  banks=[dict(address=BASE,before=raw.hex()),dict(address=0x46a0d0,before=image.read(0x46a0d0,4).hex()),dict(address=0x4ecbd0,before=struct.pack('<I',pool).hex())]
  if destination is None:banks.append(dict(address=private,before=bytes([0x63]*12).hex()))
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=object;gp[6]=dest;gp[4]=geometry.SP+n%4*4
  r=n%4 if rounding is None else rounding;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  rows.append(dict(id=n,name=f'{name}_{n}',entry=0xbbe00,matrix=0,source=0,gp=gp,banks=sorted(banks,key=lambda b:b['address']),object=object,destination=dest,private_value=destination is None,parent=parent,point_matrix=matrix,
   before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])]))
 for parent in (0xffffffff,0xe3610001,0xffff0000):
  for r in range(4):
   for d in (0,0x40,0x8000,0x8040):make(parent=parent,rounding=r,denorm=d,seed=len(rows),name='finite')
 for node in (-128,-2,-1,0,1,2,127):make(node=node,seed=21,name='signed_node')
 for offset in (-0x80,0,0x200):make(offset=offset,seed=23,name='signed_transform_offset')
 for scale in (0,0x80000000,1,0x80000001,0x3f800000,0x3f800001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(scale=scale,seed=25,name='scale')
 for address in (child+0x64,child+0x68,child+0x6c,parent_body+0x204,parent_body+0x210,parent_body+0x21c,parent_body+0x228):
  for value in (0,0x80000000,1,0x80000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(changes=((address,value),),seed=31,name='exception')
 for left,right in ((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
  make(changes=((child+0x64,left),(child+0x68,right),(parent_body+0x204,right),(parent_body+0x210,left)),seed=43,name='paired_nan')
 for left,right in ((0x7fc12345,0xffc54321),(0x7f812345,0xff854321)):
  make(changes=((child+0x6c,left),(child+0x68,right),(parent_body+0x21c,0x3f800000),(parent_body+0x210,0x3f800000)),seed=45,name='paired_nan_z_y_order')
 for parent in (0xffffffff,0xe3610001):
  for dest in (child+0x60,child+0x64,child+0x68,child+0x6c,parent_body+0x200,parent_body+0x204,parent_body+0x214,parent_body+0x228):make(parent=parent,destination=dest,seed=47,scale=0x3f000000,name='retained_alias')
 for salt in (0,0xe361,0xffff):make(object=salt<<16,parent=0xe3610001,seed=51,name='salted_object')
 make(parent=0xffffffff,destination=child+0x68,seed=53,name='fault_seed_direct_alias')
 make(destination=BASE+0x3000,scale=0x3f000000,seed=59,name='fault_seed_parent')
 make(destination=parent_body+0x204,scale=0x7f812345,seed=61,name='fault_seed_parent_alias_nan')
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
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical original object-world-point comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
