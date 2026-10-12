#!/usr/bin/env python3
"""Observe actual canister constraint/world membership; no new source inputs."""
import argparse,json
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_contact_batches_1_1 as prior
from oracle_mp_power_contact_batches_1_1 import BatchTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-constraint-membership'
MANIFEST=ROOT/'analysis/multiplayer/sandbox-source/power-contact-dispatch/final/manifest.json'
PIN='00208c9f1f3faef716fcffc48867b9351b4ec88883952a3254ba082163adcf71'
ENTRIES={0x313800:4,0x31d2b0:4,0x322320:4,0x31c520:8,0x31e7d0:0}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class ConstraintTrace(BatchTrace):
 def __init__(self,data):
  self.constraint_calls=[];self.constraint_pending=[];self.constraint_reads=[];self.constraint_stores=[]
  self.constraint_order=0;self.constraint_states=[];self.constraint_ids={};self.constraint_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.constraint_read)
 def constraint_state(self):
  old_states,old_ids=self.current_states,self.current_ids
  self.current_states,self.current_ids=self.constraint_states,self.constraint_ids
  try:return super().current_state()
  finally:self.current_states,self.current_ids=old_states,old_ids
 def constraint_return(self,a):
  for ret,sp,c in list(reversed(self.constraint_pending)):
   if ret!=a or self.g.reg_read(reg.UC_X86_REG_ESP)!=sp+4+ENTRIES[int(c['entry'],16)]:continue
   c.update(return_order=self.constraint_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.constraint_state());self.constraint_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.constraint_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.constraint_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.constraint_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.constraint_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.constraint_state(),packets=[])
   def packet(name,p,n):
    if p and n:c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   def bank(name,p,stride):
    packet(name+'_descriptor',p,12);cap=self.u(p+8)&0x7fffffff;assert cap<0x10000,(name,p,cap);packet(name+'_storage',self.u(p),cap*stride)
   def constraint(p):
    packet('constraint_c0',p,0xc0)
    for name,body in [('a',self.u(p+12)),('b',self.u(p+16))]:packet(name+'_entity_90',body,0x90)
   if a==0x313800:
    packet('world_180',ecx,0x180);constraint(arg(0))
   elif a==0x31d2b0:
    packet('island_70',ecx,0x70);bank('island_constraints',ecx+0x58,4);constraint(arg(0))
   elif a==0x322320:constraint(ecx);packet('requirements_10',arg(0),16)
   elif a==0x31c520:packet('island_70',ecx,0x70);constraint(arg(0));packet('requirements_10',arg(1),16)
   elif a==0x31e7d0:
    packet('world_180',arg(0),0x180);constraint(arg(1));packet('world_constraint_listeners_pointer_count',arg(0)+0x60,8);packet('world_constraint_listeners',self.u(arg(0)+0x60),self.u(arg(0)+0x64)*4)
   self.constraint_calls.append(c);self.constraint_pending.append((self.u(sp),sp,c));self.constraint_order+=1
  if self.constraint_pending:self.constraint_executed.add(a)
  super().hook(g,a,size,user)
 def constraint_read(self,g,access,address,size,value,user):
  if not self.constraint_pending:return
  self.constraint_reads.append(dict(call=self.constraint_pending[-1][2]['id'],order=self.constraint_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.constraint_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.constraint_pending:
   self.constraint_stores.append(dict(call=self.constraint_pending[-1][2]['id'],order=self.constraint_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.constraint_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=__import__('pathlib').Path,default=OUT/'final/measured/report.json');a=ap.parse_args();count=verify()
 power=prior
 while not hasattr(power,'power'):power=power.prior
 power=power.power
 # Exactly the prior bounded source preparation and captured platform context.
 # No new title/device/frame or active-state input is introduced here.
 t=power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=ConstraintTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return,t.cache_return,t.narrow_return,t.gjk_return,t.batch_return,t.constraint_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.constraint_pending
 previous=json.loads((ROOT/'analysis/multiplayer/sandbox-source/power-contact-batches/final/measured/report.json').read_text())
 for name,value in(('calls',t.batch_calls),('reads',t.batch_reads),('stores',t.batch_stores),('states',t.batch_states)):assert value==previous[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.constraint_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_batch_owners_and_power_schedule_identical=True,calls=t.constraint_calls,reads=t.constraint_reads,stores=t.constraint_stores,states=t.constraint_states,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=previous['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.constraint_calls),states=len(t.constraint_states),reads=len(t.constraint_reads),stores=len(t.constraint_stores))))
if __name__=='__main__':main()
