#!/usr/bin/env python3
"""Unchanged physical original path hash lookup and source segment-candidate arithmetic."""
import argparse,copy,gzip,json,os,random,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
BASE,START,END,QUERY,DEST,WORKSPACE=0x1000000,0x1000100,0x1000180,0x1000200,0x1000300,0x1004000
ENTRIES=(0x27c410,0x27c450)
sha=common.sha
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
 signed_targets=[(WORKSPACE+0xb8+node*0x44)&0xffffffff for node in(-32768,32766)]
 regions={'bank':(BASE,0x24000),'constants':(0x46a000,0x1000),'stack':(geometry.STACK,65536)}
 for k,address in enumerate(signed_targets):regions[f'signed_{k}']=(address&~4095,4096)
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
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_candidates_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p

def inputs(image):
 cases=[]
 def common_row(entry,name,banks,gp,**fields):
  n=len(cases);gp[4]=geometry.SP+n%4*4
  r=fields.pop('rounding',n%4);d=fields.pop('denorm',(0,0x40,0x8000,0x8040)[n%4]);fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|(n*7%64)]
  cases.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=0,source=0,banks=[dict(address=p,before=bytes(b).hex())for p,b in sorted(banks.items())],before_fp=fp,live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],**fields))
 def hash_case(name,key=511,chain=0,hit=True,node=1,cycle=False,missing=False):
  banks={WORKSPACE:bytearray(0x100),WORKSPACE+0x120b6:bytearray(b'\xff'*0x2000)}
  def put(p,value,width=4):
   p&=0xffffffff;raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
   for start,data in banks.items():
    if start<=p and p+width<=start+len(data):data[p-start:p-start+width]=raw;return
   banks[p]=bytearray(raw)
  initial=(key&511)<<3
  for j in range(chain):put(WORKSPACE+0x120b6+((initial+j)&4095)*2,0,2)
  put(WORKSPACE+0xb8,key^0x10000000)
  if cycle:
   banks[WORKSPACE+0x120b6][:]=b'\0'*8192
  else:
   put(WORKSPACE+0x120b6+((initial+chain)&4095)*2,node if hit else 0xffff,2)
   if hit and not missing:put(WORKSPACE+0xb8+node*0x44,key)
  gp=[0x5a010000+len(cases)*31+k*7 for k in range(8)];gp[2]=WORKSPACE;gp[6]=key
  fields=dict(workspace=WORKSPACE,key=key)
  if cycle:fields['fault']=dict(phase='before_read',ordinal=8192+chain)
  if missing:fields['actual_read_fault']=True
  common_row(0x27c410,name,banks,gp,**fields)
 for key in(0,1,255,256,511,512,0x800001ff,0xffffffff):
  for chain in(0,1,7,8,31):
   for hit in(False,True):hash_case(f'hash_{key:x}_{chain}_{hit}',key,chain,hit)
 for node in(-32768,-2,0,1,2,127,1085,32766):hash_case(f'signed_node_{node}',0x890001ff,2,node=node)
 hash_case('full_hash_wrap_hit',511,4095)
 for n in(0,2,8190):hash_case(f'full_hash_cycle_prefix_{n}',511,n,cycle=True)
 hash_case('actual_node_read_fault',511,0,node=0x4444,missing=True)
 hash_case('fault_seed_hash',511,9)
 def segment(name,a=(0.,0.,0.),b=(2.,1.,3.),q=(-.5,-.25,-.75),changes=(),start=START,end=END,query=QUERY,destination=DEST,**fields):
  banks={BASE:bytearray(0x400),0x46a0d0:bytearray(image.read(0x46a0d0,4))}
  for p,values in((START,a),(END,b),(QUERY,q)):
   for k,value in enumerate(values):struct.pack_into('<I',banks[BASE],p-BASE+k*4,bits(value))
  for p,value in changes:struct.pack_into('<I',banks[BASE],p-BASE,value)
  gp=[0x5a010000+len(cases)*31+k*7 for k in range(8)];gp[0]=start;gp[1]=destination;gp[2]=end;gp[6]=query
  common_row(0x27c450,name,banks,gp,start=start,end=end,query=query,destination=destination,**fields)
 for r in range(4):
  for d in(0,0x40,0x8000,0x8040):segment(f'finite_{r}_{d:x}',rounding=r,denorm=d)
 for fraction in(-2.,-1.,-.5,0.,.5,1.,2.):segment(f'fraction_{fraction}',q=tuple(-v*fraction for v in(2.,1.,3.)))
 for point in(START,END,QUERY):
  for lane in range(3):
   for value in(0,0x80000000,1,0x80000001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):segment(f'exception_{point:x}_{lane}_{value:x}',changes=((point+lane*4,value),))
 for first,second in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
  for a,b in((START,END),(START,QUERY),(END,QUERY)):
   segment(f'paired_nan_{a:x}_{b:x}_{first:x}_{second:x}',changes=((a,first),(b,second)))
 for p in(START,END,QUERY):
  for offset in(-4,0,4,8):segment(f'aliased_output_{p:x}_{offset}',destination=p+offset)
 for start,end,query in((START,START,QUERY),(START,END,START),(START,END,END),(START,START,START)):
  segment(f'aliased_inputs_{start:x}_{end:x}_{query:x}',start=start,end=end,query=query,destination=start+4)
 for d in(0,0x40,0x8000,0x8040):
  segment(f'zero_norm_{d:x}',a=(1.,-2.,3.),b=(1.,-2.,3.),denorm=d)
  segment(f'tiny_norm_{d:x}',a=(0.,0.,0.),b=(2.**-70,2.**-71,2.**-72),q=(-1.,0.,0.),denorm=d)
  segment(f'norm_association_{d:x}',b=(4096.,1.,1.),q=(-4096.,-1.,-1.),denorm=d,rounding=0)
 for name,kwargs in(('interpolate',{}),('copy_start',dict(end=START)),('copy_end',dict(q=(1.,1.,1.))),('alias',dict(destination=START+4))):segment('fault_seed_'+name,**kwargs)
 segment('actual_query_read_fault',query=0x40000000,actual_read_fault=True)
 segment('actual_interpolation_write_fault',destination=0x40000000,actual_write_fault=True)
 return cases

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original candidate repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix)+3,actual_source_faults=3,original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_path_candidates_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_path_candidates_tu15.cpp')},scope_limits=['Masked SSE exceptions; no unmasked trap owner','Typed data/geometry APIs only; no numeric ABI or successful path query supplied','Full occupied hash cycle retains source looping behavior, checked through two real wrap fault prefixes','Complete27B340/200D30 search/update and campaign acceptance remain open'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} original candidate comparisons/all{len(visited)} sites; fixture{a.output.stat().st_size} bytes')
if __name__=='__main__':main()
