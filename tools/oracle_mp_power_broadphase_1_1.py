#!/usr/bin/env python3
"""Unchanged real convex/wrapper bounds and allocator-owned broadphase pairs."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_world_insertion_1_1 as prior
from oracle_mp_power_world_insertion_1_1 import WorldTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-broadphase'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='a29afe0dfc8a6957e61b217cf132ec99b7d553167a4f29161cefe1b342716625'
ENTRIES={0x3144d0,0x2e2380,0x303e30,0x1d1c00,0x2e8a00,0x2e7500,
 0x2e6240,0x2e7da0,0x2e8090,0x2df780,0x2df710,0x2df360,
 0x313c90,0x313e50,0x3141d0,0x31d980,0x313ef0,0x313f80,
 0x378eb0,0x379140,0x148a10,0x22e950,0x148a70,0x22eb20}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class BroadTrace(WorldTrace):
 def __init__(self,data):
  self.broad_pending=[];self.broad_calls=[];self.broad_reads=[];self.broad_stores=[]
  self.broad_states=[];self.broad_ids={};self.broad_order=0;self.broad_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.broad_read)
 def broad_state(self):
  # Do not allocate an ancestor observer's IDs when taking this snapshot.
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);capacity=self.u(pool+0x20)
  from oracle_mp_spawn_biped_physics import RAW_HAVOK,RAW_COMPONENTS
  r=dict(raw_backing=[dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex())
   for p,n in((RAW_HAVOK,0x106470),(RAW_COMPONENTS,0x1409b),(pool,0x4c),(data,capacity*12))],
   globals=[dict(address=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex())
    for p,n in((0x487638,4),(0x480db0,0x18),(0x480d98,12),(0x4e78d0,4),
     (0x526248,4),(0x526234,4),(0x4eb114,4),(0x518444,4),(0x486577,1),(0x486564,4),(0x54f248,1),(0x54f26c,4),(0x486558,4),(0x526538,4))])
  tag=self.selected_fixup['tag'];p=tag['offset']+self.cache.magic;n=tag['size']
  r['raw_backing'].append(dict(pointer=f'{p:08x}',size=n,raw_hex=bytes(self.g.mem_read(p,n)).hex()))
  p=self.u(data+(self.slot_object&0xffff)*12+8);r['object_pointer']=f'{p:08x}';r['object_382_hex']=bytes(self.g.mem_read(p,0x382)).hex()
  h=hashlib.sha256(encoded(r)).hexdigest()
  if h not in self.broad_ids:
   self.broad_ids[h]=len(self.broad_states);self.broad_states.append(dict(id=len(self.broad_states),sha256=h,**r))
  return self.broad_ids[h]
 def broad_return(self,a):
  matches=[x for x in self.broad_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.broad_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
    return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.broad_state())
   self.broad_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.broad_pending.remove((ret,entry_sp,c))
 def hook(self,g,a,size,user):
  self.broad_return(a);sp=g.reg_read(reg.UC_X86_REG_ESP)
  root=self.slot_ready and a==0x3144d0
  if self.broad_pending or root:
   self.broad_executed.add(a)
   if a in(0x3144fe,0x2e8a3b):
    entry=0x3144d0 if a==0x3144fe else 0x2e8a00
    c=next(c for _,_,c in reversed(self.broad_pending)if int(c['entry'],16)==entry)
    c['packets'].append(dict(name='owning_frame_8c',pointer=f'{sp:08x}',size=0x8c,before_hex=bytes(g.mem_read(sp,0x8c)).hex(),captured_at=f'{a:08x}'))
   if a in ENTRIES:
    ecx=g.reg_read(reg.UC_X86_REG_ECX)
    c=dict(id=len(self.broad_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
     order=self.broad_order,registers=self.registers(),entry_esp=f'{sp:08x}',
     stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],before_state=self.broad_state(),
     x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',packets=[])
    self.broad_order+=1
    def packet(name,p,n):c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
    arg=lambda i:self.u(sp+4+i*4)
    if a==0x3144d0:packet('world_280',ecx,0x280);packet('body_a0',arg(0),0xa0)
    if a in(0x2e2380,0x303e30):packet('transform_40',arg(0),0x40);packet('bounds_20',arg(2),0x20)
    if a==0x1d1c00:packet('bounds_20',arg(2),0x20)
    if a==0x2e8a00:packet('backend_90',ecx,0x90);packet('handle_word',arg(0),4);packet('bounds_20',arg(1),0x20);packet('pairs_14',arg(2),0x14)
    if a==0x2e7500:packet('axis_c',ecx,12);packet('min_index',arg(4),2);packet('max_index',arg(5),2)
    if a==0x2e6240:packet('backend_90',ecx,0x90);packet('body_record_10',arg(2),0x10)
    if a in(0x2e7da0,0x2e8090):packet('scratch_descriptor_20',ecx,0x20)
    if a in(0x313c90,0x313e50):packet('pairs_14',ecx,0x14)
    self.broad_calls.append(c);self.broad_pending.append((self.u(sp),sp,c))
  super().hook(g,a,size,user)
 def broad_read(self,g,access,address,size,value,user):
  if not self.broad_pending:return
  self.broad_reads.append(dict(call=self.broad_pending[-1][2]['id'],order=self.broad_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.broad_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.broad_pending:
   self.broad_stores.append(dict(call=self.broad_pending[-1][2]['id'],order=self.broad_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.broad_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.power;data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=BroadTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.broad_pending,[(hex(ret),hex(sp),c['entry'])for ret,sp,c in t.broad_pending]
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.world_calls),('reads',t.world_reads),('stores',t.world_stores),('states',t.world_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES|prior.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.broad_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_world_and_power_schedule_identical=True,calls=t.broad_calls,reads=t.broad_reads,stores=t.broad_stores,states=t.broad_states,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.broad_calls),states=len(t.broad_states),reads=len(t.broad_reads),stores=len(t.broad_stores))))
if __name__=='__main__':main()
