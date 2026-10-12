#!/usr/bin/env python3
"""New read-only actual crate body-slot/rebuild/world-insertion observation.

Runs the immutable original preparation, factory and six selected phases.
No additional guest inputs, admission, contacts, positions or return values.
"""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_body_factory_1_1 as prior
from oracle_mp_power_body_factory_1_1 import FactoryTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-body-slots'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='d92accead4c9b8679b6bbe401ee013c9788634888350aaf10fe2dc574bc9ef24'
ENTRIES={0x1d9090,0x1d9330,0x1d95e0,0x1d98b0,0x1d9f50,0x1da020,
 0x1da3c0,0x1daac0,0x1d3a70,0x1dc0c0,0x1d29a0,0x1d4260,0x1d2ca0,
 0x1d2d20,0x1d2da0,0x318900,0x318a90,0x1da4c0,0x1db1c0,0x1d3bb0,
 0x1c7050,0x1c6f50,0x1c72e0,0x27f190,0x27f9f0,0x2df710,0x2df780,
 0x148360,0x1d2aa0,0x1d2b10,0x1d4f30,0x1d90e0,0x1d5e90,
 0x1dacf0,0x1dadf0,0x1c70d0,0x31b250,0x9d900,0x317970}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class SlotTrace(FactoryTrace):
 def __init__(self,data):
  self.slot_ready=False;self.slot_component=None;self.slot_object=None
  self.slot_pending=[];self.slot_calls=[];self.slot_reads=[];self.slot_stores=[]
  self.slot_states=[];self.slot_ids={};self.slot_order=0;self.slot_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.slot_read)
 def slot_state(self):
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
  r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
   for p,n in ((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
   globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
    for p,n in ((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
      (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4),(0x486560,1))])
  if self.slot_component is not None:
   r['component_pointer']=f'{self.slot_component:08x}'
   r['component_a0_hex']=bytes(self.g.mem_read(self.slot_component,0xa0)).hex()
   p=self.u(data+(self.slot_object&0xffff)*12+8)
   r['object_pointer']=f'{p:08x}';r['object_382_hex']=bytes(self.g.mem_read(p,0x382)).hex()
  h=hashlib.sha256(encoded(r)).hexdigest()
  if h not in self.slot_ids:
   self.slot_ids[h]=len(self.slot_states);self.slot_states.append(dict(id=len(self.slot_states),sha256=h,**r))
  return self.slot_ids[h]
 def slot_return(self,a):
  matches=[x for x in self.slot_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.slot_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
    return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.slot_state())
   self.slot_order+=1
   for q in c.get('packets',[]):q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.slot_pending.remove((ret,entry_sp,c))
 def hook(self,g,a,size,user):
  if self.slot_ready:
   self.slot_return(a)
   sp=g.reg_read(reg.UC_X86_REG_ESP);esi=g.reg_read(reg.UC_X86_REG_ESI)
   if a==0x1c7050 and self.slot_object is None and self.active=='original_authored_scenario_crate_factory':
    self.slot_object=self.u(sp+8)
   if a==0x1d9090 and self.slot_component is None:
    self.slot_component=esi;self.slot_object=self.u(esi+8)
   root=((a==0x1d9090 and esi==self.slot_component)or
    (a==0x1d3bb0 and self.u(sp+4)==self.slot_component)or
    (a==0x1c7050 and self.u(sp+8)==self.slot_object)or
    (a==0x1c70d0 and self.u(sp+4)==self.slot_object))
   if self.slot_pending or root:
    self.slot_executed.add(a)
    if a in ENTRIES:
     c=dict(id=len(self.slot_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
      order=self.slot_order,registers=self.registers(),entry_esp=f'{sp:08x}',
      stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],before_state=self.slot_state(),
      x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',packets=[])
     self.slot_order+=1
     def packet(name,p,n):
      c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
     if a==0x1d95e0:
      packet('body_slot_result_4c',self.u(sp+32),0x4c)
      packet('runtime_context_54',self.u(sp+8),0x54)
     if a==0x1dc0c0:packet('retained_body_slot_60',g.reg_read(reg.UC_X86_REG_EDI),0x60)
     if a==0x1d3a70:packet('component_a0',self.u(sp+4),0xa0)
     self.slot_calls.append(c);self.slot_pending.append((self.u(sp),sp,c))
  super().hook(g,a,size,user)
 def slot_read(self,g,access,address,size,value,user):
  if not self.slot_ready or not self.slot_pending:return
  self.slot_reads.append(dict(call=self.slot_pending[-1][2]['id'],order=self.slot_order,
   instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
   raw_hex=bytes(g.mem_read(address,size)).hex()));self.slot_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.slot_ready and self.slot_pending:
   self.slot_stores.append(dict(call=self.slot_pending[-1][2]['id'],order=self.slot_order,
    instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
    before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),
    source_stack=STACK-0x100000<=address<STACK+0x100000));self.slot_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'world-measured/report.json');args=ap.parse_args()
 count=verify();data=json.loads((OWNER/'raw-host-context.json').read_text());t=prior.prior.power.player.prepare(data,trace_type=SlotTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 t.slot_return(STOP);t.sandbox_enabled=False;assert prior.prior.power.player.spawn_and_activate(t,0,activate=False)
 t.power_observe=True;expected=json.loads((prior.prior.power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(a,16)for a in s['arguments']))
  t.bridge_return(STOP);t.factory_return(STOP);t.slot_return(STOP)
 assert not t.slot_pending and not t.factory_pending and not t.bridge_pending,[(f'{a:08x}',f'{sp:08x}',c['entry'],c['caller'])for a,sp,c in t.slot_pending]
 for label,values in [('factory',dict(calls=t.factory_calls,reads=t.factory_reads,stores=t.factory_stores,states=t.factory_states,math=t.factory_math)),
  ('bridge',dict(calls=t.bridge_calls,reads=t.bridge_reads,stores=t.bridge_stores,states=t.bridge_states))]:
  p=prior.OUT/'final/measured/report.json'if label=='factory'else prior.MANIFEST.parent/'measured/report.json'
  old=json.loads(p.read_text())
  for name,value in values.items():assert value==old[name],(label,name)
 for name,value in [('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),
  ('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)]:assert value==expected[name],name
 args.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for a in sorted(ENTRIES):
  f=_trace(t.code,a,t.starts|ENTRIES|prior.ENTRIES|prior.prior.ENTRIES|prior.prior.power.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=args.output.parent/f'{a:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  listings.append(dict(entry=f'{a:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.slot_executed for i in ins)))
 r=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,
  entire_prior_factory_bridge_and_power_schedule_identical=True,component=f'{t.slot_component:08x}',object=f'{t.slot_object:08x}',
  calls=t.slot_calls,reads=t.slot_reads,stores=t.slot_stores,states=t.slot_states,instruction_listings=listings,
  new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=expected['boundaries']+[
   'Only actual retained crate component rebuild/disposal and its object world-insertion wrapper roots are newly measured. Their child instructions are unchanged.',
   'No extra solver admission, contact, body, shape, allocator success, position, clock or pose is supplied. Player-versus-crate positive collision and repeated natural frames remain unproved.'])
 verify();frozen_write(args.output,encoded(r));print(json.dumps(dict(report=sha(args.output),calls=len(t.slot_calls),states=len(t.slot_states),reads=len(t.slot_reads),stores=len(t.slot_stores))))
if __name__=='__main__':main()
