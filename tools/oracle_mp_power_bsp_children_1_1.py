#!/usr/bin/env python3
"""Measure original resident BSP child query/geometry on the retained real schedule."""
import argparse,json,hashlib
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_current_broadphase_1_1 as prior
from oracle_mp_power_current_broadphase_1_1 import CurrentTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-bsp-children'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='b82760c074e4658932bf9e628b2a7fd1f1b1cd52d71e1c8a994e5b04980ccd9c'
ENTRIES={0x1f2420,0x2e5bc0,0x3066e0,0x305e40,0x1f26b0,0x1f2db0,0x1f3320,0x1f16f0,0x2e0a60}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class ChildTrace(CurrentTrace):
 def __init__(self,data):
  self.child_calls=[];self.child_pending=[];self.child_reads=[];self.child_stores=[]
  self.child_order=0;self.child_states=[];self.child_ids={};self.child_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.child_read)
 def child_state(self):
  old_states,old_ids=self.current_states,self.current_ids
  self.current_states,self.current_ids=self.child_states,self.child_ids
  try:return super().current_state()
  finally:self.current_states,self.current_ids=old_states,old_ids
 def child_return(self,a):
  for ret,sp,c in list(reversed(self.child_pending)):
   if ret!=a:continue
   pop={0x1f2db0:0,0x1f26b0:4,0x1f16f0:20,0x3066e0:12}.get(int(c['entry'],16),8)
   if self.g.reg_read(reg.UC_X86_REG_ESP)!=sp+4+pop:continue
   c.update(return_order=self.child_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.child_state())
   self.child_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.child_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.child_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.child_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.child_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.child_state(),packets=[])
   def packet(name,p,n):
    if p and n:c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   if a==0x2e0a60:
    packet('relative_output_40',ecx,0x40);packet('left_transform_40',arg(0),0x40);packet('right_transform_40',arg(1),0x40)
   if a in(0x1f2420,0x2e5bc0):
    packet('shape_40',ecx,0x40);packet('bounds_20',arg(0),32);packet('output_descriptor_c',arg(1),12)
   if a==0x3066e0:
    packet('query_context_34',ecx,0x34);packet('mopp_header_30',arg(0),0x30);packet('bounds_20',arg(1),32);packet('output_descriptor_c',arg(2),12)
   if a==0x305e40:
    packet('query_context_34',ecx,0x34);packet('classifier_38',arg(0),0x38);packet('local_classifier_38',sp-0x38,0x38);packet('output_descriptor_c',self.u(ecx),12)
   if a==0x1f26b0:packet('output_descriptor_c',g.reg_read(reg.UC_X86_REG_ESI),12)
   if a==0x1f16f0:
    packet('output_triangle_100',ecx,0x100);packet('collision_bsp_40',arg(0),0x40);packet('optional_transform_40',g.reg_read(reg.UC_X86_REG_EDI),0x40)
   if a==0x1f3320:packet('output_child_100',arg(1),0x100)
   self.child_calls.append(c);self.child_pending.append((self.u(sp),sp,c));self.child_order+=1
  if self.child_pending:self.child_executed.add(a)
  super().hook(g,a,size,user)
 def child_read(self,g,access,address,size,value,user):
  if not self.child_pending:return
  self.child_reads.append(dict(call=self.child_pending[-1][2]['id'],order=self.child_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.child_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.child_pending:
   self.child_stores.append(dict(call=self.child_pending[-1][2]['id'],order=self.child_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.child_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.power
 t=power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=ChildTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.child_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.current_calls),('reads',t.current_reads),('stores',t.current_stores),('states',t.current_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.child_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_current_broadphase_and_power_schedule_identical=True,calls=t.child_calls,reads=t.child_reads,stores=t.child_stores,states=t.child_states,original_scheduler_call_tree=t.agent_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.child_calls),states=len(t.child_states),reads=len(t.child_reads),stores=len(t.child_stores))))
if __name__=='__main__':main()
