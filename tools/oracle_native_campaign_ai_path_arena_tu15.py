#!/usr/bin/env python3
"""Unchanged physical TU1.5 workspace/arena ownership; missing real owners stay boundaries."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
ALLOCATOR,SECONDARY,CUSTOM_VTABLE,BACKING=0x1001000,0x1001100,0x1001200,0x1005000
ENTRIES=(0x149680,0x1498e0,0x1d2d90,0x1d2e30,0x233120)
sha=common.sha
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(0x1000000,0x40000),'vtablebase':(0x45f000,0x1000),'vtable':(0x463000,0x1000),'arena':(0x486000,0x1000),'descriptors':(0x502000,0x1000),'backing':(0x51d000,0x1000),'lease':(0x52b000,0x1000),'stack':(geometry.STACK,65536)}
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
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_arena_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs(image):
 rows=[]
 def make(entry,name,primary=ALLOCATOR,secondary=0,initialized=0,had=1,used=(0,0),workspaces=(BACKING,BACKING+0x22974),backing=BACKING,release=BACKING,arena=ALLOCATOR,vtable=0x46393c,unknown=False,secondary_vtable=0x46393c):
  n=len(rows);regions={ALLOCATOR:bytearray((k*13+0x53)&255 for k in range(0x40)),SECONDARY:bytearray((k*7+0x83)&255 for k in range(0x40)),CUSTOM_VTABLE:bytearray(0x30),0x486050:bytearray((k*17+0x93)&255 for k in range(0x30)),0x502080:bytearray((k*11+0x67)&255 for k in range(0x30)),0x51df40:bytearray((k*19+0x31)&255 for k in range(0x10)),0x52be50:bytearray((k*23+0x47)&255 for k in range(0x10)),0x46395c:bytearray(image.read(0x46395c,4)),0x45f4c0:bytearray(image.read(0x45f4c0,4))}
  def put(p,v,width=4):
   b=int(v&((1<<(width*8))-1)).to_bytes(width,'little')
   for start,data in regions.items():
    if start<=p and p+width<=start+len(data):data[p-start:p-start+width]=b;return
   raise ValueError(('Missing declared arena input',hex(p)))
  put(ALLOCATOR,vtable);put(SECONDARY,secondary_vtable);put(CUSTOM_VTABLE+0x20,0x232aa0)
  put(0x48605c,arena-0x105410);put(0x486060,0x1008000);put(0x486070,primary);put(0x486074,secondary)
  put(0x51df44,backing);put(0x51df48,0x9b,1);put(0x52be54,had);put(0x52be58,initialized,1)
  for k in range(2):put(0x50208c+k*12,used[k],1);put(0x502090+k*12,workspaces[k]);put(0x502094+k*12,0x88aa0000+k)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;gp[2]=release
  if entry==0x233120:gp[1]=ALLOCATOR
  row=dict(id=n,name=name,entry=entry,gp=gp,matrix=0,source=0,workspace=release,banks=[dict(address=p,before=b.hex())for p,b in sorted(regions.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|n*7%64],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])])
  # All real233120 callers in this profile pass delete=false. Direct owner
  # cases use the same input argument; no deleting result is supplied.
  if entry==0x233120:row['matrix']=0
  retires=entry==0x1498e0 or(entry==0x1d2d90 and not initialized and bool(primary))
  if retires and unknown and primary:
   row.update(stop_pc=0x232aa0,required_owner=0x232aa0,identity_gp=1)
  elif retires and secondary:
   if secondary_vtable==CUSTOM_VTABLE:row.update(stop_pc=0x232aa0,required_owner=0x232aa0,identity_gp=1)
   else:row.update(stop_pc=0x130030,required_owner=0x130030,identity_gp=6)
  rows.append(row)
 for entry in(0x149680,0x1d2d90,0x1d2e30):
  for offset in range(16):make(entry,f'backing_alignment_{entry:x}_{offset}',backing=BACKING+offset)
 for entry in(0x149680,0x1d2e30):make(entry,f'wrapped_null_allocator_{entry:x}',arena=0)
 for shift in(1,3):make(0x149680,f'unaligned_allocator_{shift}',arena=ALLOCATOR+shift)
 for entry in(0x1498e0,0x1d2d90):
  for primary in(0,ALLOCATOR):
   for secondary in(0,SECONDARY):
    for vtable in(0x46393c,0x45f4a0):make(entry,f'ownership_{entry:x}_{primary:x}_{secondary:x}_{vtable:x}',primary=primary,secondary=secondary,vtable=vtable)
 for initialized in(0,1,255):
  for used in((0,0),(1,0),(0,1),(1,1),(255,2)):
   for primary in(0,ALLOCATOR):make(0x1d2d90,f'lease_{initialized}_{used}_{primary:x}',initialized=initialized,used=used,primary=primary)
 for used in((0,0),(1,0),(0,1),(1,1),(255,2)):
  for release in(BACKING,BACKING+0x22974,0,0xffffffff,0x12345678):
   for had in(0,1,0xdeadbeef):make(0x1d2e30,f'release_{used}_{release:x}_{had:x}',used=used,release=release,had=had)
 make(0x1498e0,'aliased_allocator_identities',secondary=ALLOCATOR)
 make(0x1d2d90,'aliased_allocator_identities_lease',secondary=ALLOCATOR)
 make(0x1d2d90,'null_backing_cold',backing=0)
 make(0x1d2d90,'null_returned_descriptor',initialized=1,workspaces=(0,BACKING))
 make(0x1d2e30,'duplicate_leased_identity',used=(1,1),workspaces=(BACKING,BACKING))
 for entry in(0x1498e0,0x1d2d90):
  make(entry,f'required_primary_selector_{entry:x}',unknown=True,vtable=CUSTOM_VTABLE)
  make(entry,f'required_secondary_selector_{entry:x}',secondary=SECONDARY,secondary_vtable=CUSTOM_VTABLE)
 for vtable in(0x46393c,0x45f4a0):make(0x233120,f'arg0_destructor_{vtable:x}',vtable=vtable)
 for entry in(0x149680,0x1498e0,0x1d2d90,0x1d2e30):make(entry,f'fault_seed_{entry:x}',used=(1,1)if entry==0x1d2e30 else(0,0))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;m=common.metadata(image);owned=prepare(image,m,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in m['sites']if s['instruction']not in visited]
 expected={0x149921,0x149927,0x233130,0x233131,0x233136};unexpected=[pc for pc in uncovered if pc not in expected]
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=m['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=not unexpected,cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum('required_owner'in r for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),uncovered_sites=uncovered,unexpected_uncovered_sites=unexpected,owners=m['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Only233120 delete=false destructor selector is native; other selectors are real required-owner boundaries','Secondary130030 backing-free remains required, so post-free149921/149927 are unqualified','No numeric ABI/router or host allocation/lifted callback; graph search/full campaign remain open'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_path_arena_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_path_arena_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');assert not unexpected,unexpected;print(f'{len(rows)} physical arena comparisons, {len(visited)} sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
