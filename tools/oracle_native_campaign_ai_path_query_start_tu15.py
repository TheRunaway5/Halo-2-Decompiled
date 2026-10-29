#!/usr/bin/env python3
"""Unchanged physical27B100, real framed distance/CRT conversion/native heap reference.

Code and dependency returns are never patched or supplied. Canonical cases use
the exact original scalar literals. Separately labeled adversarial literal-bank
inputs exercise otherwise unreachable CRT arithmetic; these are not canonical
engine-input or startup-data qualification.
"""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
WORK,NAVIGATION,SECTORS,FRAMES,POOL,OBJECTS,BODY=0x1000000,0x1021000,0x1022000,0x1025000,0x1026000,0x1026100,0x1027000
ENTRIES=(0x27b100,0x218c40,0x218b20,0x218780,0x219030,0x145070,0x32cc0c,0x27bce0,0x27bb60)
sha=common.sha
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(WORK,0x40000),'axis':(0x44a000,0x1000),'maximum':(0x456000,0x1000),'constants':(0x46a000,0x1000),'objects':(0x4ec000,0x1000),'frames':(0x505000,0x1000),'stack':(geometry.STACK,65536)}
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
''';(p/'reference.s').write_text(assembly)
 (p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+f'.stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_query_start_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs(image):
 rows=[];scale_bits=int.from_bytes(image.read(0x46a110,4),'little')
 def make(name,flag=1,root=NAVIGATION,index=0,count=512,first_frame=0xffff,second_frame=0xffff,frame_state='valid',node=0,tail=1,sector_flags=0x13c0,changes=(),rounding=None,precision=None,denorm=None,scale=None,axis=0,variant=0,sector_rows=SECTORS):
  n=len(rows);regions={WORK:bytearray((k*13+0x53)&255 for k in range(0x600)),WORK+0x11090:bytearray(0xb0),WORK+0x120b0:bytearray(0x2020),NAVIGATION:bytearray(0x20),SECTORS:bytearray(0x1000),FRAMES-0x20:bytearray(0x60),POOL:bytearray(0x50),OBJECTS:bytearray(0x10),BODY:bytearray(0x300)}
  def put(p,v,width=4):
   raw=int(v&((1<<(width*8))-1)).to_bytes(width,'little')
   for first,data in regions.items():
    if first<=p and p+width<=first+len(data):data[p-first:p-first+width]=raw;return
   raise ValueError(('Missing declared query input',hex(p)))
  put(WORK+0x8c,root);put(WORK+0x24,index);put(WORK+0x54,flag,1);put(WORK+0xae,node,2)
  for p,xyz,frame in((WORK+0x14,(0.,0.,1.),first_frame),(WORK+0x58,(1.25,-.5,2.),second_frame)):
   for k,v in enumerate(xyz):put(p+k*4,bits(v))
   put(p+0xc,0x12340000|frame)
  for p in(NAVIGATION,WORK)if root==WORK else(NAVIGATION,):put(p,count);put(p+4,sector_rows)
  if 0<=index<512:put(SECTORS+index*8,sector_flags,2)
  put(WORK+0x110b0,tail,2)
  for k in range(1,9):put(WORK+0x110b2+k*4,8+k,2);put(WORK+0x110b4+k*4,(900,-5,2000,10)[(k-1)%4],2)
  put(POOL+0x44,OBJECTS);put(OBJECTS+8,BODY);put(BODY+0x114,3*52,2);put(BODY+0x116,0x200,2)
  for k in range(3):
   matrix=BODY+0x200+k*52
   for offset,value in((0,1.),(4,1.),(0x14,1.),(0x24,1.),(0x28,.25*k),(0x2c,-.5*k),(0x30,.125*k)):put(matrix+offset,bits(value))
  for k in range(-2,4):
   record=FRAMES+k*16;put(record,0x99fe0000);put(record+6,abs(k)%3,2);put(record+0xa,axis,1);put(record+0xb,variant,1)
   if frame_state=='missing_object':put(record,0xffffffff)
   elif frame_state=='negative_node':put(record+6,-1,2)
   elif frame_state=='equal_bound':put(record+6,3,2)
   elif frame_state=='above_bound':put(record+6,4,2)
  if frame_state=='zero_bytes':put(BODY+0x114,0,2)
  elif frame_state=='negative_bytes':put(BODY+0x114,-1,2)
  for p,v in changes:put(p,v)
  literals=((0x44acb0,36),(0x45695c,4),(0x46a0d0,4),(0x46a110,4),(0x46a460,4))
  banks=[]
  for p,b in sorted(regions.items()):
   if p==SECTORS:
    at=max(0,min(511,index))*8;banks.append(dict(address=p+at,before=b[at:at+2].hex()))
   elif p==WORK+0x120b0:
    at=6+(index&0x1ff)*16;banks.append(dict(address=p+at,before=b[at:at+2].hex()))
   else:banks.append(dict(address=p,before=b.hex()))
  banks +=[dict(address=p,before=(struct.pack('<I',scale)if p==0x46a110 and scale is not None else image.read(p,size)).hex())for p,size in literals]
  banks +=[dict(address=0x4ecbd0,before=struct.pack('<I',POOL).hex()),dict(address=0x505e78,before=struct.pack('<I',FRAMES).hex())]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;gp[6]=WORK
  r=n%4 if rounding is None else rounding;p=(0,2,3)[n%3]if precision is None else precision;d=(0,0x40,0x8000,0x8040)[n%4]if denorm is None else denorm
  rows.append(dict(id=n,name=name,entry=0x27b100,workspace=WORK,gp=gp,matrix=0,source=0,banks=banks,before_fp=[0x7f|(p<<8)|(r<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|(r<<13)|d|n*7%64],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,6)[n%4])],adversarial_literal_bank=scale is not None and scale!=scale_bits))
 for r in range(4):
  for p in(0,2,3):
   for d in(0,0x40,0x8000,0x8040):make(f'finite_{r}_{p}_{d:x}',rounding=r,precision=p,denorm=d,tail=(1,2,4)[len(rows)%3])
 make('missing_root',root=0)
 for index in(-2147483648,-1,0,1,2,511,512,2147483647):
  for count in(-1,0,1,512):make(f'sector_bounds_{index}_{count}',index=index,count=count)
 for flag in(0,1,2,255):
  for node in(0,1,3,0xfffe,0xffff):
   for tail in(-1,1,2,4,0x400):make(f'node_heap_{flag}_{node:x}_{tail}',flag=flag,node=node,tail=tail)
 for first,second in((0xffff,0xffff),(0xfffe,0xfffe),(0x8000,0x8000),(0,0),(0xffff,0),(0,0xffff),(0,1),(0xfffe,1)):
  for state in('valid','missing_object','negative_node','equal_bound','above_bound','zero_bytes','negative_bytes'):make(f'frames_{first:x}_{second:x}_{state}',first_frame=first,second_frame=second,frame_state=state)
 for axis in range(3):
  for variant in range(2):make(f'permutation_{axis}_{variant}',first_frame=0,second_frame=1,axis=axis,variant=variant)
 for value in(0,0x80000000,1,bits(.75),0x7f800000,0x7fc12345,0x7f812345):
  make(f'matrix_scale_{value:x}',first_frame=0,second_frame=1,changes=tuple((BODY+0x200+k*52,value)for k in range(3)))
 fields=(WORK+0x14,WORK+0x18,WORK+0x1c,WORK+0x58,WORK+0x5c,WORK+0x60)
 for field in fields:
  for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc54321,0xff854321):make(f'exception_{field:x}_{value:x}',changes=((field,value),))
 for first,second in((0x7fc12345,0xffc54321),(0x7f812345,0xffc54321),(0x7fc12345,0xff854321),(0x7f812345,0xff854321)):
  make(f'paired_nan_{first:x}_{second:x}',changes=((WORK+0x14,first),(WORK+0x58,second)))
 for distance in(0.,.005,.015,.025,327.66,327.67,327.68,42949672.,42949672.96):
  for r in range(4):make(f'quantization_boundary_{distance}_{r}',changes=((WORK+0x58,bits(distance)),(WORK+0x5c,bits(0.)),(WORK+0x60,bits(1.))),rounding=r)
 # Noncanonical readonly operand inputs are explicitly separate from authentic
 # engine-input coverage. They prove all signed CRT correction branches using
 # unchanged code, never a precomputed conversion or dependency return.
 for scale in(bits(-100.),bits(-1.),bits(.5),bits(-.5),bits(4294967296.),0x7f800000,0xff800000,0x7fc12345,0x7f812345):
  for r in range(4):make(f'adversarial_scale_{scale:x}_{r}',scale=scale,rounding=r,changes=((WORK+0x58,bits(1.25)),(WORK+0x5c,bits(0.)),(WORK+0x60,bits(1.))))
 make('adversarial_zero_low_nonzero_high',scale=bits(4294967296.),changes=((WORK+0x58,bits(1.)),(WORK+0x5c,bits(0.)),(WORK+0x60,bits(1.))))
 for flags in(0,0x40,0x80,0x100,0x200,0x400,0x800,0x1000,0xffff):make(f'sector_flags_{flags:x}',sector_flags=flags)
 make('alias_root',root=WORK);make('alias_sector_pose',sector_rows=WORK+0x14)
 make('fault_seed_canonical');make('fault_seed_projected',first_frame=0,second_frame=1,tail=4);make('fault_seed_fallback',first_frame=0,second_frame=1,frame_state='negative_node');make('fault_seed_alias',node=0xfffe,tail=4)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;meta=common.metadata(image);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');whole=[r for r in rows if r['completed']];visited=set().union(*(set(r['executed_sites'])for r in whole));canonical=set().union(*(set(r['executed_sites'])for r in whole if not r['adversarial_literal_bank']));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited]
 #218B20 checks the captured frame before calling218780, so its local NONE
 # copy cannot call the callee's NONE entry branch. Those ten sites are already
 # proved by the independent direct-owner geometry regression, rerun natively.
 unreachable={0x218858,0x21885b,0x21885d,0x218860,0x218863,0x218866,0x218869,0x21886b,0x21886c,0x21886f}
 unexpected=[pc for pc in uncovered if pc not in unreachable]
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=not unexpected,cases=len(rows),complete_calls=len(whole),adversarial_operand_calls=sum(r['adversarial_literal_bank']for r in whole),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),canonical_sites=len(canonical),noncanonical_only_sites=sorted(visited-canonical),uncovered_sites=uncovered,unexpected_uncovered_sites=unexpected,unreachable_from_query_start=dict(sites=sorted(unreachable),reason='218B20 branches to its local copy on captured frameFFFF before218780 call; direct-owner geometry regression covers these sites'),owners=meta['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Valid ordinary callers with room for two x87 temporaries; unmasked traps excluded','Private original stack fields and intermediate FP values are not resumable ABI outputs','Adversarial readonly literal inputs separated from canonical engine data','Actual arena lifetime, graph search and full navigation remain open'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_path_query_start_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_path_query_start_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');assert not unexpected,unexpected;print(f'{len(rows)} physical query-start comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
