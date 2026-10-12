#!/usr/bin/env python3
"""Actual ordinary effect update and natural outer-frame stop, no time input."""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
import unicorn
from unicorn.x86_const import *
import oracle_smg_particle_owner_1_1 as birth
from oracle_mp_spawn_process_heap_world import KernelBoundary
from functions import _trace

ROOT=birth.ROOT;OUT=ROOT/'analysis/multiplayer/integration/inventory-source/smg-effect-lifecycle'
FROZEN={**birth.FROZEN,
 'tools/oracle_smg_particle_owner_1_1.py':'f57311f72a977e9e1ce18ba461327baec9db6ca3b39b0de70a8a2f0bb8c67c43',
 'analysis/multiplayer/integration/inventory-source/smg-particle-owner/report.json':'f96a611cae7fc4feec0606c9f2086653ea6206134f2a1e043633d7cd16a6bf17',
 'analysis/multiplayer/integration/inventory-source/smg-particle-owner/manifest.json':'abe8cd8fbbca908a4f3ea7d0b36da07553205fce9162dadf12a3bd22bef81c6f'}
ENTRIES={0x1788b0,0x17af50,0x17b1a0,0x17ad20,0x178f60,0x179000,0x178c10,0x178e10,0x178ed0,
 0x12cbc0,0x12c850,0x147dd0,0x139640,0x1789d0,0x176410,0x1764b0,0x24a820,0x249210,0x249880,
 0x24a410,0x24a7b0,0x176e70,0x175c00,0x16d4c0,0x17c9f0,0x326983}

class LifecycleTrace(birth.ParticleTrace):
 def __init__(self,*a,**kw):
  self.life_on=False;self.life_calls=[];self.life_pending=[];self.life_reads=[];self.life_stores=[];self.life_points=[];self.life_states=[];self.life_state_ids={};self.life_order=0;self.life_executed=set();self.life_external=[];self.life_stages=[];self.ever_slots={}
  super().__init__(*a,**kw)
  self.life_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES};self.life_owners={a:e for e,r in self.life_routines.items()for a in r.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.life_read)
 def life_snapshot(self):
  state=self.snapshot();retained=[]
  for pool in state['pools']:
   key=pool['global_address'];slots=self.ever_slots.setdefault(key,set());slots.update(q['index']for q in pool.get('occupied',[]))
   if 'stride'in pool:
    data=int(pool['data_pointer'],16);retained.append(dict(global_address=key,slots=[dict(index=i,pointer=f'{data+i*pool["stride"]:08x}',raw_hex=self.raw(data+i*pool['stride'],pool['stride']))for i in sorted(slots)]))
  return dict(current=state,retained_ever_allocated_slots=retained,source_frame_globals=[dict(address=f'{p:08x}',raw_hex=self.raw(p,n))for p,n in((0x4c1554,4),(0x4c1560,8),(0x48d040,4),(0x4f4864,4),(0x4f496c,2),(0x510b30,1),(0x54fa7a,1),(0x54fabe,10))])
 def life_state(self):
  state=self.life_snapshot();key=hashlib.sha256(birth.encoded(state)).hexdigest()
  if key not in self.life_state_ids:self.life_state_ids[key]=len(self.life_states);self.life_states.append(dict(id=len(self.life_states),sha256=key,**state))
  return self.life_state_ids[key]
 def life_return(self,a):
  matches=[q for q in self.life_pending if q[0]==a]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,row in reversed(matches):
   if sp!=depth:continue
   row.update(return_order=self.life_order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,after_state=self.life_state());self.life_order+=1;self.life_pending.remove((ret,sp,row))
 def hook(self,g,a,n,user):
  if self.life_on:
   self.life_return(a)
   if self.code.at(a)is not None:self.life_executed.add(a)
   else:self.life_external.append(dict(order=self.life_order,address=f'{a:08x}',size=n,cpu=self.cpu(),guest_bytes_hex=self.raw(a,n),scope='Outside source text; explicit compatibility/return sentinel boundary, not audited original instructions.'))
   if a in self.starts or a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row=dict(id=len(self.life_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.life_order,entry_cpu=self.cpu(),stack12_words=[f'{self.u(sp+4+i*4):08x}'for i in range(12)])
    if a in ENTRIES:row['before_state']=self.life_state()
    self.life_order+=1;self.life_calls.append(row);self.life_pending.append((self.u(sp),sp,row))
   if a in self.life_owners:
    i=self.code.at(a)
    if i.mnemonic.startswith(('j','f'))or i.mnemonic=='call'or i.mnemonic.endswith('ss'):self.life_points.append(dict(order=self.life_order,instruction=f'{a:08x}',owner=f'{self.life_owners[a]:08x}',cpu=self.cpu(),stack80_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x80)));self.life_order+=1
  super().hook(g,a,n,user)
 def life_read(self,g,access,p,n,v,user):
  if self.life_on:self.life_reads.append(dict(order=self.life_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,raw_hex=self.raw(p,n)));self.life_order+=1
 def mem_write(self,g,access,p,n,v,user):
  if self.life_on:self.life_stores.append(dict(order=self.life_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,before_hex=self.raw(p,n),written_hex=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()));self.life_order+=1
  super().mem_write(g,access,p,n,v,user)
 def life_stage(self,name,entry):
  self.active=name;self.new_invalid=None;first=len(self.events);before=self.life_state()
  try:self.run(entry);completion='complete_original_return';self.life_return(birth.local.base.STOP)
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None;completion='blocked_original_instruction'
  row=dict(name=name,entry=f'{entry:08x}',arguments=[],before_state=before,after_state=self.life_state(),completion=completion,stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',invalid=self.new_invalid,services=self.events[first:],after_cpu=self.cpu(),pending_original_calls=[r['id']for _,_,r in self.life_pending]);self.life_stages.append(row);return completion=='complete_original_return'

def prepare(data):
 local=birth.local;t=local.base.owner.player_source.prepare(data,trace_type=LifecycleTrace);assert local.base.owner.player_source.spawn_and_activate(t,0,activate=False)
 prior=json.loads((local.base.owner.OUT/'report.json').read_text());t.item_observe=True
 for r in prior['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 fields=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{v:08x}'for v in sorted(t.executed)],metadata=local.base.owner.metadata(t))
 for k,v in fields.items():assert v==prior[k],('prior item flow changed',k)
 t.item_observe=False;t.fp_resources();t.effect_on=True;t.birth_on=True;prior=json.loads((local.OUT/'audio-unavailable/report.json').read_text())
 for r in prior['stages']:
  if r['name']=='declared_local_view_request_owner':
   for e in(0x168866,0x16a356,0x107b50,0x179b90):t.services.pop(e,None)
   t.services[0x18ad70]=(prior['audio_boundary']['scope'],-1,24)
  if r['name']=='source_control_conversion':
   cmd=int(prior['declared_requests']['command']['pointer'],16);assert t.raw(cmd,0x5c)==prior['declared_requests']['command']['before5c_hex'];t.w(cmd,0x50000);t.w(cmd+0x14,0x3f800000)
  assert t.effect_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['registers'].items()))
 t.finish_birth(local.base.STOP);assert not t.birth_pending
 for k,v in(dict(stages=t.effect_stages,calls=t.effect_calls,states=t.effect_states,reads=t.effect_reads,stores=t.effect_stores,points=t.effect_points,final_state=t.snapshot())).items():assert v==prior[k],('prior local flow changed',k)
 prior_birth=json.loads((birth.OUT/'report.json').read_text())
 for k,v in dict(calls=t.birth_calls,points=t.birth_points,reads=t.birth_reads,stores=t.birth_stores).items():assert v==prior_birth[k],('prior birth flow changed',k)
 t.birth_on=False;t.effect_on=False;t.life_on=True
 return t

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');a=p.parse_args();pin=birth.sha(__file__)
 for f,h in FROZEN.items():assert birth.sha(ROOT/f)==h,f
 source=birth.local.base.owner.player_source;proc=subprocess.Popen(['wine',str(source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t=prepare(data)
  assert t.life_stage('original_fixed_effect_phase_after_actual_birth',0x1788b0)
  print(json.dumps(t.life_stages[-1]),flush=True)
  t.life_stage('original_outer_frame_owner_without_elapsed_input',0x12cbc0)
  print(json.dumps(t.life_stages[-1]),flush=True)
  listings=[]
  for label,addresses in [(f'{e:08x}',r.code)for e,r in sorted(t.life_routines.items())]+[('all-executed',t.life_executed),('all-stores',{int(v['instruction'],16)for v in t.life_stores if t.code.at(int(v['instruction'],16))is not None})]:
   ins=[t.code.at(p)for p in sorted(addresses)]
   for i in ins:assert i.bytes==t.image.read(i.address,i.size)==bytes(t.g.mem_read(i.address,i.size))
   path=a.output.parent/'instructions'/f'{label}.asm';birth.save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(a.output.parent)),sha256=birth.sha(path),rows=len(ins),executed_rows=sum(i.address in t.life_executed for i in ins)))
  for f,h in FROZEN.items():assert birth.sha(ROOT/f)==h,f
  assert pin==birth.sha(__file__)
  report=dict(schema='source-smg-effect-lifecycle-v1',oracle_sha256=pin,source_xbe_sha256=birth.local.base.PIN,source_map_sha256=birth.local.base.CACHE_PIN,frozen_pins=FROZEN,prior_item_local_and_birth_all_measured_fields_equal=True,stages=t.life_stages,calls=t.life_calls,states=t.life_states,reads=t.life_reads,stores=t.life_stores,points=t.life_points,instructions=listings,outside_source_text=t.life_external,new_guest_inputs=[],new_supplied_service_results=[],instruction_patches=[],boundaries=['Complete original1788B0 fixed effect phase is a declared root after immutable positive birth; no game tick count or complete frame admission inferred.','Whole original12CBC0 is attempted with no explicit elapsed input; first unavailable dependency is retained.','Existing externalview/fire/failed-audio/resource/finiteclock/platform boundaries remain. No new timing, positions, node poses, PVS, contacts, allocation success or tick increments supplied.','Particle aging via139640→1789D0→176410→1764B0→24A820→249210 is listed-only unless naturally entered here; actual effect dt and outer elapsed ownership are distinct. No rendered flash or continuous lifetime inferred.'])
  birth.save(a.output,birth.encoded(report));print(json.dumps(dict(output=str(a.output),sha256=birth.sha(a.output),stages=[dict(name=r['name'],completion=r['completion'],eip=r['stopped_eip'],invalid=r['invalid'])for r in t.life_stages],calls=len(t.life_calls),states=len(t.life_states),stores=len(t.life_stores))),flush=True)
 finally:
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='worker shutdown timeout'
 if proc.returncode:raise RuntimeError((proc.returncode,err))
if __name__=='__main__':main()
