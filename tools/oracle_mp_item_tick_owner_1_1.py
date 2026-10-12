#!/usr/bin/env python3
"""Independent unchanged owning clock/tick investigation after frozen item state."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX
import oracle_mp_item_contact_producers_1_1 as contact
from oracle_mp_item_contact_producers_1_1 import ROOT,digest,encoded,frozen_write
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STOP
from oracle_mp_inventory_spatial_1_1 import cache_record
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/inventory-source/item-tick-owner'
FROZEN={**contact.FROZEN,
 'tools/oracle_mp_item_contact_producers_1_1.py':'65c0ea6425389540e9cadf253d4239ca82f19b62040711e566a14a9e53033b2a',
 'tools/oracle_mp_item_contact_producers_audit_1_1.py':'fdd537f7f7026da8ccdebbd92624766cc223a903b9fa8afd707ca05676d0f438',
 'docs/MP_ITEM_CONTACT_PRODUCERS_RESEARCH.md':'4b757bbe310df869191d6fa2a14851f764e8754f0a75ff3768c86425ef109aea',
 'analysis/multiplayer/integration/inventory-source/item-contact-producers/report.json':'b7b2f788611edd56c3ab3c3b8c03b7f45267b0d254c1d76c470970a182eb9c9a',
 'analysis/multiplayer/integration/inventory-source/item-contact-producers/audit.json':'48a3e69797d183d8246469cbdf3a60e2e54109fe4587e683aee7192eb6762deb',
 'analysis/multiplayer/integration/inventory-source/item-contact-producers/manifest.json':'347cbb44ab53d1b42166da475829c8628b4495eb505eb37495a350e9d1e92896'}
ENTRIES={0x147cc0,0x147d20,0x147d80,0x147dd0,0x1480f0,0x147fd0,0x139750,
 0x1396e0,0x69b20,0x69540,0x12cbc0,0x68300,0x683f0,0x68460,0x684f0,
 0x694a0,0x685b0,0x686f0,0x68560,0x84070,0x68040,0x68170,0x680f0,
 0x156a30,0x156ad0,0x327866,0x326af0,0x12d600,0x139390,0x139410,
 0x337a40,0x336983,0x32803e,0x197450,0x68e10,0x6b650,0x139540}
STATE_ENTRIES={0x147cc0,0x147d20,0x139750,0x684f0,0x685b0,0x686f0,
 0x68040,0x68170,0x680f0,0x68300,0x68460,0xb7e00,0x1c7660,0xb7f50,0xb8040}

class TickTrace(contact.ContactTrace):
 def __init__(self,*args,**kw):
  self.tick_enabled=False;self.tick_calls=[];self.tick_pending=[];self.tick_stores=[]
  self.tick_reads=[];self.tick_points=[];self.tick_states=[];self.tick_state_ids={}
  self.tick_stages=[];self.tick_order=0;self.tick_executed=set()
  super().__init__(*args,**kw)
  self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.tick_read)
 def tick_snapshot(self):
  result=dict(item=self.ct_snapshot(),global_slots_hex={f'{p:08x}':self.raw(p,4)for p in(0x518444,0x51845c,0x4d6d44,0x4d6d48)},
   simulation_flags=dict(address='004d6d38',raw18_hex=self.raw(0x4d6d38,0x18)),
   callback17=dict(address='0044768c',raw24_hex=self.raw(0x447428+17*36,36)),
   arena_owner=dict(address='004ed650',raw10_hex=self.raw(0x4ed650,16)),
   kernel_time_slots={f'{p:08x}':self.raw(p,4)for p in(0x417cd4,0x417bc8)},
   original_globals={f'{p:08x}':self.raw(p,n)for p,n in((0x4f496c,2),(0x54fabe,10),(0x4eda40,4),(0x4eda70,4))})
  for name,p,n in(('clock',self.u(0x518444),0x24),('camera',self.u(0x51845c),0x78),
   ('game',self.u(0x4edf18),0x1200),('simulation',self.u(0x4d6d44),0x1224),('watcher',self.u(0x4d6d48),0xc34)):
   result[name]=dict(pointer=f'{p:08x}',size=n,raw_hex=self.raw(p,n)if p else None)
  return result
 def tick_state(self):
  result=self.tick_snapshot();h=hashlib.sha256(json.dumps(result,sort_keys=True,separators=(',',':')).encode()).hexdigest()
  if h not in self.tick_state_ids:
   self.tick_state_ids[h]=len(self.tick_states);self.tick_states.append(dict(id=len(self.tick_states),sha256=h,**result))
  return self.tick_state_ids[h]
 def tick_return(self,a):
  # Original326AF0 allocates the caller's frame and returns at a lowerESP.
  # Match its actual returnPC too; a conventional positiveESP delta would
  # silently leave this source-owned alloca frame unfinished.
  hits=[r for r in self.tick_pending if r[0]==a]
  if not hits:return
  depth=hits[-1][1]
  for ret,sp,r in reversed(hits):
   if sp!=depth:continue
   r.update(return_order=self.tick_order,return_cpu=self.cpu(),return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp)
   self.tick_order+=1
   if int(r['entry'],16)in STATE_ENTRIES:r['after_state']=self.tick_state()
   if 'frame_pointer'in r:r['frame_after_hex']=self.raw(int(r['frame_pointer'],16),0x4048)
   self.tick_pending.remove((ret,sp,r))
 def hook(self,g,a,size,user):
  if self.tick_enabled:
   if a==0x337a45:
    target=g.reg_read(UC_X86_REG_EAX);assert target==0x8000009c
    self.new_invalid=dict(boundary='unresolved original kernel data import; its encoded target aliases cache bytes and is not a runtime clock',
     instruction=f'{a:08x}',kernel_target=f'{target:08x}',kernel_ordinal=156,original_return_site=f'{self.u(g.reg_read(UC_X86_REG_ESP)):08x}',
     source_import_slot='00417cd4',slot4_hex=self.raw(0x417cd4,4),aliased_cache4_hex=self.raw(target,4),actual_backing_cache=cache_record(self,target,4),
     registers=self.registers(),cpu=self.cpu(),instruction_unexecuted=True)
    raise KernelBoundary()
   self.tick_return(a)
   if self.code.at(a)is not None:self.tick_executed.add(a)
   if a in self.starts or a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);r=dict(id=len(self.tick_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
     order=self.tick_order,cpu=self.cpu(),stack16_words=[f'{self.u(sp+4+i*4):08x}'for i in range(16)])
    self.tick_order+=1
    if a in STATE_ENTRIES:r['before_state']=self.tick_state()
    if a==0x684f0:r.update(frame_pointer=f'{g.reg_read(UC_X86_REG_EAX):08x}',frame_before_hex=self.raw(g.reg_read(UC_X86_REG_EAX),0x4048))
    self.tick_calls.append(r);self.tick_pending.append((self.u(sp),sp,r))
   if a in (0x13977d,0x139783,0x139850,0x13985e,0x139861,0x139868)or a in range(0x68460,0x684a0):
    self.tick_points.append(dict(order=self.tick_order,instruction=f'{a:08x}',cpu=self.cpu(),state=self.tick_state(),stack40_hex=self.raw(g.reg_read(UC_X86_REG_ESP),0x40)))
    self.tick_order+=1
  super().hook(g,a,size,user)
 def tick_read(self,g,access,address,size,value,user):
  if self.tick_enabled:
   ip=g.reg_read(UC_X86_REG_EIP)
   self.tick_reads.append(dict(order=self.tick_order,instruction=f'{ip:08x}',address=f'{address:08x}',size=size,raw_hex=self.raw(address,size)));self.tick_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.tick_enabled:
   self.tick_stores.append(dict(order=self.tick_order,phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
    before_hex=self.raw(address,size),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()));self.tick_order+=1
  super().mem_write(g,access,address,size,value,user)
 def tick_stage(self,name,entry,args=(),regs=()):
  self.active=name;self.new_invalid=None;before=self.tick_state();first=len(self.events);first_call=len(self.tick_calls)
  try:self.run(entry,args,regs);completion='complete_original_return';self.tick_return(STOP)
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None;completion='blocked_original_instruction'
  r=dict(name=name,entry=f'{entry:08x}',arguments=[f'{v:08x}'for v in args],input_registers={str(k):f'{v:08x}'for k,v in regs},
   completion=completion,stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',invalid=self.new_invalid,before_state=before,after_state=self.tick_state(),
   before_cpu=self.tick_calls[first_call]['cpu'],after_cpu=self.cpu(),services=self.events[first:],pending_calls=[q[2]['id']for q in self.tick_pending])
  self.tick_stages.append(r);return completion=='complete_original_return'

def prepare(data):
 m=contact.motion;t=m.owning.player_source.prepare(data,trace_type=TickTrace)
 assert t.player_stages[-1]['completion']=='complete_original_return';assert m.owning.player_source.spawn_and_activate(t,0,activate=False)
 t.item_observe=True;base=json.loads((m.owning.OUT/'report.json').read_text());correlations={}
 for r in base['stages']:assert t.item_stage(r['name'],int(r['entry'],16),tuple(int(v,16)for v in r['arguments']),tuple((int(k),int(v,16))for k,v in r['input_registers'].items()))
 groups=[(m.OUT,dict(calls=t.motion_calls,states=t.motion_states,reads=t.motion_reads,stores=t.motion_stores,points=t.motion_points,definitions=list(t.motion_definitions.values()),executed_in_scope={str(row):[f'{a:08x}'for a in sorted(v)]for row,v in t.motion_executed.items()})),
  (m.owning.OUT,dict(preparation=t.producers,player_prefix_stages=t.player_stages,stages=t.item_stages,calls=t.item_calls,callbacks=t.item_callbacks,stores=t.item_stores,states=t.item_states,resources=t.resource_delivery,services=t.events,executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],metadata=m.owning.metadata(t))),
  (contact.transform.OUT,dict(calls=t.tp_calls,scopes=t.tp_scopes,states=t.tp_states,reads=t.tp_reads,stores=t.tp_stores,points=t.tp_points,executed_in_scope=[f'{a:08x}'for a in sorted(t.tp_executed)])),
  (contact.OUT,dict(calls=t.ct_calls,scopes=t.ct_scopes,states=t.ct_states,reads=t.ct_reads,stores=t.ct_stores,points=t.ct_points,executed_in_scope=[f'{a:08x}'for a in sorted(t.ct_executed)]))]
 for out,result in groups:
  expected=json.loads((out/'report.json').read_text())
  for k,v in result.items():assert v==expected[k],('Prior runtime changed',out.name,k)
  correlations[out.name]=dict(report=str((out/'report.json').relative_to(ROOT)),report_sha256=digest(out/'report.json'),keys=list(result),runtime_sha256=hashlib.sha256(encoded(result)).hexdigest(),byte_identical=True)
 t.item_observe=False;t.tick_enabled=True
 return t,correlations

def listings(traces,out):
 t=traces[0];executed=set().union(*(q.tick_executed for q in traces));result=[]
 def emit(name,ins,**fields):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and all(bytes(q.g.mem_read(i.address,len(i.bytes)))==i.bytes for q in traces)
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode();p=out/'instructions'/name;frozen_write(p,raw)
  result.append(dict(file=str(p.relative_to(out)),sha256=digest(p),rows=len(ins),**fields))
 entries=ENTRIES|{int(r['entry'],16)for q in traces for r in q.tick_calls}
 for entry in sorted(entries):
  r=_trace(t.code,entry,t.starts|entries);emit(f'{entry:08x}.asm',[t.code.at(a)for a in sorted(r.code)],entry=f'{entry:08x}',executed_rows=sum(a in executed for a in r.code),
   direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}',executed=a in executed)for a,b,k in r.sites if k=='call'])
 emit('executed-tick-owner-instructions.asm',[t.code.at(a)for a in sorted(executed)],kind='executed_in_extension')
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 own=digest(__file__);m=contact.motion;proc=subprocess.Popen(['wine',str(m.owning.player_source.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(m.owning.player_source.CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());cases=[];traces=[]
  for constructor in (False,True):
   t,correlations=prepare(data);traces.append(t)
   stages=[('original_clock_storage_callback17',0x147cc0),('original_clock_newmap_callback17',0x147d20)]
   if constructor:stages += [('original_simulation_storage_callback23',0x68040),('original_readiness_gated_scheduler',0x68300)]
   stages += [('original_whole_tick_owner',0x139750)]
   if constructor:stages += [('original_empty_simulation_disposal',0x68170),('original_simulation_newmap_callback23',0x680f0)]
   for name,entry in stages:
    if not t.tick_stage(name,entry):break
   cases.append(dict(name='source_watcher_constructor_and_tick_gate'if constructor else'missing_watcher_whole_tick_stop',prior_runtime_correlations=correlations,
    stages=t.tick_stages,calls=t.tick_calls,states=t.tick_states,reads=t.tick_reads,stores=t.tick_stores,points=t.tick_points,executed_in_extension=[f'{a:08x}'for a in sorted(t.tick_executed)]))
   print(json.dumps(dict(case=cases[-1]['name'],stages=[dict(name=r['name'],completion=r['completion'],eip=r['stopped_eip'],invalid=r['invalid'])for r in t.tick_stages])),flush=True)
  for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
  assert digest(__file__)==own
  result=dict(schema=1,oracle_sha256=own,source_pins=FROZEN,cases=cases,instructions=listings(traces,args.output.parent),
   instruction_patches=[],new_guest_inputs=[],new_supplied_service_results=[],boundaries=['Selected original clock callback17 follows the unchanged earlier item composition as an independently declared producer root; complete title/callback lifecycle placement remains unresolved.',
    'Original139750 is called with no arguments: its4048-byte frame+4 active flag must be produced by684F0/694A0/68460; no active flag or tick increment is assigned.',
    'The preserved existing options input has signed tick rateGame+0E=0. Original147D20 emits rate0 and dt7F800000; no30Hz or finite dt is substituted. Earlier frozen item motion separately declares its inheriteddt3D088889. No infinity-fed item physics is admitted here.',
    'The separate watcher case executes original68040 constructor only before139750. Its sourceBSS simulationkind/state are0/0 and Game+1120 remains0. This is constructor-state negative admission, not complete simulation initialization. Complete68170 disposal and680F0 newmap are subsequently attempted; no partial newmap is followed by another tick.',
    'Original680F0 subsequently stops at original337A45 before consuming encoded ordinal156 target8000009C. That unrelocated import target aliases actual Lockout bytesFDFFBF2F at fileoffset1BD389C under cachemagic7E42C800. These bytes are not a measured kernel boot clock and are never admitted as a service result. No original instruction is patched and no source clock/result is supplied.',
    'The unchanged original compatibility/storage/cache/resident-resource boundaries from all earlier freezes remain explicit. No item root, contact, local/current pose, PVS, queue, impulse or Havok result is supplied.',
    'No repeated admitted ordinary item schedule, continuous settling, full title/frame or Xbox hardware parity is claimed.'])
  frozen_write(args.output,encoded(result));print(json.dumps(dict(output=str(args.output),sha256=digest(args.output),calls=sum(len(t.tick_calls)for t in traces),states=sum(len(t.tick_states)for t in traces),stores=sum(len(t.tick_stores)for t in traces))))
 finally:
  forced=False
  try:_,err=proc.communicate('x',timeout=15)
  except subprocess.TimeoutExpired:forced=True;proc.kill();_,err=proc.communicate(timeout=15)
 assert proc.returncode==0 or forced,err
if __name__=='__main__':main()
