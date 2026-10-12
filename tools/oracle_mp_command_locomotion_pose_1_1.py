#!/usr/bin/env python3
"""Declared command axes through unchanged allocated-owner control/pose producers.

The prior neutral command runtime is reproduced first. Every probe restores that
actual guest state, changes only the explicitly declared command axis words, then
executes the original submission/delivery/player/object roots. No clock, physical
mode, velocity, classification, channel, pose, contact or success is supplied.
"""
import argparse,hashlib,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EDX,UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_spawn_command_input as command
from oracle_mp_item_current_producers_1_1 import CurrentTrace
from oracle_mp_item_motion_boundaries_1_1 import MotionTrace
from oracle_mp_spartan_owned_pose_1_1 import OwnedPoseTrace,metadata as pose_metadata,ENTRIES as POSE_ENTRIES,FROZEN as POSE_PINS
from oracle_mp_item_active_schedule_1_1 import encoded,frozen_write,digest
from oracle_smg_firing_1_1 import STOP,SEED
from functions import _trace

ROOT=command.ROOT
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/command-locomotion-pose'
FROZEN={**command.FROZEN,**POSE_PINS,
 'tools/oracle_mp_spawn_command_input.py':'de382cb5f14ccbc64fbc5609419382f2a574fc1f46ba00e371efbfa3a641f4fc',
 'analysis/multiplayer/integration/spawn-source/command-input/report.json':'1fc1b58c65fd804477ac9bbdcd3bcad1ad10bd954f98b0f5ea17348891ba0a76',
 'tools/oracle_mp_spartan_owned_pose_1_1.py':'cee290577a992a2b8b799409da666ba49cc148fe11982bdcb44c49525c81b6e2',
 'analysis/multiplayer/integration/inventory-source/spartan-owned-pose-measured/report.json':'4b9e9ed383e0081fe7d64c57636731304d66d322b3aef23259024a27061733cb',
 'tools/research_mp_live_locomotion_owners_u5.py':'82db82bdbbf29b269fdfca6a95cf798d41c591d98f400e5d956c01901bc1627d',
 'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json':'104075c71753a770ba82863a63aa793248b5bad418e397a76eb363c619b82fa8',
 'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/manifest.json':'e4ca9d072d3637da224b5519b4fe2749893c252ec2bdfa30503c01cd3786c07b'}
ENTRIES=POSE_ENTRIES|{0x151a20,0x151980,0x151aa0,0x151c90,0xc8140,0xc8250,0xe7b10,0xe5680,0x14e780,0x852a0,0x69350,0x85460,0x150dd0,0x150f00,0x11dda0}
STATE_ENTRIES={0x151a20,0x151aa0,0x151c90,0xc8140,0xc7e90,0xde960,0xe1800,0xe3b20,0x1f52f0,0xe44c0,0x112e10,0xbdf80,0x113830}
CASES=[('neutral',0,0),('positive_forward',0x3f800000,0),('negative_forward',0xbf800000,0),
 ('positive_side',0,0x3f800000),('negative_side',0,0xbf800000),('positive_diagonal',0x3f800000,0x3f800000),
 ('mixed_diagonal',0x3f800000,0xbf800000),('small_declared_forward',0x3c23d70a,0)]
CASES=[(name,x,y,False)for name,x,y in CASES]+[(name,x,y,True)for name,x,y in[
 ('authored_heading_neutral',0,0),('authored_heading_forward',0x3f800000,0),('authored_heading_back',0xbf800000,0),
 ('authored_heading_positive_side',0,0x3f800000),('authored_heading_negative_side',0,0xbf800000),
 ('authored_heading_below_deadzone',0x3d800000,0),('authored_heading_above_deadzone',0x3e800000,0)]]

class CommandPoseTrace(command.InputTrace):
 raw=MotionTrace.raw
 cpu=MotionTrace.cpu
 item_objects=CurrentTrace.item_objects
 def __init__(self,*a,**kw):
  self.observe=False;self.calls=[];self.pending=[];self.stores=[];self.reads=[];self.points=[];self.states=[];self.state_ids={};self.order=0;self.probe=None;self.scope_executed=set()
  super().__init__(*a,**kw)
  self.routines={e:_trace(self.code,e,self.starts|ENTRIES)for e in ENTRIES}
  self.owners={a:e for e,r in self.routines.items()for a in r.code}
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.read_hook)
 def snapshot(self):
  unit=self.spawned_unit;pp=self.u(0x4f01f4);player=self.u(pp+0x44)+(self.spawned_player&0xffff)*self.u(pp+0x24)
  return dict(objects=self.item_objects(),player=dict(datum=f'{self.spawned_player:08x}',pointer=f'{player:08x}',raw21c_hex=self.raw(player,0x21c)),
   clock=dict(pointer=f'{self.u(0x518444):08x}',raw24_hex=self.raw(self.u(0x518444),0x24)),
   seed=dict(pointer=f'{SEED:08x}',raw4_hex=self.raw(SEED,4)),physics=self.state(),command5c_hex=self.raw(command.COMMAND,0x5c))
 def state_id(self):
  v=self.snapshot();h=hashlib.sha256(encoded(v)).hexdigest()
  if h not in self.state_ids:self.state_ids[h]=len(self.states);self.states.append(dict(id=len(self.states),sha256=h,**v))
  return self.state_ids[h]
 def packets(self,e,reg,s):
  v=OwnedPoseTrace.packets(self,e,reg,s)
  def add(name,p,n):v.append(dict(field=name,pointer=f'{p:08x}',size=n,raw_hex=self.raw(p,n)if p else None))
  if e==0x151c90:add('stack1_command5c',s[1],0x5c)
  if e==0x151aa0:
   add('ebx_axes8',reg['ebx'],8)
   for i,name in enumerate(('stack0_aim12','stack1_world12','stack2_local12')):add(name,s[i],12)
  if e==0x151980:
   for k,name in (('eax','aim'),('edi','fallback'),('esi','basis')):add(k+'_'+name,reg[k],24 if k=='esi'else 12)
  if e==0x30bd0:add('eax_vector12',reg['eax'],12)
  if e==0xc8140:add('ecx_control7c',reg['ecx'],0x7c)
  if e==0xc8250:add('edx_control7c_output',reg['edx'],0x7c)
  if e==0xe7b10:
   for k,name in (('eax','world_movement12'),('edi','unit_forward12'),('esi','aim12')):add(k+'_'+name,reg[k],12)
   add('stack1_selected_facing12_output',s[1],12)
  if e==0x852a0:
   add('stack0_simulation_record88',s[0],0x88);add('stack1_submitted_command5c',s[1],0x5c)
  if e==0x69350:
   add('stack0_simulation_context1224',s[0],0x1224);add('stack1_packet4048_output',s[1],0x4048)
  if e==0x85460:
   add('stack0_record88',s[0],0x88);add('stack1_delivered_command5c',s[1],0x5c)
  if e==0xe1800:add('stack1_activity_request12',s[1],12)
  if e==0x11dda0:
   add('edx_actual_authored_start34',reg['edx'],0x34)
   add('ecx_heading_output4',reg['ecx'],4)
  return v
 def probe_returns(self,a):
  matches=[q for q in self.pending if q[0]==a]
  if not matches:return
  depth=matches[-1][1]
  for ret,sp,r in reversed(matches):
   if sp!=depth:continue
   r.update(return_order=self.order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,
    after_packets=[dict(p,raw_hex=self.raw(int(p['pointer'],16),p['size'])if int(p['pointer'],16)else None)for p in r['packets']])
   self.order+=1
   if int(r['entry'],16)in STATE_ENTRIES:r['after_state']=self.state_id()
   self.pending.remove((ret,sp,r))
 def hook(self,g,a,size,user):
  if self.observe:
   self.probe_returns(a)
   if a in self.owners:self.scope_executed.add(a)
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);reg={k:int(v,16)for k,v in self.registers().items()};s=[self.u(sp+4+i*4)for i in range(24)]
    r=dict(id=len(self.calls),probe=self.probe,phase=self.active,entry=f'{a:08x}',caller=f'{self.u(sp):08x}',order=self.order,entry_cpu=self.cpu(),
     stack24_words=[f'{v:08x}'for v in s],packets=self.packets(a,reg,s))
    self.order+=1
    if a in STATE_ENTRIES:r['before_state']=self.state_id()
    self.calls.append(r);self.pending.append((self.u(sp),sp,r))
   if a in self.owners:
    i=self.code.at(a)
    if i.mnemonic.startswith(('j','f'))or i.mnemonic=='call'or i.mnemonic.endswith('ss'):
     self.points.append(dict(probe=self.probe,phase=self.active,order=self.order,instruction=f'{a:08x}',owner=f'{self.owners[a]:08x}',cpu=self.cpu(),stack40_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x40)));self.order+=1
  super().hook(g,a,size,user)
 def read_hook(self,g,access,p,n,value,user):
  if self.observe and g.reg_read(UC_X86_REG_EIP)in self.owners:
   self.reads.append(dict(probe=self.probe,order=self.order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,raw_hex=self.raw(p,n)));self.order+=1
 def mem_write(self,g,access,p,n,value,user):
  if self.observe and self.pending:
   self.stores.append(dict(probe=self.probe,phase=self.active,order=self.order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,before_hex=self.raw(p,n),written_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()));self.order+=1
  super().mem_write(g,access,p,n,value,user)

def prepare(data):
 # Verbatim frozen positive command composition with an observer-only subclass.
 t=command.prepare(data,trace_type=CommandPoseTrace)
 assert t.command_action('source_network_configuration_default_handler',0x663b0)
 for n,e in [('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0)]:assert t.command_action(n,e)
 assert command.spawn_and_activate(t)
 assert t.command_action('source_tick_packet_producer',0x684f0,regs=((UC_X86_REG_EAX,command.PACKET),))
 assert t.u(command.PACKET+8)==0 and t.u(t.u(0x4edf18)+0x1120,1)==0
 assert t.command_action('source_tick_packet_preplayer_dispatch',0x685b0,(command.PACKET,))
 assert t.command_action('source_complete_early_player_phase',0x14eae0,(command.PACKET,))
 t.source_fixture=[(lo,hi,p,bytes(t.g.mem_read(lo,hi-lo+1)))for lo,hi,p in t.g.mem_regions()]
 t.source_fixture_cpu=t.g.context_save()
 assert t.command_action('source_neutral_command_initializer',0x14e780,regs=((UC_X86_REG_EDX,command.COMMAND),))
 record=t.u(0x4d6d44)+0x80+(t.spawned_player&0xffff)*0x88
 assert t.command_action('source_neutral_command_submission_explicit_root',0x852a0,(record,command.COMMAND))
 assert t.command_action('source_packet_command_assembly_explicit_root',0x69350,(t.u(0x4d6d44),command.PACKET))
 assert t.u(command.PACKET+8)==1
 assert t.command_action('source_early_player_phase_with_assembled_command',0x14eae0,(command.PACKET,))
 for n,e in [('prephysics',0xb7e00),('world',0x1c7660),('postphysics',0xb7f50),('newflags',0xb8040)]:assert t.command_action('source_command_motion_'+n,e)
 old=json.loads((command.OUT/'report.json').read_text())['cases'][1]
 keys={'stages':t.player_stages,'input_calls':t.input_calls,'clock_calls':t.clock_calls,'original_calls':t.new_calls,'preparation':t.producers,'services':t.events,
  'input_state':t.input_state(),'source_input_stores':t.input_stores,'player_and_object_stores':t.player_stores,'motion_request_producers_consumers':t.motion_requests,
  'resource_delivery':t.resource_delivery,'resource_requests':t.resource_requests,'raw_services':t.events,'physical_raw_services':t.physical_services,'compatibility_platform_scope':t.platform_input,
  'compatibility_lock_calls':t.lock_calls,'source_raw_impact_pool_allocations':t.os_pool_allocs,'creation_input_hex':t.creation_input_hex,'default_variant_hex':t.default_variant}
 for k,v in keys.items():assert v==old[k],('Prior command runtime changed',k)
 return t,dict(report=str((command.OUT/'report.json').relative_to(ROOT)),report_sha256=digest(command.OUT/'report.json'),case=old['case'],keys=list(keys),runtime_sha256=hashlib.sha256(encoded(keys)).hexdigest(),byte_identical=True)

def probe(t,name,x,y,authored_heading=False):
 t.probe=name;t.observe=True;begin=dict(calls=len(t.calls),states=len(t.states),reads=len(t.reads),stores=len(t.stores),points=len(t.points));stages=[]
 def run(label,e,args=(),regs=()):
  before=t.state_id();start=len(t.events);ok=t.stage(name+'_'+label,e,args,regs);t.probe_returns(STOP)
  stages.append(dict(name=label,entry=f'{e:08x}',before_state=before,after_state=t.state_id(),completion=t.player_stages[-1]['completion'],invalid=t.new_invalid,services=t.events[start:],stopped_eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}'))
  return ok
 assert run('neutral_initializer',0x14e780,regs=((UC_X86_REG_EDX,command.COMMAND),))
 old=t.raw(command.COMMAND+0xc,8);t.g.mem_write(command.COMMAND+0xc,struct.pack('<II',x,y));declared=dict(pointer=f'{command.COMMAND+0xc:08x}',before_hex=old,after_hex=struct.pack('<II',x,y).hex(),only_fields=['command+0C','command+10'])
 heading=None
 if authored_heading:
  objects=t.item_objects();unit=next(o for o in objects if o['datum']==f'{t.spawned_unit:08x}');root=unit['root']['bits']
  rows=[(i,p)for i,p in enumerate(t.cache.block(t.cache.scenario+0x100,0x34))if [f'{v:08x}'for v in struct.unpack('<3I',t.cache.data[p:p+12])]==root]
  assert len(rows)==1
  i,p=rows[0];heading=dict(declared_external_command_look_input=True,source_row=i,file_offset=f'{p:08x}',address=f'{p+t.cache.magic:08x}',raw34_hex=t.raw(p+t.cache.magic,0x34),before_command_heading_bits=f'{t.u(command.COMMAND+4):08x}')
  assert run('authored_start_heading_delivery_explicit_root',0x11dda0,regs=((UC_X86_REG_EAX,0),(UC_X86_REG_EDX,p+t.cache.magic),(unicorn.x86_const.UC_X86_REG_ECX,command.COMMAND+4)))
  heading['after_command_heading_bits']=f'{t.u(command.COMMAND+4):08x}'
 record=t.u(0x4d6d44)+0x80+(t.spawned_player&0xffff)*0x88
 for label,e,args in [('submission',0x852a0,(record,command.COMMAND)),('assembly',0x69350,(t.u(0x4d6d44),command.PACKET)),('player',0x14eae0,(command.PACKET,)),
  ('prephysics',0xb7e00,()),('world',0x1c7660,()),('postphysics',0xb7f50,()),('newflags',0xb8040,())]:
  if not run(label,e,args):break
 t.observe=False
 unreturned=[r['id']for _,_,r in t.pending]
 if unreturned:assert stages[-1]['completion']=='blocked_original_instruction',('Unclosed returned original calls',unreturned)
 t.pending=[] # Observer frames from a recorded stop, not a guest return.
 assert t.replay_cursor==3,'Unexpected new clock input request'
 return dict(case=name,declared_command_axis_write=declared,declared_authored_heading_delivery=heading,ranges={k:[v,len(getattr(t,k))]for k,v in begin.items()},stages=stages,unreturned_original_calls=unreturned,final_cpu=t.cpu())

def listings(t,out):
 result=[]
 def save(name,addresses):
  ins=[t.code.at(a)for a in sorted(addresses)]
  for i in ins:assert t.image.read(i.address,i.size)==i.bytes==bytes(t.g.mem_read(i.address,i.size))
  p='instructions/'+name;frozen_write(out/p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  return dict(file=p,sha256=digest(out/p),rows=len(ins),executed_rows=sum(a in t.scope_executed for a in addresses))
 for e,r in sorted(t.routines.items()):result.append(dict(entry=f'{e:08x}',**save(f'{e:08x}.asm',r.code)))
 result.append(save('executed-observed-instructions.asm',t.scope_executed))
 result.append(save('all-observed-store-instructions.asm',{int(r['instruction'],16)for r in t.stores}))
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--case-limit',type=int,default=len(CASES));args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 pin=digest(__file__);proc=subprocess.Popen(['wine',str(command.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(command.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());t,correlation=prepare(data)
  # Snapshot complete guest backing and CPU context only after proven original
  # output. Restore this exact fixture per case; no individual gameplay defaults.
  backing=t.source_fixture;cpu=t.source_fixture_cpu
  assert [(lo,hi,p)for lo,hi,p,_ in backing]==list(t.g.mem_regions()),'Source resource mapping changed after fixture capture'
  memory=[dict(lo=f'{lo:08x}',hi=f'{hi:08x}',permissions=p,size=len(raw),sha256=hashlib.sha256(raw).hexdigest())for lo,hi,p,raw in backing]
  print(json.dumps(dict(prior_runtime_identical=True,fixture_bytes=sum(len(q[3])for q in backing),regions=len(memory))),flush=True)
  cases=[]
  for name,x,y,heading in CASES[:args.case_limit]:
   for lo,hi,permissions,raw in backing:t.g.mem_write(lo,raw)
   t.g.context_restore(cpu);r=probe(t,name,x,y,heading);cases.append(r)
   print(json.dumps(dict(case=name,completion=r['stages'][-1]['completion'],calls=r['ranges']['calls'][1]-r['ranges']['calls'][0])),flush=True)
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  assert digest(__file__)==pin
  report=dict(schema=1,oracle_sha256=pin,source_pins=FROZEN,prior_runtime_correlation=correlation,restored_exact_source_guest_state=memory,
   source_metadata=pose_metadata(t),cases=cases,calls=t.calls,states=t.states,reads=t.reads,stores=t.stores,points=t.points,
   instructions=listings(t,args.output.parent),instruction_patches=[],new_supplied_classification_pose_clock_physical_mode_contact_admission_or_success=[],
   restored_source_stage='source_complete_early_player_phase: after source zero-mask player phase, before first explicit neutral initializer/submission/assembly/player/motion',
   boundaries=['Each probe restores the actual state after the zero-mask early player phase in the byte-identical positive neutral command fixture, before its first explicit command/motion composition. New guest stimuli are the two declared 5C command axis words and, only in the separately named authored-heading cases, explicit11DDA0 delivery of the actual chosen start-row heading to command+4. This is declared command-look input, not natural device/local-member/scheduler ownership.',
    'Exact source14E780→852A0 submission/codec→69350 assembly→14EAE0 control→B7E00→1C7660→B7F50→B8040 order is retained. Source phase leaves and actual negative physics/terrain contacts execute unchanged; any inherited explicit services and first unavailable instructions remain captured.',
    'The previous source ABI/FP environment, selected startup/cache/resource/raw platform input boundaries remain explicit. Clock tick0 and dt3D088889 are inherited. No clock increment, repeated admitted frames, continuous movement/settling or Xbox reciprocal parity is asserted.',
    'Only original1.1 code executes dynamically here. Original1.5 owners must be connected by the separately audited complete static correspondence; external helpers and a full1.5 owning runtime are not silently inferred.'])
  frozen_write(args.output,encoded(report));print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),calls=len(t.calls),states=len(t.states),stores=len(t.stores))),flush=True)
 finally:
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:
   proc.kill();proc.stdout.close();proc.stderr.close();proc.wait(timeout=5);err='host shutdown pipe timeout'
 if proc.returncode!=0:print(json.dumps(dict(host_shutdown_return=proc.returncode,detail=err)),flush=True)
if __name__=='__main__':main()
