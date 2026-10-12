#!/usr/bin/env python3
"""Original source local-view/PVS/SMG effect with standalone sound-stop evidence.

View0 and a delivered firing command are declared requests. Constructors,
registration, PVS, world/FP markers, effect location/event/particle owners run
unchanged; no view/PVS/current pose/allocation result is assigned. A separate
case retains the immutable FP oracle's explicitly unavailable audio result.
"""
import argparse,json,os,subprocess,hashlib
from pathlib import Path
from unicorn.x86_const import *
import oracle_smg_effect_owner_1_1 as base
from functions import _trace
ROOT=base.ROOT;OUT=ROOT/'analysis/multiplayer/integration/inventory-source/smg-local-effect-owner'
FROZEN={**base.FROZEN,'tools/oracle_smg_effect_owner_1_1.py':base.sha(ROOT/'tools/oracle_smg_effect_owner_1_1.py'),
 'analysis/multiplayer/integration/inventory-source/smg-effect-owner/report.json':'eb4f414d206873f734cbe18437fb0aa7b7649b288f1dc3027a35764585d029cc',
 'tools/oracle_smg_fp_clock_1_1.py':base.sha(ROOT/'tools/oracle_smg_fp_clock_1_1.py')}
LOCAL_ENTRIES={0x1874d0,0x1ea3d0,0x156a30,0x187520,0x1ea410,0x156ad0,0x1677c3,0x1677e5,
 0x1508b0,0x188d00,0x169d8a,0x169965,0x1678c1,0x168866,0x16a356,0x13a380,0x13a2a0,0x151ec0,0x16f8a0,
 0x17c8c0,0x17c930,0x167fcb,0x17a920,0x17a6f0,0x17aa90,0x17ba20,0x17ac00,0x179690,0x175a50,0x247e2a,0x21f366}
class LocalEffectTrace(base.EffectTrace):
 def __init__(self,*a,**kw):
  super().__init__(*a,**kw)
  self.local_entries=LOCAL_ENTRIES|{e for e in self.starts if 0x175000<=e<0x177500 or 0x243000<=e<0x249000}
  for e in sorted(self.local_entries):
   if e not in self.effect_routines and e not in(0x247e2a,0x21f366):self.effect_routines[e]=_trace(self.code,e,self.starts|base.ENTRIES|self.local_entries)
  self.effect_owners={a:e for e,r in self.effect_routines.items()for a in r.code}
 def snapshot(self):
  r=super().snapshot();extra=[]
  for name,address,size in(('users',0x4f01f0,0xa0),('controls',0x4f4854,0x264),('view',0x526250,0x6c8),('camera_time',0x51845c,0x78),('camera_flag',0x4f0204,4),('first_person',0x4f1198,0x8330)):
   p=self.u(address);extra.append(dict(name=name,global_address=f'{address:08x}',pointer=f'{p:08x}',raw_hex=self.raw(p,size)if p else None))
  r['local_owner_storage']=extra;return r
 def packets(self,a,reg,s):
  r=super().packets(a,reg,s)
  def add(name,p,n):
   if p:r.append(dict(field=name,pointer=f'{p:08x}',size=n,raw_hex=self.raw(p,n)))
  if a in(0x17bdf0,0x17a6f0):add('effect_body190',s[0],0x190)
  if a==0x17c930:add('matrix_output34',reg['ecx'],0x34)
  if a==0x167fcb:add('matrix_output34',s[2],0x34)
  if a==0x17a6f0:
   add('particle_system54',s[1],0x54);add('authored_system38',s[2],0x38)
  return r
 def hook(self,g,a,n,user):
  if self.effect_on and a in self.local_entries-base.ENTRIES:
   self.effect_returns(a);sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(12)]
   row=dict(id=len(self.effect_calls),phase=self.active,entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.effect_order,entry_cpu=self.cpu(),stack_words=[f'{v:08x}'for v in s],packets=self.packets(a,reg,s),before_state=self.sid_state())
   if a not in(0x247e2a,0x21f366):self.effect_pending.append((self.u(sp),sp,row))
   else:row['checkpoint_only']=True
   self.effect_calls.append(row);self.effect_order+=1
  super().hook(g,a,n,user)
 def fp_resources(self):
  resident=max((int(r['resident_pointer'],16)+r['size']+4095)&~4095 for r in self.resident.values());c=self.cache;tag=c.tags[0xe5aa0435];rows=[]
  with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
   for index,off in enumerate(c.block(tag['offset']+0xac,20)):
    size,pointer=c.u(off+4),c.u(off+8);location,at=pointer>>30,pointer&0x3fffffff
    if location==2:shared.seek(at);raw=shared.read(size)
    else:assert location==0;raw=bytes(c.data[at:at+size])
    assert len(raw)==size;self.g.mem_write(resident,raw)
    row=dict(index=index,descriptor=f'{off+c.magic:08x}',descriptor_hex=bytes(c.data[off:off+20]).hex(),size=size,location=location,file_offset=at,file='assets/disc/maps/shared.map'if location==2 else'assets/disc/maps/lockout.map',resident_pointer=f'{resident:08x}',raw_sha256=hashlib.sha256(raw).hexdigest(),graph=tag['name'])
    self.resident[off+c.magic]=row;self.resource_delivery.append(row);rows.append(row);resident+=(size+4095)&~4095
  assert resident<=0x8000000;return rows

def prepare(data):
 t=base.owner.player_source.prepare(data,trace_type=LocalEffectTrace);assert base.owner.player_source.spawn_and_activate(t,0,activate=False)
 old=json.loads((base.owner.OUT/'report.json').read_text());t.item_observe=True
 for r in old['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 result=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=base.owner.metadata(t))
 for k,v in result.items():assert v==old[k],('prior owning runtime changed',k)
 t.item_observe=False
 return t,dict(report_sha256=base.sha(base.owner.OUT/'report.json'),all_fields_byte_identical=True,runtime_sha256=hashlib.sha256(base.encoded(result)).hexdigest())
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--audio',choices=('original-stop','unavailable'),required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args();pin=base.sha(__file__)
 for path,h in FROZEN.items():assert base.sha(ROOT/path)==h,path
 proc=subprocess.Popen(['wine',str(base.owner.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(base.owner.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t,correlation=prepare(data);resources=t.fp_resources();t.effect_on=True
  for name,e in [('source_control_storage',0x1874d0),('source_view_storage',0x1ea3d0),('source_camera_storage',0x156a30),('source_control_reset',0x187520),('source_view_reset',0x1ea410),('source_camera_reset',0x156ad0),('source_fp_storage',0x1677c3),('source_fp_reset',0x1677e5),('source_effect_storage',0x1775d0),('source_effect_reset',0x177690)]:assert t.effect_stage(name,e)
  removed={}
  for e in(0x168866,0x16a356,0x107b50,0x179b90):
   if e in t.services:removed[f'{e:08x}']=t.services.pop(e)
  audio=None
  if args.audio=='unavailable':
   audio=dict(entry='0018ad70',returned_eax='ffffffff',ret_pop=24,scope='Explicit unavailable audio resource/playback leaf, same boundary as immutable original FP clock oracle; no successful sound engine claimed.')
   t.services[0x18ad70]=(audio['scope'],-1,24)
  unit=t.spawned_unit;body=t.u(t.u(t.u(0x4e78d0)+0x44)+(unit&65535)*12+8);weapon=t.u(body+0x218);cmd=0x10ec000
  for name,e,arguments,registers in [('declared_local_view_request_owner',0x1508b0,(t.spawned_player,0),()),('source_fp_registration_and_clock',0x1678c1,(),()),('source_local_pvs_owner',0x13a380,(),()),('source_neutral_command_initializer',0x14e780,(),((UC_X86_REG_EDX,cmd),))]:assert t.effect_stage(name,e,arguments,registers)
  before=t.raw(cmd,0x5c);t.w(cmd,0x50000);t.w(cmd+0x14,0x3f800000)
  declared=dict(view='Explicit source1508B0(actual allocated player,view0) request; original setter owns all users/player/camera/FP stores. Machine identity/session admission is not inferred.',command=dict(pointer=f'{cmd:08x}',before5c_hex=before,after5c_hex=t.raw(cmd,0x5c),changed_fields=['command+00=00050000','command+14=3F800000']))
  for name,e,arguments,registers in [('source_control_conversion',0x151c90,(unit,cmd),((UC_X86_REG_EAX,t.spawned_player),)),('source_unit_wrapper',0xc7e90,(unit,),()),('source_weapon_update',0x1021d0,(weapon,),())]:
   if not t.effect_stage(name,e,arguments,registers):break
  for path,h in FROZEN.items():assert base.sha(ROOT/path)==h,path
  assert pin==base.sha(__file__)
  report=dict(schema='source-smg-local-effect-owner-v1',oracle_sha256=pin,xbe_sha256=base.PIN,cache_sha256=base.CACHE_PIN,frozen_pins=FROZEN,prior_runtime_correlation=correlation,declared_requests=declared,audio_boundary=audio,
   removed_failed_services=removed,resident_fp_resources=resources,metadata=base.metadata(t),stages=t.effect_stages,calls=t.effect_calls,states=t.effect_states,reads=t.effect_reads,stores=t.effect_stores,points=t.effect_points,
   instructions=base.listings(t,args.output.parent),supplied_services=t.events,unreturned_original_calls=[r['id']for _,_,r in t.effect_pending],final_state=t.snapshot(),
   new_supplied_view_pvs_marker_pose_effect_or_particle_results=[],instruction_patches=[],boundaries=[
    'Actual immutable player/item owner composition is repeated identically. Additional source storage/reset, local setter, registration/clock/PVS roots are bounded composition, not complete authored startup/frame/device membership.',
    'View0 and two already-delivered firing command words are declared external requests. No guest view/PVS, camera override, source node pose, allocation success, birth result or RNG state is assigned.',
    'Exact FP jmad descriptors/cache/shared bytes use the retained original-resource adapter. Original codecs/current FP nodes, markers and attachment location arithmetic execute.',
    'Original sound-stop is retained in its own case. Optional unavailable-audio continuation is explicitly failure-conditioned; it does not prove positive sound/resource lifetime.',
    'Original finite clock dt/raw backing/platform/optional sound and attachment boundaries remain named. No complete particle render/draw, continuous lifecycle, full1.5 execution or Xbox reciprocal/hardware parity inferred.'])
  base.save(args.output,base.encoded(report));print(json.dumps(dict(output=str(args.output),sha256=base.sha(args.output),calls=len(t.effect_calls),states=len(t.effect_states),stores=len(t.effect_stores),final_completion=t.effect_stages[-1]['completion'],stop=t.effect_stages[-1]['stopped_eip'],pools=[dict(global_address=q['global_address'],occupied=len(q.get('occupied',[])))for q in report['final_state']['pools']])),flush=True)
 finally:
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host worker shutdown timeout'
 if proc.returncode:raise RuntimeError((proc.returncode,err))
if __name__=='__main__':main()
