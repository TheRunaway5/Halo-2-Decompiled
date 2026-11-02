#!/usr/bin/env python3
"""Unchanged physical2F5E0/2F620/2F690 integer layout, aliases and divide faults."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,GRID,SPAN,TILE,OUT,INSET=0x1000000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000410
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'data':(BASE,4096),'window':(0x492000,4096),'stack':(geometry.STACK,65536)}
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
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_camera_view_layout.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs():
 rows=[]
 def make(name,entry,count=3,mode=0,ordinal=0,grid=(2,2),span=(1,1),tile=(0,0),window=(0,0,480,640),safe=(8,12,472,628),grid_address=GRID,span_address=SPAN,tile_address=TILE,outer=OUT,inset=INSET,changes=(),input_changes=(),**fields):
  n=len(rows);banks={BASE:bytearray((k*13+n*7+3)&255 for k in range(0x600)),0x4922aa:bytearray(16)}
  def put(at,value,width=2):
   raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
   for first,data in banks.items():
    if first<=at and at+width<=first+len(data):data[at-first:at-first+width]=raw;return
   banks[at]=bytearray(raw)
  for address,values in((GRID,grid),(SPAN,span),(TILE,tile)):
   for k,value in enumerate(values):put(address+k*2,value)
  for k,value in enumerate(window):put(0x4922aa+k*2,value)
  for k,value in enumerate(safe):put(0x4922b2+k*2,value)
  for address,value,width in changes:put(address,value,width)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[4]=geometry.SP+n%4*4
  if entry==0x2f5e0:gp[6]=count&0xffffffff;gp[2]=mode&0xffffffff;gp[7]=grid_address;args=(0,0,0)
  elif entry==0x2f620:gp[0]=ordinal&0xffffffff;gp[1]=grid_address;gp[2]=span_address;args=(count&0xffffffff,mode&0xffffffff,tile_address)
  else:gp[6]=tile_address;gp[3]=inset;args=(grid_address,span_address,outer)
  rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=args[0],source=args[1],output_argument=args[2],count=count,mode=mode,ordinal=ordinal,grid=grid_address,span=span_address,tile=tile_address,outer=outer,inset=inset,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=list(input_changes),**fields))
 for mode in(-2147483648,-1,0,1,2,2147483647):
  for count in(-2147483648,-1,0,1,2,3,4,5,6,7,8,9,10,15,16,17,31,32,33,64,127):make(f'grid_{mode}_{count}',0x2f5e0,count=count,mode=mode)
 for mode in(-1,0,1,2):
  for count in(-1,0,1,3,4,5):
   for ordinal in(-5,-1,0,1,2,3,4,5):make(f'tile_{mode}_{count}_{ordinal}',0x2f620,count=count,mode=mode,ordinal=ordinal)
 for grid in((-1,-1),(-2,3),(3,-2),(-32768,32767),(32767,-32768),(1,1)):
  for ordinal in(-2147483648,-32768,-1,0,1,32767,2147483647):make(f'tile_signed_{grid}_{ordinal}',0x2f620,grid=grid,ordinal=ordinal)
 for output in(GRID,GRID+2,SPAN,TILE):make(f'tile_alias_{output:x}',0x2f620,ordinal=2,tile_address=output)
 for output in(GRID,GRID+2,TILE):make(f'span_alias_{output:x}',0x2f620,ordinal=1,span_address=output)
 make('trap_first_adjustment_zero',0x2f620,grid=(0,2),count=-1,mode=0,ordinal=1)
 make('trap_first_adjustment_overflow',0x2f620,grid=(-1,-1),count=0,mode=0,ordinal=-2147483648)
 make('trap_first_coordinate_zero',0x2f620,grid=(0,1),count=0)
 make('trap_first_coordinate_overflow',0x2f620,grid=(-1,-1),count=0,mode=1,ordinal=2147483647)
 make('trap_fresh_coordinate_overflow',0x2f620,grid=(1,1),count=1,ordinal=-2147483648,input_changes=(dict(after_store=2,address=GRID,bytes='ffff'),))
 for grid in((1,1),(2,1),(1,2),(2,2),(3,3),(-2,2),(2,-2),(-32768,32767),(32767,-32768)):
  for tile in((0,0),(1,0),(0,1),(1,1),(2,2),(65535,65535),(32768,32767)):
   for span in((1,1),(2,1),(1,2),(65535,65535)):make(f'rectangle_{grid}_{tile}_{span}',0x2f690,grid=grid,tile=tile,span=span)
 for safe in((0,0,480,640),(-32768,32767,32767,-32768),(32767,-32768,-32768,32767),(10,20,10,20),(100,200,-100,-200)):
  make(f'signed_window_{safe}',0x2f690,window=(safe[1],safe[0],safe[3],safe[2]),safe=safe,tile=(1,1))
 for output in(GRID,SPAN,TILE,INSET,INSET+4,0x4922aa,0x4922b2):make(f'outer_alias_{output:x}',0x2f690,outer=output,tile=(1,1))
 for output in(GRID,SPAN,TILE,OUT,OUT+4,0x4922aa,0x4922b2):make(f'inset_alias_{output:x}',0x2f690,inset=output,tile=(1,1))
 make('trap_rectangle_width_zero',0x2f690,grid=(0,1))
 make('trap_rectangle_height_zero',0x2f690,grid=(1,0))
 make('fresh_boundary_grid_after_first_publish',0x2f690,tile=(1,1),input_changes=(dict(after_store=0,address=GRID,bytes='01000100'),))
 make('captured_window_after_first_publish',0x2f690,input_changes=(dict(after_store=0,address=0x4922aa,bytes='5500660077008800'),))
 make('fault_seed_grid',0x2f5e0,count=7)
 make('fault_seed_tile',0x2f620,count=3,ordinal=1)
 make('fault_seed_tile_span',0x2f620,count=3,ordinal=0,mode=1)
 make('fault_seed_rectangle_outer',0x2f690,tile=(0,0))
 make('fault_seed_rectangle_inset',0x2f690,grid=(3,3),tile=(1,1))
 make('actual_missing_grid_output',0x2f5e0,grid_address=0x40000000,actual_write_fault=True)
 make('actual_missing_grid_input',0x2f620,grid_address=0x40000000,actual_read_fault=True)
 make('actual_missing_rectangle_output',0x2f690,outer=0x40000000,actual_write_fault=True)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x2f5e0,0x2f620,0x2f690);meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical view layout repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=sum(bool(r.get('actual_read_fault')or r.get('actual_write_fault'))for r in rows),pending_rmw_cuts=sum(bool(r['boundary']and'pending_rmw_read'in r['boundary'])for r in rows),actual_integer_traps=sum(r['boundary']is not None and r['boundary']['phase']=='integer_divide'for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_camera_view_layout.cpp':sha(ROOT/'tools/observe_native_tu15_camera_view_layout.cpp')},scope_limits=['Integer layout only; no SIMD/x87 computation or private-stack reconstruction','Actual229A90 caller profiles; producer retirement requires separate integration','Original signed-count loop retains original termination behavior; no arbitrary iteration cap'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical layout cases/{len(visited)} sites')
if __name__=='__main__':main()
