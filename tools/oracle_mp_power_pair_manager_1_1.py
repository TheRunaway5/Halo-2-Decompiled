#!/usr/bin/env python3
"""Unchanged actual pair filter/static/PVS-island callback ownership."""
import argparse,hashlib,json
from pathlib import Path
import unicorn
from unicorn import x86_const as reg
import oracle_mp_power_broadphase_1_1 as prior
from oracle_mp_power_broadphase_1_1 import BroadTrace,ROOT,OWNER,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
OUT=ROOT/'analysis/multiplayer/sandbox-source/power-pair-manager'
MANIFEST=prior.OUT/'final/manifest.json'
PIN='34ba3017efc8c81396bf420ca29a44f4c2daa7fa33a3409150c644bc678bf8ad'
ENTRIES={0x378eb0,0x2e5f50,0x314020,0x3140f0,0x314180,0x31d360,0x31d980,0x379140,
 0x313ef0,0x313f80,0x3131c0,0x31d210,0x3136b0,0x2df780,0x2df710,
 0x148a10,0x22e950,0x148a70,0x22eb20,0x31f770,0x31f6c0,
 0x322590,0x322060,0x3220d0,0x312320,0x312220,0x300170,0x2efb60,
 0x31c430,0x31c4b0,0x312010,0x311f80,0x311490,0x3114f0,
 0x27da90,0x27e4a0,0x1d2c70,0xa0120,0xa47b0,0x9d900,0x9b560}
def verify():
 prior.verify();assert sha(MANIFEST)==PIN
 m=json.loads(MANIFEST.read_text())
 for p,h in m['files'].items():assert sha(ROOT/p)==h,p
 return len(m['files'])
class PairTrace(BroadTrace):
 def __init__(self,data):
  self.pair_pending=[];self.pair_calls=[];self.pair_reads=[];self.pair_stores=[]
  self.pair_states=[];self.pair_ids={};self.pair_order=0;self.pair_executed=set()
  super().__init__(data);self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.pair_read)
 def pair_state(self):
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
  if h not in self.pair_ids:
   self.pair_ids[h]=len(self.pair_states);self.pair_states.append(dict(id=len(self.pair_states),sha256=h,**r))
  return self.pair_ids[h]
 def pair_return(self,a):
  matches=[x for x in self.pair_pending if x[0]==a]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,c in reversed(matches):
   if entry_sp!=sp:continue
   c.update(return_order=self.pair_order,returned_eax=f'{self.g.reg_read(reg.UC_X86_REG_EAX):08x}',
    return_esp_delta=self.g.reg_read(reg.UC_X86_REG_ESP)-sp,after_state=self.pair_state())
   self.pair_order+=1
   for q in c['packets']:q['after_hex']=bytes(self.g.mem_read(int(q['pointer'],16),q['size'])).hex()
   self.pair_pending.remove((ret,entry_sp,c))
 def hook(self,g,a,size,user):
  self.pair_return(a);sp=g.reg_read(reg.UC_X86_REG_ESP)
  root=self.slot_ready and a in(0x378eb0,0x31d980)
  if self.pair_pending or root:
   self.pair_executed.add(a)
   if a==0x31d9a7:
    c=next(c for _,_,c in reversed(self.pair_pending)if c['entry']=='0031d980')
    c['packets'].append(dict(name='island_frame_34',pointer=f'{sp:08x}',size=0x34,before_hex=bytes(g.mem_read(sp,0x34)).hex(),captured_at=f'{a:08x}'))
   if a in ENTRIES:
    ecx=g.reg_read(reg.UC_X86_REG_ECX)
    c=dict(id=len(self.pair_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
     order=self.pair_order,registers=self.registers(),entry_esp=f'{sp:08x}',
     stack16=[f'{self.u(sp+4+n*4):08x}'for n in range(16)],before_state=self.pair_state(),
     x87_control_word=f'{g.reg_read(reg.UC_X86_REG_FPCW):04x}',mxcsr=f'{g.reg_read(reg.UC_X86_REG_MXCSR):08x}',packets=[])
    self.pair_order+=1
    def packet(name,p,n):c['packets'].append(dict(name=name,pointer=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex()))
    arg=lambda i:self.u(sp+4+i*4)
    if a==0x378eb0:
     packet('manager_110',ecx,0x110)
     if arg(1):packet('added_pairs',arg(0),arg(1)*8)
     if arg(3):packet('removed_pairs',arg(2),arg(3)*8)
    if a==0x2e5f50:
     packet('filter_94',ecx,0x94);packet('admission_byte',arg(0),1)
     packet('collidable_a_2c',arg(1),0x2c);packet('collidable_b_2c',arg(2),0x2c)
    if a==0x314020:packet('static_list_14',ecx,0x14);packet('pair_8',arg(0),8)
    if a==0x31d360:packet('island_b0',ecx-0x1c,0xb0);packet('pair_8',arg(0),8)
    if a==0x31d980:
     packet('island_b0',ecx,0xb0)
     if arg(1):packet('added_pairs',arg(0),arg(1)*8)
     if arg(3):packet('removed_pairs',arg(2),arg(3)*8)
    if a in(0x313ef0,0x313f80):packet('scratch_descriptor_14',ecx,0x14)
    if a==0x31f6c0:packet('contact_manager_120',ecx,0x120)
    if a==0x322590:packet('constraint_110',ecx,0x110)
    if a in(0x322060,0x3220d0):packet('constraint_110',ecx,0x110);packet('body_a0',arg(0),0xa0)
    if a in(0x300170,0x2efb60)and a==0x2efb60:packet('agent_40',ecx,0x40)
    if a==0x27da90:packet('admission_byte',arg(0),1)
    self.pair_calls.append(c);self.pair_pending.append((self.u(sp),sp,c))
  super().hook(g,a,size,user)
 def pair_read(self,g,access,address,size,value,user):
  if not self.pair_pending:return
  self.pair_reads.append(dict(call=self.pair_pending[-1][2]['id'],order=self.pair_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,raw_hex=bytes(g.mem_read(address,size)).hex()));self.pair_order+=1
 def mem_write(self,g,access,address,size,value,user):
  if self.pair_pending:
   self.pair_stores.append(dict(call=self.pair_pending[-1][2]['id'],order=self.pair_order,instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex(),source_stack=STACK-0x100000<=address<STACK+0x100000));self.pair_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');a=ap.parse_args();count=verify()
 power=prior.prior.prior.prior.prior.prior.prior.prior.power;data=json.loads((OWNER/'raw-host-context.json').read_text());t=power.player.prepare(data,trace_type=PairTrace)
 t.selected_physics_fixup(0xe6f80583);t.bridge_enabled=True;t.slot_ready=True
 factory=t.factory(0xe6f80583);old=json.loads((OWNER/'report.json').read_text())['case'];keys=set(factory)-{'calls','ordered_stores'};assert {k:factory[k]for k in keys}=={k:old[k]for k in keys}
 def close():
  for method in(t.bridge_return,t.factory_return,t.slot_return,t.prepare_return,t.group_return,t.dispose_return,t.world_return,t.broad_return,t.pair_return):method(STOP)
 close();t.sandbox_enabled=False;assert power.player.spawn_and_activate(t,0,activate=False);t.power_observe=True;expected=json.loads((power.OUT/'frozen/measured/report.json').read_text())
 for s in expected['stages']:
  assert t.phase(s['name'],int(s['entry'],16),tuple(int(v,16)for v in s['arguments']));close()
 assert not t.pair_pending,[(hex(ret),hex(sp),c['entry'])for ret,sp,c in t.pair_pending]
 old=json.loads((MANIFEST.parent/'measured/report.json').read_text())
 for name,value in(('calls',t.broad_calls),('reads',t.broad_reads),('stores',t.broad_stores),('states',t.broad_states)):assert value==old[name],name
 for name,value in(('stages',t.power_stages),('calls',t.power_calls),('stores',t.power_stores),('callbacks',t.power_callbacks),('states',t.power_states),('actual_broadphase_calls',t.power_broadphase)):assert value==expected[name],name
 a.output.parent.mkdir(parents=True,exist_ok=True);listings=[]
 for entry in sorted(ENTRIES):
  f=_trace(t.code,entry,t.starts|ENTRIES|prior.ENTRIES);ins=[t.code.at(p)for p in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=a.output.parent/f'{entry:08x}.asm';frozen_write(p,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=sha(p),rows=len(ins),executed_rows=sum(i.address in t.pair_executed for i in ins)))
 result=dict(schema=1,oracle_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_verified_files=count,entire_prior_broadphase_and_power_schedule_identical=True,calls=t.pair_calls,reads=t.pair_reads,stores=t.pair_stores,states=t.pair_states,instruction_listings=listings,new_source_fields_or_results_supplied=[],instruction_patches=[],boundaries=old['boundaries'])
 verify();frozen_write(a.output,encoded(result));print(json.dumps(dict(report=sha(a.output),calls=len(t.pair_calls),states=len(t.pair_states),reads=len(t.pair_reads),stores=len(t.pair_stores))))
if __name__=='__main__':main()
