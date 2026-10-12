#!/usr/bin/env python3
"""Measure real shape-motion/separation owners without supplying narrowphase outputs."""
import argparse,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_child_agents_1_1 as prior
from oracle_mp_power_child_agents_1_1 import CacheTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-narrowphase-owners'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='9440be04c95ba7f35c3f1a0421ca3146209c7425d9379765683d6c880ea7dc01'
ENTRIES={0x2e1430:0,0x2e14e0:20,0x2f4bd0:20,0x30ae40:20,0x309860:12,0x2e0c70:8}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class NarrowTrace(CacheTrace):
 def __init__(self,data):
  self.narrow_calls=[];self.narrow_pending=[];self.narrow_reads=[];self.narrow_stores=[]
  self.narrow_order=0;self.narrow_states=[];self.narrow_ids={};self.narrow_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.narrow_read)
 def narrow_state(self):
  old_states,old_ids=self.current_states,self.current_ids
  self.current_states,self.current_ids=self.narrow_states,self.narrow_ids
  try:return super().current_state()
  finally:self.current_states,self.current_ids=old_states,old_ids
 def narrow_return(self,a):
  for ret,sp,c in list(reversed(self.narrow_pending)):
   if ret!=a or self.g.reg_read(reg.UC_X86_REG_ESP)!=sp+4+ENTRIES[int(c['entry'],16)]:continue
   c.update(return_order=self.narrow_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.narrow_state());self.narrow_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.narrow_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.narrow_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.narrow_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.narrow_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.narrow_state(),packets=[])
   def packet(name,p,n):
    if p and n:c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   if a==0x2e1430:
    packet('output_bool',arg(0),1);packet('dispatch_table',arg(1)+0x18c,0x1000)
   if a in(0x2e14e0,0x2f4bd0):
    packet('retained_child_agent_100',ecx,0x100);packet('output_bool',arg(0),1)
    packet('collidable_a_2c',arg(1),0x2c);packet('collidable_b_2c',arg(2),0x2c)
    packet('step_30',arg(3),0x30);packet('step_mode_byte',self.u(arg(3)+12),1);packet('output_50',arg(4),0x50)
    for label,col in(('a',arg(1)),('b',arg(2))):
     packet('shape_'+label+'_40',self.u(col),0x40)
     packet('transform_'+label+'_60',self.u(col+8),0x60)
     while self.u(col+12):col=self.u(col+12)
     prop=self.u(col+32)
     if prop:packet('root_property_'+label+'_50',prop,0x50);packet('motion_'+label+'_60',self.u(prop+0x3c),0x60)
   if a==0x30ae40:
    packet('gjk_workspace_1b0',ecx,0x1b0);packet('shape_a_40',arg(0),0x40);packet('shape_b_40',arg(1),0x40)
    packet('relative_transform_40',arg(2),0x40);packet('support_state_10',arg(3),0x10);packet('output_vector_10',arg(4),0x10)
   if a==0x309860:
    packet('gjk_workspace_1b0',ecx,0x1b0);packet('support_state_10',arg(0),0x10);packet('result_vector_10',arg(1),0x10);packet('output_50',arg(2),0x50)
   if a==0x2e0c70:
    packet('output_vector_10',ecx,0x10);packet('matrix_30',arg(0),0x30);packet('input_vector_10',arg(1),0x10)
   self.narrow_calls.append(c);self.narrow_pending.append((self.u(sp),sp,c));self.narrow_order+=1
  if self.narrow_pending:self.narrow_executed.add(a)
  super().hook(g,a,size,user)
 def narrow_read(self,g,access,address,size,value,user):
  if not self.narrow_pending:return
  self.narrow_reads.append(dict(call=self.narrow_pending[-1][2]['id'],order=self.narrow_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.narrow_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.narrow_pending:
   self.narrow_stores.append(dict(call=self.narrow_pending[-1][2]['id'],order=self.narrow_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.narrow_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.power
 t=power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=NarrowTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return,t.cache_return,t.narrow_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.narrow_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.cache_calls),('reads',t.cache_reads),('stores',t.cache_stores),('states',t.cache_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.narrow_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_child_agents_and_power_schedule_identical=True,calls=t.narrow_calls,reads=t.narrow_reads,stores=t.narrow_stores,states=t.narrow_states,original_scheduler_call_tree=t.agent_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.narrow_calls),states=len(t.narrow_states),reads=len(t.narrow_reads),stores=len(t.narrow_stores))))
if __name__=='__main__':main()
