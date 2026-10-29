#!/usr/bin/env python3
"""Physical original resource retirement with real host-page protection, no supplied game results."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
ENTRIES=(0x130030,0x140310,0x12fd10,0x2db4ca,0x16e8a0)
BASE,PHYSICAL,USER=0x1000000,0x81000000,0x81000200
META,NODES,ALT_NODES,CONTAINER=BASE+0x1000,BASE+0x2200,BASE+0x2300,BASE+0x2000
META_ROWS,NODE_ROWS,ALT_ROWS=PHYSICAL+0x1200,BASE+0x4000,BASE+0x4800
META_BITS,NODE_BITS,ALT_BITS=BASE+0x6000,BASE+0x6100,BASE+0x6200
sha=common.sha

def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();s=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(s));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'bank':(BASE,0x10000),'import':(0x41b000,0x1000),'globals':(0x4f2000,0x1000),'physical':(PHYSICAL,0x4000),'stack':(geometry.STACK,65536)}
 assembly=f'.code32\n.section .original,"ax"\n.incbin "{p/"text.bin"}"\n'
 for name,(_,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
 assembly+='''.section .kernel,"ax"
.org 0xb6
// Platform adaptation only: real i386 Linux mprotect on the original pages.
// The original stdcall import arguments and callee-saved EBX are respected.
push %ebx
mov 8(%esp),%ebx
mov 12(%esp),%ecx
add %ebx,%ecx
add $4095,%ecx
and $-4096,%ecx
and $-4096,%ebx
sub %ebx,%ecx
mov $3,%edx
cmpl $2,16(%esp)
jne 1f
mov $1,%edx
1: mov $125,%eax
int $0x80
pop %ebx
ret $12
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
'''
 (p/'reference.s').write_text(assembly)
 (p/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{s.va:x}:{{*(.original)}} '+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+f'.kernel 0x80000000:{{*(.kernel)}} .stop 0x{geometry.STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_resource_retirement_tu15.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p

def inputs(image):
 rows=[]
 def make(entry,name,index=3,previous=0,next=0,node_previous=0xffffffff,node_next=0xffffffff,callback=0x12fd10,poison=False,meta_stride=40,node_stride=24,ro=True,bytes_=0x1fff,protected=PHYSICAL+1,highwater=None,cursor=63,alias=None,association=0,import_=0x800000b6):
  n=len(rows);regions={BASE+0x1000:bytearray(0x200),CONTAINER:bytearray(0x100),NODES:bytearray(0x80),ALT_NODES:bytearray(0x80),NODE_ROWS:bytearray(0x1400),META_BITS:bytearray(0x300),PHYSICAL:bytearray(0x2000),0x4f2d20:bytearray(0x30),0x41b648:bytearray(image.read(0x41b648,4))}
  for start,data in regions.items():
   if start!=0x41b648:
    for k in range(len(data)):data[k]=(k%17)*7+3
  def put(p,v,width=4):
   raw=int(v&((1<<(width*8))-1)).to_bytes(width,'little')
   for start,data in regions.items():
    if start<=p and p+width<=start+len(data):data[p-start:p-start+width]=raw;return
   raise ValueError(('Missing declared resource input',hex(p),width))
  datum=0xcafe0000|index
  for pool,record_rows,stride,bitmap in((META,META_ROWS,meta_stride,META_BITS),(NODES,NODE_ROWS,node_stride,NODE_BITS),(ALT_NODES,ALT_ROWS,node_stride,ALT_BITS)):
   put(pool+0x24,stride);put(pool+0x2a,8 if poison else 0,1);put(pool+0x34,cursor);put(pool+0x38,index+1 if highwater is None else highwater);put(pool+0x3c,50);put(pool+0x44,record_rows);put(pool+0x48,bitmap)
   for k in range(64):put(record_rows+k*stride,0x3100+k,2)
   for k in range(2):put(bitmap+k*4,0xffffffff)
  put(0x4f2d24,META);put(0x4f2d34,CONTAINER);put(0x4f2d2c,0x1234abcd);put(0x41b648,import_)
  put(CONTAINER+0x20,callback);put(CONTAINER+0x64,NODES);put(CONTAINER+0x3c,datum);put(CONTAINER+0x40,datum)
  record=META_ROWS+index*40;put(record+2,0x57,1);put(record+0x10,association)
  node=NODE_ROWS+index*24;put(node+0xc,node_next);put(node+0x10,node_previous)
  put(USER-0x24,datum);put(USER-0x20,0xabcdef12);put(USER-0x1c,protected);put(USER-0x18,bytes_);put(USER-8,next);put(USER-4,previous)
  if alias=='next_clobbers_container':
   put(USER-8,0x4f2d14);put(USER-4,0)
  elif alias=='callback_changes_pool':
   assert index==8
   put(CONTAINER+0x64,ALT_NODES);put(META+0x48,CONTAINER+0x64)
   old=ALT_ROWS+index*24;put(old+0xc,node_next);put(old+0x10,node_previous)
  elif alias=='previous_nulls_meta_pool':
   put(USER-4,0x4f2d08);put(USER-8,0)
  elif alias=='previous_changes_meta_pool':
   alternate=META+0x100
   for offset in(0x24,0x34,0x38,0x3c,0x44,0x48):
    put(alternate+offset,{0x24:40,0x34:63,0x38:64,0x3c:50,0x44:META_ROWS+0x800,0x48:META_BITS+0x40}[offset])
   put(alternate+0x2a,0,1);put(META_ROWS+0x800+index*40+0x10,0);put(META_BITS+0x40,0xffffffff);put(USER-4,0x4f2d08);put(USER-8,alternate)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4;gp[6]=USER;gp[7]=CONTAINER;gp[3]=datum
  row=dict(id=n,name=name,entry=entry,gp=gp,matrix=datum,source=0,user=USER,container=CONTAINER,datum=datum,initial_read_only=ro and entry==0x130030,banks=[dict(address=p,before=b.hex())for p,b in sorted(regions.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|n*7%64],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])])
  if import_!=0x800000b6 and bytes_ and entry==0x130030:row.update(stop_pc=import_,required_owner=import_,required_identity=protected)
  elif callback not in(0,0x12fd10)and entry!=0x12fd10:row.update(stop_pc=callback,required_owner=callback,required_identity=datum)
  elif association and(callback or entry==0x12fd10):row.update(stop_pc=0x12fd40,required_owner=0x12fd40,required_identity=record)
  if alias=='previous_nulls_meta_pool':row['actual_read_fault']=True
  rows.append(row)
 for entry in(0x130030,0x140310):
  for prev in(0,PHYSICAL+0x800):
   for nxt in(0,PHYSICAL+0xc00):
    for p in(0xffffffff,0xa0010001):
     for q in(0xffffffff,0xb0020005):make(entry,f'links_{entry:x}_{prev:x}_{nxt:x}_{p:x}_{q:x}',previous=prev,next=nxt,node_previous=p,node_next=q)
 for entry in(0x130030,0x140310,0x12fd10):
  for index in(0,1,3,31,32):
   for poison in(False,True):make(entry,f'delete_{entry:x}_{index}_{poison}',index=index,poison=poison)
  make(entry,f'unknown_gpu_association_{entry:x}',association=BASE+0x7000)
 for entry in(0x130030,0x140310):
  make(entry,f'no_delete_callback_{entry:x}',callback=0)
  make(entry,f'unknown_delete_callback_{entry:x}',callback=0x12fc70)
  for stride in(1,2,3,23,24,25,39,40,41):
   make(entry,f'generic_node_stride_{entry:x}_{stride}',node_stride=stride,highwater=64,poison=True)
 for protected,bytes_ in((PHYSICAL,0x2000),(PHYSICAL+0x1fff,1),(PHYSICAL+0xfff,0x1001),(PHYSICAL+0x123,17),(PHYSICAL+1,0)):
  make(0x130030,f'byte_range_{protected:x}_{bytes_:x}',protected=protected,bytes_=bytes_,ro=bytes_!=0 and protected<=PHYSICAL+0xfff and protected+bytes_>=PHYSICAL+0x2000)
 make(0x130030,'next_publication_replaces_container_global',alias='next_clobbers_container')
 make(0x130030,'previous_publication_replaces_metadata_pool',alias='previous_changes_meta_pool')
 for entry in(0x130030,0x140310):make(entry,f'callback_replaces_container_pool_{entry:x}',index=8,alias='callback_changes_pool',node_previous=0xffffffff,node_next=5,highwater=64)
 make(0x130030,'unknown_actual_protection_import',import_=0x800000b7)
 make(0x130030,'actual_source_read_fault_after_partial_unlink',alias='previous_nulls_meta_pool')
 for entry in(0x130030,0x140310,0x12fd10):make(entry,f'fault_seed_{entry:x}',previous=PHYSICAL+0x800,next=PHYSICAL+0xc00,node_previous=1,node_next=5,poison=True,highwater=64)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;m=common.metadata(image);owned=prepare(image,m,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 # A read/modify/write instruction has no externally observable read/store
 # midpoint. Check its before-read and after-store boundaries, not a fictitious
 # callback between the same instruction's two memory effects.
 read_modify={s['instruction']for s in m['sites']if any(o['read']and o['write']for o in s['operands'])}
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n,event in enumerate(events):
    if phase=='before_write'and event['instruction']in read_modify:continue
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in m['sites']if s['instruction']not in visited]
 # The associated GPU/fence wait and cleanup body is explicitly required at
 # its first real source entry. Nothing executes that unconverted branch.
 excluded={s['instruction']for s in m['sites']if 0x12fd40<=s['instruction']<=0x12fde6}
 unexpected=[pc for pc in uncovered if pc not in excluded]
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=m['owners'],instruction_patches=[],supplied_original_outcomes=False,platform_service='real-linux-i386-mprotect-stdcall-bridge',physical_allocation=dict(address=PHYSICAL,bytes=0x4000),cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent original repeat differs'
 report=dict(passed=not unexpected,cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum('required_owner'in r for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),uncovered_sites=uncovered,unexpected_uncovered_sites=unexpected,owners=m['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),platform_protections=sum(len(r['services'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Actual unassociated12FD10 callback profile only; associated GPU/fence resources stop at12FD40','Import800000B6 adapts actual protection arguments to real Linux mprotect, no Xbox cache/device parity claim','No numeric ABI/router, original source CPU packet or game outcomes supplied; full navigation remains open'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_resource_retirement_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_resource_retirement_tu15.cpp')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');assert not unexpected,unexpected;print(f'{len(rows)} physical resource comparisons, {len(visited)} source sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
