#!/usr/bin/env python3
"""Measure genuine canister packet batching, retention and manager delivery."""
import argparse,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_gjk_owners_1_1 as prior
from oracle_mp_power_gjk_owners_1_1 import GjkTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-contact-batches'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='243b14c36bd310200849eaf744337307a9ab9c2f469493f00f49b79582dc9024'
ENTRIES={0x300870:16,0x300650:20,0x309210:0,0x309450:0,0x3001d0:4,0x300260:0,0x300400:0,0x300100:4,0x31f1b0:16,0x2e0ba0:8,0x31feb0:8}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class BatchTrace(GjkTrace):
 def __init__(self,data):
  self.batch_calls=[];self.batch_pending=[];self.batch_reads=[];self.batch_stores=[]
  self.batch_order=0;self.batch_states=[];self.batch_ids={};self.batch_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.batch_read)
 def batch_state(self):
  old_states,old_ids=self.current_states,self.current_ids
  self.current_states,self.current_ids=self.batch_states,self.batch_ids
  try:return super().current_state()
  finally:self.current_states,self.current_ids=old_states,old_ids
 def batch_return(self,a):
  for ret,sp,c in list(reversed(self.batch_pending)):
   if ret!=a or self.g.reg_read(reg.UC_X86_REG_ESP)!=sp+4+ENTRIES[int(c['entry'],16)]:continue
   c.update(return_order=self.batch_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.batch_state());self.batch_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.batch_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.batch_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.batch_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.batch_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.batch_state(),packets=[])
   def packet(name,p,n):
    if p and n:c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   def bank(name,p,stride):
    packet(name+'_descriptor',p,12)
    count=self.u(p+4);cap=self.u(p+8)&0x7fffffff
    assert cap<0x10000,(name,p,cap)
    packet(name+'_storage',self.u(p),cap*stride)
   def col(name,p):
    packet(name+'_collidable_30',p,0x30)
    packet(name+'_transform_60',self.u(p+8),0x60)
   def manager(p):
    packet('manager_120',p,0x120)
    bank('manager_index_bank',p+0x30,1);bank('manager_point_bank',p+0x44,32);bank('manager_scalar_bank',p+0xa0,28)
   if a==0x300870:
    packet('parent_agent_30',ecx,0x30);bank('child_agents',ecx+12,12)
    col('a',arg(0));col('b',arg(1));packet('step_34',arg(2),0x34);packet('output_10',arg(3),16)
   elif a==0x300650:
    packet('candidate_20',arg(0),32);bank('packets',arg(1),128);packet('context_10',arg(4),16)
   elif a==0x309210:
    col('a',arg(0));col('b',arg(1));packet('step_34',arg(2),0x34);packet('packet_80',arg(3),128);manager(arg(4));packet('retained_rows_c0',arg(5),192);packet('retained_count',arg(6),4)
   elif a==0x309450:
    col('a',arg(0));col('b',arg(1));packet('step_34',arg(2),0x34);packet('retained_rows_c0',arg(4),192);packet('retained_count',arg(5),4);packet('output_10',arg(6),16);bank('emitted_rows',self.u(arg(6)),48);manager(arg(7))
   elif a==0x3001d0:packet('scratch_descriptor_14',ecx,20)
   elif a==0x300260:packet('scratch_descriptor_14',ecx,20);packet('scratch_storage',self.u(ecx),self.u(ecx+16)*256)
   elif a==0x300400:bank('packets',ecx,128)
   elif a==0x300100:packet('destination_packet',ecx,128);packet('input_packet',arg(0),128)
   elif a==0x31f1b0:
    manager(ecx);col('a',arg(0));col('b',arg(1));packet('step_34',arg(2),0x34);packet('world_contact_20',arg(3),32)
   elif a==0x31feb0:
    packet('motion_b0',ecx,176);packet('world_point',arg(0),16);packet('point_velocity',arg(1),16)
   elif a==0x2e0ba0:
    packet('output_vector',ecx,16);packet('transform_40',arg(0),64);packet('input_vector',arg(1),16)
   self.batch_calls.append(c);self.batch_pending.append((self.u(sp),sp,c));self.batch_order+=1
  if self.batch_pending:self.batch_executed.add(a)
  super().hook(g,a,size,user)
 def batch_read(self,g,access,address,size,value,user):
  if not self.batch_pending:return
  self.batch_reads.append(dict(call=self.batch_pending[-1][2]['id'],order=self.batch_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.batch_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.batch_pending:
   self.batch_stores.append(dict(call=self.batch_pending[-1][2]['id'],order=self.batch_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.batch_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/measured/report.json');a=ap.parse_args();count=verify()
 power=prior
 while not hasattr(power,'power'):power=power.prior
 power=power.power
 t=power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=BatchTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return,t.cache_return,t.narrow_return,t.gjk_return,t.batch_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.batch_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.gjk_calls),('reads',t.gjk_reads),('stores',t.gjk_stores),('states',t.gjk_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.batch_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_gjk_owners_and_power_schedule_identical=True,calls=t.batch_calls,reads=t.batch_reads,stores=t.batch_stores,states=t.batch_states,original_scheduler_call_tree=t.agent_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.batch_calls),states=len(t.batch_states),reads=len(t.batch_reads),stores=len(t.batch_stores))))
if __name__=='__main__':main()
