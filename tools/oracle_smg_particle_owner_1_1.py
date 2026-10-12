#!/usr/bin/env python3
"""Measure real mapped emitter/birth calls from immutable local SMG effect flow.

No additional guest writes, allocation/birth result or substituted timing.
The prior complete unavailable-audio case is repeated byte-identically first.
"""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
from unicorn.x86_const import *
import oracle_smg_local_effect_owner_1_1 as local
from oracle_smg_effect_owner_1_1 import sha,save,encoded
from functions import _trace
ROOT=local.ROOT;OUT=ROOT/'analysis/multiplayer/integration/inventory-source/smg-particle-owner'
FROZEN={**local.FROZEN,'tools/oracle_smg_local_effect_owner_1_1.py':'577031ac811ec66d7b2bf6ee4ca9b5749fbfebda1d0f0e6ef449997c2044f503',
 'analysis/multiplayer/integration/inventory-source/smg-local-effect-owner/manifest.json':'7b24920488467c88276fe9eb7e7d24557cf54335bfd49b06cec807c92dae0903',
 'analysis/multiplayer/integration/inventory-source/smg-local-effect-owner/audio-unavailable/report.json':'e30bf2ee9f87012a8778e370196679d2ee9739bbbb3409b85e238f611656dcc5'}
ENTRIES={0x249540,0x249a20,0x24b240,0x175620,0x24aec0,0x24a0a0,0x175390}
CHECKPOINTS={0x249a75,0x249efa}
class ParticleTrace(local.LocalEffectTrace):
 def __init__(self,*a,**kw):
  self.birth_on=False;self.birth_calls=[];self.birth_pending=[];self.birth_stores=[];self.birth_reads=[];self.birth_points=[];self.birth_executed=set();self.birth_order=0
  super().__init__(*a,**kw)
  self.birth_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES};self.birth_owners={p:e for e,r in self.birth_routines.items()for p in r.code}
  import unicorn
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.birth_read)
 def payload(self,emitter,cache,system):
  loc=self.u(cache+0x4c);effect=self.u(system+0x18);effectptr=self.u(self.u(0x4f1f0c)+0x44)+(effect&65535)*0x190 if effect!=0xffffffff else 0
  return dict(emitter_address=f'{emitter:08x}',emitter_hex=self.raw(emitter,0x4c),cache_address=f'{cache:08x}',cache_hex=self.raw(cache,0x54),system_address=f'{system:08x}',system_hex=self.raw(system,0x54),
   location_address=f'{loc:08x}',location_hex=self.raw(loc,0x34)if loc else None,effect_address=f'{effectptr:08x}',effect_hex=self.raw(effectptr,0x190)if effectptr else None,
   clock_address=f'{self.u(0x518444):08x}',clock_hex=self.raw(self.u(0x518444),12),camera_address='0048cb38',camera_hex=self.raw(0x48cb38,36),seed8_hex=self.raw(self.u(0x4ee9d8),8))
 def finish_birth(self,a):
  matches=[v for v in self.birth_pending if v[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,esp,c in reversed(matches):
   if esp!=sp:continue
   c.update(return_order=self.birth_order,return_cpu=self.cpu(),after=self.payload(c['_emitter'],c['_cache'],c['_system']))
   for k in('_emitter','_cache','_system'):c.pop(k)
   self.birth_pending.remove((ret,esp,c));self.birth_order+=1
 def hook(self,g,a,n,user):
  if self.birth_on:
   self.finish_birth(a);self.birth_executed.add(a)
   if a in(0x249540,0x249a20):
    sp=g.reg_read(UC_X86_REG_ESP);s=[self.u(sp+4+i*4)for i in range(6)];emitter=g.reg_read(UC_X86_REG_ECX)
    cache,system,definition=(s[3],s[1],s[2])if a==0x249540 else(g.reg_read(UC_X86_REG_EAX),s[0],s[1])
    c=dict(id=len(self.birth_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.birth_order,entry_cpu=self.cpu(),stack_words=[f'{v:08x}'for v in s],before=self.payload(emitter,cache,system),definition_address=f'{definition:08x}',definition_hex=self.raw(definition,0xb8),_emitter=emitter,_cache=cache,_system=system)
    if a==0x249540:c.update(frame_address=f'{s[4]:08x}',frame_hex=self.raw(s[4],0x34)if s[4]else None)
    self.birth_calls.append(c);self.birth_pending.append((self.u(sp),sp,c));self.birth_order+=1
   if a in CHECKPOINTS:
    c=next(c for _,_,c in reversed(self.birth_pending)if c['entry']=='00249a20');q=dict(instruction=f'{a:08x}',order=self.birth_order,cpu=self.cpu())
    if a==0x249a75:
     datum=g.reg_read(UC_X86_REG_EAX);pool=g.reg_read(UC_X86_REG_EDX);p=self.u(pool+0x44)+(datum&65535)*0x40
     q.update(actual_allocated_datum=f'{datum:08x}',actual_pool=f'{pool:08x}',actual_particle_address=f'{p:08x}',actual_particle40_hex=self.raw(p,0x40))
    else:
     p=g.reg_read(UC_X86_REG_EBP);q.update(particle_address=f'{p:08x}',particle40_hex=self.raw(p,0x40),state=self.payload(c['_emitter'],c['_cache'],c['_system']))
    c.setdefault('checkpoints',[]).append(q);self.birth_order+=1
   if a in self.birth_owners:
    i=self.code.at(a)
    if i.mnemonic.startswith(('j','f'))or i.mnemonic=='call'or i.mnemonic.endswith('ss'):
     self.birth_points.append(dict(order=self.birth_order,instruction=f'{a:08x}',owner=f'{self.birth_owners[a]:08x}',cpu=self.cpu(),stack80_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x80)));self.birth_order+=1
  super().hook(g,a,n,user)
 def birth_read(self,g,access,p,n,v,user):
  ip=g.reg_read(UC_X86_REG_EIP)
  if self.birth_on and ip in self.birth_owners:self.birth_reads.append(dict(order=self.birth_order,instruction=f'{ip:08x}',address=f'{p:08x}',size=n,raw_hex=self.raw(p,n)));self.birth_order+=1
 def mem_write(self,g,access,p,n,v,user):
  if self.birth_on:self.birth_stores.append(dict(order=self.birth_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,before_hex=self.raw(p,n),written_hex=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()));self.birth_order+=1
  super().mem_write(g,access,p,n,v,user)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');args=p.parse_args();pin=sha(__file__)
 for f,h in FROZEN.items():assert sha(ROOT/f)==h,f
 proc=subprocess.Popen(['wine',str(local.base.owner.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(local.base.owner.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t,correlation=local.prepare(data) if False else local.base.owner.player_source.prepare(data,trace_type=ParticleTrace),None
  # Repeat the immutable genuine preparation with this read-only observer class.
  assert local.base.owner.player_source.spawn_and_activate(t,0,activate=False);old=json.loads((local.base.owner.OUT/'report.json').read_text());t.item_observe=True
  for r in old['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
  result=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{v:08x}'for v in sorted(t.executed)],metadata=local.base.owner.metadata(t))
  for k,v in result.items():assert v==old[k],('owning runtime changed',k)
  t.item_observe=False;t.fp_resources();t.effect_on=True;t.birth_on=True
  prior=json.loads((local.OUT/'audio-unavailable/report.json').read_text())
  for r in prior['stages']:
   if r['name']=='declared_local_view_request_owner':
    for e in(0x168866,0x16a356,0x107b50,0x179b90):t.services.pop(e,None)
    t.services[0x18ad70]=(prior['audio_boundary']['scope'],-1,24)
   if r['name']=='source_control_conversion':
    cmd=int(prior['declared_requests']['command']['pointer'],16);assert t.raw(cmd,0x5c)==prior['declared_requests']['command']['before5c_hex'];t.w(cmd,0x50000);t.w(cmd+0x14,0x3f800000)
   assert t.effect_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['registers'].items()))
  t.finish_birth(local.base.STOP);assert not t.birth_pending
  for k,v in(dict(stages=t.effect_stages,calls=t.effect_calls,states=t.effect_states,reads=t.effect_reads,stores=t.effect_stores,points=t.effect_points,final_state=t.snapshot())).items():assert v==prior[k],('frozen local effect flow changed',k)
  listings=[]
  for name,addresses in [(f'{e:08x}',r.code)for e,r in sorted(t.birth_routines.items())]+[('all-executed',t.birth_executed),('all-stores',{int(v['instruction'],16)for v in t.birth_stores})]:
   ins=[t.code.at(a)for a in sorted(addresses)];path=args.output.parent/'instructions'/f'{name}.asm'
   for i in ins:assert i.bytes==t.image.read(i.address,i.size)==bytes(t.g.mem_read(i.address,i.size))
   save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(args.output.parent)),sha256=sha(path),rows=len(ins)))
  for f,h in FROZEN.items():assert sha(ROOT/f)==h,f
  assert pin==sha(__file__)
  report=dict(schema='source-smg-particle-owner-v1',oracle_sha256=pin,source_xbe_sha256=local.base.PIN,source_map_sha256=local.base.CACHE_PIN,frozen_pins=FROZEN,prior_local_effect_report_sha256=sha(local.OUT/'audio-unavailable/report.json'),prior_all_measured_fields_byte_identical=True,
   calls=t.birth_calls,points=t.birth_points,reads=t.birth_reads,stores=t.birth_stores,instructions=listings,metadata=prior['metadata'],instruction_patches=[],new_guest_assignments=[],scope='Actual full249540 emitter and249A20 creation through measured249EFA successful prefix, using real source allocated slot/marker/location/cache/RNG/request. Prior externalview/fire/unavailableaudio and resource/finiteclock boundaries retained; no continuous expiry or graphics submission inferred.')
  save(args.output,encoded(report));print(json.dumps(dict(output=str(args.output),sha256=sha(args.output),calls=len(t.birth_calls),stores=len(t.birth_stores),reads=len(t.birth_reads))),flush=True)
 finally:
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='worker pipe timeout'
 if proc.returncode:raise RuntimeError((proc.returncode,err))
if __name__=='__main__':main()
