#!/usr/bin/env python3
"""Unchanged arena lease/secondary retirement composed with real native page protection."""
import argparse,copy,gzip,json,os,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
import oracle_native_campaign_screen_lifecycle_tu15 as common
import oracle_native_campaign_ai_resource_retirement_tu15 as resource
import validate_native_geometry_tu15 as geometry
ENTRIES=(0x149680,0x1498e0,0x1d2d90,0x1d2e30,0x233120)+resource.ENTRIES
PRIMARY,SECONDARY=resource.BASE+0x8000,resource.BASE+0x8100
WORKSPACE=0x82000000
sha=common.sha

def prepare(image,meta,scratch):
 p=resource.prepare(image,meta,scratch)
 regions={'arena':(0x486000,0x1000),'descriptors':(0x502000,0x1000),'backing':(0x51d000,0x1000),'lease':(0x52b000,0x1000),'vtable':(0x463000,0x1000),'base_vtable':(0x45f000,0x1000),'workspaces':(WORKSPACE,0x46000)}
 assembly=(p/'reference.s').read_text()
 for name,(_,size)in regions.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
 (p/'reference.s').write_text(assembly)
 linker=(p/'link.ld').read_text();assert linker.endswith('}\n');linker=linker[:-2]+''.join(f'.{n} 0x{a:x}:{{*(.{n})}} 'for n,(a,_)in regions.items())+'}\n';(p/'link.ld').write_text(linker)
 for cmd in(['as','--32',str(p/'reference.s'),'-o',str(p/'reference.o')],['ld','-melf_i386','-T',str(p/'link.ld'),str(p/'reference.o'),'-o',str(p/'reference')]):
  r=subprocess.run(cmd,text=True,capture_output=True);assert not r.returncode,r.stderr
 return p

def inputs(image):
 cases=[]
 source=[r for r in resource.inputs(image)if r['entry']==0x130030 and(r['name'].startswith('fault_seed')or r['name']in('delete_130030_0_True','delete_130030_32_True','next_publication_replaces_container_global','previous_publication_replaces_metadata_pool','callback_replaces_container_pool_130030','unknown_actual_protection_import','unknown_delete_callback_130030','unknown_gpu_association_130030','actual_source_read_fault_after_partial_unlink'))]
 def make(seed,entry,primary=True,secondary=True,initialized=0,vtable=0x46393c):
  row=copy.deepcopy(seed);row.update(id=len(cases),entry=entry,name=f'{entry:x}_primary{int(primary)}_secondary{int(secondary)}_ready{initialized}_'+seed['name']);row['gp'][4]=geometry.SP+len(cases)%4*4
  extras={PRIMARY:bytearray(0x40),SECONDARY:bytearray(0x40),0x486050:bytearray(0x30),0x502080:bytearray(0x30),0x51df40:bytearray(0x10),0x52be50:bytearray(0x10),0x46395c:bytearray(image.read(0x46395c,4)),0x45f4c0:bytearray(image.read(0x45f4c0,4))}
  def put(p,v,width=4):
   for start,data in extras.items():
    if start<=p and p+width<=start+len(data):data[p-start:p-start+width]=int(v&((1<<(width*8))-1)).to_bytes(width,'little');return
   raise ValueError(hex(p))
  put(PRIMARY,vtable);put(SECONDARY,vtable);put(0x48605c,PRIMARY-0x105410);put(0x486060,resource.USER);put(0x486070,PRIMARY if primary else 0);put(0x486074,SECONDARY if secondary else 0)
  put(0x51df44,WORKSPACE);put(0x51df48,0x9b,1);put(0x52be54,0xdeadbeef);put(0x52be58,initialized,1)
  row['banks']+= [dict(address=p,before=b.hex())for p,b in extras.items()];row['banks'].sort(key=lambda b:b['address'])
  if entry==0x1d2d90 and(initialized or not primary):
   row.pop('stop_pc',None);row.pop('required_owner',None);row.pop('required_identity',None);row.pop('actual_read_fault',None)
  if not secondary:
   row.pop('stop_pc',None);row.pop('required_owner',None);row.pop('required_identity',None);row.pop('actual_read_fault',None)
  cases.append(row)
 for seed in source:
  for entry in(0x1498e0,0x1d2d90):
   for primary in(False,True):make(seed,entry,primary=primary)
 seed=next(r for r in source if r['name']=='fault_seed_130030')
 for entry in(0x1498e0,0x1d2d90):
  make(seed,entry,secondary=False)
  make(seed,entry,vtable=0x45f4a0)
 make(seed,0x1d2d90,initialized=1)
 return cases

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--verify',type=Path);a=p.parse_args();assert not a.output.exists();assert sha(NEW_DEFAULT)==NEW_PIN
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);image=Xbe(NEW_DEFAULT);common.ENTRIES=ENTRIES;m=common.metadata(image);owned=prepare(image,m,scratch);rows=geometry.physical(owned,inputs(image),'complete');prefix=[]
 read_modify={s['instruction']for s in m['sites']if any(o['read']and o['write']for o in s['operands'])}
 seeds=[r for r in rows if r['name'].endswith('fault_seed_130030')and'primary1_secondary1_ready0_'in r['name']][:2]
 for seed in seeds:
  for phase,events in(('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
   for n,event in enumerate(events):
    if phase=='before_write'and event['instruction']in read_modify:continue
    row=copy.deepcopy(seed);row.update(id=len(rows)+len(prefix),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));prefix.append(row)
 rows+=geometry.physical(owned,prefix,'prefix');visited=set().union(*(set(r['executed_sites'])for r in rows))
 fixture=dict(image_sha256=NEW_PIN,backend='physical-unchanged-i386-offline',owners=m['owners'],instruction_patches=[],supplied_original_outcomes=False,platform_service='real-linux-i386-mprotect-stdcall-bridge',physical_allocation=dict(address=resource.PHYSICAL,bytes=0x4000),workspace_allocation=dict(address=WORKSPACE,request_bytes=0x452e8,bytes=0x46000),cases=rows);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0))
 repeat=None
 if a.verify:repeat=a.output.read_bytes()==a.verify.read_bytes();assert repeat,'Independent composed original repeat differs'
 report=dict(passed=True,cases=len(rows),complete_calls=sum(r['completed']for r in rows),required_owner_boundaries=sum('required_owner'in r for r in rows),actual_read_faults=sum(r.get('actual_read_fault',False)for r in rows),fault_prefixes=len(prefix),original_instructions=sum(r['original_instructions']for r in rows),original_sites=len(visited),executed_sites=sorted(visited),owners=m['owners'],original_sha256=NEW_PIN,fixture=str(a.output),fixture_bytes=a.output.stat().st_size,fixture_sha256=sha(a.output),repeat_identical=repeat,ordered_reads=sum(len(r['reads'])for r in rows),ordered_stores=sum(len(r['writes'])for r in rows),platform_protections=sum(len(r['services'])for r in rows),owned_scratch=str(scratch),retained_large_artifacts=[],scope_limits=['Composed1498E0/1D2D90 secondary retirement with actual233120(arg0) and unassociated12FD10 callback','Real workspace backing and protected allocation are separately owned; no path search or result supplied','Associated GPU retirement and unknown callback/import selectors remain real required owners'],source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),'tools/observe_native_campaign_ai_resource_retirement_tu15.cpp':sha(ROOT/'tools/observe_native_campaign_ai_resource_retirement_tu15.cpp'),'tools/oracle_native_campaign_ai_resource_retirement_tu15.py':sha(ROOT/'tools/oracle_native_campaign_ai_resource_retirement_tu15.py')})
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(f'{len(rows)} physical composed arena comparisons, {len(visited)} source sites; fixture {a.output.stat().st_size} bytes')
if __name__=='__main__':main()
