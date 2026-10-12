#!/usr/bin/env python3
"""Original canister path with real host-address-space backing at NT184 only.

No whole Xbox VM/kernel/PTE equivalence is inferred. The12-byte source request
uses genuine Linux mmap storage shared directly by Unicorn.mem_map_ptr.
"""
import argparse,ctypes,errno,hashlib,json,os,platform
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_canister_particle_owner_20261010 as parent
from oracle_mp_item_current_producers_1_1 import InventoryTrace
from functions import _trace
ROOT=parent.ROOT
SCOPE=ROOT/'analysis/multiplayer/integration/canister-vertex-cache-20261010'
PINS={'analysis/multiplayer/integration/canister-particle-owner-20261010/frozen/manifest.json':'f96219d626e8f45824253973e836131aa833a9a963fbc524baffc8ab872f8539',
 'tools/oracle_canister_particle_owner_20261010.py':'0016dba95ecdbb6a2e49dc53b6d98940fe3aae09048d762610a92df01d3b20e1',
 'analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source/VMManager.cpp':'40a7ae0c8d00f198f353c017ff7a8024ea22e1b91e8b7fa4cffe6e9306f2220e',
 'analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source/CxbxKrnl.h':'7cebac11841c538365f6ee5b01144094f81bb12490a1b1d3c042b6311a8097f8',
 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h':'988e679c3e042bb9f0c3836a77d3aee24117e83683da9cf839ba2a84c7fe32fb',
 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h':'7223ea0a9fb7e31e804008d5b0b1b3167908ab9c16d43fc2c8a67375c0f58f88'}
NEW_ENTRIES={0x33785c,0x43880,0x13e8e0,0x17ed50,0x16d220,0x180980,0x12ed10,0x13eae0,0x12d840}
class AddressSpace:
 def __init__(self):
  self.lib=ctypes.CDLL(None,use_errno=True)
  self.lib.mmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long];self.lib.mmap.restype=ctypes.c_void_p
  self.lib.munmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t];self.lib.munmap.restype=ctypes.c_int
  self.page=os.sysconf('SC_PAGE_SIZE');assert self.page==4096
  self.regions=[];self.calls=[]
 def allocate(self,g,requested,size,zerobits,kind,protect):
  assert (requested,size,zerobits,kind,protect)==(0,12,0,0x101000,4)
  rounded=(size+self.page-1)&~(self.page-1);failures=[];skipped=[]
  guest_regions=[dict(first=f'{a:08x}',last=f'{b:08x}',permissions=p)for a,b,p in g.mem_regions()]
  # Explicit native adapter: top-down within the pinned compatibility user VA
  # domain, avoiding all real existing guest mappings. Linux enforces host
  # nonreplacement; the exact successful address must match the request.
  hi=0x7ffeffff;lo=0x10000
  address=(hi+1-rounded)&~0xffff
  while address>=lo:
   if any(a<address+rounded and b>=address for a,b,p in g.mem_regions()):skipped.append(f'{address:08x}');address-=0x10000;continue
   ctypes.set_errno(0);actual=self.lib.mmap(address,rounded,3,0x22|0x100000,-1,0);error=ctypes.get_errno()
   if actual==ctypes.c_void_p(-1).value:
    failures.append(dict(requested_address=f'{address:08x}',errno=error))
    if error!=errno.EEXIST:raise OSError(error,os.strerror(error))
    address-=0x10000;continue
   if actual!=address:
    assert self.lib.munmap(actual,rounded)==0
    raise RuntimeError('MAP_FIXED_NOREPLACE was not honored; no result is supplied')
   try:g.mem_map_ptr(address,rounded,unicorn.UC_PROT_READ|unicorn.UC_PROT_WRITE,actual)
   except Exception:
    assert self.lib.munmap(actual,rounded)==0;raise
   raw=ctypes.string_at(actual,rounded);assert raw==bytes(rounded)==bytes(g.mem_read(address,rounded))
   host_lines=[line for line in Path('/proc/self/maps').read_text().splitlines()if int(line.split('-')[0],16)<=address<int(line.split()[0].split('-')[1],16)]
   assert len(host_lines)==1 and host_lines[0].split()[1].startswith('rw-')
   row=dict(requested_base='00000000',requested_size=size,zero_bits=zerobits,allocation_type=f'{kind:08x}',protection=f'{protect:08x}',rounded_size=rounded,actual_mmap_address=f'{actual:08x}',guest_address=f'{address:08x}',linux_mmap_flags='00100022',linux_protection=3,guest_protection=3,allocation_failures=failures,guest_regions_before=guest_regions,skipped_occupied_guest_candidates=skipped,initial_full_hex=raw.hex(),host_maps=host_lines,source_abi_status='00000000',scope='Actual shared host mmap success translated to declared NT184 raw allocation ABI; no Xbox VM/PTE/physical-memory-manager execution')
   self.regions.append((address,rounded));self.calls.append(row);return address,rounded,row
  raise MemoryError('No unoccupied compatible32-bit host/guest address range; no Source result is supplied')
 def snapshot(self,g):
  result=[]
  for address,size in self.regions:
   raw=ctypes.string_at(address,size);assert raw==bytes(g.mem_read(address,size))
   result.append(dict(address=f'{address:08x}',size=size,raw_hex=raw.hex(),host_guest_bytes_identical=True))
  return result
 def close(self,g):
  result=[]
  for address,size in reversed(self.regions):
   g.mem_unmap(address,size);ctypes.set_errno(0);actual=self.lib.munmap(address,size)
   result.append(dict(address=f'{address:08x}',size=size,actual_munmap_result=actual,errno=ctypes.get_errno()));assert actual==0
  self.regions=[];return result
class VertexTrace(parent.CanisterTrace):
 def __init__(self,*args,**kwargs):
  self.vertex_enabled=False;self.platform=AddressSpace();self.platform_abis=[]
  super().__init__(*args,**kwargs)
 def contexts(self):
  result=super().contexts()
  result['source_arena10_hex']=bytes(self.g.mem_read(0x4ed650,16)).hex()
  result['source_vertex_globals8_hex']=bytes(self.g.mem_read(0x510c34,8)).hex()
  result['source_vertex_lookup1000_hex']=bytes(self.g.mem_read(0x4cfcb8,0x1000)).hex()
  for address,size in ((0x4f1f20,0x100cc),(0x510c38,0x613c)):
   p=self.u(address);result[f'{address:08x}']=dict(pointer=f'{p:08x}',source_allocation_extent=size,raw_hex=bytes(self.g.mem_read(p,size)).hex()if p else None)
  p=self.u(0x510c34)
  if p:
   full=(self.u(0x4ed650)-self.u(0x4ed658)+0x3fe000)&0xffffffff
   assert self.u(p+4)==(full&0xfffffff)
   result['source_vertex_tail']=dict(unmasked_source_address=f'{full:08x}',stored_gpu_offset=f'{self.u(p+4):08x}',source_allocation_extent=0x14000,raw_hex=bytes(self.g.mem_read(full,0x14000)).hex())
  result['texture_allocator_global_hex']=bytes(self.g.mem_read(0x4eda34,4)).hex()
  return result
 def hook(self,g,address,size,user):
  if self.vertex_enabled and address==0x33785c:
   sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
   assert args==[0,12,0x101000,4],args
   # Bypass ONLY the inherited Python12-byte unprovided-service stop and
   # older VirtualAlloc return observer. Actual unchanged wrapper instructions
   # execute, including the ordinal184 call and its signed return handling.
   self.effect_executed.add(address);self.effect_returns(address)
   row=dict(id=len(self.effect_calls),entry=f'{address:08x}',caller=f'{self.u(sp):08x}',phase=self.active,order=self.effect_order,registers=self.registers(),stack_words=[f'{self.u(sp+4+i*4):08x}'for i in range(12)])
   self.effect_order+=1;self.effect_calls.append(row);self.effect_pending.append((self.u(sp),sp,row))
   InventoryTrace.hook(self,g,address,size,user);return
  if self.vertex_enabled and address==0x800000b8:
   sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(5)]
   bp,zb,np,kind,protection=args;before=self.registers();requested=self.u(bp);n=self.u(np)
   assert self.u(0x417c34)==address
   base,rounded,row=self.platform.allocate(g,requested,n,zb,kind,protection)
   self.w(bp,base);self.w(np,rounded)
   abi=dict(phase=self.active,original_import='00417c34',ordinal=184,caller=f'{self.u(sp):08x}',stack14_hex=bytes(g.mem_read(sp+4,20)).hex(),registers_before=before,base_pointer=f'{bp:08x}',size_pointer=f'{np:08x}',base_before=f'{requested:08x}',size_before=n,base_after=f'{base:08x}',size_after=rounded,provider_call=len(self.platform.calls)-1)
   g.reg_write(UC_X86_REG_EAX,0);g.reg_write(UC_X86_REG_ESP,sp+24);g.reg_write(UC_X86_REG_EIP,self.u(sp));abi['registers_after']=self.registers();self.platform_abis.append(abi);return
  super().hook(g,address,size,user)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();assert not args.output.exists()
 inputs=PINS|json.loads((SCOPE/'primary/files.json').read_text())['files']
 for n,h in inputs.items():assert parent.sha(ROOT/n)==h,n
 prior=json.loads((parent.base.OWNER/'report.json').read_text());context=parent.base.OWNER/'raw-host-context.json'
 t=parent.base.player.prepare(json.loads(context.read_text()),trace_type=VertexTrace)
 t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583)
 keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:prior['case'][k]for k in keys}
 assert factory['calls']==prior['case']['calls'][:len(factory['calls'])] and factory['ordered_stores']==prior['case']['ordered_stores'][:len(factory['ordered_stores'])]
 assert t.selected_fixup==prior['selected_original_physics_postprocess']
 t.sandbox_enabled=False;assert parent.base.player.spawn_and_activate(t,0,activate=False);assert t.stage('original_source_player_PVS',0x13a380)
 removed={f'{k:08x}':list(v)for k,v in t.services.items()};t.services.clear()
 t.power_observe=True;t.canister_observe=True;t.vertex_enabled=True
 parent.ENTRIES.update(NEW_ENTRIES)
 roots=[('source_effect_pool_constructor',0x1775d0,()),('source_effect_map_reset',0x177690,()),('source_breakable_mask_constructor',0x1858e0,()),('source_breakable_mask_reset',0x185960,()),('source_complete_decal_constructor',0x17ec90,()),('source_complete_decal_map_reset',0x17ed50,()),('source_current_canister_owner',0xd7cd0,(0xe26f0000,parent.PLACEMENT+0x800)),('explicit_source_canister_death',0xd7e00,(0xe26f0000,parent.PLACEMENT+0x800,1,0))]
 roots +=[(f'source_damage_update_{i}',0xd73e0,(0xe26f0000,))for i in range(1,8)]
 roots +=[(f'source_effect_update_{i}',0x1788b0,())for i in range(1,5)]
 stages=[]
 try:
  for name,entry,arguments in roots:
   before=t.effect_pools();contexts=t.contexts();raw=t.platform.snapshot(t.g);ok=t.phase(name,entry,arguments);t.effect_returns(parent.base.STOP)
   stages.append(dict(**t.power_stages[-1],pools_before=before,pools_after=t.effect_pools(),contexts_before=contexts,contexts_after=t.contexts(),platform_before=raw,platform_after=t.platform.snapshot(t.g)))
   print(json.dumps(dict(stage=name,completed=ok,eip=t.power_stages[-1]['stopped_eip'],invalid=t.new_invalid,pools=[(q['global_address'],len(q.get('occupied',[])))for q in t.effect_pools()])),flush=True)
   if not ok:break
  directory=args.output.parent/'instructions';directory.mkdir(parents=True);listings=[]
  for entry in sorted(parent.ENTRIES):
   routine=_trace(t.code,entry,t.starts|parent.ENTRIES);lines=[]
   for address in sorted(routine.code):
    i=t.code.at(address);assert i is not None and i.bytes==t.image.read(i.address,len(i.bytes))==bytes(t.g.mem_read(i.address,len(i.bytes)))
    lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   path=directory/f'{entry:08x}.asm';path.write_text(''.join(lines));listings.append(dict(entry=f'{entry:08x}',path=str(path.relative_to(args.output.parent)),sha256=parent.sha(path),rows=len(lines)))
  for label,addresses in [('executed',t.effect_executed),('stores',{int(q['instruction'],16)for q in t.effect_stores})]:
   lines=[]
   for address in sorted(addresses):
    i=t.code.at(address)
    if i is None:continue
    assert i.bytes==t.image.read(i.address,len(i.bytes))==bytes(t.g.mem_read(i.address,len(i.bytes)))
    lines.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   path=directory/(label+'.asm');path.write_text(''.join(lines));listings.append(dict(path=str(path.relative_to(args.output.parent)),sha256=parent.sha(path),rows=len(lines)))
  cleanup=t.platform.close(t.g)
  report=dict(schema='original-canister-vertex-cache-host-storage-v1',tool_sha256=parent.sha(__file__),protected_inputs=inputs,recorded_context_sha256=parent.sha(context),factory_correlated=True,player_preparation=t.player_stages,removed_inherited_services=removed,platform_calls=t.platform.calls,platform_abis=t.platform_abis,platform_cleanup=cleanup,stages=stages,calls=t.effect_calls,stores=t.effect_stores,world_states=t.power_states,instructions=listings,services=t.events,host=dict(uname=platform.uname()._asdict(),page_size=t.platform.page),boundaries=['Unchanged source43880/33785C wrapper executes; only original NT184 raw allocation boundary uses actual directly shared Linux mmap storage.','Native top-down search/64KiB-alignment/4KiB-rounding and translated success0 are explicitly named platform adaptation; complete pinned Cxbx VMManager/PTE/physical-memory bodies do not execute.','All inherited return-only services are removed after unchanged factory/player preparation; no optional sound/effect success/failure is supplied in added roots.','No contact, pose, mask, effect/event/particle identity, phase, readiness or clock is supplied. Full class8/collider/world/rendering admission stays separate.'])
  args.output.write_text(parent.encoded(report));print(json.dumps(dict(report=str(args.output),sha256=parent.sha(args.output))),flush=True)
 finally:
  if t.platform.regions:t.platform.close(t.g)
if __name__=='__main__':main()
