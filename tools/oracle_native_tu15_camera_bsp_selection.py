#!/usr/bin/env python3
"""Physical unchanged2B700/14CDB0 BSP selection, retained history and fault prefixes."""
import argparse,copy,gzip,json,os,struct,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import validate_native_geometry_tu15 as geometry
sha=common.sha
BASE,BSP,TREE,NODES,PLANES,POINT,LEAF,CLUSTER,LEAVES=0x1000000,0x1000100,0x1000300,0x1000400,0x1000600,0x1000800,0x1000900,0x1000904,0x1000a00
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def prepare(image,meta,scratch):
 p=scratch/'physical-original';p.mkdir();section=image.section('.text');(p/'text.bin').write_bytes(image.section_bytes(section));(p/'metadata.json').write_text(json.dumps(meta))
 regions={'data':(BASE,4096),'binding':(0x4ec000,4096),'stack':(geometry.STACK,65536)}
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
 for command in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')],['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_tu15_camera_bsp_selection.cpp'),'-o',str(p/'observer')]):
  r=subprocess.run(command,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p
def inputs():
 rows=[]
 def make(name,entry=0x2b700,negative=0x800002,positive=0x800003,point=(1.,2.,3.),plane=(1.,0.,0.,0.),old_leaf=1,old_cluster=2,leaf_count=4,cluster_count=4,cluster_value=2,leaf_output=LEAF,cluster_output=CLUSTER,plane_index=0,node=0,plane_pointer=None,chain=False,changes=(),input_changes=(),**fields):
  n=len(rows);data=bytearray((k*13+n*7+3)&255 for k in range(0xc00));banks={BASE:data,0x4ecc18:bytearray(struct.pack('<I',BSP))}
  def put(at,value,width=4):
   raw=int(value&((1<<(width*8))-1)).to_bytes(width,'little')
   if BASE<=at and at+width<=BASE+len(data):data[at-BASE:at-BASE+width]=raw
   else:banks[at]=bytearray(raw)
  for at,v in[(BSP+0x18,TREE),(BSP+0x2c,leaf_count),(BSP+0x30,LEAVES),(BSP+0x9c,cluster_count),(TREE+4,NODES),(TREE+0xc,PLANES if plane_pointer is None else plane_pointer),(LEAF,old_leaf),(CLUSTER,old_cluster)]:put(at,v)
  put(NODES,plane_index,2);put(NODES+2,negative,3);put(NODES+5,positive,3)
  for k,v in enumerate(plane):put(PLANES+k*4,bits(v))
  for k,v in enumerate(point):put(POINT+k*4,bits(v))
  for k in range(8):put(LEAVES+k*8,cluster_value,2)
  if chain:
   put(NODES+2,1,3);put(NODES+5,1,3)
   for k in range(1,4):
    put(NODES+k*8,k,2);put(NODES+k*8+2,(k+1 if k<3 else negative),3);put(NODES+k*8+5,(k+1 if k<3 else positive),3)
    for lane,value in enumerate((0.,0.,0.,float(k-2))):put(PLANES+k*16+lane*4,bits(value))
  for address,value,width in changes:put(address,value,width)
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[0]=node;gp[1]=TREE;gp[2]=POINT;gp[7]=leaf_output;gp[4]=geometry.SP+n%4*4
  rows.append(dict(id=n,name=name,entry=entry,gp=gp,matrix=cluster_output,source=0,tree=TREE,point=POINT,node=node,leaf_output=leaf_output,cluster_output=cluster_output,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n*0x100)&0x4700)|(n*3%64),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]|(n*7%64)],live_x87=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range((0,1,3,8)[n%4])],input_changes=list(input_changes),**fields))
 for entry in(0x2b700,0x14cdb0):
  for x in(-1.,0.,-0.,1.):make(f'sides_{entry:x}_{bits(x):x}',entry=entry,point=(x,0.,0.))
  make(f'none_{entry:x}',entry=entry,positive=0xffffff,negative=0xffffff)
  make(f'chain_{entry:x}',entry=entry,chain=True)
  for index in(-32768,-1,32767):make(f'signed_plane_{entry:x}_{index}',entry=entry,plane_index=index,plane_pointer=(PLANES-index*16)&0xffffffff)
 for node in(0x800000,0xff800000):make(f'masked_start_{node:x}',entry=0x14cdb0,node=node)
 for leaf in(0,1,3,4,-1,-2147483648,2147483647):
  for cluster in(-1,0,3,4):make(f'history_{leaf}_{cluster}',positive=0xffffff,negative=0xffffff,old_leaf=leaf,old_cluster=cluster)
 for count in(-1,0,1,2147483647):
  make(f'signed_leaf_count_{count}',positive=0xffffff,old_leaf=1,leaf_count=count)
  make(f'signed_cluster_count_{count}',positive=0xffffff,old_cluster=1,cluster_count=count)
 for value in(-32768,-1,0,32767):make(f'signed_cluster_{value}',cluster_value=value)
 for at in(POINT,POINT+4,POINT+8,PLANES,PLANES+4,PLANES+8,PLANES+12):
  for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):
   make(f'fp_{at:x}_{value:x}',changes=((at,value,4),),plane=(1.,1.,1.,0.))
 for k in range(16):make(f'rounding_denorm_{k}',point=(1.000000119,1.000000119,1.000000119),plane=(1.000000119,1.000000119,1.000000119,3.000000715))
 make('association',point=(1.,1.,1.),plane=(1.,-33554432.,33554432.,.5))
 for output in(LEAF,CLUSTER,LEAVES+3*8,BSP+0x30,BSP+0x2c,TREE+4,0x4ecc18):make(f'leaf_alias_{output:x}',leaf_output=output,actual_read_fault=True) if output==BSP+0x30 else make(f'leaf_alias_{output:x}',leaf_output=output)
 for output in(LEAF,BSP+0x30,POINT+8):make(f'cluster_alias_{output:x}',cluster_output=output)
 make('fresh_leaf_bank_after_publish',input_changes=(dict(after_store=0,address=BSP+0x30,bytes=struct.pack('<I',LEAVES+8).hex()),),changes=((LEAVES+4*8,0xfffd,2),))
 make('cached_bsp_binding_after_publish',input_changes=(dict(after_store=0,address=0x4ecc18,bytes=struct.pack('<I',0x40000000).hex()),))
 make('fault_seed_success');make('fault_seed_none',positive=0xffffff,old_leaf=-1)
 make('fault_seed_valid_history',positive=0xffffff);make('fault_seed_helper',entry=0x14cdb0,chain=True)
 make('actual_missing_point',changes=((TREE+4,NODES,4),),actual_read_fault=True)
 rows[-1]['gp'][2]=0x40000000;rows[-1]['point']=0x40000000
 make('actual_missing_leaf_output',leaf_output=0x40000000,actual_write_fault=True)
 make('actual_missing_cluster_output',cluster_output=0x40000000,actual_write_fault=True)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 image=Xbe(NEW_DEFAULT);common.ENTRIES=(0x2b700,0x14cdb0);meta=common.metadata(image);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);owned=prepare(image,meta,scratch);rows=geometry.physical(owned,inputs(),'complete');prefix=[]
 for seed in(r for r in rows if r['name'].startswith('fault_seed')):
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n in range(len(events)):
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows));uncovered=[s['instruction']for s in meta['sites']if s['instruction']not in visited];assert not uncovered,uncovered
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=meta['owners'],instruction_patches=[],supplied_original_outcomes=False,cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0));repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Physical BSP selection repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),fault_prefixes=len(prefix),actual_source_faults=sum(r.get('actual_read_fault',False)or r.get('actual_write_fault',False)for r in rows),original_sites=len(visited),original_instructions=sum(r['original_instructions']for r in rows),ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),owners=meta['owners'],fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),original_sha256=NEW_PIN,repeat_identical=repeat,owned_scratch=str(scratch),retained_large_artifacts=[],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_tu15_camera_bsp_selection.cpp':sha(ROOT/'tools/observe_native_tu15_camera_bsp_selection.cpp')},scope_limits=['RealBSP retained point selection, not whole scene/campaign closure','Masked SIMD exceptions; original cycle behavior is not replaced with a cap','Actual2B7E6 caller binding only; producer retirement requires separate integration'])
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical BSP cases/{len(visited)} sites')
if __name__=='__main__':main()
