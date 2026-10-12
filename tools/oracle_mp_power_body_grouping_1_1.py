#!/usr/bin/env python3
"""Independent unchanged1D9330 grouping/context and body orchestration observation."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_body_prepare_1_1 as prior
from oracle_mp_power_body_prepare_1_1 import PrepareTrace,ROOT,OWNER,sha,tmath
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-body-grouping'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='ff11162fc2996ba76c0cc6659736d6b2dbcd0c6dafec82d9df6f4b60f923279e'
ENTRIES=prior.ENTRIES|{0x1d9330,0x20e370,0xbb050,0x143860,0x1441d0,0x183500,0x183770,0x183650,0x1eead0,0x1eeb70,0x1eeb20,0xdcb40,0xdcac0,0x1da4c0,0x1db1c0}
MATH=prior.MATH
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class GroupTrace(PrepareTrace):
 def __init__(self,data):
  self.group_pending=[];self.group_calls=[];self.group_reads=[];self.group_stores=[]
  self.group_states=[];self.group_ids={};self.group_order=0;self.group_executed=set();self.group_math=[]
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.group_read)
 def group_state(self):
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
  r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
   for p,n in ((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
   globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
    for p,n in ((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
      (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4),(0x486577,1),(0x486564,4))])
  if hasattr(self,'selected_fixup'):
   tag=self.selected_fixup['tag'];p=tag['offset']+self.cache.magic;n=tag['size']
   r['raw_backing'].append(dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex()))
  if self.slot_component is not None:
   p=self.u(data+(self.slot_object&0xffff)*12+8);r['object_pointer']=f'{p:08x}';r['object_382_hex']=bytes(self.g.mem_read(p,0x382)).hex()
  h=hashlib.sha256(encoded(r)).hexdigest()
  if h not in self.group_ids:
   self.group_ids[h]=len(self.group_states);self.group_states.append(dict(id=len(self.group_states),sha256=h,**r))
  return self.group_ids[h]
 def group_return(self,a):
  matches=[x for x in self.group_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.group_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
    returned_xmm={str(n):f'{self.g.reg_read(getattr(reg,f"UC_X86_REG_XMM{n}")):032x}'for n in range(8)},
    return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.group_state())
   self.group_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.group_pending.remove((ret,entry_sp,c))
 def hook(self,g,a,size,user):
  self.group_return(a)
  sp=g.reg_read(reg.UC_X86_REG_ESP)
  root=self.slot_ready and a==0x1d9330 and self.u(sp+4)==self.slot_component
  if self.group_pending or root:
   self.group_executed.add(a)
   frames={0x1d9363:(0x1d9330,0x4e94),0x1d95f2:(0x1d95e0,0x100),0x1d98ce:(0x1d98b0,0x460),0x1db1d5:(0x1db1c0,0x460)}
   if a in frames:
    entry,n=frames[a]
    c=next(c for _,_,c in reversed(self.group_pending)if int(c['entry'],16)==entry)
    c['packets'].append(dict(name='owning_frame',pointer=f'{sp:08x}',size=n,before_hex=bytes(g.mem_read(sp,n)).hex(),captured_at=f'{a:08x}'))
   if a in MATH:
    self.group_math.append(dict(order=self.group_order,instruction=f'{a:08x}',
     xmm={str(n):f'{g.reg_read(getattr(reg,f"UC_X86_REG_XMM{n}")):032x}'for n in range(8)},
     esp=f'{sp:08x}',stack50_hex=bytes(g.mem_read(sp,0x50)).hex(),registers=self.registers()))
    self.group_order+=1
   if 0x184e6b<=a<=0x184f80 and tmath(self.code.at(a)):
    self.group_math.append(dict(order=self.group_order,instruction=f'{a:08x}',xmm={str(n):f'{g.reg_read(getattr(reg,f"UC_X86_REG_XMM{n}")):032x}'for n in range(8)},esp=f'{sp:08x}',stack50_hex=bytes(g.mem_read(sp,0x50)).hex(),registers=self.registers()));self.group_order+=1
   if a in ENTRIES:
    c=dict(id=len(self.group_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
     order=self.group_order,registers=self.registers(),entry_esp=f'{sp:08x}',
     stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],before_state=self.group_state(),
     x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',packets=[])
    self.group_order+=1
    def packet(name,p,n):c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
    if a==0x20e370:packet('runtime_context_54',g.reg_read(reg.UC_X86_REG_ESI),0x54)
    if a==0x183500:packet('group_bank_4448',g.reg_read(reg.UC_X86_REG_ESI),0x4448)
    if a==0xbb050:packet('root_matrix_34',self.u(sp+4),0x34)
    if a==0x143860:
     for name,r in(('up',reg.UC_X86_REG_EAX),('forward',reg.UC_X86_REG_ECX),('point',reg.UC_X86_REG_ESI)):packet(name,g.reg_read(r),12)
     packet('root_matrix_34',g.reg_read(reg.UC_X86_REG_EDX),0x34)
    if a==0x183770:
     packet('region_output',g.reg_read(reg.UC_X86_REG_EBX),4);packet('permutation_output',self.u(sp+12),4)
    if a==0x183650:packet('slot_map_108',self.u(sp+4),0x108)
    if a==0x1eead0:packet('iterator_8',g.reg_read(reg.UC_X86_REG_EDI),8)
    if a==0x1eeb70:
     packet('stride_output',g.reg_read(reg.UC_X86_REG_ECX),4);packet('iterator_type',g.reg_read(reg.UC_X86_REG_EDX),4)
    if a==0x1eeb20:packet('iterator_8',g.reg_read(reg.UC_X86_REG_EDI),8)
    if a==0x1eead0:packet('iterator_stride_scratch',sp-8,4)
    if a==0x1d9330:packet('component_a0',self.u(sp+4),0xa0)
    if a==0x1da4c0:packet('runtime_context_54',g.reg_read(reg.UC_X86_REG_EDI),0x54)
    if a==0x1d95e0:
     packet('body_slot_result_4c',self.u(sp+32),0x4c);packet('runtime_context_54',self.u(sp+8),0x54)
    if a==0x1d98b0:
     for n,size,name in((3,0x40,'indices'),(4,4,'count'),(8,4,'mass'),(9,16,'inertia'),(10,0x3c,'tensor'),(11,1,'flag12'),(12,4,'request'),(13,1,'flag14'),(14,1,'flag15')):
      packet(name,self.u(sp+4+n*4),size)
    if a==0x184510:
     packet('ecx_output',g.reg_read(reg.UC_X86_REG_ECX),4);packet('edx_output',g.reg_read(reg.UC_X86_REG_EDX),4)
    if a==0x184800:
     packet('stack16_output',self.u(sp+16),4);packet('stack20_output',self.u(sp+20),4);packet('transform_40',self.u(sp+12),0x40)
    if a in(0x2e2530,0x2e2260):packet('wrapper_shape_40',g.reg_read(reg.UC_X86_REG_ECX),0x40)
    if a==0x2e2260:packet('configuration_input_14',self.u(sp+4),0x14)
    if a==0x2e5780:packet('configuration_input_4',self.u(sp+4),4)
    if a in(0x2e25a0,0x2e5720,0x2e5750,0x2e5760):packet('configuration_14',g.reg_read(reg.UC_X86_REG_ECX),0x14)
    self.group_calls.append(c);self.group_pending.append((self.u(sp),sp,c))
  super().hook(g,a,size,user)
 def group_read(self,g,access,address,size,value,user):
  if not self.group_pending:return
  self.group_reads.append(dict(call=self.group_pending[-1][2]['id'],order=self.group_order,
   instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.group_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.group_pending:
   self.group_stores.append(dict(call=self.group_pending[-1][2]['id'],order=self.group_order,
    instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),
    written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.group_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.power;data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=GroupTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 t.slot_return(STOP);t.prepare_return(STOP);t.group_return(STOP);t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False)
 t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']))
  t.bridge_return(STOP);t.factory_return(STOP);t.slot_return(STOP);t.prepare_return(STOP);t.group_return(STOP)
 assert not t.group_pending and not t.prepare_pending and not t.slot_pending and not t.factory_pending and not t.bridge_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.prepare_calls),('reads',t.prepare_reads),('stores',t.prepare_stores),('states',t.prepare_states),('math',t.prepare_math)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES|prior.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.group_executed for i in ins)))
 result=dict(schema=2,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_prepare_wrapper_and_power_schedule_identical=True,calls=t.group_calls,reads=t.group_reads,stores=t.group_stores,states=t.group_states,math=t.group_math,factory_calls=t.factory_calls,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.group_calls),states=len(t.group_states),reads=len(t.group_reads),stores=len(t.group_stores),math=len(t.group_math))))
if __name__=='__main__':main()
