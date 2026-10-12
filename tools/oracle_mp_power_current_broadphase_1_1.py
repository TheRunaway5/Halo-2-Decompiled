#!/usr/bin/env python3
"""Observe unchanged authored current broadphase, shape and narrowphase calls."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_agent_scheduler_1_1 as prior
from oracle_mp_power_agent_scheduler_1_1 import AgentTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-current-broadphase'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='e35a9c0f8cccf3c868fec4e05a6f0798e5c9b04c54b8b6311d051dac8bbeb4ab'
ENTRIES={0x31db70,0x2e68d0,0x2e2380,0x2e4670,0x300870}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class CurrentTrace(AgentTrace):
 def __init__(self,data):
  self.current_calls=[];self.current_pending=[];self.current_reads=[];self.current_stores=[]
  self.current_order=0;self.current_states=[];self.current_ids={};self.current_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.current_read)
 def current_state(self):
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
  if h not in self.current_ids:
   self.current_ids[h]=len(self.current_states);self.current_states.append(dict(id=len(self.current_states),sha256=h,**r))
  return self.current_ids[h]
 def current_return(self,a):
  for ret,sp,c in list(reversed(self.current_pending)):
   if ret!=a:continue
   c.update(return_order=self.current_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.current_state())
   self.current_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.current_pending.remove((ret,sp,c))
 def hook(self,g,a,size,user):
  self.current_return(a)
  if self.slot_ready and a in ENTRIES:
   sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
   c=dict(id=len(self.current_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.current_order,registers=self.registers(),entry_esp=f'{sp:08x}',stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',before_state=self.current_state(),packets=[])
   def packet(name,p,n):c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
   arg=lambda i:self.u(sp+4+i*4)
   if a==0x31db70:packet('island_b0',ecx,0xb0);packet('backend_90',arg(0),0x90)
   if a==0x2e68d0:
    packet('backend_90',ecx,0x90);packet('handles',arg(0),arg(2)*4);packet('bounds',arg(1),arg(2)*32)
    for n,i in(('added',3),('removed',4)):packet(n+'_descriptor_14',arg(i),0x14)
   if a in(0x2e2380,0x2e4670):packet('shape_40',ecx,0x40);packet('transform_40',arg(0),0x40);packet('bounds_20',arg(2),0x20)
   if a==0x300870:
    packet('agent_40',ecx,0x40)
    for n,i in(('collidable_a_2c',0),('collidable_b_2c',1),('step_30',2),('output_10',3)):packet(n,arg(i),0x2c if i<2 else 0x30 if i==2 else 0x10)
   self.current_calls.append(c);self.current_pending.append((self.u(sp),sp,c));self.current_order+=1
  if self.current_pending:self.current_executed.add(a)
  super().hook(g,a,size,user)
 def current_read(self,g,access,address,size,value,user):
  if not self.current_pending:return
  self.current_reads.append(dict(call=self.current_pending[-1][2]['id'],order=self.current_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.current_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.current_pending:
   self.current_stores.append(dict(call=self.current_pending[-1][2]['id'],order=self.current_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.current_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.prior.prior.prior.power
 data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=CurrentTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return,t.agent_return,t.current_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.current_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.agent_calls),('reads',t.agent_reads),('stores',t.agent_stores),('states',t.agent_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES|{int(c['entry'],16)for c in t.agent_calls}):
  f=_trace(t.code,entry,t.starts|ENTRIES|prior.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.current_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_scheduler_and_power_schedule_identical=True,calls=t.current_calls,reads=t.current_reads,stores=t.current_stores,states=t.current_states,original_scheduler_call_tree=t.agent_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.current_calls),states=len(t.current_states),reads=len(t.current_reads),stores=len(t.current_stores))))
if __name__=='__main__':main()
