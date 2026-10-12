#!/usr/bin/env python3
"""Observe original allocation/dispatch on the unchanged admitted Q owner.

Only observation is added to the immutable effect-pool/begin composition. Raw
descriptor bytes come from the pinned XBE. Body allocation/initial zero stores
are observed during actual original player/unit/weapon creation, not supplied.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
import oracle_mp_grenade_begin_body_1_1 as begin
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_EIP
from functions import _trace

ROOT=begin.ROOT
OWNERS={0xbd170,0xb8250,0xe7f30,0xe7ef0,0xe7ff0,0xe7e60,0x1eb150,0x1ea310}
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(v):return (json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
 prior=ROOT/'analysis/multiplayer/integration/spawn-source/grenade-begin-body/measured/composition/report.json'
 pins={str(prior.relative_to(ROOT)):digest(prior),
  'tools/oracle_mp_grenade_begin_body_1_1.py':digest(Path(begin.__file__))}
 old=begin.owning.GrenadeEffectTrace
 holder=[]
 class DispatchTrace(old):
  def __init__(self,*args,**kw):
   self.storage_calls=[];self.storage_pending=[];self.storage_stores=[];self.storage_order=0
   self.mark_calls=[];self.mark_pending=[];self.mark_stores=[];self.mark_order=0
   super().__init__(*args,**kw)
   self.storage_addresses=set(_trace(self.code,0xbd170,self.starts|{0xbd170}).code)
   holder.append(self)
  def object_pool(self):
   p=self.u(0x4e78d0);h=self.u(p+0x44);capacity=self.u(p+0x20)
   assert capacity==2048 and self.u(p+0x24)==12
   return dict(pointer=f'{p:08x}',raw4c_hex=self.raw(p,0x4c),headers=f'{h:08x}',capacity=capacity,
    high_water=self.u(p+0x38),headers_hex=self.raw(h,capacity*12))
  def owned_objects(self):
   p=self.u(0x4e78d0);h=self.u(p+0x44);result=[]
   for i in range(self.u(p+0x38)):
    if not self.u(h+i*12,2):continue
    at=h+i*12;body=self.u(at+8);n=self.u(at+6,2)
    result.append(dict(datum=f'{(self.u(at,2)<<16)|i:08x}',header_address=f'{at:08x}',header_hex=self.raw(at,12),
     body_address=f'{body:08x}',body_size=n,body_hex=self.raw(body,n)if body and n else ''))
   return result
  def packets(self,entry,registers,stack):
   result=super().packets(entry,registers,stack)
   if entry in (0xe7f30,0xe7ef0,0xe7ff0,0xe7e60,0xb8250,0x1eb150,0x1ea310):
    pool=self.object_pool();result.append(dict(field='original_header_pool_complete',pointer=pool['headers'],size=pool['capacity']*12,raw_hex=pool['headers_hex']))
   return result
  def hook(self,g,address,size,user):
   sp=g.reg_read(UC_X86_REG_ESP)
   for ret,entry_sp,row in self.storage_pending[:]:
    if ret!=address or sp-entry_sp!=8:continue
    row.update(return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',after_pool=self.object_pool(),after_objects=self.owned_objects(),return_order=self.storage_order)
    self.storage_order+=1;self.storage_pending.remove((ret,entry_sp,row))
   for ret,entry_sp,row in self.mark_pending[:]:
    if ret!=address or sp-entry_sp!=4:continue
    row.update(after_pool=self.object_pool(),after_objects=self.owned_objects(),return_order=self.mark_order)
    self.mark_order+=1;self.mark_pending.remove((ret,entry_sp,row))
   if address==0xb8250:
    row=dict(entry='000b8250',caller=f'{self.u(sp):08x}',unit=f'{g.reg_read(UC_X86_REG_EAX):08x}',order=self.mark_order,
     before_pool=self.object_pool(),before_objects=self.owned_objects())
    self.mark_order+=1;self.mark_calls.append(row);self.mark_pending.append((self.u(sp),sp,row))
   if address==0xbd170:
    row=dict(entry='000bd170',caller=f'{self.u(sp):08x}',order=self.storage_order,requested_size=struct.unpack('<h',bytes(g.mem_read(sp+4,2)))[0],
     before_pool=self.object_pool(),before_objects=self.owned_objects(),entry_cpu=self.cpu())
    self.storage_order+=1;self.storage_calls.append(row);self.storage_pending.append((self.u(sp),sp,row))
   if self.storage_pending and address==0x18fce0:
    row=self.storage_pending[-1][2];id=g.reg_read(UC_X86_REG_ESI);h=self.u(self.u(0x4e78d0)+0x44)+(id&65535)*12
    row['allocation_call']=dict(datum=f'{id:08x}',eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',
     heap=f'{self.u(0x4e58bc):08x}',header_address=f'{h:08x}',header_hex=self.raw(h,12),
     pointer_output=f'{self.u(sp+4):08x}',signed_bytes=self.u(sp+8),zero_operand=self.u(sp+12))
   if self.storage_pending and address==0xbd1ad:
    row=self.storage_pending[-1][2];row['allocation_return_al']=g.reg_read(UC_X86_REG_EAX)&255
    if row['allocation_return_al']:
     id=g.reg_read(UC_X86_REG_ESI);h=self.u(self.u(0x4e78d0)+0x44)+(id&65535)*12;b=self.u(h+8);n=row['requested_size'];assert n>=0
     row['before_source_clear']=dict(datum=f'{id:08x}',header_address=f'{h:08x}',header_hex=self.raw(h,12),body_address=f'{b:08x}',body_size=n,body_hex=self.raw(b,n))
   super().hook(g,address,size,user)
  def mem_write(self,g,access,at,n,value,user):
   pc=g.reg_read(UC_X86_REG_EIP)
   if self.storage_pending and pc in getattr(self,'storage_addresses',set()):
    self.storage_stores.append(dict(call=len(self.storage_calls)-1,instruction=f'{pc:08x}',address=f'{at:08x}',size=n,
     before_hex=self.raw(at,n),written_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex(),order=self.storage_order));self.storage_order+=1
   if self.mark_pending and pc==0xb8288:
    self.mark_stores.append(dict(call=len(self.mark_calls)-1,instruction=f'{pc:08x}',address=f'{at:08x}',size=n,
     before_hex=self.raw(at,n),written_hex=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex(),order=self.mark_order));self.mark_order+=1
   super().mem_write(g,access,at,n,value,user)
 begin.owning.ENTRIES.update(OWNERS)
 begin.owning.motion.ENTRIES.update(OWNERS)
 begin.owning.motion.STATE_ENTRIES.update(OWNERS-{0xbd170})
 # Capture every actual handler reachable through the imported descriptor bank.
 from oracle_smg_firing_1_1 import Xbe,PIN
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert digest(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN
 descriptors=[];callbacks=set()
 for i in range(60):
  pointer=int.from_bytes(image.read(0x46ec68+i*4,4),'little');raw=image.read(pointer,16)
  targets=struct.unpack('<4I',raw);callbacks.update(v for v in targets if v)
  descriptors.append(dict(type=i,pointer=f'{pointer:08x}',raw10_hex=raw.hex(),callbacks=[f'{v:08x}'for v in targets]))
 begin.owning.ENTRIES.update(callbacks);begin.owning.motion.ENTRIES.update(callbacks);begin.owning.motion.STATE_ENTRIES.update(callbacks)
 begin.owning.GrenadeEffectTrace=DispatchTrace
 saved=sys.argv
 try:
  sys.argv=[saved[0],'--output',str(out/'begin-observation')];begin.main()
 finally:sys.argv=saved;begin.owning.GrenadeEffectTrace=old
 t=holder[-1];assert not t.storage_pending and not t.mark_pending
 r=json.loads((out/'begin-observation/composition/report.json').read_text());previous=json.loads(prior.read_text())
 def state(report,index):return {k:v for k,v in report['states'][index].items()if k not in('id','sha256')}
 for e in (0xe95e0,0xe7f30,0xe7ef0,0xe7ff0,0xe7e60):
  rows=[c for c in r['calls']if int(c['entry'],16)==e];oldrows=[c for c in previous['calls']if int(c['entry'],16)==e];assert len(rows)==len(oldrows)
  for c,oldc in zip(rows,oldrows):
   assert c['entry_cpu']==oldc['entry_cpu'] and c['return_cpu']==oldc['return_cpu']
   assert state(r,c['before_state'])==state(previous,oldc['before_state']) and state(r,c['after_state'])==state(previous,oldc['after_state'])
 # Keep complete unchanged listings for the newly observed storage/dispatch set.
 listings=[]
 for e in sorted(OWNERS|{0x18fce0}):
  routine=_trace(t.code,e,t.starts|OWNERS|callbacks)
  raw=''.join(f'{q.address:08x} {q.bytes.hex()} {q.mnemonic} {q.op_str}\n'for q in(t.code.at(pc)for pc in sorted(routine.code))).encode()
  name=f'instructions/{e:08x}.asm';save(out/name,raw);listings.append(dict(entry=f'{e:08x}',file=name,sha256=digest(out/name)))
 for name,pin in pins.items():assert digest(ROOT/name)==pin,name
 result=dict(schema='source-unit-action-dispatch-storage-v1',tool_sha256=digest(__file__),source_pins=pins,
  original_xbe_sha256=PIN,original_cache_sha256=r['original_cache_sha256'],owning_report=str((out/'begin-observation/composition/report.json').relative_to(ROOT)),owning_report_sha256=digest(out/'begin-observation/composition/report.json'),
  prior_complete_begin_dispatch_states_identical=True,descriptors=descriptors,body_allocation_calls=t.storage_calls,body_allocation_stores=t.storage_stores,
  mark_update_calls=t.mark_calls,mark_update_stores=t.mark_stores,
  dispatcher_calls=[c for c in r['calls']if int(c['entry'],16)in OWNERS-{0xbd170}],instructions=listings,
  new_gameplay_fields=[],new_source_returns=[],instruction_patches=[],boundaries=r['boundaries']+[
   'New observation only: complete BD170 occurs during the unchanged actual player/biped/SMG creation. Original16D2C0/18FCE0 run; no body size, header, allocation success or initial contents are supplied.',
   'All original action descriptor pointers/callback records are read from pinned executable bytes. Complete dispatcher callbacks still require their own real implementations; recording them is not native completion.',
   'Ordinary owning fixture is nonlocal: source player+28 isNONE. Local-user view notification constructor/storage/clock admission is required separately and is not defaulted.',
   'Native Unit currently retains typed inventory/zoom/grenade counts only, not complete source body/manager/action storage. BD170/dispatch do not close B8A30/C52C0 extension/variant/install producers or grenade attachment/release/world/explosion.'])
 save(out/'report.json',encoded(result));print(json.dumps(dict(report=str(out/'report.json'),sha256=digest(out/'report.json'),body_allocations=len(t.storage_calls),dispatcher_calls=len(result['dispatcher_calls']))))
if __name__=='__main__':main()
