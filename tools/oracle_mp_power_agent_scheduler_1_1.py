#!/usr/bin/env python3
"""Unchanged real-world retained-agent schedule, actual virtual delivery only."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_pair_manager_1_1 as prior
from oracle_mp_power_pair_manager_1_1 import PairTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-agent-scheduler'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='0ad62c09f5ba062951a0ebe7d87527cb2b7c675dc1dbc081f408239eb7b0770a'
ENTRIES={0x317650,0x316200,0x316440,0x31e2c0,0x31db70,0x31dee0,0x31e190,
 0x31cfe0,0x31d0f0,0x31ee50,0x31ed30,0x31ec90,0x31edc0,0x300870,
 0x31f1b0,0x31f400,0x31f550,0x31f6b0,0x31f4d0,0x31f4b0,0x31f540,
 0x31d5f0,0x31d690,0x31c980,0x31ca20,0x31e9d0,0x31c3c0,0x27f0f0,0x27f110,0x148800,0x1d3df0,0x1d2df0,0x1d2f10,
 0x1d3fc0,0x1d40b0}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class AgentTrace(PairTrace):
 def __init__(self,data):
  self.agent_pending=[];self.agent_next=None;self.agent_calls=[];self.agent_reads=[];self.agent_stores=[]
  self.agent_states=[];self.agent_ids={};self.agent_order=0;self.agent_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.agent_read)
 def agent_state(self):
  # Separate observer IDs: do not mutate any ancestor report/state table.
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
  from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
  r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
   for p,n in((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
   globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
    for p,n in((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
     (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4),(0x486577,1),(0x486564,4),(0x54f248,1),(0x54f26c,4),(0x486558,4),(0x526538,4),(0x487674,8))])
  tag=self.selected_fixup['tag'];p=tag['offset']+self.cache.magic;n=tag['size']
  r['raw_backing'].append(dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex()))
  p=self.u(data+(self.slot_object&0xffff)*12+8);r['object_pointer']=f'{p:08x}';r['object_382_hex']=bytes(self.g.mem_read(p,0x382)).hex()
  h=hashlib.sha256(encoded(r)).hexdigest()
  if h not in self.agent_ids:
   self.agent_ids[h]=len(self.agent_states);self.agent_states.append(dict(id=len(self.agent_states),sha256=h,**r))
  return self.agent_ids[h]
 def agent_return(self,a):
  matches=[x for x in self.agent_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.agent_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp)
   if c['measured_owner']:c['after_state']=self.agent_state()
   self.agent_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.agent_pending.remove((ret,entry_sp,c))
 def hook(self,g,a,size,user):
  self.agent_return(a);sp=g.reg_read(reg.UC_X86_REG_ESP)
  root=self.slot_ready and a==0x317650
  if root or self.agent_next is not None:
   site=self.agent_next;self.agent_next=None;ecx=g.reg_read(reg.UC_X86_REG_ECX)
   selected=a in ENTRIES
   c=dict(id=len(self.agent_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',call_site=None if site is None else f'{site:08x}',phase=self.active,depth=len(self.agent_pending),order=self.agent_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],measured_owner=selected,x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',packets=[])
   if selected:c['before_state']=self.agent_state()
   self.agent_order+=1
   def packet(name,p,n):
    try:b=bytes(g.mem_read(p,n)).hex()
    except unicorn.UcError:
     print(json.dumps(dict(observer_packet_failure=name,pointer=f'{p:08x}',size=n,call=c)),flush=True);raise
    c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=b))
   arg=lambda i:self.u(sp+4+i*4)
   if a in(0x317650,0x316200,0x316440):packet('world_274',ecx,0x274);packet('step_8',arg(0),8)
   if a in(0x31ed30,0x31ec90):packet('world_274',arg(0),0x274);packet('step_8',arg(1),8)
   if a in(0x31e9d0,0x31c3c0):packet('listener_packet_14',arg(1),0x14)
   if a==0x31e2c0:packet('island_b0',ecx,0xb0);packet('step_30',arg(1),0x30)
   if a in(0x31db70,0x31dee0,0x31e190,0x31cfe0,0x31d0f0):packet('island_b0',ecx,0xb0)
   if a in(0x31dee0,0x31e190):packet('step_30',arg(0),0x30)
   if a==0x300870:
    packet('agent_40',ecx,0x40);packet('collidable_a_2c',arg(0),0x2c);packet('collidable_b_2c',arg(1),0x2c);packet('step_30',arg(2),0x30);packet('output_10',arg(3),0x10)
   if a in(0x31f1b0,0x31f400,0x31f550,0x31f6b0,0x31f4d0,0x31f4b0,0x31f540):packet('manager_120',ecx,0x120)
   if a==0x31f550:
    packet('output_packet_10',arg(3),0x10);d=self.u(arg(3));packet('emitted_descriptor_14',d,0x14)
    capacity=self.u(d+8)&0x7fffffff
    if capacity:packet('emitted_rows_capacity',self.u(d),capacity*0x30)
   if a in(0x31d5f0,0x31d690):packet('output_bank_14',ecx,0x14)
   self.agent_calls.append(c);self.agent_pending.append((self.u(sp),sp,c))
  if self.agent_pending:
   self.agent_executed.add(a)
   i=self.code.at(a)
   if i.mnemonic=='call':self.agent_next=a
  super().hook(g,a,size,user)
 def agent_read(self,g,access,address,size,value,user):
  if not self.agent_pending:return
  self.agent_reads.append(dict(call=self.agent_pending[-1][2]['id'],order=self.agent_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.agent_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.agent_pending:
   self.agent_stores.append(dict(call=self.agent_pending[-1][2]['id'],order=self.agent_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.agent_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.prior.prior.power
 data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=AgentTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.agent_pending,[(hex(ret),hex(sp),c['entry'])for ret,sp,c in t.agent_pending]
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.pair_calls),('reads',t.pair_reads),('stores',t.pair_stores),('states',t.pair_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES|{int(c['entry'],16)for c in t.agent_calls}):
  f=_trace(t.code,entry,t.starts|ENTRIES|prior.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.agent_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_pair_manager_and_power_schedule_identical=True,calls=t.agent_calls,reads=t.agent_reads,stores=t.agent_stores,states=t.agent_states,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.agent_calls),states=len(t.agent_states),reads=len(t.agent_reads),stores=len(t.agent_stores))))
if __name__=='__main__':main()
