#!/usr/bin/env python3
"""Original canister texture request with genuine retained native queue objects.

The XAPI creation/release adapters reuse unchanged frozen NativeSynchronization.
Original wait wrappers execute through ordinal234; acquired native ownership is
translated to the pinned compatibility STATUS_SUCCESS only after acquisition.
No worker creation, file token, read result, GPU completion or particle is given.
"""
import argparse,ctypes,json,platform
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
import oracle_canister_texture_cache_20261010 as prior
from functions import _trace
ROOT=prior.ROOT;parent=prior.parent;SCOPE=ROOT/'analysis/multiplayer/integration/canister-async-queue-20261010'
NATIVE=ROOT/'analysis/multiplayer/integration/native-worker-threads-20261009/frozen'
QUEUE=0x4e7924;QUEUE_SIZE=0x25a0
PINS={**prior.PINS,
 'analysis/multiplayer/integration/canister-texture-cache-20261010/frozen/manifest.json':'22e60be0dbfe8fb3acac7f5f737208ef3f5208ea1dc5b50f015879dd217ccb3b',
 'analysis/multiplayer/integration/native-worker-threads-20261009/frozen/manifest.json':'64040b6f32814cb750c96ff5250dd43671a8848c35defd90321f3b7bc5ef010f',
 'analysis/multiplayer/integration/sound-promotion-production-20261009/frozen/manifest.json':'a7ca3a07efe83429451090080344d26e247eec442c35516ed7192a530bd39df1',
 'tools/oracle_canister_texture_cache_20261010.py':parent.sha(ROOT/'tools/oracle_canister_texture_cache_20261010.py'),
 'tools/oracle_sound_cache_request_native.py':parent.sha(ROOT/'tools/oracle_sound_cache_request_native.py'),
}
for n in('include/halo2/native_synchronization.hpp','src/native_synchronization.cpp'):
 PINS[str((NATIVE/'source'/n).relative_to(ROOT))]=parent.sha(NATIVE/'source'/n)
for n in('provider.cpp','provider.so'):
 PINS[str((SCOPE/'native'/n).relative_to(ROOT))]=parent.sha(SCOPE/'native'/n)
ENTRIES=prior.ENTRIES|{0x1221b0,0x122350,0x122360,0x121fd0,0x122020,0x1220e0,0x122150,0x1a5080,0x1a47d0,0x1a4250,0x19d820,0x19dad0,0x19ee30,0x2dc83a,0x2dc7b9,0x2d8606,0x2d83f6,0x2dc9c8,0x2dc8b2,0x2dc70a,0x2dc892,0x2dc814,0x2d84c4,0x2d88c8,0x2d8681}
class AsyncTrace(prior.TextureTrace):
 def __init__(self,*args,**kwargs):
  self.async_enabled=False;self.native_calls=[];self.async_stops=[];self.wait_pending=[]
  self.native=ctypes.CDLL(str(SCOPE/'native/provider.so'))
  signatures={'create_provider':([],ctypes.c_void_p),'destroy_provider':([ctypes.c_void_p],None),
   'create_mutex':([ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_int)],ctypes.c_uint32),
   'create_semaphore':([ctypes.c_void_p,ctypes.c_int32,ctypes.c_int32,ctypes.POINTER(ctypes.c_int)],ctypes.c_uint32),
   'native_wait':([ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.POINTER(ctypes.c_int)],ctypes.c_int),
   'release_mutex':([ctypes.c_void_p,ctypes.c_uint32],ctypes.c_int),
   'release_semaphore':([ctypes.c_void_p,ctypes.c_uint32,ctypes.c_int32,ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_int)],ctypes.c_int),
   'object_count':([ctypes.c_void_p],ctypes.c_uint32)}
  for name,(arguments,result)in signatures.items():
   f=getattr(self.native,name);f.argtypes=arguments;f.restype=result
  self.native_owner=self.native.create_provider();assert self.native_owner
  super().__init__(*args,**kwargs)
 def contexts(self):
  r=super().contexts();r['async_queue_raw_hex']=bytes(self.g.mem_read(QUEUE,QUEUE_SIZE)).hex()
  r['async_descriptor_hex']=bytes(self.g.mem_read(0x5098a0,12)).hex()
  r['current_node_hex']=bytes(self.g.mem_read(0x518000,4)).hex()
  r['actual_file_bank_hex']=bytes(self.g.mem_read(0x55f798,0x3860)).hex()
  r['native_object_count']=self.native.object_count(self.native_owner)
  return r
 def stop(self,address,reason,**fields):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  self.new_invalid=dict(boundary=reason,instruction=f'{address:08x}',caller=f'{self.u(sp):08x}',registers=self.registers(),**fields)
  self.async_stops.append(dict(phase=self.active,**self.new_invalid));raise parent.base.KernelBoundary()
 def native_return(self,pop,result=None):
  sp=self.g.reg_read(UC_X86_REG_ESP);ret=self.u(sp)
  if result is not None:self.g.reg_write(UC_X86_REG_EAX,result)
  self.g.reg_write(UC_X86_REG_ESP,sp+4+pop);self.g.reg_write(UC_X86_REG_EIP,ret)
 def hook(self,g,address,size,user):
  if self.async_enabled:
   for ret,row in self.wait_pending[:]:
    if address==ret:
     row['wrapper_return_cpu']=self.registers();self.wait_pending.remove((ret,row))
   sp=g.reg_read(UC_X86_REG_ESP)
   if address in(0x2dc83a,0x2dc7b9):
    count=3 if address==0x2dc83a else 4;args=[self.u(sp+4+i*4)for i in range(count)]
    assert args==([0,0,0]if count==3 else[0,0,150,0]),args
    error=ctypes.c_int()
    handle=self.native.create_mutex(self.native_owner,args[1],ctypes.byref(error))if count==3 else self.native.create_semaphore(self.native_owner,args[1],args[2],ctypes.byref(error))
    row=dict(phase=self.active,boundary='native XAPI creation adapter',source_entry=f'{address:08x}',caller=f'{self.u(sp):08x}',arguments=[f'{a:08x}'for a in args],native_handle=f'{handle:08x}',native_error=error.value,registers_before=self.registers())
    self.native_calls.append(row)
    if error.value or not handle:self.stop(address,'Actual native creation failed; no Xbox status or error mapping supplied',native_error=error.value)
    self.native_return(count*4,handle);row['registers_after']=self.registers();return
   if address==0x2d8606:
    self.stop(address,'Original worker/thread/TLS/lifetime producer unavailable; no creation result supplied',arguments=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
   if address==0x800000ea:
    args=[self.u(sp+4+i*4)for i in range(4)];assert args[1:]==[1,0,0] and self.u(0x417c98)==address,args
    handle=args[0];assert handle in(self.u(0x4e7928),self.u(0x4e792c),self.u(0x4e7930))
    error=ctypes.c_int();actual=self.native.native_wait(self.native_owner,handle,0,ctypes.byref(error))
    row=dict(phase=self.active,boundary='native ownership translated at original NtWaitForSingleObjectEx ordinal234',source_entry=f'{address:08x}',caller=f'{self.u(sp):08x}',arguments=[f'{a:08x}'for a in args],native_wait_milliseconds=0,requested_source_timeout='infinite_NULL',actual_wait_state=actual,native_error=error.value,registers_before=self.registers())
    self.native_calls.append(row)
    if actual!=0 or error.value:self.stop(address,'Actual native wait did not acquire; no Source return or post-wait stores supplied',actual_wait_state=actual,native_error=error.value)
    # Pinned nxdk NtWaitForSingleObjectEx contract + Cxbx X_STATUS_SUCCESS=0.
    # Only real acquired ownership is translated; other outcomes stop above.
    self.native_return(16,0);row.update(translated_status='00000000',registers_after=self.registers());return
   if address==0x2dc9c8:
    row=dict(phase=self.active,entry=f'{address:08x}',caller=f'{self.u(sp):08x}',arguments=[f'{self.u(sp+4):08x}',f'{self.u(sp+8):08x}'],wrapper_entry_cpu=self.registers())
    self.wait_pending.append((self.u(sp),row));self.native_calls.append(row)
   if address==0x2dc892:
    handle=self.u(sp+4);before=self.registers();actual=self.native.release_mutex(self.native_owner,handle)
    self.native_calls.append(dict(phase=self.active,boundary='native XAPI release-mutex adapter',source_entry=f'{address:08x}',caller=f'{self.u(sp):08x}',handle=f'{handle:08x}',native_error=actual,registers_before=before))
    if actual:self.stop(address,'Actual native release failed; no Source return supplied',native_error=actual)
    self.native_return(4);assert self.registers()==before;return
   if address==0x2dc814:
    handle,count,output=[self.u(sp+4+i*4)for i in range(3)];before=self.registers();previous=ctypes.c_int32();error=ctypes.c_int()
    actual=self.native.release_semaphore(self.native_owner,handle,count,ctypes.byref(previous),ctypes.byref(error))
    row=dict(phase=self.active,boundary='native XAPI release-semaphore adapter',source_entry=f'{address:08x}',caller=f'{self.u(sp):08x}',arguments=[f'{a:08x}'for a in(handle,count,output)],actual_released=actual,native_error=error.value,previous_count=previous.value if actual else None,registers_before=before)
    self.native_calls.append(row)
    if not actual or error.value:self.stop(address,'Actual native semaphore release failed; no Source result supplied',native_error=error.value)
    if output:self.w(output,previous.value)
    self.native_return(12);assert self.registers()==before;return
   if address==0x1a47d0:
    payload=self.u(sp+4);words=[self.u(payload+i*4)for i in range(10)];destination,n=words[1:3]
    aliases=[]
    for datum,tag in self.cache.tags.items():
     if tag['offset']is None:continue
     lo=tag['offset']+self.cache.magic;hi=lo+tag['size']
     if lo<destination+n and hi>destination:
      aliases.append(dict(datum=f'{datum:08x}',group=tag['group'],path=tag['name'],original_address=f'{lo:08x}',extent=tag['size']))
    self.stop(address,'Actual source file token/path producer absent; texture destination retains inherited whole-cache-file/physical overlap. No IO or callback result supplied',payload=f'{payload:08x}',payload28_hex=bytes(g.mem_read(payload,40)).hex(),destination_before_hex=bytes(g.mem_read(destination,n)).hex(),overlapping_imported_tags=aliases,cache_file_offset=f'{destination-self.cache.magic:08x}',inherited_cache_file_mapping_base=f'{self.cache.magic:08x}',inherited_cache_file_mapping_size=len(self.cache.data))
   if address==0x2d84c4:
    self.stop(address,'Actual pending request reaches XAPI yield; source worker/file ownership unavailable, no completion supplied')
  super().hook(g,address,size,user)
def prepare():
 previous=json.loads((parent.base.OWNER/'report.json').read_text());context=parent.base.OWNER/'raw-host-context.json'
 t=parent.base.player.prepare(json.loads(context.read_text()),trace_type=AsyncTrace)
 t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583);keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:previous['case'][k]for k in keys}
 assert factory['calls']==previous['case']['calls'][:len(factory['calls'])]and factory['ordered_stores']==previous['case']['ordered_stores'][:len(factory['ordered_stores'])]
 assert t.selected_fixup==previous['selected_original_physics_postprocess']
 t.sandbox_enabled=False;assert parent.base.player.spawn_and_activate(t,0,activate=False);assert t.stage('original_source_player_PVS',0x13a380)
 removed={f'{k:08x}':list(v)for k,v in t.services.items()};t.services.clear()
 t.power_observe=t.canister_observe=t.vertex_enabled=t.texture_enabled=t.async_enabled=True
 parent.ENTRIES.update(ENTRIES);return t,removed
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args();assert not args.output.exists()
 for n,h in PINS.items():assert parent.sha(ROOT/n)==h,n
 t,removed=prepare();stages=[]
 roots=[('source_async_constructor_prefix',0x1221b0,()),('source_texture_constructor',0x12d840,()),('source_texture_reset',0x12d950,()),('source_effect_pool_constructor',0x1775d0,()),('source_effect_map_reset',0x177690,()),('source_breakable_mask_constructor',0x1858e0,()),('source_breakable_mask_reset',0x185960,()),('source_complete_decal_constructor',0x17ec90,()),('source_complete_decal_map_reset',0x17ed50,()),('source_current_canister_owner',0xd7cd0,(0xe26f0000,parent.PLACEMENT+0x800)),('explicit_source_canister_death',0xd7e00,(0xe26f0000,parent.PLACEMENT+0x800,1,0))]
 roots +=[(f'source_damage_update_{i}',0xd73e0,(0xe26f0000,))for i in range(1,8)]+[(f'source_effect_update_{i}',0x1788b0,())for i in range(1,5)]
 try:
  for name,entry,arguments in roots:
   contexts=t.contexts();raw=t.platform.snapshot(t.g);pools=t.effect_pools();ok=t.phase(name,entry,arguments);t.effect_returns(parent.base.STOP)
   stage=dict(**t.power_stages[-1],contexts_before=contexts,contexts_after=t.contexts(),pools_before=pools,pools_after=t.effect_pools(),platform_before=raw,platform_after=t.platform.snapshot(t.g));stages.append(stage)
   if not ok:
    # Observer only: a deliberately stopped root must not be marked returned
    # when another explicit root later reaches the common STOP address.
    stage['unreturned_effect_call_ids']=[q['id']for _,_,q in t.effect_pending]
    t.effect_pending.clear();t.wait_pending.clear()
   print(json.dumps(dict(stage=name,completed=ok,invalid=t.new_invalid,eip=t.power_stages[-1]['stopped_eip'])),flush=True)
   if not ok and name!='source_async_constructor_prefix':break
  # A separate original worker root consumes only the actually published native
  # semaphore permit. It is not a fabricated return to the missing thread owner.
  if t.u(0x4e9ebc):
   contexts=t.contexts();pools=t.effect_pools();raw=t.platform.snapshot(t.g);ok=t.phase('explicit_original_worker_to_actual_file_boundary',0x122360);t.effect_returns(parent.base.STOP)
   stages.append(dict(**t.power_stages[-1],contexts_before=contexts,contexts_after=t.contexts(),pools_before=pools,pools_after=t.effect_pools(),platform_before=raw,platform_after=t.platform.snapshot(t.g)))
   print(json.dumps(dict(stage=stages[-1]['name'],completed=ok,invalid=t.new_invalid)),flush=True)
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
  cleanup=t.platform.close(t.g);objects=t.native.object_count(t.native_owner);t.native.destroy_provider(t.native_owner);t.native_owner=None
  report=dict(schema='original-canister-async-queue-real-native-ownership-v1',tool_sha256=parent.sha(__file__),protected_inputs=PINS,factory_correlated=True,player_preparation=t.player_stages,removed_inherited_services=removed,platform_calls=t.platform.calls,platform_abis=t.platform_abis,platform_cleanup=cleanup,applied_access_calls=t.access_calls,native_calls=t.native_calls,native_objects_retained_until_end=objects,native_owner_destroyed=True,stages=stages,calls=t.effect_calls,stores=t.effect_stores,world_states=t.power_states,instructions=listings,services=t.events,host=dict(uname=platform.uname()._asdict(),page_size=t.platform.page),boundaries=['Unchanged source1221B0 owns queue/pointer stores using actual frozen NativeSynchronization mutex/semaphore aliases; original worker creation stops before any supplied return. Following roots are bounded explicit composition, not full original title scheduling.','Creation/release adapters are native XAPI boundaries. Complete original2DC9C8/2DC8B2 wait wrappers execute; ordinal234 success0 is translated ONLY from actual acquired ownership, using pinned nxdk wait ABI/Cxbx status definition. No Xbox thread/TLS/scheduler/NTSTATUS general equivalence is claimed.','The separate original122360 worker root runs synchronously on the actual native owning thread, consumes only actual published semaphore permits and stops before unavailable file ownership/IO. No original worker creation, file token/done/completion/GPU readiness or particle is supplied.','Inherited source physical backing overlaps the imported whole cache-file mapping at80061000; this observed destination intersects no tagged payload. Exact destination bytes and actual tag intersections are retained; no buffer remap, capacity alteration or real IO is performed.','Earlier full source factory/player/PVS/platform/resource/default-variant/clock preparation boundaries persist; whole required decal/event/class8/collider/particle/render admission remains unresolved.'])
  args.output.write_text(parent.encoded(report));print(json.dumps(dict(report=str(args.output),sha256=parent.sha(args.output))),flush=True)
 finally:
  if t.platform.regions:t.platform.close(t.g)
  if t.native_owner:t.native.destroy_provider(t.native_owner);t.native_owner=None
if __name__=='__main__':main()
