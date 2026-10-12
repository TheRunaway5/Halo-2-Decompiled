#!/usr/bin/env python3
"""Separate original stock-SMG effect storage/request/location/visibility evidence.

The immutable actual player/item runtime is repeated first. Only two declared
command words request firing. Full original effect storage/reset and consumer
bytes execute; no pool, marker, visibility, allocation or particle result is
supplied by this extension. The original negative visibility path is retained.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import *
import oracle_mp_item_active_schedule_1_1 as owner
from oracle_mp_item_motion_boundaries_1_1 import MotionTrace
from oracle_smg_firing_1_1 import STOP,PIN,CACHE_PIN
from functions import _trace
ROOT=owner.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/smg-effect-owner'
FROZEN={**owner.FROZEN,
 'tools/oracle_mp_item_active_schedule_1_1.py':'fd63e85b9b799bcf5654b11873ae08bb782d7d0f4e22b64a1c5075287c54dc23',
 'analysis/multiplayer/integration/inventory-source/item-active-schedule/report.json':'e347e495f8c37fb3f56b69835692754cb65b77947a8459fea34352ce61ce4be2',
 'analysis/multiplayer/integration/inventory-source/item-active-schedule/manifest.json':'aeabd0ea8833e620c403ea301e805e819fb4882740e8baddcf9e507c4bc84150'}
ENTRIES={0x1775d0,0x175860,0x177690,0x16d070,0x16d0d0,0x16d210,0x16d2c0,0x16d5e0,
 0x151c90,0xc8140,0xc7e90,0xc6c50,0x1021d0,0x107160,0x107b50,0x106070,0x17cbd0,
 0x1781f0,0x177da0,0xbacc0,0x177a10,0x179b90,0x179d10,0x179cb0,0x17a540,
 0x167d32,0x167d71,0x167daf,0x167a60,0x179dd0,0xb9c20,0x1dc8f0,0x1dc940,
 0x17a030,0x178d80,0x17b2c0,0x17af50,0x17b1a0,0x17b280,0x17ad20,
 0x11d620,0x11d650,0x3eb10,0x179080,0x178f60,0x178e10,0x175ee0,
 0x17bdf0,0x17c840,0x1751d0,0x247470,0x247950,0x1788b0,0x18aa90,0x18ad70}
POOL_GLOBALS=(0x4f1f0c,0x4f1f08,0x518464,0x526514,0x526518,0x52651c)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(v):return (json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def save(p,raw):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==raw,p
 else:p.write_bytes(raw)
class EffectTrace(owner.ActiveTrace):
 raw=MotionTrace.raw
 cpu=MotionTrace.cpu
 def __init__(self,*a,**kw):
  self.effect_on=False;self.effect_calls=[];self.effect_pending=[];self.effect_reads=[];self.effect_stores=[];self.effect_points=[];self.effect_order=0;self.effect_executed=set();self.effect_states=[];self.effect_ids={};self.effect_stages=[]
  super().__init__(*a,**kw)
  self.effect_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES}
  self.effect_owners={a:e for e,r in self.effect_routines.items()for a in r.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.effect_read)
 def snapshot(self):
  game=self.u(0x4edf18);fps=self.u(0x4f1198);playerpool=self.u(0x4f01f4);p=self.u(playerpool+0x44)+(self.spawned_player&0xffff)*self.u(playerpool+0x24)
  pools=[]
  for a in POOL_GLOBALS:
   base=self.u(a);q=dict(global_address=f'{a:08x}',pointer=f'{base:08x}')
   if base:
    cap,stride,data,bitmap=(self.u(base+n)for n in(0x20,0x24,0x44,0x48))
    assert cap<=8192 and stride<=0x200
    q.update(header4c_hex=self.raw(base,0x4c),capacity=cap,stride=stride,data_pointer=f'{data:08x}',bitmap_pointer=f'{bitmap:08x}',bitmap_hex=self.raw(bitmap,(cap+31)//32*4),occupied=[])
    for i in range(self.u(base+0x38)):
     if self.u(bitmap+(i>>5)*4)&(1<<(i&31)):
      slot=data+i*stride;q['occupied'].append(dict(index=i,datum=f'{self.u(slot,2)<<16|i:08x}',pointer=f'{slot:08x}',raw_hex=self.raw(slot,stride)))
   pools.append(q)
  return dict(objects=self.item_objects(),player=dict(datum=f'{self.spawned_player:08x}',pointer=f'{p:08x}',raw21c_hex=self.raw(p,0x21c)),
   game=dict(pointer=f'{game:08x}',raw8_hex=self.raw(game,8),pvs1138_hex=self.raw(game+0x1138,0x40),local_pvs1178_hex=self.raw(game+0x1178,0x40),activation11b8_hex=self.raw(game+0x11b8,0x40)),
   camera=dict(fps_pointer=f'{fps:08x}',head20_hex=self.raw(fps,0x20)if fps else None,global518440_pointer=f'{self.u(0x518440):08x}',rawc_hex=self.raw(self.u(0x518440),12)if self.u(0x518440)else None),
   clock=dict(pointer=f'{self.u(0x518444):08x}',raw24_hex=self.raw(self.u(0x518444),0x24)),pools=pools,
   shared_seed=dict(pointer=f'{self.u(0x4ee9d8):08x}',raw8_hex=self.raw(self.u(0x4ee9d8),8)))
 def sid_state(self):
  s=self.snapshot();h=hashlib.sha256(encoded(s)).hexdigest()
  if h not in self.effect_ids:self.effect_ids[h]=len(self.effect_states);self.effect_states.append(dict(id=len(self.effect_states),sha256=h,**s))
  return self.effect_ids[h]
 def packets(self,a,reg,s):
  result=[]
  def add(name,p,n):result.append(dict(field=name,pointer=f'{p:08x}',size=n,raw_hex=self.raw(p,n)if p else None))
  if a==0x151c90:add('command5c',s[1],0x5c)
  if a==0x1781f0:add('optional_owner3',reg['ebx'],12)
  if a==0x177da0:add('input_position12',reg['ecx'],12);add('output_directions38',reg['eax'],0x38)
  if a==0xbacc0:add('output_center12',reg['esi'],12)
  if a==0x177a10:
   add('effect_creation60',s[0],0x60)
   add('creation_directions',self.u(s[0]+0x1c),self.u(s[0]+0x20)*0x1c)
  if a==0x179b90:add('owner3',s[0],12)
  if a==0x179dd0:add('directions',s[2],s[3]*0x1c)
  if a==0xb9c20:add('marker_records',reg['ebx'],s[2]*0x70)
  if a==0x17a030:add('actual_marker70',reg['eax'],0x70)
  if a==0x167daf:add('fps_marker_records',s[2],s[3]*0x70)
  if a in(0x17b1a0,0x17af50,0x17b280,0x17ad20,0x179080,0x178f60,0x178d80):
   datum=reg['ebx']if a==0x17b1a0 else reg['eax']if a in(0x17b280,0x179080,0x178f60)else s[0]
   if a==0x178d80:datum=s[0]
   base=self.u(0x4f1f0c)
   if base and(datum&0xffff)<self.u(base+0x20):add('effect190',self.u(base+0x44)+(datum&0xffff)*0x190,0x190)
  return result
 def effect_returns(self,a):
  matches=[v for v in self.effect_pending if v[0]==a]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,r in reversed(matches):
   if sp!=depth:continue
   r.update(return_order=self.effect_order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,
    after_packets=[dict(v,raw_hex=self.raw(int(v['pointer'],16),v['size'])if int(v['pointer'],16)else None)for v in r['packets']],after_state=self.sid_state())
   self.effect_order+=1;self.effect_pending.remove((ret,sp,r))
 def hook(self,g,a,size,user):
  if self.effect_on:
   self.effect_returns(a);self.effect_executed.add(a)
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(12)]
    r=dict(id=len(self.effect_calls),phase=self.active,entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.effect_order,entry_cpu=self.cpu(),stack_words=[f'{v:08x}'for v in s],packets=self.packets(a,reg,s),before_state=self.sid_state())
    if a==0x107b50:r['effect_inputs']=dict(effect=f'{reg["eax"]:08x}',weapon=f'{reg["ecx"]:08x}',previous=f'{s[0]:08x}',a_bits=f'{s[1]:08x}',b_bits=f'{s[2]:08x}')
    self.effect_calls.append(r);self.effect_pending.append((self.u(sp),sp,r));self.effect_order+=1
   if a in self.effect_owners:
    i=self.code.at(a)
    if i.mnemonic.startswith(('j','f'))or i.mnemonic=='call'or i.mnemonic.endswith('ss'):
     self.effect_points.append(dict(order=self.effect_order,phase=self.active,instruction=f'{a:08x}',owner=f'{self.effect_owners[a]:08x}',cpu=self.cpu(),stack40_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x40)));self.effect_order+=1
  super().hook(g,a,size,user)
 def effect_read(self,g,access,p,n,v,user):
  ip=g.reg_read(UC_X86_REG_EIP)
  if self.effect_on and ip in self.effect_owners:
   self.effect_reads.append(dict(order=self.effect_order,phase=self.active,instruction=f'{ip:08x}',address=f'{p:08x}',size=n,raw_hex=self.raw(p,n)));self.effect_order+=1
 def mem_write(self,g,access,p,n,v,user):
  if self.effect_on:
   self.effect_stores.append(dict(order=self.effect_order,phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,before_hex=self.raw(p,n),written_hex=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()));self.effect_order+=1
  super().mem_write(g,access,p,n,v,user)
 def effect_stage(self,n,e,args=(),regs=()):
  before=self.sid_state();at=len(self.events);ok=self.stage(n,e,args,regs);self.effect_returns(STOP)
  self.effect_stages.append(dict(name=n,entry=f'{e:08x}',arguments=[f'{v:08x}'for v in args],registers={str(k):f'{v:08x}'for k,v in regs},before_state=before,after_state=self.sid_state(),completion=self.player_stages[-1]['completion'],stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',invalid=self.new_invalid,services=self.events[at:]))
  return ok

def prepare(data):
 t=owner.player_source.prepare(data,trace_type=EffectTrace);assert owner.player_source.spawn_and_activate(t,0,activate=False)
 old=json.loads((owner.OUT/'report.json').read_text());t.item_observe=True
 for r in old['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 result=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=owner.metadata(t))
 for k,v in result.items():assert v==old[k],('Prior original runtime changed',k)
 t.item_observe=False
 return t,dict(report=str((owner.OUT/'report.json').relative_to(ROOT)),report_sha256=sha(owner.OUT/'report.json'),all_fields_byte_identical=True,runtime_sha256=hashlib.sha256(encoded(result)).hexdigest())

def metadata(t):
 c=t.cache;rec=lambda p,n:dict(file_offset=f'{p:08x}',address=f'{p+c.magic:08x}',raw_hex=bytes(c.data[p:p+n]).hex())
 sid=lambda n:c.text(c.u(368)+c.u(c.u(364)+(n&0xffff)*4))
 weap=c.tags[0xe57b0406];barrel=c.block(weap['offset']+0x2d0,0xec)[0];firing=[];effects=[]
 for p in c.block(barrel+0xe4,0x34):
  firing.append(dict(**rec(p,0x34),count_min=c.h(p),count_max=c.h(p+2),references=[c.reference(c.u(p+8+i*8))for i in range(6)]))
  for kind in range(3):
   datum=c.u(p+8+kind*8)
   if datum==0xffffffff or c.tags[datum]['group']!='effe':continue
   tag=c.tags[datum];root=tag['offset'];locations=[dict(**rec(q,4),string_id=f'{c.u(q):08x}',name=sid(c.u(q)))for q in c.block(root+0xc,4)];events=[]
   for q in c.block(root+0x14,0x38):
    systems=[]
    for at in c.block(q+0x30,0x38):
     emitters=[]
     for ep in c.block(at+0x30,0xb8):
      properties=[]
      for off in(8,0x18,0x28,0x38,0x48,0x58,0x68,0x7c,0x8c):
       n,ptr=c.u(ep+off+8),c.u(ep+off+12);curve=ptr-c.magic
       properties.append(dict(relative_offset=f'{off:02x}',header=rec(ep+off,0x10),curve=None if not n else rec(curve,n)))
      emitters.append(dict(**rec(ep,0xb8),movement=c.reference(c.u(ep+4)),properties=properties))
     systems.append(dict(**rec(at,0x38),particle=c.reference(c.u(at+4)),location=c.h(at+8),coordinates=c.h(at+0xc),camera_mode=c.h(at+0x12),emitters=emitters))
    events.append(dict(**rec(q,0x38),parts=[dict(**rec(at,0x38),reference=c.reference(c.u(at+0x10)))for at in c.block(q+0x18,0x38)],systems=systems))
   effects.append(dict(tag=tag,root=rec(root,0x1c),locations=locations,events=events))
 models=[];hlmt=c.tags[c.u(weap['offset']+0x38)];world=c.tags[c.u(hlmt['offset']+4)];fps=c.tags[c.u(c.block(weap['offset']+0x2a8,16)[0]+4)]
 for tag in(world,fps):
  groups=[]
  for p in c.block(tag['offset']+0x58,12):
   if c.u(p)==0xf0000db:groups.append(dict(**rec(p,12),string_id=f'{c.u(p):08x}',name=sid(c.u(p)),markers=[rec(q,0x24)for q in c.block(p+4,0x24)]))
  models.append(dict(tag=tag,primary_trigger_groups=groups))
 return dict(weapon=weap,barrel=rec(barrel,0xec),firing=firing,effects=effects,models=models,
  source_direction_globals=[dict(global_address=f'{a:08x}',pointer=f'{t.u(a):08x}',raw12_hex=t.raw(t.u(a),12))for a in(0x46fc5c,0x46fc50)],
  pool_descriptors=[dict(global_address=f'{a:08x}',pointer=f'{t.u(a):08x}',header4c_hex=t.raw(t.u(a),0x4c))for a in POOL_GLOBALS])

def listings(t,out):
 rows=[]
 for name,addresses in[(f'{e:08x}',r.code)for e,r in sorted(t.effect_routines.items())]+[('all-executed',t.effect_executed),('all-store-sites',{int(v['instruction'],16)for v in t.effect_stores})]:
  ins=[t.code.at(a)for a in sorted(addresses)]
  for i in ins:assert i.bytes==t.image.read(i.address,i.size)==bytes(t.g.mem_read(i.address,i.size))
  path=out/'instructions'/f'{name}.asm';save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  rows.append(dict(file=str(path.relative_to(out)),sha256=sha(path),rows=len(ins),executed_rows=sum(i.address in t.effect_executed for i in ins)))
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');a=p.parse_args();pin=sha(__file__)
 for path,h in FROZEN.items():assert sha(ROOT/path)==h,path
 proc=subprocess.Popen(['wine',str(owner.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owner.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t,correlation=prepare(data);t.effect_on=True
  for n,e in [('source_effect_storage_constructor',0x1775d0),('source_effect_new_map_reset',0x177690)]:assert t.effect_stage(n,e)
  removed={}
  for q in(0x107b50,0x179b90):
   if q in t.services:removed[f'{q:08x}']=t.services.pop(q)
  unit=t.spawned_unit;pool=t.u(0x4e78d0);body=t.u(t.u(pool+0x44)+(unit&65535)*12+8);weapon=t.u(body+0x218);cmd=0x10ec000
  assert t.effect_stage('source_neutral_command_initializer',0x14e780,regs=((UC_X86_REG_EDX,cmd),))
  before=t.raw(cmd,0x5c);t.w(cmd,0x50000);t.w(cmd+0x14,0x3f800000)
  declared=dict(source='explicit already-delivered firing command stimulus; no device/local identity or readiness admission claimed',pointer=f'{cmd:08x}',before5c_hex=before,after5c_hex=t.raw(cmd,0x5c),changed_fields=['command+00 flags=00050000','command+14 trigger=3F800000'])
  for n,e,args,regs in [('source_control_conversion',0x151c90,(unit,cmd),((UC_X86_REG_EAX,t.spawned_player),)),('source_unit_wrapper',0xc7e90,(unit,),()),('source_weapon_update',0x1021d0,(weapon,),())]:
   if not t.effect_stage(n,e,args,regs):break
  t.effect_returns(STOP)
  assert not t.effect_pending
  for path,h in FROZEN.items():assert sha(ROOT/path)==h,path
  assert pin==sha(__file__)
  report=dict(schema='source-smg-effect-owner-v1',oracle_sha256=pin,xbe_sha256=PIN,cache_sha256=CACHE_PIN,frozen_pins=FROZEN,prior_runtime_correlation=correlation,
   declared_command=declared,removed_failed_effect_services=removed,metadata=metadata(t),stages=t.effect_stages,calls=t.effect_calls,states=t.effect_states,reads=t.effect_reads,stores=t.effect_stores,points=t.effect_points,instructions=listings(t,a.output.parent),
   supplied_services=t.events,unreturned_original_calls=[r['id']for _,_,r in t.effect_pending],new_supplied_allocation_marker_visibility_pose_or_particle_results=[],instruction_patches=[],
   boundaries=['The actual allocated player/biped/SMG/PVS/world/item composition is repeated unchanged before this extension. Additional owning effect storage/reset roots execute after it; a complete source startup callback order is not claimed.',
    'Only two explicitly declared command words request held/pressed firing. Original151C90/C7E90/1021D0 produce the preparation, effect tag, object/source scalars and pending projectile state. Device/local-member/readiness admission and a complete frame/tick remain open.',
    'Effect pools, generic allocation, actual current world marker lookup, source location allocation and visibility/cull/removal all execute unchanged. No positive effect/particle result is supplied. Registered local FPS/current-camera producer is absent in this genuine nonlocal fixture; its stored local PVS mask is preserved.',
    'The original first unavailable/negative consumer is recorded literally; particles, event emission, current first-person marker matrices and rendered flash are not inferred from a request or allocation.',
    'Inherited exact source cache/resource, raw platform backing, finite clock dt and optional sound/attachment boundaries remain named. No Xbox reciprocal parity, continuous world/particle lifecycle or TU1.5 runtime execution asserted.'])
  save(a.output,encoded(report));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),calls=len(t.effect_calls),states=len(t.effect_states),stores=len(t.effect_stores),particles=sum(r['entry']=='00247950'for r in t.effect_calls))),flush=True)
 finally:
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host worker shutdown pipe timeout'
 if proc.returncode:raise RuntimeError((proc.returncode,err))
if __name__=='__main__':main()
