#!/usr/bin/env python3
"""Actual canister death with genuine effect-pool constructors; retain next stop.

No effect/particle handle, position, event, clock, contact or service result is
assigned. The recorded original canister/world preparation remains explicit.
"""
import argparse,hashlib,json
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_mp_power_module_schedule_1_1 as base
from oracle_mp_spawn_biped_physics import PLACEMENT
from functions import _trace
ROOT=base.ROOT
ENTRIES={0x1775d0,0x175860,0x177690,0x179b90,0x179d10,0x177a10,
 0x17a030,0x17a460,0x178d80,0x17b2c0,0x17ad20,0x17b1a0,0x17bdf0,
 0x17a920,0x17aa90,0x17a6f0,0x175a50,0x17ac00,0x175ee0,
 0x247470,0x247950,0x249540,0x249a20,0x1788b0,0x1789d0,0x176410,
 0x1764b0,0x24a820,0x249210,0x2c06c0,0x2bf0c0,0x2bf130,0x2bf380,
 0x18aa90,0x18ad70,0x17c8c0,0x17c930,0x17c840,0x11d650,0x179080,0x1858e0,0x185960,0x17ec90,0x17ed50,0x43880,0x13e8e0,0x180980}
GLOBALS=(0x4f1f0c,0x4f1f08,0x518464,0x526514,0x526518,0x52651c,0x4f1f20)
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(x):return json.dumps(x,sort_keys=True,separators=(',',':'))+'\n'
class CanisterTrace(base.PowerTrace):
 def __init__(self,*a,**kw):
  self.canister_observe=False;self.effect_calls=[];self.effect_pending=[];self.effect_stores=[];self.effect_executed=set();self.effect_order=0
  super().__init__(*a,**kw)
 def effect_pools(self):
  rows=[]
  for address in GLOBALS:
   p=self.u(address);row=dict(global_address=f'{address:08x}',pointer=f'{p:08x}')
   if p:
    capacity,stride,data,bits=(self.u(p+n)for n in(0x20,0x24,0x44,0x48));assert capacity<=8192 and stride<=0x200
    row.update(complete_storage_hex=bytes(self.g.mem_read(data,capacity*stride)).hex(),header4c_hex=bytes(self.g.mem_read(p,0x4c)).hex(),capacity=capacity,stride=stride,data_pointer=f'{data:08x}',bitmap_hex=bytes(self.g.mem_read(bits,(capacity+31)//32*4)).hex(),occupied=[])
    for i in range(self.u(p+0x38)):
     if self.u(bits+(i>>5)*4)&(1<<(i&31)):
      q=data+i*stride;row['occupied'].append(dict(index=i,datum=f'{self.u(q,2)<<16|i:08x}',pointer=f'{q:08x}',raw_hex=bytes(self.g.mem_read(q,stride)).hex()))
   rows.append(row)
  return rows
 def contexts(self):
  result={}
  for address,size in ((0x4f4850,0x4204),(0x4f1f1c,0x3c2c)):
   p=self.u(address);result[f'{address:08x}']=dict(pointer=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,size)).hex()if p else None)
  game=self.u(0x4edf18);result['source_PVS40_hex']=bytes(self.g.mem_read(game+0x1138,64)).hex()
  return result
 def effect_returns(self,address):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  for ret,initial_sp,row in self.effect_pending[:]:
   if ret==address and sp>initial_sp:
    row.update(return_order=self.effect_order,return_cpu=self.registers(),return_esp_delta=sp-initial_sp);self.effect_order+=1
    self.effect_pending.remove((ret,initial_sp,row))
 def hook(self,g,address,size,user):
  if self.canister_observe:
   self.effect_executed.add(address);self.effect_returns(address)
   if address in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP)
    row=dict(id=len(self.effect_calls),entry=f'{address:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.effect_order,registers=self.registers(),stack_words=[f'{self.u(sp+4+i*4):08x}'for i in range(12)])
    if address==0x177a10:
     p=self.u(sp+4);row['creation60_hex']=bytes(g.mem_read(p,0x60)).hex()
    if address==0x11d650:
     p=g.reg_read(UC_X86_REG_EAX);game=self.u(0x4edf18);row.update(location6_hex=bytes(g.mem_read(p,6)).hex(),source_PVS40_hex=bytes(g.mem_read(game+0x1138,64)).hex())
    self.effect_order+=1;self.effect_calls.append(row);self.effect_pending.append((self.u(sp),sp,row))
  if self.canister_observe and address==0x33785c:
   sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
   if args==[0,12,0x101000,4]:
    self.new_invalid=dict(boundary='unprovided VirtualAlloc request; no platform result or raw backing is supplied',instruction=f'{address:08x}',caller=f'{self.u(sp):08x}',arguments=[f'{v:08x}'for v in args],registers=self.registers())
    raise base.KernelBoundary()
  super().hook(g,address,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.canister_observe and not base.STACK-0x100000<=address<base.STACK+0x100000:
   self.effect_stores.append(dict(order=self.effect_order,phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()));self.effect_order+=1
  super().mem_write(g,access,address,size,value,user)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--with-player',action='store_true');p.add_argument('--with-mask-owner',action='store_true');p.add_argument('--with-decal-owner',action='store_true');args=p.parse_args();assert not args.output.exists()
 pins=base.FROZEN|{
  'tools/oracle_mp_power_module_schedule_1_1.py':sha(ROOT/'tools/oracle_mp_power_module_schedule_1_1.py'),
  'analysis/multiplayer/sandbox-source/live-prop/death-measured/report.json':sha(ROOT/'analysis/multiplayer/sandbox-source/live-prop/death-measured/report.json'),
  'analysis/multiplayer/integration/particle-physics-movement-20261010/frozen/manifest.json':'11b5eed3caf4a066ec9d691a5df2c1b9bb2b02fdf3b81a012dd72a9c4b2b84f7'}
 for name,h in pins.items():assert sha(ROOT/name)==h,name
 prior=json.loads((base.OWNER/'report.json').read_text());context=base.OWNER/'raw-host-context.json'
 t=base.player.prepare(json.loads(context.read_text()),trace_type=CanisterTrace)
 t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583)
 assert factory['completed'] and factory['returned_eax']=='e26f0000'
 keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:prior['case'][k]for k in keys}
 assert factory['calls']==prior['case']['calls'][:len(factory['calls'])]
 assert factory['ordered_stores']==prior['case']['ordered_stores'][:len(factory['ordered_stores'])]
 assert t.selected_fixup==prior['selected_original_physics_postprocess']
 t.sandbox_enabled=False
 if args.with_player:
  assert base.player.spawn_and_activate(t,0,activate=False)
  assert t.stage('original_current_player_PVS',0x13a380)
 t.power_observe=True;t.canister_observe=True
 before_pools=t.effect_pools();stages=[]
 raw_callback34=t.image.read(0x447428+34*36,36)
 assert int.from_bytes(raw_callback34[:4],'little')==0x1858e0 and int.from_bytes(raw_callback34[8:12],'little')==0x185960
 roots=[('source_effect_pool_constructor',0x1775d0,()),('source_effect_map_reset',0x177690,()),
        ('original_current_canister_owner',0xd7cd0,(0xe26f0000,PLACEMENT+0x800)),
        ('explicit_original_canister_death',0xd7e00,(0xe26f0000,PLACEMENT+0x800,1,0))]
 if args.with_mask_owner:
  roots[2:2]=[('source_breakable_mask_storage_constructor',0x1858e0,()),('source_breakable_mask_map_reset',0x185960,())]
 if args.with_decal_owner:
  assert args.with_mask_owner and args.with_player
  roots[4:4]=[('source_decal_storage_constructor_to_platform_boundary',0x17ec90,())]
 roots +=[(f'original_damage_update_{i}',0xd73e0,(0xe26f0000,))for i in range(1,8)]
 roots +=[(f'original_fixed_effect_update_{i}',0x1788b0,())for i in range(1,4)]
 for name,entry,arguments in roots:
  before=t.effect_pools();before_contexts=t.contexts();ok=t.phase(name,entry,arguments);t.effect_returns(base.STOP)
  stages.append(dict(**t.power_stages[-1],effect_pools_before=before,effect_pools_after=t.effect_pools(),contexts_before=before_contexts,contexts_after=t.contexts(),pending_original_effect_calls=[q['id']for _,_,q in t.effect_pending]))
  print(json.dumps(dict(stage=name,completed=ok,stop=t.power_stages[-1]['stopped_eip'],invalid=t.new_invalid,pools=[(q['global_address'],len(q.get('occupied',[])))for q in t.effect_pools()])),flush=True)
  if not ok:break
 directory=args.output.parent/'instructions';directory.mkdir(parents=True)
 listings=[]
 for entry in sorted(ENTRIES):
  routine=_trace(t.code,entry,t.starts|ENTRIES);lines=[]
  for address in sorted(routine.code):
   i=t.code.at(address);assert i is not None and i.bytes==t.image.read(i.address,len(i.bytes))==bytes(t.g.mem_read(i.address,len(i.bytes)))
   lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
  path=directory/f'{entry:08x}.asm';path.write_text(''.join(lines));listings.append(dict(entry=f'{entry:08x}',path=str(path.relative_to(args.output.parent)),sha256=sha(path),rows=len(lines),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 for label,addresses in [('executed',t.effect_executed),('stores',{int(q['instruction'],16)for q in t.effect_stores})]:
  lines=[]
  for address in sorted(addresses):
   i=t.code.at(address)
   if i is None:continue
   assert i.bytes==t.image.read(i.address,len(i.bytes))==bytes(t.g.mem_read(i.address,len(i.bytes)))
   lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
  path=directory/(label+'.asm');path.write_text(''.join(lines));listings.append(dict(path=str(path.relative_to(args.output.parent)),sha256=sha(path),rows=len(lines)))
 report=dict(schema='original-canister-effect-owner-v1',tool_sha256=sha(__file__),source_xbe_sha256=base.PIN,cache_sha256=base.CACHE_PIN,protected_inputs=pins,recorded_context_sha256=sha(context),factory_correlates_byte_identically=True,
  source_player_PVS_composed=args.with_player,source_breakable_mask_owner_composed=args.with_mask_owner,source_decal_constructor_composed=args.with_decal_owner,callback34_raw24_hex=raw_callback34.hex(),player_preparation=t.player_stages,before_effect_pools=before_pools,stages=stages,calls=t.effect_calls,stores=t.effect_stores,world_states=t.power_states,final_effect_pools=t.effect_pools(),instructions=listings,services=t.events,
  boundaries=['Actual retained power_module factory/state/ABI/call/store prefix repeats unchanged before added original effect-pool constructor/reset roots.',
  'D7E00 remains an explicit diagnostic death request. D73E0 and1788B0 are bounded separate original roots; existing options,30Hz clock, recorded complete compatibility context and resource/backing boundaries remain.',
  'No new source result, raw pool/header, effect/particle identity, event/phase, source clock/tick, position, contact or readiness field is supplied.',
  'Selected additional callback roots are an explicit bounded composition after the preserved factory, not the complete authored 68-row initializer schedule. Original callback34 allocation requests4204 bytes and reset writes4201FF; callback37 constructor is never resumed after its platform stop.',
  'Original first unresolved dependency is retained; no new failed/successful audio, decal/light, bloc, collider or graphics service replaces it. No native whole effect or full title/frame admission claim.'])
 args.output.write_text(encoded(report));print(json.dumps(dict(report=str(args.output),sha256=sha(args.output),calls=len(t.effect_calls),stores=len(t.effect_stores))),flush=True)
if __name__=='__main__':main()
