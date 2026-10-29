#!/usr/bin/env python3
"""Physical unchanged3EA20→actual266EF9 publication and complete30BD0 vector math."""
import argparse,copy,gzip,hashlib,json,math,os,random,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,FIRST,SECOND,DEST=0x1000000,0x1000100,0x1000140,0x1000180
sha=common.sha
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 (p/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{p/'text.bin'}"
.section .bank,"aw"
.space 16384
.section .constants,"aw"
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
''');(p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} .bank 0x{BASE:x}:{{*(.bank)}} .constants 0x46a000:{{*(.constants)}} .stack 0x{geometry.STACK:x}:{{*(.stack)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def metadata(image):
 common.ENTRIES=(0x3ea20,0x30bd0);m=common.metadata(image);c=_Code(image,'.text');i=c.at(0x266ef9)
 assert i.mnemonic=='fstp'and i.op_str=='dword ptr [ebp + 0x28]'
 assert c.at(0x266ef4).mnemonic=='call'and c.at(0x266ef4).op_str=='0x3ea20'
 m['sites'].append(dict(instruction=i.address,bytes=i.bytes.hex(),operands=[dict(base=5,index=-1,scale=1,displacement=0x28,bytes=4,read=False,write=True)]))
 m['owners']['0x266ef9']=dict(sites=1,listing_sha256=hashlib.sha256(f'{i.address:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'.encode()).hexdigest())
 m['caller_profile']=dict(call=0x266ef4,call_bytes=c.at(0x266ef4).bytes.hex(),return_pc=0x266ef9,publication_bytes=i.bytes.hex(),stop_pc=0x266efc)
 return m
def inputs(image):
 rows=[]
 def make(kind,name,first=FIRST,second=SECOND,destination=DEST,xyz=(3.,4.,5.),other=(-.25,.5,1.75),changes=(),rounding=None,precision=None,denorm=None):
  n=len(rows);raw=bytearray((k*13+0x53)&255 for k in range(0x100))
  def put(p,v):struct.pack_into('<I',raw,p-(FIRST-0x10),v)
  for p,values in((first,xyz),(second,other)):
   for k,v in enumerate(values):put(p+k*4,bits(v))
  for p,v in changes:put(p,v)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=first;gp[4]=geometry.SP+n%4*4
  row=dict(id=n,name=name,kind=kind,entry=0x3ea20 if kind=='distance'else 0x30bd0,first=first,second=second,destination=destination,gp=gp,matrix=0,source=0,
    banks=[dict(address=FIRST-0x10,before=raw.hex()),dict(address=0x46a0d0,before=image.read(0x46a0d0,4).hex()),dict(address=0x46a0ec,before=image.read(0x46a0ec,4).hex())])
  if kind=='distance':gp[1]=second;gp[5]=destination-0x28;row.update(return_pc=0x266ef9,stop_pc=0x266efc)
  r=n%4 if rounding is None else rounding;p=(0,2,3)[n%3]if precision is None else precision;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  row.update(before_fp=[0x7f|(p<<8)|(r<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|n*7%64],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(n%4)])
  rows.append(row)
 for kind in('distance','normalize'):
  for r in range(4):
   for p in(0,2,3):
    for d in(0,0x40,0x8000,0x8040):make(kind,f'finite_{kind}_{r}_{p}_{d:x}',rounding=r,precision=p,denorm=d)
  fields=(FIRST,FIRST+4,FIRST+8,SECOND,SECOND+4,SECOND+8)if kind=='distance'else(FIRST,FIRST+4,FIRST+8)
  for field in fields:
   for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc54321,0xff854321):make(kind,f'exception_{kind}_{field:x}_{value:x}',changes=((field,value),))
  for a,b in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
   for first,second in((FIRST,FIRST+4),(FIRST,FIRST+8),(FIRST,SECOND))if kind=='distance'else((FIRST,FIRST+4),(FIRST,FIRST+8)):
    make(kind,f'paired_nan_{kind}_{a:x}_{b:x}_{first:x}_{second:x}',changes=((first,a),(second,b)))
  for first,second,destination in((FIRST,FIRST+4,DEST),(FIRST,FIRST,DEST),(FIRST,SECOND,FIRST),(FIRST,SECOND,FIRST+4),(FIRST,SECOND,SECOND),(FIRST,SECOND,SECOND+4)):
   make(kind,f'alias_{kind}_{first:x}_{second:x}_{destination:x}',first=first,second=second,destination=destination)
  for shift in(1,3):make(kind,f'unaligned_{kind}_{shift}',first=FIRST+shift,second=SECOND+shift,destination=DEST+shift)
  for xyz in((0.,0.,0.),(-0.,-0.,-0.),(16777216.,1.,1.),(1.,16777216.,1.),(1.,1.,16777216.),(1e-35,2e-35,-3e-35),(1e30,-2e30,3e30)):
   for r in range(4):make(kind,f'association_{kind}_{xyz}_{r}',xyz=xyz,other=(0.,0.,0.),rounding=r)
 epsilon=struct.unpack('<f',image.read(0x46a0ec,4))[0]
 for scale in(0.,.5,1.,1.0000001,2.):
  for shape in((epsilon*scale,0.,0.),(epsilon*scale/math.sqrt(2),epsilon*scale/math.sqrt(2),0.),(epsilon*scale,1e-35,0.)):
   for r in range(4):make('normalize',f'epsilon_{shape}_{r}',xyz=shape,rounding=r)
 for kind in('distance','normalize'):
  make(kind,'fault_seed_'+kind);rows[-1]['before_fp'][2]&=~0x3f
  make(kind,'fault_seed_nan_'+kind,changes=((FIRST,0x7f812345),(FIRST+4,0xffc54321)))
 make('distance','fault_seed_alias_distance',destination=FIRST+4,xyz=(1e30,-2e30,3e30))
 make('normalize','fault_seed_zero_normalize',xyz=(0.,0.,0.))
 for name,xyz,changes in(('finite',(3.,4.,5.),()),('nan',(3.,4.,5.),((FIRST,0x7f812345),)),('overflow',(3e38,-3e38,3e38),())):
  make('distance','actual_rejected_store_'+name,destination=0x1100000,xyz=xyz,changes=changes,rounding=0,precision=2)
  rows[-1]['actual_write_fault']=True;rows[-1]['before_fp'][1]&=~0x3f
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);m=metadata(image);owned=prepare(image,m,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');whole=[r for r in rows if r['completed']];visited=set().union(*(set(r['executed_sites'])for r in whole));uncovered=[s['instruction']for s in m['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=m['owners'],caller_profile=m['caller_profile'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=len(whole),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),owners=m['owners'],caller_profile=m['caller_profile'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),pending_publication_cases=sum(bool(r['pending_extended'])for r in rows),actual_write_faults=sum(r.get('actual_write_fault',False)for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Composed3EA20→266EF9 distance publication only; no arbitrary numeric entry return contract','Ordinary callers with room for five actual x87 temporaries; occupied inputs0..3 tested','Masked FP exceptions; precise scalar-store rejection/committed observer boundaries','Full campaign/navigation/perception and runtime activation remain open'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_vector_math_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical vector comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
