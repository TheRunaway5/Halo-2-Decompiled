#!/usr/bin/env python3
"""Independent owning clock/callback18 constructors before original map start."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_spawn_local_map_protection import ProtectionTrace,FROZEN as OLDER,ENTRIES as PRIOR_ENTRIES,OUT as PRIOR_OUT,build_adapter
from oracle_mp_spawn_local_map_pools import CLOCK_FIXTURE
from oracle_mp_spawn_player_visibility import prepare,ROOT,CONTEXT_OUT,digest
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import PIN,CACHE_PIN,Trace as SourceTrace
from functions import _trace
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-storage'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_local_map_protection.py':'1a716ec285fbd08ed14de2e4e05e6b5eea4f909f316b8640a676baf41a5fffb6',
 'docs/MP_LOCAL_MAP_PROTECTION_RESEARCH.md':'8c3eda9da6d8b5b339b446251f1d36a65b236bf498824aa2ab33f48e8901d8a6',
 'analysis/multiplayer/integration/spawn-source/local-map-protection/report.json':'aaa95ea9b7904ae16182a8e7a19396b25cd204f71b597fb05d17d932ab5076cc',
 'analysis/multiplayer/integration/spawn-source/local-map-protection/independent-freeze-verification.json':'72e4ad33ca7aa0ce9017ddc2fdb67763b32d92285493b7381931c23dc50e573b',
 'tools/oracle_mp_spawn_lobby_source.py':'badf20f4ff1663a5594e62a2854455e30c50f56585b2d95bcce1ce8173a082e7',
 'docs/MP_LOBBY_SOURCE_RESEARCH.md':'5d05ce7fd0e1049c33d9ccdba9aca3248bf61a9181eb9eb82766ed54ee0ea073',
 'analysis/multiplayer/integration/spawn-source/lobby-source/report.json':'448ae9b68b0ee335e7a4503382e84d1f2a84b79b0a4a5ceb71964ac28520cdec',
 'analysis/multiplayer/integration/spawn-source/lobby-source/independent-freeze-verification.json':'995b32b4793641f587321fe89d7be49dda7568020dfe0fc65bf7e5d39ea2322c'}
_prior=json.loads((PRIOR_OUT/'report.json').read_text())
for _r in _prior['instruction_listings']+[_prior['executed_original_listing'],_prior['observed_map_protection_store_listing']]:FROZEN[str((PRIOR_OUT/'instructions'/_r['file']).relative_to(ROOT))]=_r['sha256']
for _n,_r in _prior['primary_sources'].items():FROZEN[str((PRIOR_OUT/'primary-source'/_n).relative_to(ROOT))]=_r['sha256']
for _n,_field in [('compiled-compatibility-access.cpp','source_sha256'),('compiled-compatibility-access.so','library_sha256')]:FROZEN[str((PRIOR_OUT/_n).relative_to(ROOT))]=_prior['compiled_compatibility_access'][_field]
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+(0x14ccd0,0x158c10,0x197450)))

class StorageTrace(ProtectionTrace):
 def __init__(self,data,replay,adapter):
  self.storage_enabled=False;self.storage_calls=[];self.storage_stores=[];self.platform_storage_stores=[];self.storage_stages=[];self.allocated_owned_records=[];self.owner_stops=[]
  super().__init__(data,replay,adapter);self.storage_enabled=True
 def hook(self,g,a,size,user):
  if self.instrument and self.storage_enabled and a in ENTRIES:
   sp=g.reg_read(UC_X86_REG_ESP);self.storage_calls.append(dict(phase=self.active,entry=f'{a:08x}',return_address=f'{self.u(sp):08x}',registers=self.registers(),stack32_hex=bytes(g.mem_read(sp+4,32)).hex()))
  if self.post_prepare_source_only and self.active=='source_complete_map_ready_owner'and a in(0x336983,0x337a40):
   # Execute the original XAPI instructions, bypassing only earlier clock
   # observers/providers at these entries. No new clock observation is supplied.
   self.executed.add(a);SourceTrace.hook(self,g,a,size,user);return
  if self.post_prepare_source_only and self.active=='source_complete_map_ready_owner'and a in(0x80000080,0x337a45):
   sp=g.reg_read(UC_X86_REG_ESP)
   details=dict(boundary='new original map-reset clock request has no supplied current observation',instruction=f'{a:08x}',phase=self.active,kernel_ordinal=128 if a==0x80000080 else 156,original_import_slot='00417bc8'if a==0x80000080 else'00417cd4',original_import4_hex=self.image.read(0x417bc8 if a==0x80000080 else 0x417cd4,4).hex(),runtime_import4_hex=bytes(g.mem_read(0x417bc8 if a==0x80000080 else 0x417cd4,4)).hex(),return_address=f'{self.u(sp):08x}',registers=self.registers(),stack40_hex=bytes(g.mem_read(sp,0x40)).hex(),current_storage_state=self.callback_state(),recent=[f'{v:08x}'for v in self.recent],result_supplied=False)
   if a==0x80000080:
    p=self.u(sp+4);details.update(requested_filetime_output_pointer=f'{p:08x}',output8_before_hex=bytes(g.mem_read(p,8)).hex())
   self.new_invalid=details;self.owner_stops.append(details);raise KernelBoundary()
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.storage_enabled and self.track_players and self.post_prepare_source_only:
   ip=g.reg_read(UC_X86_REG_EIP);row=dict(phase=self.active,instruction=f'{ip:08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex())
   if self.code.at(ip)is None:
    assert 0x9000000<=ip<0x9001000 or 0x9400000<=ip<0x9401000 or 0x9500000<=ip<0x9501000,hex(ip)
    self.platform_storage_stores.append(row)
   else:self.storage_stores.append(row)
  super().mem_write(g,access,address,size,value,user)
 def storage_action(self,name,entry,args=(),regs=()):
  before=self.callback_state();cursor=self.u(0x4ed654);base=self.u(0x4ed650);ok=self.pool_action(name,entry,args,regs);row=self.pool_stages[-1];row.update(complete_owning_state_before=before,complete_owning_state_after=self.callback_state());self.storage_stages.append(row)
  if name in('source_clock_storage_record17','source_callback18_storage')and ok:
   slot,n=(0x518444,0x24)if entry==0x147cc0 else(0x4fcdc4,0xe4);p=self.u(slot)
   assert p==base+cursor and self.u(0x4ed654)==cursor+n
   self.allocated_owned_records.append(dict(name=name,entry=f'{entry:08x}',source_slot=f'{slot:08x}',source_address=f'{p:08x}',source_bytes=n,arena_cursor_before=cursor,arena_cursor_after=self.u(0x4ed654),raw_after_constructor_hex=bytes(self.g.mem_read(p,n)).hex(),checksum_after4_hex=bytes(self.g.mem_read(0x4ed65c,4)).hex()))
  return ok

def execute(data,replay,adapter):
 t=prepare(data,trace_type=lambda d:StorageTrace(d,replay,adapter))
 t.removed_post_prepare_services={f'{a:08x}':dict(name=name,return_value=f'{r&0xffffffff:08x}'if r is not None else None,stack_cleanup=pop)for a,(name,r,pop)in t.services.items()};t.services.clear();t.post_prepare_source_only=True
 for name,entry in [('source_control_storage_record20',0x1874d0),('source_view_storage_record21',0x1ea3d0),('source_camera_storage_record35',0x156a30),('source_control_newmap_record20',0x187520),('source_view_newmap_record21',0x1ea410),('source_camera_newmap_record35',0x156ad0),('source_network_configuration_default_handler',0x663b0),('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0),('source_readiness_gated_simulation_scheduler',0x68300),('source_random_storage_record3',0x1479b0),('source_callback13_storage',0x19f160),('source_clock_storage_record17',0x147cc0),('source_callback18_storage',0x1e2e50)]:
  if not t.storage_action(name,entry):return t
 t.storage_action('source_complete_map_ready_owner',0x190a30,regs=((UC_X86_REG_EAX,t.u(0x4edf18)+8),))
 assert len(t.allocated_owned_records)==2
 assert t.u(t.u(0x4edf18)+0x1120,1)==0 and t.extra_replay_cursor==len(t.extra_replay)
 assert t.pool_stages[-1]['completion']=='blocked_original_instruction'
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for s,h in FROZEN.items():assert digest(ROOT/s)==h,s
 adapter=build_adapter();replay=json.loads(CLOCK_FIXTURE.read_text())
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n');t=execute(data,replay,adapter)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();proc.stderr.close()
 assert proc.returncode==0
 print(json.dumps(dict(owning_allocations=t.allocated_owned_records,callbacks=[dict(index=r['index'],target=r['target'])for r in t.map_callbacks],stop={k:v for k,v in t.pool_stages[-1]['invalid'].items()if k!='current_storage_state'})),flush=True)
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 def emit(name,ins):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in t.executed for i in ins))
 whole=emit('executed-original-instructions.asm',[t.code.at(a)for a in sorted(t.executed)])
 for e in ENTRIES:
  r=_trace(t.code,e,t.starts|set(ENTRIES));listings.append(dict(entry=f'{e:08x}',**emit(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)]),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call']))
 stores=emit('observed-map-storage-source-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for r in t.storage_stores+t.pool_stores+t.local_stores+t.input_stores+t.player_stores+t.protection_stores})])
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,tool_sha256=digest(__file__),frozen_pins=FROZEN,stages=t.storage_stages,actual_source_owned_allocations=t.allocated_owned_records,storage_calls=t.storage_calls,storage_stores=t.storage_stores,pool_calls=t.pool_calls,pool_stores=t.pool_stores,local_stores=t.local_stores,input_stores=t.input_stores,player_stores=t.player_stores,protection_stores=t.protection_stores,map_newmap_callbacks=t.map_callbacks,callback_before_after_states=t.callback_states,applied_memory_protection=t.access_calls,owner_dependency_stops=t.owner_stops,extra_clock_calls=t.extra_clock_calls,inherited_clock_calls=t.clock_calls,preparation=t.producers,services=t.events,physical_services=t.physical_services,platform=t.platform_input,compatibility_lock_calls=t.lock_calls,compatibility_initializer=t.compiled,compatibility_initializer_calls=t.compat_calls,compatibility_initializer_stores=t.compat_stores,post_prepare_platform_storage_stores=t.platform_storage_stores,source_raw_impact_pool_allocations=t.os_pool_allocs,source_requested_host_storage=t.host_raw_storage,removed_post_prepare_services=t.removed_post_prepare_services,compiled_compatibility_access=t.compiled_metadata,final_owning_state=t.callback_state(),instruction_listings=listings,instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),executed_original_listing=whole,observed_map_storage_store_listing=stores,executed_original_addresses_verified=len(t.executed),instruction_patches=[],new_supplied_gameplay_or_physics_or_clock_results=[],boundaries=[
 'Every preceding freeze, protection primary source/body/library and lobby-source note stays immutable. Existing complete compatibility thread/process/raw allocation and real-cache/options delivery scope remains explicit.',
 'The same selected preparation is followed by the original17 and18 storage callbacks before the same full190A30 root. This is a bounded prerequisite composition from actual139390 storage table ownership, not full title startup/all68callback order or authenticated local lobby/profile/session creation.',
 'Original147CC0 reserves24 bytes from the existing source-owned game arena, checksums through1656A0, clears its whole record and binds518444 at147D0E. No clock record, pointer, tick,dt or timing result is assigned. This replaces the inherited clock pointer through the original producer.',
 'Original1E2E50 reservesE4 bytes from that same arena, executes1656A0, clears39 dwords and binds4FCDC4 at1E2E92. No callback18 record fields or successful reset result are supplied. Actual allocations, cursor/checksum, whole before/after owner records and stores are retained.',
 'Earlier raw OS clock observations replay the existing fixture verbatim only for their originally recorded preparation roots. During the new full map-reset path, prior clock-provider observers are bypassed only at336983/337A40 so unchanged XAPI bytes execute; the first newly requested raw clock observation is stopped before any result/data read. No new clock observation, assumed platform success, readiness, membership, control binding or admitted input is supplied.',
 'Original shared map-reset callbacks retain ascending table order. Each before/after callback record, return flag, complete source-created clock record, callback18 record and other local/pool/options records is captured. The first exact remaining dependency is retained rather than replaced.',
 'Any actual retained compatibility initializer/lock guest instructions and stores remain separately identified as platform code. They are not passed to the original-XBE instruction emitter; only verified original store sites enter that listing. The exact inherited compiled initializer/call/store ledger is retained.',
 'No actual local participant count or menu/lobby/countdown ownership is established by this bounded map-start. The separate original static lobby report is preserved; literals10/3 andvalid-user count do not admit a controller/session/time lifecycle here.',
 'No original instruction bytes or production files are edited. Original Havok/world readiness, complete11D780/190A6E, source state4 and ordinary local command scheduling stay outside any incomplete map-start claim.'])
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 (args.output.parent/'raw-host-allocator-storage.json').write_text(json.dumps(t.host_raw_inputs,indent=2,sort_keys=True)+'\n')
 for s,h in FROZEN.items():assert digest(ROOT/s)==h,s
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),tool_sha256=out['tool_sha256'],manifest=out['instruction_manifest_sha256'],executed=len(t.executed))),flush=True)
if __name__=='__main__':main()
