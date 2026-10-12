#!/usr/bin/env python3
"""Measure original retained terrain key-cache/child-agent orchestration on the retained real schedule."""
import argparse,json,hashlib
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_bsp_children_1_1 as prior
from oracle_mp_power_bsp_children_1_1 import ChildTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-child-agents'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='852cb649d852311b39b7ba97ee74341c822626a248532a8239ee7a1e04866723'
ENTRIES={0x300870,0x2efc30,0x2ef8f0,0x2ef970,0x2efa10,0x1c6c30,0x2e5e20,0x2ef680,0x2f48a0,0x2e13b0,0x305560,0x307fe0}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class CacheTrace(ChildTrace):
 def __init__(self,data):
  self.cache_calls=[];self.cache_pending=[];self.cache_reads=[];self.cache_stores=[]
  self.cache_order=0;self.cache_states=[];self.cache_ids={};self.cache_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.cache_read)
 def cache_state(self):
  old_states,old_ids=self.current_states,self.current_ids
  self.current_states,self.current_ids=self.cache_states,self.cache_ids
  try:return super().current_state()
  finally:self.current_states,self.current_ids=old_states,old_ids
 def cache_return(self,a):
  for ret,sp,c in list(reversed(self.cache_pending)):
   if ret!=a:continue
   pop={0x300870:16,0x2efc30:16,0x2ef8f0:0,0x2ef970:4,0x2efa10:0,0x1c6c30:24,0x2e5e20:12,0x2ef680:0,0x2f48a0:12,0x2e13b0:0,0x305560:12,0x307fe0:0}[int(c['entry'],16)]
   if self.g.reg_read(reg.UC_X86_REG_ESP)!=sp+4+pop:continue
   c.update(return_order=self.cache_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.cache_state())
   self.cache_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.cache_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.cache_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.cache_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.cache_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.cache_state(),packets=[])
   def packet(name,p,n):
    if p and n:c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   if a in(0x300870,0x2efc30):
    packet('retained_agent_40',ecx,0x40)
    packet('collidable_a_2c',arg(0),0x2c);packet('collidable_b_2c',arg(1),0x2c)
    packet('step_30',arg(2),0x30)
    if a==0x300870:packet('output_descriptor_10',arg(3),0x10)
    else:c['child_factory_entry']=f'{arg(3):08x}'
    packet('retained_child_rows',self.u(ecx+12),self.u(ecx+16)*12)
   if a==0x2ef8f0:packet('child_keys',arg(0),max(0,arg(2)+1)*4)
   if a in(0x2ef970,0x2efa10):packet('temporary_child_bank_14',ecx,0x14)
   if a==0x1c6c30:
    packet('filter_80',ecx-12,0x80);packet('output_bool',arg(0),1)
    packet('step_30',arg(1),0x30);packet('collidable_a_2c',arg(2),0x2c);packet('collidable_b_2c',arg(3),0x2c)
   if a==0x2e5e20:packet('filter_80',ecx,0x80);packet('output_bool',arg(0),1)
   if a==0x2ef680:
    packet('collidable_a_2c',arg(0),0x2c);packet('child_collidable_2c',arg(1),0x2c);packet('step_30',arg(2),0x30)
   if a==0x2f48a0:
    packet('new_child_agent_100',ecx,0x100);packet('collidable_a_2c',arg(0),0x2c);packet('child_collidable_2c',arg(1),0x2c);packet('contact_manager_30',arg(2),0x30)
   if a==0x305560:packet('support_state_10',ecx,0x10)
   self.cache_calls.append(c);self.cache_pending.append((self.u(sp),sp,c));self.cache_order+=1
  if self.cache_pending:self.cache_executed.add(a)
  super().hook(g,a,size,user)
 def cache_read(self,g,access,address,size,value,user):
  if not self.cache_pending:return
  self.cache_reads.append(dict(call=self.cache_pending[-1][2]['id'],order=self.cache_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.cache_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.cache_pending:
   self.cache_stores.append(dict(call=self.cache_pending[-1][2]['id'],order=self.cache_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.cache_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.power
 t=power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=CacheTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return,t.cache_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.cache_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.child_calls),('reads',t.child_reads),('stores',t.child_stores),('states',t.child_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.cache_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_bsp_children_and_power_schedule_identical=True,calls=t.cache_calls,reads=t.cache_reads,stores=t.cache_stores,states=t.cache_states,original_scheduler_call_tree=t.agent_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.cache_calls),states=len(t.cache_states),reads=len(t.cache_reads),stores=len(t.cache_stores))))
if __name__=='__main__':main()
