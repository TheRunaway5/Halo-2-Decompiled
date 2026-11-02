#!/usr/bin/env python3
"""Physical unchanged TU1.5 frame policy and retained target query authority."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
CALLERS={0x2c470:(0x229b9e,),0x25940:(0x14d67,0x14d9a,0x198a8,0x258f7)}
REGIONS={'literals1':(0x456000,4096),'literals2':(0x46a000,4096),'scale':(0x473000,4096),'targets':(0x4c1000,4096),'policy':(0x4c6000,4096),'stack':(geometry.STACK,65536)}
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
 for missing in('',*REGIONS.keys()):
  if missing=='stack':continue
  suffix='-'+missing if missing else '';regions={key:value for key,value in REGIONS.items()if key!=missing}
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
  (p/f'reference{suffix}.s').write_text(assembly);(p/f'link{suffix}.ld').write_text('ENTRY(_start)\nPHDRS{original PT_LOAD FLAGS(5); '+''.join(n+' PT_LOAD FLAGS(6); 'for n in regions)+'stop PT_LOAD FLAGS(5);bridge PT_LOAD FLAGS(5); }\n'+f'SECTIONS{{.original 0x{section.va:x}:{{*(.original)}}:original '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}}:{n} 'for n,(a,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}}:stop .bridge 0x70000000:{{*(.bridge)}}:bridge }}\n')
  for command in(['as','--32',str(p/f'reference{suffix}.s'),'-o',str(p/f'reference{suffix}.o')],['ld','-melf_i386','-T',str(p/f'link{suffix}.ld'),str(p/f'reference{suffix}.o'),'-o',str(p/f'reference{suffix}')]):
   r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
 r=subprocess.run(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_camera_frame_policy.cpp'),'-o',str(p/'observer')],text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs(image):
 rows=[]
 def make(name,owner=0x2c470,count=1,target20=0,retiring20=0,target18=0,retiring18=0,changes=(),input_changes=(),omit=(),missing='',caller=0,**fields):
  n=len(rows);banks={0x456910:bytearray(image.read(0x456910,12)),0x46a000:bytearray(image.read(0x46a000,0x300)),0x473644:bytearray([0xb6]*4),0x4c683c:bytearray([0x96]*20),0x4c1eb0:bytearray([0xa3]*14),0x4c1fe0:bytearray([0x7a]*14)}
  def put(address,value,width=4):
   for first,raw in banks.items():
    if first<=address and address+width<=first+len(raw):raw[address-first:address-first+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
   raise AssertionError(hex(address))
  for address,value,width in((0x4c1fe0,target20,4),(0x4c1fed,retiring20,1),(0x4c1eb0,target18,4),(0x4c1ebd,retiring18,1),*changes):put(address,value,width)
  for first in omit:banks.pop(first)
  if missing:
   first,size=REGIONS[missing]
   banks={address:raw for address,raw in banks.items()if not first<=address<first+size}
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=count;gp[4]=geometry.SP+n%4*4
  rows.append(dict(id=n,name=name,owner=owner,entry=caller or owner,caller=caller,stop=caller+5 if caller else geometry.STOP,gp=gp,matrix=0,source=0,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=list(input_changes),elf_variant='-'+missing if missing else '',**fields))
 for count in(0,1,2,3,4,5,0xffffffff,0xfffffffe,0x80000000,0x7fffffff):make(f'count_{count:x}',count=count)
 for address,count in((0x456918,2),(0x456910,3),(0x46a0cc,2),(0x46a130,4),(0x46a22c,4),(0x46a140,1),(0x46a0d0,1)):
  for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffffffff):make(f'raw_scalar_{address:x}_{value:x}',count=count,changes=((address,value,4),))
 for count,address,ordinal in((1,0x46a0d0,0),(1,0x46a0d0,1),(4,0x46a130,0),(4,0x46a22c,1),(2,0x46a0cc,0),(2,0x456910,0),(3,0x456910,1)):
  make(f'changed_after_store_{count}_{address:x}_{ordinal}',count=count,input_changes=(dict(after_store=ordinal,address=address,bytes='4523817f'),))
 for target20 in(0,1,0x80000000,0xffffffff):
  for retiring20 in(0,1,2,255):
   for target18,retiring18 in((0,0),(0,255),(1,0),(0xffffffff,1),(0x80000000,2),(0x80000000,255)):
    make(f'target_{target20:x}_{retiring20}_{target18:x}_{retiring18}',owner=0x25940,target20=target20,retiring20=retiring20,target18=target18,retiring18=retiring18)
 make('target20_does_not_read18',owner=0x25940,target20=1,omit=(0x4c1eb0,))
 for count in(1,2,3,4):make(f'fault_seed_policy_{count}',count=count)
 for args in((1,0,0,0),(1,1,1,0),(0,0,0,0),(0,0,1,1)):
  make('fault_seed_target_'+'_'.join(map(str,args)),owner=0x25940,target20=args[0],retiring20=args[1],target18=args[2],retiring18=args[3])
 for owner,callers in CALLERS.items():
  for caller in callers:
   for profile in range(4):make(f'actual_caller_{caller:x}_{profile}',owner=owner,caller=caller,count=(1,2,3,4)[profile],target20=profile%2,target18=1,retiring20=profile//2)
 for missing,count in(('literals1',2),('literals2',1),('scale',4),('policy',1)):
  make('actual_fault_'+missing,count=count,missing=missing,actual_read_fault=missing.startswith('literals'),actual_write_fault=not missing.startswith('literals'))
 make('actual_fault_targets',owner=0x25940,missing='targets',actual_read_fault=True)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=tuple(CALLERS);meta=common.metadata(image)
 from oracle_smg_firing_1_1 import _Code
 code=_Code(image,'.text')
 for owner,callers in CALLERS.items():
  for caller in callers:
   i=code.at(caller);assert i.mnemonic=='call'and i.operands[0].imm==owner;meta['sites'].append(dict(instruction=caller,bytes=i.bytes.hex(),operands=[]))
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical frame-policy repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=sum(r.get('actual_read_fault',False)or r.get('actual_write_fault',False)for r in rows),original_sites=len(visited),leaf_sites=sum(v['sites']for v in meta['owners'].values()),actual_caller_cases=sum(bool(r['caller'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_camera_frame_policy.cpp':sha(ROOT/'tools/observe_native_tu15_camera_frame_policy.cpp')},scope_limits=['Complete2C470 and retained-row25940, not GPU allocation/capture ownership','Actual258F7 original caller covered but native binding excluded; real Renderer capability service retained','No floating arithmetic; original x87/SIMD controls unchanged','Fixed absolute fields have disjoint source extents; publication-time input changes test their lazy access contract'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical policy cases/{len(visited)} sites')
if __name__=='__main__':main()
