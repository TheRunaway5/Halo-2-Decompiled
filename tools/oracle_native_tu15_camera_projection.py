#!/usr/bin/env python3
"""Physical unchanged2FB50/2FD70/144090 complete projection/matrix reference."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,CAMERA,BOUNDS,PROGRAM,PROJECTION,FRAME,INVERSE,LAYOUT,AUX=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000600,0x1000700,0x1000800,0x1000840
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'data':(BASE,4096),'constants':(0x46a000,4096),'wide':(0x492000,4096),'layout':(0x4f3000,4096),'aux':(0x51d000,4096),'stack':(geometry.STACK,65536)}
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
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_camera_projection.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs(image):
 rows=[];ordinary=struct.pack('<QH',0xc000000000000000,0x3fff).hex()
 def make(name,entry=0x2fd70,camera=CAMERA,bounds=BOUNDS,destination=None,source=FRAME,forward=(1.,0.,0.),up=(0.,0.,1.),near=.015625,far=100.,plane=(0.,0.,1.,0.),scale=1.,wide=0,layout=0,layout_flag=0,layout_index=-1,layout_mode=0,auxiliary=0,aux_flag=0,depth=None,occupied=None,older=None,changes=(),input_changes=(),**fields):
  n=len(rows);depth=(0,1,3,5,7,8)[n%6]if depth is None else depth;mask=(1<<depth)-1 if occupied is None else occupied
  destination=({0x2fb50:PROGRAM,0x2fd70:PROJECTION,0x144090:INVERSE}[entry]if destination is None else destination)
  values=[ordinary]*8
  if older is not None:values[0]=older
  banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x1000))}
  def put(address,value,width=4):
   raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
   for first,data in banks.items():
    if first<=address and address+width<=first+len(data):data[address-first:address-first+width]=raw;return
   banks[address]=bytearray(raw)
  for address in(0x46a0d0,0x46a0dc,0x46a0cc,0x46a0d4,0x46a514,0x46a670,0x46a260,0x46a0ec):put(address,int.from_bytes(image.read(address,4),'little'))
  for offset,value in [(0,0.),(4,0.),(8,0.),*(zip((0xc,0x10,0x14),forward)),*(zip((0x18,0x1c,0x20),up)),(0x28,1.1),(0x2c,1.),(0x40,near),(0x44,far),*(zip((0x48,0x4c,0x50,0x54),plane))]:put(CAMERA+offset,bits(value))
  for k,value in enumerate((0,0,480,640)):put(CAMERA+0x30+k*2,value,2)
  for k,value in enumerate((-1.,1.,-1.,1.)):put(BOUNDS+k*4,bits(value))
  for k,value in enumerate((scale,1.,0.,0.,0.,1.,0.,0.,0.,1.,2.,3.,4.)):put(FRAME+k*4,bits(value))
  put(0x4f3218,layout);put(0x4922e2,wide,1);put(0x51df50,auxiliary)
  put(LAYOUT+1,layout_flag,1);put(LAYOUT+2,layout_index,2);put(LAYOUT+8,layout_mode);put(AUX+5,aux_flag,1)
  for address,value,width in changes:put(address,value,width)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
  if entry==0x2fb50:gp[0]=destination;gp[1]=camera;gp[2]=bounds
  elif entry==0x2fd70:gp[6]=camera;gp[2]=bounds;gp[3]=destination
  else:gp[1]=source;gp[0]=destination
  rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=0,source=0,camera=camera,bounds=bounds,frame=source,destination=destination,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=values,x87_occupied=mask,input_changes=list(input_changes),**fields))
 for entry in(0x2fb50,0x2fd70):
  for bounds in(0,BOUNDS):
   for wide in(0,1):
    for layout,flag,index,mode in((0,0,-1,0),(LAYOUT,0,0,3),(LAYOUT,1,-1,3),(LAYOUT,1,0,0),(LAYOUT,1,0,3)):
     for aux,af in((0,0),(AUX,0),(AUX,1)):
      make(f'context_{entry:x}_{bounds:x}_{wide}_{layout:x}_{flag}_{index}_{mode}_{aux:x}_{af}',entry=entry,bounds=bounds,wide=wide,layout=layout,layout_flag=flag,layout_index=index,layout_mode=mode,auxiliary=aux,aux_flag=af)
 for entry in(0x2fb50,0x2fd70):
  for depth in range(9):
   for fov in(0,0x80000000,bits(1.1),0x7f7fffff,0xff7fffff):make(f'older_{entry:x}_{depth}_{fov:x}',entry=entry,depth=depth,changes=((CAMERA+0x28,fov,4),))
  for p in(CAMERA+0x28,BOUNDS,BOUNDS+4,BOUNDS+8,BOUNDS+12,0x46a0cc,0x46a0d0,0x46a0d4,0x46a514,0x46a670,0x46a260):
   for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
    make(f'fp_{entry:x}_{p:x}_{value:x}',entry=entry,changes=((p,value,4),),wide=1 if p in(0x46a514,0x46a670)else 0,auxiliary=AUX if p in(0x46a670,0x46a260)else 0,aux_flag=1)
 for depth in(0,1,3,6,7,8):
  for significand,exponent in((0,0),(0,0x8000),(1,0),(0x8000000000000000,0),(0x8000000000000000,0x7fff),(0xc123456789abcdef,0x7fff),(0x8123456789abcdef,0x7fff),(0x123456789abcdef,0x3fff)):
   make(f'older_tag_{depth}_{significand:x}_{exponent:x}',depth=depth,older=struct.pack('<QH',significand,exponent).hex(),changes=((CAMERA+0x28,0x7f7fffff,4),))
 for mask in(2,0x55,0xaa,0x80,0xfe):make(f'sparse_{mask:x}',occupied=mask,depth=0)
 for forward,up in(((0.,0.,0.),(0.,0.,0.)),((1e-6,0.,0.),(0.,0.,1.)),((1.,1.,1.),(1.,1.,1.)),((1.,2.,3.),(4.,5.,6.)),((1e20,1e-20,1.),(1e-20,1e20,2.))):make(f'basis_{forward}_{up}',forward=forward,up=up)
 for near in(0.,-0.,.01,-.01):
  for plane in((0.,0.,1.,0.),(-1.,0.,0.,1.),(1.,0.,0.,-1.),(1.,2.,3.,4.),(0.,0.,0.,0.)):
   for far in(0.,-1.,100.,1e30):make(f'plane_{near}_{plane}_{far}',near=near,plane=plane,far=far)
 for p in(CAMERA+0xc,CAMERA+0x10,CAMERA+0x14,CAMERA+0x18,CAMERA+0x1c,CAMERA+0x20,CAMERA+0x40,CAMERA+0x44,CAMERA+0x48,CAMERA+0x4c,CAMERA+0x50,CAMERA+0x54,0x46a0ec):
  for value in(0,0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'projection_fp_{p:x}_{value:x}',near=0.,changes=((p,value,4),))
 for scale in(0.,-0.,1.,2.,-2.,1e-30,1e30):make(f'inverse_scale_{scale}',entry=0x144090,scale=scale)
 for p in(range(FRAME,FRAME+0x34,4)):
  for value in(0,0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'inverse_fp_{p:x}_{value:x}',entry=0x144090,changes=((p,value,4),),scale=2.)
 for destination in(FRAME,FRAME+4,FRAME+16,FRAME-4,FRAME-16):make(f'inverse_alias_{destination:x}',entry=0x144090,destination=destination,scale=2.)
 for entry in(0x2fb50,0x2fd70):
  for destination in(CAMERA,CAMERA+4,CAMERA+0x10,BOUNDS,LAYOUT,AUX):make(f'output_alias_{entry:x}_{destination:x}',entry=entry,destination=destination)
 make('lazy_layout_after_program_width',entry=0x2fb50,input_changes=(dict(after_store=0,address=0x4f3218,bytes=LAYOUT.to_bytes(4,'little').hex()),),layout_flag=1,layout_index=0,layout_mode=3,wide=1)
 make('lazy_camera_after_basis_publish',input_changes=(dict(after_store=0,address=CAMERA,bytes=bits(17.).to_bytes(4,'little').hex()),))
 for entry in(0x2fb50,0x2fd70,0x144090):
  make(f'fault_seed_{entry:x}',entry=entry,depth=3)
  make(f'fault_seed_alias_{entry:x}',entry=entry,destination=CAMERA if entry!=0x144090 else FRAME,depth=3)
  make(f'actual_missing_output_{entry:x}',entry=entry,destination=0x40000000,actual_write_fault=True)
 make('fault_seed_projection_zero',forward=(0.,0.,0.),up=(0.,0.,0.),near=0.,depth=0)
 make('fault_seed_projection_oblique',near=0.,plane=(-1.,0.,0.,1.),depth=3)
 make('actual_missing_camera',camera=0x40000000,actual_read_fault=True)
 make('actual_missing_inverse_source',entry=0x144090,source=0x40000000,actual_read_fault=True)
 # Independent inverse-frame fault witness: the MAX_FLOAT*2 multiply occurs
 # after the 144194 publication, although its source component is read before.
 # A clean MXCSR makes any premature overflow/precision status observable.
 witness=[(FRAME+k*4,0,4)for k in range(13)]
 witness+=[(FRAME,bits(1.),4),(FRAME+0x24,0x7f7fffff,4),(FRAME+0x30,bits(-2.),4)]
 make('fault_seed_inverse_fp_store',entry=0x144090,depth=0,changes=witness)
 rows[-1]['before_fp']=[0x27f,0,0x1f80]
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x2fb50,0x2fd70,0x144090);meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical projection repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=sum(bool(r.get('actual_read_fault')or r.get('actual_write_fault'))for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_camera_projection.cpp':sha(ROOT/'tools/observe_native_tu15_camera_projection.cpp')},scope_limits=['FiniteTU1.5 projection graph; no host projection substitution or sourceCPU private frame','Masked FP exceptions and real sourcehardware x87 states, including sparse/overflow','Only authenticated numerical caller profiles; complete camera parent/GPU closure remains open'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical projection cases/{len(visited)} sites')
if __name__=='__main__':main()
