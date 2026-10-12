#!/usr/bin/env python3
"""Continue the bounded original sword pickup through its actual looping pool.

Only the constructor's virtual raw-storage leaf and game-arena backing are
provided. Original18A200/16D070/16D210 initialize and bind the pool; original
16D2C0 allocates sound owners. Inherited equipped fixture, resource delivery,
physics and service boundaries remain explicit. Not live startup/audio proof.
"""
import hashlib,json
from pathlib import Path
from oracle_covenant_pickup_shader_state_1_1 import Trace,ROOT,PIN,CACHE_PIN
from oracle_mp_inventory_1_1 import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
BASE=0x9000000
class PoolTrace(Trace):
 def hook(self,g,a,size,user):
  if a==BASE+0xf000:
   sp=g.reg_read(UC_X86_REG_ESP);n=self.u(sp+4);assert n==0x60cc
   self.raw_storage.append(dict(kind='virtual_raw_storage',bytes=n,address=f'{BASE:08x}',allocator=f'{g.reg_read(UC_X86_REG_ECX):08x}'))
   g.mem_write(BASE,b'\xa5'*n);g.reg_write(UC_X86_REG_EAX,BASE)
   g.reg_write(UC_X86_REG_ESP,sp+8);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
  super().hook(g,a,size,user)

 def action(self,name,address,args=(),registers=()):
  if name=='original_authored_target_allocate' and not getattr(self,'looping_pool_prepared',False):
   self.looping_pool_prepared=True;self.g.mem_map(BASE,0x10000);self.raw_storage=[]
   self.prior_globals={f'{a:08x}':f'{self.u(a):08x}' for a in (0x51841c,0x4ed650,0x4ed654,0x4f485c)}
   self.w(BASE+0x7000,BASE+0x7010);self.w(BASE+0x7010,BASE+0xf000)
   self.w(0x51841c,BASE+0x7000);self.w(0x4ed650,BASE+0x8000);self.w(0x4ed654,0)
   self.active='original_looping_pool_constructor';self.run(0x18a200)
   assert self.u(0x4f485c)==BASE
   self.active='original_looping_pool_enable';self.run(0x16d210,regs=((UC_X86_REG_EAX,BASE),))
   self.pool_constructor_header=bytes(self.g.mem_read(BASE,0x4c)).hex()
  return super().action(name,address,args,registers)

def main():
 t=PoolTrace(2);before=t.shader_state();inputs=t.prior_globals
 pool_before=t.pool_constructor_header
 try:
  t.seeded_duration(2,232);t.poll('original_sword_233ms_pickup',{2:255},milliseconds=1)
  result=dict(completed=True,after=t.shader_state())
 except Exception as error:
  result=dict(completed=False,first_stop_instruction=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',error=str(error),after=t.shader_state())
 result.update(scope=__doc__,xbe_sha256=PIN,cache_sha256=CACHE_PIN,before=before,
   prior_globals=inputs,raw_storage=t.raw_storage,game_arena_raw_backing=f'{BASE+0x8000:08x}',
   pool_constructor_header=pool_before,pool_final_header=bytes(t.g.mem_read(BASE,0x4c)).hex(),
   pool_count=t.u(BASE+0x3c),pool_records=bytes(t.g.mem_read(t.u(BASE+0x44),t.u(BASE+0x38)*0x18)).hex(),
   used_services=sorted({e['service']for e in t.events if 'service' in e}),
   service_calls=[{k:v for k,v in e.items()if k!='state'} for e in t.events if e.get('service') in ('00107b50','0016a356','000c10e0')],
   ready_countdown=t.u(t.target_pointer+0x19e,2),blocking_countdown=t.u(t.target_pointer+0x17c,2),
   new_supplied_gameplay_results=[],instruction_patches=[],oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
 path=ROOT/'analysis/multiplayer/integration/sword-looping-pool-pickup-20261012.json'
 path.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
 print(json.dumps({k:v for k,v in result.items()if k in ('completed','before','after','first_stop_instruction','error','pool_count','raw_storage')}),flush=True)
if __name__=='__main__':main()
