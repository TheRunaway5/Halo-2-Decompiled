#!/usr/bin/env python3
"""Measure genuine GJK retrieve/support/simplex owners on retained source-created shapes."""
import argparse,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_narrowphase_owners_1_1 as prior
from oracle_mp_power_narrowphase_owners_1_1 import NarrowTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-gjk-owners'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='17d06f534bd5ed83c4f97b4bae3e3f6c3d3511eb784d407605a8939c0398d9c8'
ENTRIES={0x2e2320:12,0x3039c0:12,0x1f1da0:12,0x2e4320:12,0x30ae40:20,0x30eb30:0,0x309900:12,0x309d40:24,0x309ae0:0,0x30a370:16}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class GjkTrace(NarrowTrace):
 def __init__(self,data):
  self.gjk_calls=[];self.gjk_pending=[];self.gjk_reads=[];self.gjk_stores=[]
  self.gjk_order=0;self.gjk_states=[];self.gjk_ids={};self.gjk_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.gjk_read)
 def gjk_state(self):
  old_states,old_ids=self.current_states,self.current_ids
  self.current_states,self.current_ids=self.gjk_states,self.gjk_ids
  try:return super().current_state()
  finally:self.current_states,self.current_ids=old_states,old_ids
 def gjk_return(self,a):
  for ret,sp,c in list(reversed(self.gjk_pending)):
   if ret!=a or self.g.reg_read(reg.UC_X86_REG_ESP)!=sp+4+ENTRIES[int(c['entry'],16)]:continue
   c.update(return_order=self.gjk_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.gjk_state());self.gjk_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.gjk_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.gjk_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.gjk_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.gjk_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.gjk_state(),packets=[])
   def packet(name,p,n):
    if p and n:c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   if a in(0x2e2320,0x3039c0,0x1f1da0,0x2e4320):
    packet('shape_40',ecx,0x40);packet('support_keys_u16',arg(0),arg(1)*2);packet('retrieved_vectors',arg(2),arg(1)*16)
   if a==0x30ae40:
    packet('gjk_workspace_1b0',ecx,0x1b0);packet('shape_a_40',arg(0),0x40);packet('shape_b_40',arg(1),0x40)
    packet('relative_transform_40',arg(2),0x40);packet('support_state_10',arg(3),0x10);packet('output_vector_10',arg(4),0x10)
   if a==0x30eb30:
    for k in range(4):packet('input_vector_'+str(k),arg(k),0x10)
    packet('closest_result_24',arg(4),0x24)
   if a==0x309900:
    packet('gjk_workspace_1b0',ecx,0x1b0);packet('input_vector_10',arg(0),0x10);packet('input_simplex_60',arg(1),0x60)
   self.gjk_calls.append(c);self.gjk_pending.append((self.u(sp),sp,c));self.gjk_order+=1
  if self.gjk_pending:self.gjk_executed.add(a)
  super().hook(g,a,size,user)
 def gjk_read(self,g,access,address,size,value,user):
  if not self.gjk_pending:return
  self.gjk_reads.append(dict(call=self.gjk_pending[-1][2]['id'],order=self.gjk_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.gjk_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.gjk_pending:
   self.gjk_stores.append(dict(call=self.gjk_pending[-1][2]['id'],order=self.gjk_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.gjk_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/measured/report.json');a=ap.parse_args();count=verify()
 power=prior
 while not hasattr(power,'power'):power=power.prior
 power=power.power
 t=power.player.prepare(json.loads((OWNER/'raw-host-context.json').read_text()),trace_type=GjkTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return,t.child_return,t.cache_return,t.narrow_return,t.gjk_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.gjk_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.narrow_calls),('reads',t.narrow_reads),('stores',t.narrow_stores),('states',t.narrow_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.gjk_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_narrowphase_owners_and_power_schedule_identical=True,calls=t.gjk_calls,reads=t.gjk_reads,stores=t.gjk_stores,states=t.gjk_states,original_scheduler_call_tree=t.agent_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.gjk_calls),states=len(t.gjk_states),reads=len(t.gjk_reads),stores=len(t.gjk_stores))))
if __name__=='__main__':main()
