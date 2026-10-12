#!/usr/bin/env python3
"""Separate original texture constructor/reset before required canister decal.

Only genuine shared Linux raw allocations and the already independently pinned
compatibility access-conversion/apply body are platform adaptations. No texture,
IO, graphics, game admission or particle result is supplied.
"""
import argparse,ctypes,errno,json,os,platform
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_canister_vertex_cache_20261010 as vertex
from oracle_mp_item_current_producers_1_1 import InventoryTrace
from functions import _trace
parent=vertex.parent;ROOT=vertex.ROOT
SCOPE=ROOT/'analysis/multiplayer/integration/canister-texture-cache-20261010'
PROTECTION=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-protection'
PINS={**vertex.PINS,
 'analysis/multiplayer/integration/canister-vertex-cache-20261010/frozen/manifest.json':'bc98f424d7a5254814ea38cdeb7e9bf635956b52208dcc3470ba084efd90051b',
 'tools/oracle_canister_vertex_cache_20261010.py':parent.sha(ROOT/'tools/oracle_canister_vertex_cache_20261010.py'),
 str((PROTECTION/'compiled-compatibility-access.cpp').relative_to(ROOT)):'7658c40ade81ad9513652926c561fd2a9f0b4526cc87764742f266501afd48a4',
 str((PROTECTION/'compiled-compatibility-access.so').relative_to(ROOT)):'61b354c312ced6fa882eeb1371715eafc82328f5e780017c1688e29509741e23',
 str((PROTECTION/'independent-freeze-verification.json').relative_to(ROOT)):'72e4ad33ca7aa0ce9017ddc2fdb67763b32d92285493b7381931c23dc50e573b'}
ENTRIES=vertex.NEW_ENTRIES|{0x11e150,0x12d840,0x12d950,0x13f020,0x12e570,0x12e0f0,0x12ea60,0x12ee40,0x19d8c0,0x1a4870,0x121fd0,0x2d7a1a}
class Storage(vertex.AddressSpace):
 def allocate(self,g,requested,size,zerobits,kind,protect):
  assert (requested,zerobits,kind,protect)==(0,0,0x101000,4) and size in(12,0x508c,0x6a8,0x30fc)
  rounded=(size+self.page-1)&~(self.page-1);failures=[];skipped=[]
  guest_regions=[dict(first=f'{a:08x}',last=f'{b:08x}',permissions=p)for a,b,p in g.mem_regions()]
  address=(0x7ffeffff+1-rounded)&~0xffff
  while address>=0x10000:
   if any(a<address+rounded and b>=address for a,b,p in g.mem_regions()):skipped.append(f'{address:08x}');address-=0x10000;continue
   ctypes.set_errno(0);actual=self.lib.mmap(address,rounded,3,0x100022,-1,0);error=ctypes.get_errno()
   if actual==ctypes.c_void_p(-1).value:
    failures.append(dict(requested_address=f'{address:08x}',errno=error))
    if error!=errno.EEXIST:raise OSError(error,os.strerror(error))
    address-=0x10000;continue
   if actual!=address:
    assert self.lib.munmap(actual,rounded)==0
    raise RuntimeError('Exact MAP_FIXED_NOREPLACE address not returned; no Source outcome')
   try:g.mem_map_ptr(address,rounded,3,actual)
   except Exception:
    assert self.lib.munmap(actual,rounded)==0;raise
   raw=ctypes.string_at(actual,rounded);assert raw==bytes(rounded)==bytes(g.mem_read(address,rounded))
   maps=[line for line in Path('/proc/self/maps').read_text().splitlines()if int(line.split('-')[0],16)<=address<int(line.split()[0].split('-')[1],16)]
   assert len(maps)==1 and maps[0].split()[1].startswith('rw-')
   row=dict(requested_base=f'{requested:08x}',requested_size=size,zero_bits=zerobits,allocation_type=f'{kind:08x}',protection=f'{protect:08x}',rounded_size=rounded,actual_mmap_address=f'{actual:08x}',guest_address=f'{address:08x}',linux_mmap_flags='00100022',linux_protection=3,guest_protection=3,allocation_failures=failures,guest_regions_before=guest_regions,skipped_occupied_guest_candidates=skipped,initial_full_hex=raw.hex(),host_maps=maps,source_abi_status='00000000',scope='Actual shared Linux mmap translated only at original NT184 raw allocation boundary; no whole Xbox VM/PTE/physical allocator')
   self.regions.append((address,rounded));self.calls.append(row);return address,rounded,row
  raise MemoryError('No genuine backing; no Source status supplied')
class TextureTrace(vertex.VertexTrace):
 def __init__(self,*a,**kw):
  self.texture_enabled=False;self.access_calls=[]
  super().__init__(*a,**kw);self.platform=Storage()
  self.access=ctypes.CDLL(str(PROTECTION/'compiled-compatibility-access.so'))
  self.cb_type=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32))
  self.access.contract.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];self.access.contract.restype=ctypes.c_int
  self.access.apply.argtypes=[ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,self.cb_type];self.access.apply.restype=ctypes.c_int
 def contexts(self):
  r=super().contexts();r['texture_globals70_hex']=bytes(self.g.mem_read(0x4ed9f0,0x70)).hex()
  for address,size in ((0x4eda24,0x508c),(0x4eda28,0x6a8),(0x4eda34,0x30fc)):
   p=self.u(address);r[f'{address:08x}']=dict(pointer=f'{p:08x}',source_allocation_extent=size,raw_hex=bytes(self.g.mem_read(p,size)).hex()if p else None)
  return r
 def hook(self,g,address,size,user):
  if self.texture_enabled and address==0x33785c:
   sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
   assert args[0]==0 and args[2:]==[0x101000,4] and args[1]in(12,0x508c,0x6a8,0x30fc)
   if args[1]!=12:
    self.effect_executed.add(address);self.effect_returns(address)
    row=dict(id=len(self.effect_calls),entry=f'{address:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.effect_order,registers=self.registers(),stack_words=[f'{self.u(sp+4+i*4):08x}'for i in range(12)])
    self.effect_order+=1;self.effect_calls.append(row);self.effect_pending.append((self.u(sp),sp,row))
    InventoryTrace.hook(self,g,address,size,user);return
  if self.texture_enabled and address==0x800000b6:
   sp=g.reg_read(UC_X86_REG_ESP);base,n,p=(self.u(sp+4+i*4)for i in range(3));ret=self.u(sp)
   assert (base,n,p,ret)==(0x80061000,0x2ce6000,0x404,0x12d9c3)
   assert self.u(0x417ba8)==address and base%4096==n%4096==0
   derived=(ctypes.c_uint32*4)();assert self.access.contract(p,derived)==1
   before=bytes(g.mem_read(base,n));regs=self.registers();regions=[dict(first=f'{a:08x}',last=f'{b:08x}',permissions=p)for a,b,p in g.mem_regions()if a<base+n and b>=base]
   applied=[];errors=[]
   def apply(a,extent,windows,old):
    try:
     assert (a,extent,windows)==(base,n,derived[3]) and windows==0x40
     assert {q['permissions']for q in regions}=={7}
     g.mem_protect(a,extent,7)
     actual=[dict(first=f'{lo:08x}',last=f'{hi:08x}',permissions=perm)for lo,hi,perm in g.mem_regions()if lo<a+extent and hi>=a]
     assert all(q['permissions']==7 for q in actual)
     old[0]=0x40;applied.append(actual);return 1
    except Exception as e:errors.append(str(e));return 0
   callback=self.cb_type(apply);actual=self.access.apply(base,n,p,callback)
   assert actual==1 and len(applied)==1 and not errors,(actual,errors)
   after=bytes(g.mem_read(base,n));assert before==after
   row=dict(phase=self.active,ordinal=182,caller=f'{ret:08x}',base=f'{base:08x}',size=n,protection=f'{p:08x}',compiled_compatibility_outputs=[f'{v:08x}'for v in derived],registers_before=regs,regions_before=regions,regions_after=applied[0],contents_before_sha256=parent.hashlib.sha256(before).hexdigest(),contents_after_sha256=parent.hashlib.sha256(after).hexdigest(),actual_mem_protect_completed=True,void_abi_no_status_supplied=True)
   g.reg_write(UC_X86_REG_ESP,sp+16);g.reg_write(UC_X86_REG_EIP,ret);assert regs==self.registers();row['registers_after']=self.registers();self.access_calls.append(row);return
  super().hook(g,address,size,user)
def prepare():
 prior=json.loads((parent.base.OWNER/'report.json').read_text());context=parent.base.OWNER/'raw-host-context.json'
 t=parent.base.player.prepare(json.loads(context.read_text()),trace_type=TextureTrace)
 t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583);keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:prior['case'][k]for k in keys}
 assert factory['calls']==prior['case']['calls'][:len(factory['calls'])] and factory['ordered_stores']==prior['case']['ordered_stores'][:len(factory['ordered_stores'])]
 assert t.selected_fixup==prior['selected_original_physics_postprocess']
 t.sandbox_enabled=False;assert parent.base.player.spawn_and_activate(t,0,activate=False);assert t.stage('original_source_player_PVS',0x13a380)
 removed={f'{k:08x}':list(v)for k,v in t.services.items()};t.services.clear()
 t.power_observe=t.canister_observe=t.vertex_enabled=t.texture_enabled=True
 parent.ENTRIES.update(ENTRIES)
 return t,removed
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();assert not args.output.exists()
 for n,h in PINS.items():assert parent.sha(ROOT/n)==h,n
 t,removed=prepare();stages=[]
 roots=[('source_texture_constructor',0x12d840,()),('source_texture_reset',0x12d950,()),('source_effect_pool_constructor',0x1775d0,()),('source_effect_map_reset',0x177690,()),('source_breakable_mask_constructor',0x1858e0,()),('source_breakable_mask_reset',0x185960,()),('source_complete_decal_constructor',0x17ec90,()),('source_complete_decal_map_reset',0x17ed50,()),('source_current_canister_owner',0xd7cd0,(0xe26f0000,parent.PLACEMENT+0x800)),('explicit_source_canister_death',0xd7e00,(0xe26f0000,parent.PLACEMENT+0x800,1,0))]
 roots +=[(f'source_damage_update_{i}',0xd73e0,(0xe26f0000,))for i in range(1,8)]+[(f'source_effect_update_{i}',0x1788b0,())for i in range(1,5)]
 try:
  for name,entry,arguments in roots:
   contexts=t.contexts();raw=t.platform.snapshot(t.g);pools=t.effect_pools();ok=t.phase(name,entry,arguments);t.effect_returns(parent.base.STOP)
   stages.append(dict(**t.power_stages[-1],contexts_before=contexts,contexts_after=t.contexts(),pools_before=pools,pools_after=t.effect_pools(),platform_before=raw,platform_after=t.platform.snapshot(t.g)))
   print(json.dumps(dict(stage=name,completed=ok,invalid=t.new_invalid,eip=t.power_stages[-1]['stopped_eip'])),flush=True)
   if not ok:break
  directory=args.output.parent/'instructions';directory.mkdir(parents=True);listings=[]
  for entry in sorted(parent.ENTRIES):
   f=_trace(t.code,entry,t.starts|parent.ENTRIES);lines=[]
   for address in sorted(f.code):
    i=t.code.at(address);assert i is not None and i.bytes==t.image.read(address,len(i.bytes))==bytes(t.g.mem_read(address,len(i.bytes)))
    lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   path=directory/f'{entry:08x}.asm';path.write_text(''.join(lines));listings.append(dict(entry=f'{entry:08x}',path=str(path.relative_to(args.output.parent)),rows=len(lines),sha256=parent.sha(path)))
  for label,addresses in [('executed',t.effect_executed),('stores',{int(q['instruction'],16)for q in t.effect_stores})]:
   lines=[]
   for address in sorted(addresses):
    i=t.code.at(address)
    if i is None:continue
    assert i.bytes==t.image.read(address,len(i.bytes))==bytes(t.g.mem_read(address,len(i.bytes)))
    lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   path=directory/(label+'.asm');path.write_text(''.join(lines));listings.append(dict(path=str(path.relative_to(args.output.parent)),rows=len(lines),sha256=parent.sha(path)))
  cleanup=t.platform.close(t.g)
  r=dict(schema='original-canister-texture-owner-real-platform-v1',tool_sha256=parent.sha(__file__),protected_inputs=PINS,factory_correlated=True,player_preparation=t.player_stages,removed_inherited_services=removed,platform_calls=t.platform.calls,platform_abis=t.platform_abis,platform_cleanup=cleanup,applied_access_calls=t.access_calls,stages=stages,calls=t.effect_calls,stores=t.effect_stores,world_states=t.power_states,instructions=listings,services=t.events,host=dict(uname=platform.uname()._asdict(),page_size=t.platform.page),boundaries=['Only named actual Linux shared mmap at NT184 and unchanged pinned compatibility access conversion plus actual Unicorn.mem_protect at NT182 are added platform adapters; no complete Xbox VM/PTE/cache policy or hardware parity.','All source-requested texture storage/metadata/capacity/validity changes are original instruction outputs. No texture/IO/GPU/game admission/completion result is supplied.','Actual recorded complete compatibility context, original bounded factory/player/resource/default-variant/clock/preparation boundaries remain; roots are not complete title/frame scheduling.','No required authored effect part/system/controller is discarded. Class8/collider/whole effect birth/lifetime/render remain separate until actually reached.'])
  args.output.write_text(parent.encoded(r));print(json.dumps(dict(report=str(args.output),sha256=parent.sha(args.output))),flush=True)
 finally:
  if t.platform.regions:t.platform.close(t.g)
if __name__=='__main__':main()
