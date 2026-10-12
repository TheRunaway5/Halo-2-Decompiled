#!/usr/bin/env python3
"""Unchanged source-owned power-module world insertion; no guest inputs added."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_body_disposal_1_1 as prior
from oracle_mp_power_body_disposal_1_1 import DisposeTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-world-insertion'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='77b9e7917a017bbe23f56b55bd233ca179202c93562750d923628a754f2660d8'
ENTRIES={0x1d4f30,0x1c7320,0x315820,0x312dd0,0x3144d0,0x31e610,
 0x312360,0x3123a0,0x31d730,0x31d250,0x313c90,0x313e50,0x31d980,
 0x313ef0,0x313f80,0x379140,0x378eb0,0x1d5210,0x1d47c0,0x1d4840,
 0x148a10,0x22e950,0x2df780,0x2df710,0x31f8a0,0x31c5a0}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class WorldTrace(DisposeTrace):
 def __init__(self,data):
  self.world_pending=[];self.world_calls=[];self.world_reads=[];self.world_stores=[]
  self.world_states=[];self.world_ids={};self.world_order=0;self.world_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.world_read)
 def world_state(self):
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
  r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
   for p,n in((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
   globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
    for p,n in((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
     (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4),(0x486577,1),(0x486564,4),(0x54f248,1),(0x54f26c,4),(0x486558,4),(0x526538,4))])
  if hasattr(self,'selected_fixup'):
   tag=self.selected_fixup['tag'];p=tag['offset']+self.cache.magic;n=tag['size']
   r['raw_backing'].append(dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex()))
  if self.slot_component is not None:
   p=self.u(data+(self.slot_object&0xffff)*12+8);r['object_pointer']=f'{p:08x}';r['object_382_hex']=bytes(self.g.mem_read(p,0x382)).hex()
  h=hashlib.sha256(encoded(r)).hexdigest()
  if h not in self.world_ids:
   self.world_ids[h]=len(self.world_states);self.world_states.append(dict(id=len(self.world_states),sha256=h,**r))
  return self.world_ids[h]
 def world_return(self,a):
  matches=[x for x in self.world_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.world_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
    return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.world_state())
   self.world_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.world_pending.remove((ret,entry_sp,c))
 def hook(self,g,a,size,user):
  self.world_return(a);sp=g.reg_read(reg.UC_X86_REG_ESP)
  root=self.slot_ready and a==0x1d4f30 and g.reg_read(reg.UC_X86_REG_ECX)==self.slot_component
  if self.world_pending or root:
   self.world_executed.add(a)
   if a==0x3144fe:
    c=next(c for _,_,c in reversed(self.world_pending)if c['entry']=='003144d0')
    c['packets'].append(dict(name='broadphase_frame_8c',pointer=f'{sp:08x}',size=0x8c,before_hex=bytes(g.mem_read(sp,0x8c)).hex(),captured_at=f'{a:08x}'))
   if a in ENTRIES:
    c=dict(id=len(self.world_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
     order=self.world_order,registers=self.registers(),entry_esp=f'{sp:08x}',
     stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],before_state=self.world_state(),
     x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',packets=[])
    self.world_order+=1
    def packet(name,p,n):c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
    if a==0x1d4f30:packet('component_a0',g.reg_read(reg.UC_X86_REG_ECX),0xa0)
    if a in(0x315820,0x312dd0,0x3144d0):
     packet('world_280',g.reg_read(reg.UC_X86_REG_ECX),0x280);packet('body_a0',self.u(sp+4),0xa0)
    if a==0x31d730:packet('island_b0',g.reg_read(reg.UC_X86_REG_ECX),0xb0)
    if a==0x313c90:packet('pairs_14',g.reg_read(reg.UC_X86_REG_ECX),0x14)
    if a==0x313e50:packet('pairs_14',g.reg_read(reg.UC_X86_REG_ECX),0x14)
    self.world_calls.append(c);self.world_pending.append((self.u(sp),sp,c))
  super().hook(g,a,size,user)
 def world_read(self,g,access,address,size,value,user):
  if not self.world_pending:return
  self.world_reads.append(dict(call=self.world_pending[-1][2]['id'],order=self.world_order,
   instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.world_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.world_pending:
   self.world_stores.append(dict(call=self.world_pending[-1][2]['id'],order=self.world_order,
    instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),
    written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.world_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.power;data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=WorldTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 t.slot_return(STOP);t.prepare_return(STOP);t.group_return(STOP);t.dispose_return(STOP);t.world_return(STOP);t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False)
 t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']))
  t.bridge_return(STOP);t.factory_return(STOP);t.slot_return(STOP);t.prepare_return(STOP);t.group_return(STOP);t.dispose_return(STOP);t.world_return(STOP)
 assert not t.world_pending and not t.dispose_pending and not t.group_pending and not t.prepare_pending and not t.slot_pending and not t.factory_pending and not t.bridge_pending,dict(world=[(hex(ret),hex(sp),c['entry'],c['phase'])for ret,sp,c in t.world_pending],dispose=len(t.dispose_pending),group=len(t.group_pending),prepare=len(t.prepare_pending),slot=len(t.slot_pending),factory=len(t.factory_pending),bridge=len(t.bridge_pending))
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.dispose_calls),('reads',t.dispose_reads),('stores',t.dispose_stores),('states',t.dispose_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES|prior.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.world_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_disposal_and_power_schedule_identical=True,calls=t.world_calls,reads=t.world_reads,stores=t.world_stores,states=t.world_states,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.world_calls),states=len(t.world_states),reads=len(t.world_reads),stores=len(t.world_stores))))
if __name__=='__main__':main()
