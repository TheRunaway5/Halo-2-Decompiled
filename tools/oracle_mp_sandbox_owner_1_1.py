#!/usr/bin/env python3
"""Separate original Lockout crate factory/world ownership dependency experiment.

Reuses immutable source-owned world preparation. The real scenario factory
builds placement requests and allocates objects; no header, body, pose, contact
or activation success field is supplied. First unsupported dependency retained.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EDI,UC_X86_REG_ECX
from oracle_mp_spawn_player_visibility import PlayerTrace,prepare,FROZEN as PRIOR,CONTEXT_OUT,ROOT
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STACK,STOP,PIN,CACHE_PIN,_trace
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_mp_spawn_complete_kpcr import digest
FROZEN={**PRIOR,
 'tools/oracle_mp_spawn_player_visibility.py':'aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572',
 'analysis/multiplayer/integration/spawn-source/player-visibility/report.json':'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd'}
ENTRIES=(0xbffe0,0xd6660,0xb8820,0xb8a30,0x11d1c0,0x11d2b0,0x11d330,
 0x1d5830,0x1d5a60,0x1d5e90,0x1d6f40,0x1c7050,0x1d98b0,
 0xd7160,0xdc0a0,0xd73e0,0xdb710,0xdb470,0xdb360,0xd8280,0x1243c0,0x1ee380,0x1ee8f0,
 0x16af90,0x1f3500,0x1f3900,0xda0e0,0x16a5c0,0x16a560)
class SandboxTrace(PlayerTrace):
 def __init__(self,data):
  self.sandbox_enabled=False;self.sandbox_calls=[];self.sandbox_stores=[];self.sandbox_executed=set();self.sandbox_returns=[]
  super().__init__(data)
 def hook(self,g,a,size,user):
  if self.sandbox_enabled:
   self.sandbox_executed.add(a)
   for ret,row in self.sandbox_returns[:]:
    if a==ret:row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';self.sandbox_returns.remove((ret,row))
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'entry':f'{a:08x}','phase':self.active,'registers':self.registers(),'return_site':f'{self.u(sp):08x}','stack':[f'{self.u(sp+4+4*i):08x}'for i in range(6)]}
    if a==0xb8a30:row['source_request_c4_hex']=bytes(g.mem_read(self.u(sp+4),0xc4)).hex()
    self.sandbox_calls.append(row);self.sandbox_returns.append((self.u(sp),row))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.sandbox_enabled:self.sandbox_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}','address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def selected_physics_fixup(self,datum):
  c=self.cache;definition=c.tags[datum];model=c.tags[c.u(definition['offset']+0x38)];phmo=c.tags[c.u(model['offset']+0x24)]
  p=phmo['offset']+c.magic;raw=[]
  for off,stride in ((0x38,0x90),(0x48,0x80),(0x50,0xb0),(0x58,0x50),(0x60,0x90),(0x68,0x60),(0x70,0x100),(0x90,0x38),(0xa0,0x14),(0x108,0x20)):
   for q in c.block(phmo['offset']+off,stride):raw.append({'bank':f'{off:03x}','va':f'{q+c.magic:08x}','stride':stride,'before_hex':bytes(self.g.mem_read(q+c.magic,stride)).hex()})
  it=PLACEMENT+0x200;self.w(it+12,int(phmo['datum'],16)&0xffff);self.w(it+16,0xffffffff)
  self.sandbox_enabled=True;self.active='original_selected_phmo_tag_iterator';start=len(self.events)
  self.run(0x1243c0,regs=((UC_X86_REG_EDI,it),));assert self.g.reg_read(UC_X86_REG_EAX)==int(phmo['datum'],16)
  self.fragment('original_selected_phmo_map_postprocess',0x11de40,0x11ded1,((UC_X86_REG_EAX,int(phmo['datum'],16)),))
  for r in raw:r['after_hex']=bytes(self.g.mem_read(int(r['va'],16),r['stride'])).hex()
  self.selected_fixup={'tag':phmo,'iterator_input_index':int(phmo['datum'],16)&0xffff,'iterator_result':phmo['datum'],'dispatch_start':'0011de40','dispatch_stop_exclusive':'0011ded1','whole_map_postprocess_claimed':False,'records':raw,'used_services':self.events[start:]}
  self.sandbox_enabled=False
 def collision_cases(self,datum):
  obj=next(r for r in self.state()['objects']if r['datum']==f'{datum:08x}');p=int(obj['pointer'],16)
  root=struct.unpack('<3f',bytes(self.g.mem_read(p+0x64,12)));query=PLACEMENT+0x800;rows=[]
  self.active='original_projectile_collision_mask_conversion';self.run(0x16a560,regs=((UC_X86_REG_ECX,0x2480000f),));flags=self.g.reg_read(UC_X86_REG_EAX);assert flags==9
  for axis in range(3):
   for side in (-0.5,0.,0.5):
    origin=list(root);origin[axis]-=2.;origin[(axis+1)%3]+=side;delta=[0.,0.,0.];delta[axis]=4.
    self.g.mem_write(query,struct.pack('<3f',*origin));self.g.mem_write(query+0x10,struct.pack('<3f',*delta));self.g.mem_write(query+0x20,bytes(0x5c));self.f(query+0x24,1.)
    self.active='direct_original_collision_query_source_created_crate';start=len(self.events);ci=len(self.sandbox_calls);self.new_invalid=None
    try:self.run(0x16af90,(datum,1,0x2480000f,flags,query,query+0x10,query,query+0x10,query+0x20));complete=True
    except (unicorn.UcError,KernelBoundary):complete=False;assert self.new_invalid is not None
    rows.append({'explicit_query_axis':axis,'explicit_query_side_offset':side,'explicit_origin_hex':bytes(self.g.mem_read(query,12)).hex(),'explicit_delta_hex':bytes(self.g.mem_read(query+0x10,12)).hex(),'initial_output_fixture':'zero_5c_with_float_fraction_one','original_16a560_converted_mask':flags,'target_source_datum':f'{datum:08x}','target_source_body_before':obj,'completed':complete,'return_eax':f'{self.g.reg_read(UC_X86_REG_EAX):08x}','output5c_hex':bytes(self.g.mem_read(query+0x20,0x5c)).hex(),'invalid':self.new_invalid,'used_services':self.events[start:],'calls':self.sandbox_calls[ci:],'direct_selected_object_query_not_full_world_broadphase':True})
    if not complete:break
   if not complete:break
  return rows
 def factory(self,datum):
  c=self.cache;rows=c.block(c.scenario+0x328,0x4c);pal=c.block(c.scenario+0x330,0x28)
  idx=next(i for i,p in enumerate(rows)if c.u(pal[c.h(p)]+4)==datum);p=rows[idx]
  row={'authored_tag':c.tags[datum],'authored_placement_index':idx,'authored_placement_va':f'{p+c.magic:08x}','authored_placement_hex':bytes(c.data[p:p+0x4c]).hex(),'authored_palette_block_va':f'{c.scenario+c.magic+0x330:08x}',
   'before':self.state(),'prior_options_variant_and_nonlocal_world_preparation_explicit':True}
  self.sandbox_enabled=True;self.active='original_authored_scenario_crate_factory';self.new_invalid=None;start=len(self.events)
  try:self.run(0xbffe0,(11,idx,c.scenario+c.magic+0x330,1,1),((UC_X86_REG_EAX,p+c.magic),));complete=True
  except (unicorn.UcError,KernelBoundary):complete=False;assert self.new_invalid is not None
  row.update(completed=complete,stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',invalid=self.new_invalid,
   after=self.state(),calls=self.sandbox_calls,ordered_stores=self.sandbox_stores,used_services=self.events[start:])
  row['object_header_body_pose_contacts_flags_or_allocation_success_supplied']=False
  return row
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);ap.add_argument('--tag',choices=('power_module','lift'),default='power_module');ap.add_argument('--fixup',action='store_true');ap.add_argument('--collision',action='store_true');ap.add_argument('--context-replay',type=Path);args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 args.output.parent.mkdir(parents=True,exist_ok=True);proc=None;err=None
 if args.context_replay:data=json.loads(args.context_replay.read_text())
 else:
  err=(args.output.parent/'runtime-stderr.txt').open('w');proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')));data=json.loads(proc.stdout.readline())
 try:
  t=prepare(data,trace_type=SandboxTrace);assert t.player_stages[-1]['completion']=='complete_original_return'
  datum=0xe6f80583 if args.tag=='power_module'else 0xe6ed0578
  if args.fixup:t.selected_physics_fixup(datum)
  row=t.factory(datum)
  collisions=t.collision_cases(int(row['returned_eax'],16))if args.collision and row['completed']else []
 finally:
  if proc:
   proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();err.close();assert proc.returncode==0
 context=args.output.parent/'raw-host-context.json';context.write_text(json.dumps(data,indent=2)+'\n')
 insdir=args.output.parent/'instructions';insdir.mkdir(exist_ok=True);listings=[]
 for a in ENTRIES:
  f=_trace(t.code,a,t.starts|set(ENTRIES));ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert i.bytes==t.image.read(i.address,len(i.bytes))==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=insdir/f'{a:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append({'entry':f'{a:08x}','file':p.name,'sha256':digest(p),'instructions':len(ins)})
 report={'oracle_sha256':digest(__file__),'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'prior_frozen_pins':FROZEN,'context_sha256':digest(context),'context_mode':'whole_recorded_compatibility_context'if args.context_replay else'whole_context_actual_live_Wine_worker','case':row,'instruction_listings':listings,'source_executed_addresses':[f'{p:08x}'for p in sorted(t.sandbox_executed)],
  'direct_source_collision_cases':collisions,'selected_original_physics_postprocess':getattr(t,'selected_fixup',None),'inherited_world_stages':t.live_stages,'inherited_player_stages':t.player_stages,'inherited_producers':t.producers,'inherited_services':{f'{a:08x}':str(v)for a,v in t.services.items()},'native_gameplay_update_applied':False,'full_original_scene_world_contact_explosion_lift_or_hardware_parity_proven':False}
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'completed':row['completed'],'stop':row['stopped_eip'],'datum':row['returned_eax'],'invalid':row['invalid'],'output':str(args.output)}),flush=True)
if __name__=='__main__':main()
