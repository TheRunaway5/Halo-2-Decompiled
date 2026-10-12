#!/usr/bin/env python3
"""Observer-only actual allocated/equipped Spartan pose and physics selector.

Replays the immutable owning player/item composition without new guest inputs.
Every clip, classification, channel/local/current pose and flag is source output.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_item_active_schedule_1_1 as owning
import oracle_mp_item_tick_owner_1_1 as tick_owner
from oracle_mp_item_motion_boundaries_1_1 import MotionTrace
from oracle_mp_inventory_spatial_1_1 import cache_record
from oracle_smg_firing_1_1 import STOP,SEED
from functions import _trace

ROOT=owning.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured'
FROZEN={**tick_owner.FROZEN,
 'tools/oracle_mp_item_tick_owner_1_1.py':'ce3b32bb2ac32d4030215d2ec94dd8ebb587d402c8b4fe9d39a5b6cc0d6ae750',
 'tools/oracle_mp_item_tick_owner_audit_1_1.py':'5b3a572940587ff17e30936569820249e7c3f9e0601e201eb4cfbf989a017233',
 'docs/MP_ITEM_TICK_OWNER_RESEARCH.md':'fecc07bc8493c297cd4882dd304f009df1785eb4cce645d9f77aed267088c594',
 'analysis/multiplayer/integration/inventory-source/item-tick-owner/report.json':'f5cc1f4fd58ca9d80c800bd6178dc980931392bf832288e2ac21b50051995982',
 'analysis/multiplayer/integration/inventory-source/item-tick-owner/audit.json':'df2406ae4032ada7ef41bfb22dc117c88a81618ff7ca8650f26cb437b04b4796',
 'analysis/multiplayer/integration/inventory-source/item-tick-owner/manifest.json':'ed13b0f072cf2c357975eb33057d93e399b76511dcda6424ace61fdf06a73dda',
 'tools/oracle_spartan_equipped_animation_1_1.py':'3cff76069e4770392b0cee0df2bafaec5a4def1ba4945493047cdb57977e7a65',
 'tools/oracle_spartan_equipped_unit_phase_1_1.py':'cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38',
 'tools/oracle_spartan_live_pose_1_1.py':'28afd44db12c96a67bc82bca1c617cbd9fb8fa631e949f71e9b74743d2de10f5',
 'analysis/multiplayer/map-pack/spartan-equipped-animation-suite/suite.json':'cbc99ee18a0091a48e14505936bf0eaa46a81b922bbd3be3cf4939fee3760352',
 'analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite/suite.json':'a9724a76d52551cea10c1b379ecb8503e1c9c2d96cb553bd0643d5f8289f418e'}
ENTRIES={0x14e240,0xce510,0xcfed0,0xfb560,0xe9d50,0x111490,0x111500,
 0xc7e90,0xc5d10,0xc6c50,0xc7420,0xc7210,0xc7aa0,0x116420,0xc7cf0,0xc7b70,0xc6400,
 0xde960,0xe1800,0xddbc0,0x1e9540,0x1e9480,0xe3b20,0x1e8f60,0xe44c0,
 0x1f3e10,0x1f5810,0x1f58a0,0x1f4230,0x1e94e0,0x1f52f0,0x1fe1d0,0x1f0080,0x1f0490,0x1f0c80,0x1e9210,
 0x1d14f0,0xe7e60,0x112e10,0x110bf0,0x1cf100,0x1d0a60,0x1cfce0,0x1cfde0,
 0x1ca2b0,0x1ca090,0x1c9f50,0x1d0ed0,0xbdf10,0xbdf80,0xbea50,0x1d0f80,
 0x1ca950,0x27fb10,0x280030,0x113830,0x143960,0x1441d0,0xba1c0,0xbede0,
 0xbd360,0xbd710,0x10cdf0,0x10d420,0x1d7310,0xf1420}
STATE_ENTRIES={0x14e240,0xce510,0xfb560,0xe9d50,0x111500,0xc7e90,
 0xde960,0xe3b20,0x1e8f60,0xe44c0,0xe7e60,0x112e10,0x1cf100,
 0xbdf80,0x113830,0xba1c0,0xbede0,0x1d0ed0,0x1d14f0,0x1f52f0}
FRAME_SIZES={0xde960:0x17c,0xe3b20:0xe0,0xbdf80:0xf8,0x113830:0xe4,0x112e10:0x90}

digest,encoded,frozen_write=owning.digest,owning.encoded,owning.frozen_write

class OwnedPoseTrace(owning.ActiveTrace):
 raw=MotionTrace.raw
 cpu=MotionTrace.cpu
 def __init__(self,*args,**kwargs):
  self.pose_enabled=False;self.pose_calls=[];self.pose_pending=[];self.pose_stores=[];self.pose_reads=[]
  self.pose_points=[];self.pose_states=[];self.pose_state_ids={};self.pose_order=0;self.pose_executed=set()
  super().__init__(*args,**kwargs)
  self.pose_routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES}
  self.pose_owners={a:e for e,r in self.pose_routines.items()for a in r.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.pose_read)
 def pose_snapshot(self):
  objects=self.item_objects();players=self.player_state()
  # Literal complete bodies own all node banks/extensions/channels. Extract
  # ranges only when their allocation header actually bounds them.
  for o in objects:
   p=int(o['body_address'],16);n=o['body_size']
   if not p or n<0x12c:continue
   for off,name in ((0x128,'manager'),(0x33c,'extension'),(0x340,'extra'),(0x344,'control')):
    if off+4>n:continue
    size,at=self.u(p+off,2),self.u(p+off+2,2)
    if size and at!=0xffff and at+size<=n:o[name]=dict(field=f'{off:03x}',size=size,offset=at,pointer=f'{p+at:08x}',raw_hex=self.raw(p+at,size))
   if 'manager'in o and o['manager']['size']>=0x90:
    m=int(o['manager']['pointer'],16)
    o['selected']=dict(graph=f'{self.u(m):08x}',packed=f'{self.u(m+4):08x}',frame_bits=f'{self.u(m+0x1c):08x}',
     mode=f'{self.u(m+0x70):08x}',weapon_class=f'{self.u(m+0x74):08x}',weapon_name=f'{self.u(m+0x78):08x}',activity=f'{self.u(m+0x7c):08x}')
   if n>=0x478 and self.u(int(o['header_address'],16)+3,1)==0:
    o['biped_fields']=dict(owner=f'{self.u(p+0x13c):08x}',inventory30_hex=self.raw(p+0x210,0x30),inputs7c_hex=self.raw(p+0x148,0x7c),
     physical_state_3dc10_hex=self.raw(p+0x3dc,16),crouch_bits=f'{self.u(p+0x2c4):08x}')
  return dict(objects=objects,players=players,clock=dict(pointer=f'{self.u(0x518444):08x}',raw24_hex=self.raw(self.u(0x518444),0x24)),
   physics=self.state(),seed=dict(pointer=f'{SEED:08x}',raw4_hex=self.raw(SEED,4)),object_context_hex=self.raw(self.u(0x4e58c4),0x84))
 def pose_state(self):
  value=self.pose_snapshot();h=hashlib.sha256(encoded(value)).hexdigest()
  if h not in self.pose_state_ids:
   self.pose_state_ids[h]=len(self.pose_states);self.pose_states.append(dict(id=len(self.pose_states),sha256=h,**value))
  return self.pose_state_ids[h]
 def packets(self,e,reg,s):
  result=[]
  def add(name,p,n):result.append(dict(field=name,pointer=f'{p:08x}',size=n,raw_hex=self.raw(p,n)if p else None))
  if e in (0xe3b20,0x112e10):
   add('stack1_request12',s[1],12)
   if e==0xe3b20:add('stack2_physics_request154',s[2],0x154)
  if e==0x1e8f60:
   add('eax_classification_result70',reg['eax'],0x70);add('stack0_physical_state10',s[0],16);add('stack1_physics_request154',s[1],0x154)
  if e==0xe44c0:
   add('ebx_mode4',reg['ebx'],4);add('edi_activity4',reg['edi'],4);add('stack2_blend1',s[2],1)
  if e==0x1d14f0:add('stack1_movement12',s[1],12)
  if e==0x1e9540:add('edi_physics_request154',reg['edi'],0x154)
  if e==0x1e9480:
   add('eax_physics_request154',reg['eax'],0x154);add('edx_physical_state10',reg['edx'],16);add('ecx_up12',reg['ecx'],12);add('stack0_forward12',s[0],12)
  if e==0x1f3e10:
   add('stack0_physical_state_tail_c',s[0],12);add('stack1_classification_result70',s[1],0x70);add('stack2_physics_request154',s[2],0x154)
  if e==0x1f52f0:
   add('ecx_physics_request154',reg['ecx'],0x154);add('edx_classification_result70',reg['edx'],0x70)
  if e==0x113830:
   add('stack1_node_mask_words',s[1],((s[2]+31)//32)*4);add('stack3_local_channels',s[3],s[2]*32)
  if e==0x143960:
   add('stack0_matrix34_output',s[0],0x34);add('stack1_local_channel20',s[1],0x20)
  if e==0x1441d0:
   add('ecx_matrix34_left',reg['ecx'],0x34);add('edx_matrix34_right',reg['edx'],0x34);add('stack0_matrix34_output',s[0],0x34)
  return result
 def frames(self):
  return[dict(call_id=r['id'],entry=r['entry'],pointer=f'{sp-FRAME_SIZES[e]:08x}',size=FRAME_SIZES[e]+4,
   raw_hex=self.raw(sp-FRAME_SIZES[e],FRAME_SIZES[e]+4))for _,sp,r in self.pose_pending for e in(int(r['entry'],16),)if e in FRAME_SIZES]
 def pose_returns(self,a):
  matches=[q for q in self.pose_pending if q[0]==a and q[2]['phase']==self.active]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,r in reversed(matches):
   if sp!=depth:continue
   r.update(return_order=self.pose_order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,
    after_packets=[dict(p,raw_hex=self.raw(int(p['pointer'],16),p['size'])if int(p['pointer'],16)else None)for p in r['packets']],after_frames=self.frames())
   self.pose_order+=1
   if int(r['entry'],16)in STATE_ENTRIES:r['after_state']=self.pose_state()
   self.pose_pending.remove((ret,sp,r))
 def hook(self,g,a,size,user):
  if self.pose_enabled:
   self.pose_returns(a)
   if a in self.pose_owners:self.pose_executed.add(a)
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(24)]
    r=dict(id=len(self.pose_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.pose_order,entry_cpu=self.cpu(),
     stack24_words=[f'{v:08x}'for v in s],packets=self.packets(a,reg,s),before_frames=self.frames())
    self.pose_order+=1
    if a in STATE_ENTRIES:r['before_state']=self.pose_state()
    self.pose_calls.append(r);self.pose_pending.append((self.u(sp),sp,r))
   if a in self.pose_owners:
    i=self.code.at(a)
    if i.mnemonic.startswith('j')or i.mnemonic=='call' or i.mnemonic in('divss','sqrtss','rcpss'):
     self.pose_points.append(dict(order=self.pose_order,instruction=f'{a:08x}',owner=f'{self.pose_owners[a]:08x}',cpu=self.cpu(),frames=self.frames()))
     self.pose_order+=1
  super().hook(g,a,size,user)
 def pose_read(self,g,access,p,size,value,user):
  if self.pose_enabled and g.reg_read(UC_X86_REG_EIP)in self.pose_owners:
   self.pose_reads.append(dict(order=self.pose_order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=size,raw_hex=self.raw(p,size)))
   self.pose_order+=1
 def mem_write(self,g,access,p,size,value,user):
  if self.pose_enabled:
   ip=g.reg_read(UC_X86_REG_EIP)
   # Every store while any named producer is on the actual call stack.
   if self.pose_pending:
    self.pose_stores.append(dict(order=self.pose_order,phase=self.active,instruction=f'{ip:08x}',address=f'{p:08x}',size=size,
     before_hex=self.raw(p,size),written_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
    self.pose_order+=1
  super().mem_write(g,access,p,size,value,user)

def execute(data):
 t=owning.player_source.prepare(data,trace_type=OwnedPoseTrace)
 assert t.player_stages[-1]['completion']=='complete_original_return'
 t.pose_enabled=True
 assert owning.player_source.spawn_and_activate(t,0,activate=False)
 old=json.loads((owning.OUT/'report.json').read_text())
 t.item_observe=True
 for r in old['stages']:
  assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 t.pose_returns(STOP)
 assert not t.pose_pending
 result=dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,
  stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=owning.metadata(t))
 for k,v in result.items():assert v==old[k],('Original active runtime changed',k)
 return t,dict(report=str((owning.OUT/'report.json').relative_to(ROOT)),report_sha256=digest(owning.OUT/'report.json'),keys=list(result),runtime_sha256=hashlib.sha256(encoded(result)).hexdigest(),byte_identical=True)

def listings(t,out):
 result=[]
 for e in sorted(ENTRIES):
  routine=t.pose_routines[e];ins=[t.code.at(a)for a in sorted(routine.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();p=out/'instructions'/f'{e:08x}.asm';frozen_write(p,raw)
  result.append(dict(entry=f'{e:08x}',file=str(p.relative_to(out)),sha256=digest(p),rows=len(ins),executed_rows=sum(a in t.pose_executed for a in routine.code),
   direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}',executed=a in t.pose_executed)for a,b,k in routine.sites if k=='call']))
 for name,addresses in(('executed-owned-pose-instructions',t.pose_executed),('observed-owned-pose-store-sites',{int(s['instruction'],16)for s in t.pose_stores})):
  ins=[t.code.at(a)for a in sorted(addresses)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();p=out/'instructions'/f'{name}.asm';frozen_write(p,raw)
  result.append(dict(file=str(p.relative_to(out)),sha256=digest(p),rows=len(ins)))
 return result

def metadata(t):
 c=t.cache;sid=lambda v:c.text(c.u(368)+c.u(c.u(364)+(v&0xffff)*4))
 animations=[dict(index=n,file_offset=f'{at:08x}',address=f'{at+c.magic:08x}',name=sid(c.u(at)),raw6c_hex=t.raw(at+c.magic,0x6c))for n,at in enumerate(c.block(t.graph['offset']+0x2c,0x6c))]
 return dict(biped=t.bipd,model=t.model,graph=t.graph,animations=animations,resources=t.resource_records,
  callback_type_table=dict(address='0046fad0',raw30_hex=t.raw(0x46fad0,0x30)),unit_descriptor=dict(address='0046ee50',rawb8_hex=t.raw(0x46ee50,0xb8)),biped_descriptor=dict(address='0046ef18',rawb8_hex=t.raw(0x46ef18,0xb8)))

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 own=digest(__file__);proc=subprocess.Popen(['wine',str(owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t,correlation=execute(data)
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  assert digest(__file__)==own
  result=dict(schema=1,oracle_sha256=own,source_pins=FROZEN,prior_runtime_correlation=correlation,metadata=metadata(t),
   stages=t.item_stages,calls=t.pose_calls,states=t.pose_states,reads=t.pose_reads,stores=t.pose_stores,points=t.pose_points,
   resources=t.resource_requests,services=t.events,executed_in_scope=[f'{a:08x}'for a in sorted(t.pose_executed)],instructions=listings(t,args.output.parent),
   instruction_patches=[],new_guest_inputs=[],new_supplied_pose_classification_admission_or_service_results=[],
   boundaries=['This is the unchanged actual source-owned player/loadout/PVS and selected authored-item composition, one complete ordinary/physics/dirty/new-object schedule; not a full139750 frame or successive admitted ticks.',
    'No axis, velocity, animation index, frame, classification, current/local pose, cluster, queue, contact or admission value is assigned by this extension. Original neutral/default unit state is retained.',
    'The earlier root-call ABI/FPCW/MXCSR, selected game options/variant/property delivery, raw OS/storage/cache/resource-residency and optional attachment/sound boundaries remain explicit.',
    'Clock+4 remains the earlier inherited3D088889 input. The separately preserved clock-owner freeze establishes a zero-rate source initializer and inactive/unavailable fulltick; no finite source-clock or repeated admitted tick is claimed here.',
    'No natural positive movement/device producer, positive Havok contact, full live-session/device readiness or Xbox reciprocal parity is asserted. Existing supplied-classification selector fixtures remain separate and are not substituted into this source producer.'])
  frozen_write(args.output,encoded(result));print(json.dumps(dict(output=str(args.output),sha256=digest(args.output),calls=len(t.pose_calls),states=len(t.pose_states),reads=len(t.pose_reads),stores=len(t.pose_stores))),flush=True)
 finally:
  forced=False
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:
   forced=True;proc.kill()
   try:_,err=proc.communicate(timeout=5)
   except subprocess.TimeoutExpired:
    # Wine helpers can retain inherited pipes after the selected worker exits.
    # This is host teardown after all original execution/capture, not a guest
    # service or original tick result. Do not kill other research processes.
    proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host helper retained shutdown pipe'
 assert proc.returncode==0 or forced,err
if __name__=='__main__':main()
