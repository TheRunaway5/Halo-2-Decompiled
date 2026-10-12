#!/usr/bin/env python3
"""Independent unchanged real crate motion/shape constructor observation.

No new guest writes, poses, clock, allocation results, admission or contact.
Both original physical1DAAC0 roots are selected by the actual returned component
identity; unrelated player constructors remain outside this measurement.
"""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_body_bridge_1_1 as prior
from oracle_mp_power_body_bridge_1_1 import BodyTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-body-factory'
MANIFEST=ROOT/'analysis/multiplayer/sandbox-source/power-body-bridge/final/manifest.json'
MANIFEST_PIN='9b54f4cb04528cfe41057994641651cfacd76ede4f2d1535980db50e68034fa4'
ENTRIES={0x1daac0,0x318100,0x317ea0,0x320cc0,0x321320,0x31f970,0x317a30,0x378ab0,
 0x31fd40,0x31fd50,0x321390,0x321050,0x321220,0x2e0ba0,0x31fe90,0x31fea0,0x317bf0,0x31ac90,0x31abd0,
 0x1dc7e0,0x317d10,0x303e30,0x1d1c00,0x9d8e0,0x2e0860,0x1dc750,0x148a10,
 0x22e950,0x312410,0x318af0,0x318a20,0x317890,0x31c1a0,0x31c110,0x3121a0}
SPAN={0x318100:0xa0,0x312410:0xa0,0x318af0:0xa0,0x318a20:0xa0,
 0x320cc0:0xd0,0x31f970:0xd0,0x321320:0x110,0x317a30:0xc0,
 0x2e0ba0:16,0x321220:0x110,0x31fd40:0xd0,0x31fd50:0x110,0x321390:0x110,0x321050:0x110,
 0x31fe90:0x110,0x31fea0:0x110,0x317bf0:0xa0,0x31ac90:0x60,0x31abd0:0x60,
 0x1dc7e0:0xa0,0x317d10:0xa0,0x303e30:0x40,0x9d8e0:0x40,
 0x378ab0:0x30,0x1dc750:16,0x317890:0xb8,0x31c1a0:0x20,0x31c110:0x18}
def verify():
 prior.verify();assert sha(MANIFEST)==MANIFEST_PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class FactoryTrace(BodyTrace):
 def __init__(self,data):
  self.factory_enabled=False;self.factory_root=None;self.factory_pending=[]
  self.factory_calls=[];self.factory_reads=[];self.factory_stores=[];self.factory_states=[]
  self.factory_ids={};self.factory_order=0;self.factory_executed=set();self.factory_math=[]
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.factory_read)
 def factory_state(self):
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
  r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
   for p,n in ((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
   globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
    for p,n in ((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
      (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4))])
  h=hashlib.sha256(encoded(r)).hexdigest()
  if h not in self.factory_ids:
   self.factory_ids[h]=len(self.factory_states);self.factory_states.append(dict(id=len(self.factory_states),sha256=h,**r))
  return self.factory_ids[h]
 def factory_return(self,a):
  matches=[x for x in self.factory_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.factory_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
    return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.factory_state())
   self.factory_order+=1
   if 'owned_pointer'in c:c['owned_after_hex']=bytes(self.g.mem_read(int(c['owned_pointer'],16),c['owned_size'])).hex()
   if 'output_pointer'in c:c['output_after_hex']=bytes(self.g.mem_read(int(c['output_pointer'],16),c['output_size'])).hex()
   self.factory_pending.remove((ret,entry_sp,c))
  if not self.factory_pending:self.factory_enabled=False
 def hook(self,g,a,size,user):
  if self.bridge_enabled and a==0x1daac0:
   cp=g.reg_read(reg.UC_X86_REG_EAX)
   if self.factory_root is None:self.factory_root=cp
   if cp==self.factory_root:
    assert not self.factory_pending;self.factory_enabled=True
  if self.factory_enabled:
   self.factory_return(a)
   if self.factory_enabled:
    self.factory_executed.add(a)
    if a in (0x1dc782,0x1dc786):
     self.factory_math.append(dict(order=self.factory_order,instruction=f'{a:08x}',
      xmm0=f'{g.reg_read(reg.UC_X86_REG_XMM0):032x}',xmm1=f'{g.reg_read(reg.UC_X86_REG_XMM1):032x}'))
     self.factory_order+=1
    if a in ENTRIES:
     sp=g.reg_read(reg.UC_X86_REG_ESP);ecx=g.reg_read(reg.UC_X86_REG_ECX)
     c=dict(id=len(self.factory_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
      order=self.factory_order,registers=self.registers(),entry_esp=f'{sp:08x}',
      stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],before_state=self.factory_state(),
      x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}')
     self.factory_order+=1
     if a in SPAN:
      c.update(owned_pointer=f'{ecx:08x}',owned_size=SPAN[a],owned_before_hex=bytes(g.mem_read(ecx,SPAN[a])).hex())
     if a==0x1daac0:
      c.update(owned_pointer=f'{g.reg_read(reg.UC_X86_REG_EAX):08x}',owned_size=0xa0,
       owned_before_hex=bytes(g.mem_read(g.reg_read(reg.UC_X86_REG_EAX),0xa0)).hex())
     if a in(0x318100,0x312410,0x318af0,0x318a20):
      p=self.u(sp+4);c.update(info_pointer=f'{p:08x}',info_b8_hex=bytes(g.mem_read(p,0xb8)).hex())
     if a in(0x303e30,0x1d1c00,0x2e0860):
      p=ecx if a==0x2e0860 else self.u(sp+(12 if a==0x303e30 else 20));n=16 if a==0x2e0860 else 32
      c.update(output_pointer=f'{p:08x}',output_size=n,output_before_hex=bytes(g.mem_read(p,n)).hex())
     self.factory_calls.append(c);self.factory_pending.append((self.u(sp),sp,c))
  super().hook(g,a,size,user)
 def factory_read(self,g,access,address,size,value,user):
  if not self.factory_enabled or not self.factory_pending:return
  self.factory_reads.append(dict(call=self.factory_pending[-1][2]['id'],order=self.factory_order,
   instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
   raw_hex=bytes(g.mem_read(address,size)).hex()));self.factory_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.factory_enabled and self.factory_pending:
   assert size in(1,2,4,8)
   self.factory_stores.append(dict(call=self.factory_pending[-1][2]['id'],order=self.factory_order,
    instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
    before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),
    source_stack=STACK-0x100000<=address<STACK+0x100000));self.factory_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');args=ap.parse_args()
 count=verify();data=json.loads((OWNER/'raw-host-context.json').read_text());t=prior.power.player.prepare(data,trace_type=FactoryTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 t.sandbox_enabled=False;assert prior.power.player.spawn_and_activate(t,0,activate=False)
 t.power_observe=True;expected=json.loads((prior.power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(a,16)for a in s['arguments']));t.bridge_return(STOP);t.factory_return(STOP)
 assert not t.factory_pending and not t.bridge_pending
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in [('calls',t.bridge_calls),('reads',t.bridge_reads),('stores',t.bridge_stores),('states',t.bridge_states)]:assert value==old[name],name
 for name,value in [('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),
  ('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)]:assert value==expected[name],name
 args.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for a in sorted(ENTRIES):
  f=_trace(t.code,a,t.starts|ENTRIES|prior.ENTRIES|prior.power.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=args.output.parent/f'{a:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  listings.append(dict(entry=f'{a:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.factory_executed for i in ins)))
 r=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=MANIFEST_PIN,prior_verified_files=count,
  prior_factory_body_bridge_and_full_schedule_identical=True,calls=t.factory_calls,reads=t.factory_reads,
  stores=t.factory_stores,states=t.factory_states,math=t.factory_math,instruction_listings=listings,
  new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=expected['boundaries']+[
   'Only the actual source-created crate physical component1DAAC0 roots are measured. Player motion constructors are outside this new slice.',
   'RSQRTSS actual original emulator inputs/outputs are observed; Xbox hardware reciprocal estimate parity remains explicit.',
   'No successful shape/backend/allocator/contact/pose service is supplied; all measured child routines execute original instructions.'])
 verify();frozen_write(args.output,encoded(r));print(json.dumps(dict(report=sha(args.output),calls=len(t.factory_calls),states=len(t.factory_states),reads=len(t.factory_reads),stores=len(t.factory_stores))))
if __name__=='__main__':main()
