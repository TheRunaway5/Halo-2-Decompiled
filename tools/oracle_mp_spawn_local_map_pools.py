#!/usr/bin/env python3
"""Separate original callback13 allocation and complete map-start extension."""
import argparse,hashlib,json,os,subprocess,mmap,ctypes
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_spawn_local_schedule import LocalTrace,FROZEN as OLDER,ROOT,OUT as PRIOR_OUT,ENTRIES as PRIOR_ENTRIES
from oracle_mp_spawn_player_visibility import prepare,CONTEXT_OUT,digest
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import PIN,CACHE_PIN,Trace as SourceTrace
from functions import _trace
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-pools'
CLOCK_FIXTURE=OUT/'measured-extra-clock-inputs.json'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_local_schedule.py':'0b0e90125fb792755ece1316092ea05845f49e4c50b241ae7a9110d4b263bbba',
 'docs/MP_LOCAL_COMMAND_SCHEDULING_RESEARCH.md':'6c6667803f78860197a5e876ce39697ead9af2fa48e9a86b43ea5e04b5916fd0',
 'analysis/multiplayer/integration/spawn-source/local-command-scheduling/report.json':'7f4286e44d419ece0c8948cdb2bcd8dd46c0d9b55a361a18560f07ada111f668',
 'analysis/multiplayer/integration/spawn-source/local-command-scheduling/independent-freeze-verification.json':'740359a674acaca8857b5787eda38896cfdf970dddc1115a7d9ecb02241e552a',
 'analysis/multiplayer/integration/spawn-source/local-map-pools/measured-extra-clock-inputs.json':'1775aa43de2041caf2e6b6927b8f603f1571064dddcfa905e7d8a9136c1c6fb9'}
_previous=json.loads((PRIOR_OUT/'report.json').read_text())
for _row in _previous['instruction_listings']+[_previous['executed_original_listing'],_previous['observed_local_store_listing']]:
 FROZEN[str((PRIOR_OUT/'instructions'/_row['file']).relative_to(ROOT))]=_row['sha256']
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+(0x11e150,0x11e180,0x125e50,0x16d0d0,0x1656a0,0x16d210,0x19f200,0x19f760,0x33785c,0x125370,0x2d7a1a,0x19f4c0,0x8e0b0,0x8e130,0x16d2c0,0x13f5a0,0x3279f0)))
class MapTrace(LocalTrace):
 def __init__(self,data,replay=None):
  self.map_enabled=False;self.pool_calls=[];self.pool_stores=[];self.pool_stages=[];self.pool_returns=[];self.actual_pool_allocations=[];self.host_raw_storage=[];self.host_raw_inputs=[];self.host_maps=[];self.host_guest_next=0xb200000;self.post_prepare_source_only=False;self.new_kernel_stops=[]
  super().__init__(data,replay);self.map_enabled=True
 def pool_state(self):
  pools={}
  for slot,n in((0x4f5cbc,0xf0),(0x4f5cb8,0x1e4)):
   p=self.u(slot);row=dict(slot=f'{slot:08x}',slot4_hex=bytes(self.g.mem_read(slot,4)).hex(),pointer=f'{p:08x}',source_requested_storage_bytes=n)
   if p:
    row.update(raw_storage_hex=bytes(self.g.mem_read(p,n)).hex(),header4c_hex=bytes(self.g.mem_read(p,0x4c)).hex(),capacity=self.u(p+0x20),stride=self.u(p+0x24),high_water=self.u(p+0x38),count=self.u(p+0x3c),data=f'{self.u(p+0x44):08x}',bitmap=f'{self.u(p+0x48):08x}')
   pools[f'{slot:08x}']=row
  p=self.u(0x46fbf8);v=self.u(p)if p else 0
  matg=self.u(0x4e791c);scenario=self.u(0x4e7920);inputs=dict(matg_pointer=f'{matg:08x}',matg170_10_hex=bytes(self.g.mem_read(matg+0x170,16)).hex(),scenario_pointer=f'{scenario:08x}',scenario10_2_hex=bytes(self.g.mem_read(scenario+0x10,2)).hex())
  for off,prefix,header,n_at,p_at,stride in((0x170,'first',8,0,4,0x108),(0x178,'second',0x18,0x10,0x14,0xc64)):
   q=self.u(matg+off+4)if self.u(matg+off)else 0
   inputs[prefix]=dict(pointer=f'{q:08x}',header_hex=bytes(self.g.mem_read(q,header)).hex()if q else None)
   if q:
    n=self.u(q+n_at);data=self.u(q+p_at);inputs[prefix].update(count=n,data=f'{data:08x}',records_hex=bytes(self.g.mem_read(data,n*stride)).hex()if n else '')
  return dict(pools=pools,original_allocator_slot4_hex=bytes(self.g.mem_read(0x46fbf8,4)).hex(),allocator_pointer=f'{p:08x}',allocator_record10_hex=bytes(self.g.mem_read(p,16)).hex()if p else None,allocator_vtable_pointer=f'{v:08x}',allocator_vtable8_hex=bytes(self.g.mem_read(v,8)).hex()if v else None,tracking_pairs200_hex=bytes(self.g.mem_read(0x4f5ab8,0x200)).hex(),original_cache_level_inputs=inputs)
 def hook(self,g,a,size,user):
  if self.instrument and self.map_enabled:
   for ret,row in reversed(self.pool_returns[:]):
    if a==ret:
     row['returned_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';row['pool_state_after']=self.pool_state();self.pool_returns.remove((ret,row))
     if row.get('allocator_request'):
      p=g.reg_read(UC_X86_REG_EAX);n=row['requested_bytes'];row['raw_requested_allocation_after_return_hex']=bytes(g.mem_read(p,n)).hex()if p else None
      if p:self.actual_pool_allocations.append((p,n))
     break
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row=dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(8)])
    if self.active=='source_callback13_storage'and a==0x11e150:row.update(allocator_request=True,requested_bytes=self.u(sp+4),pool_state_before=self.pool_state())
    self.pool_calls.append(row);self.pool_returns.append((self.u(sp),row))
  if self.post_prepare_source_only and a==0x33785c:
   # Bypass only inherited restricted raw-wrapper service/observer. Execute the
   # original XAPI instructions and their actual kernel import call unchanged.
   # Several older observer layers also assume a supplied33785C return entry.
   # Retain ordinary instruction journaling while bypassing those Python-only
   # observers at this one entry; guest execution continues into the wrapper.
   self.executed.add(a);SourceTrace.hook(self,g,a,size,user);return
  if self.post_prepare_source_only and a==0x800000b8:
   sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(5)];bp,zero,ns,flags,protect=args
   requested_base,requested_size=self.u(bp),self.u(ns)
   assert requested_base==0 and zero==0 and flags==0x101000 and protect==4
   page=os.sysconf('SC_PAGE_SIZE');assert page==4096
   mapped=(requested_size+page-1)//page*page
   storage=mmap.mmap(-1,mapped,prot=mmap.PROT_READ|mmap.PROT_WRITE);native=ctypes.addressof(ctypes.c_ubyte.from_buffer(storage));p=self.host_guest_next;self.host_guest_next+=mapped
   g.mem_map_ptr(p,mapped,3,native);self.host_maps.append(storage)
   self.host_raw_inputs.append(dict(phase=self.active,native_address=f'{native:016x}',guest_mirror=f'{p:08x}',requested_bytes=requested_size,mapped_bytes=mapped,initial_sha256=hashlib.sha256(storage[:]).hexdigest()))
   self.w(bp,p);self.w(ns,mapped)
   self.host_raw_storage.append(dict(phase=self.active,entry='800000b8',kernel_ordinal=184,original_return_site=f'{self.u(sp):08x}',args=[f'{v:08x}'for v in args],requested_base=f'{requested_base:08x}',requested_size=requested_size,resulting_base=f'{p:08x}',resulting_size=mapped,host_page_bytes=page,allocation_flags=f'{flags:08x}',protection=f'{protect:08x}',declared_ntstatus='00000000',initial_storage_sha256=hashlib.sha256(storage[:]).hexdigest(),scope='real owned host mmap raw storage mirrored at declared guest addresses; ordinal184 status/request adaptation boundary, not Xbox kernel allocation semantics or game fields'))
   g.reg_write(UC_X86_REG_EAX,0);g.reg_write(UC_X86_REG_ESP,sp+24);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
  if self.post_prepare_source_only and a==0x800000b6:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.new_invalid=dict(boundary='unprovided original MmSetAddressProtect request; inherited provider supports only the preceding arena request',kernel_target=f'{a:08x}',kernel_ordinal=182,original_return_site=f'{self.u(sp):08x}',phase=self.active,stack=[f'{self.u(sp+4+i*4):08x}'for i in range(3)],registers=self.registers(),recent=[f'{v:08x}'for v in self.recent])
   self.new_kernel_stops.append(self.new_invalid);raise KernelBoundary()
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.map_enabled and self.track_players:
   kind=None
   if address in(0x4f5cbc,0x4f5cb8):kind='pool_binding'
   elif 0x4f5ab8<=address<0x4f5cb8:kind='tracking_pairs+'+f'{address-0x4f5ab8:03x}'
   else:
    for p,n in self.actual_pool_allocations:
     if p<=address<p+n:kind='actual_pool_allocation_'+f'{p:08x}'+'+'+f'{address-p:03x}';break
   if kind:self.pool_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',kind=kind,address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)
 def pool_action(self,name,entry,args=(),regs=()):
  before=self.pool_state();start=len(self.pool_calls);ok=self.local_action(name,entry,args,regs);row=self.local_stages[-1];row.update(pool_state_before=before,pool_state_after=self.pool_state(),pool_calls=self.pool_calls[start:]);self.pool_stages.append(row);return ok

def execute(data,replay=None):
 t=prepare(data,trace_type=lambda d:MapTrace(d,replay))
 # The previous selected preparation needs only its declared raw OS backing.
 # New post-preparation gameplay/Havok paths execute without inherited stubs.
 t.removed_post_prepare_services={f'{a:08x}':dict(name=n,return_value=f'{r&0xffffffff:08x}'if r is not None else None,stack_cleanup=p)for a,(n,r,p)in t.services.items()};t.services.clear();t.post_prepare_source_only=True
 for name,entry in [('source_control_storage_record20',0x1874d0),('source_view_storage_record21',0x1ea3d0),('source_camera_storage_record35',0x156a30),('source_control_newmap_record20',0x187520),('source_view_newmap_record21',0x1ea410),('source_camera_newmap_record35',0x156ad0),('source_network_configuration_default_handler',0x663b0),('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0),('source_readiness_gated_simulation_scheduler',0x68300),('source_random_storage_record3',0x1479b0),('source_callback13_storage',0x19f160)]:
  if not t.pool_action(name,entry):return t
 t.pool_action('source_complete_map_ready_owner',0x190a30,regs=((UC_X86_REG_EAX,t.u(0x4edf18)+8),))
 assert t.pool_stages[-2]['completion']=='complete_original_return'
 assert [(s['requested_size'],s['resulting_size'])for s in t.host_raw_storage]==[(0xf0,0x1000),(0x1e4,0x1000)]
 assert [(p['capacity'],p['stride'])for p in t.pool_state()['pools'].values()]==[(20,8),(50,8)]
 assert t.pool_stages[-1]['stopped_eip']=='800000b6'and t.new_invalid['stack']==['82d47000','003be000','00000004']
 assert [c['index']for c in t.map_callbacks]==[2,9,13,16]
 assert t.u(t.u(0x4edf18)+0x1120,1)==0 and t.u(t.u(0x4d6d44)+0x18)==2
 assert t.extra_replay is None or t.extra_replay_cursor==len(t.extra_replay)
 assert not any(c['entry']in('001395b0','0011d780','00188ad0','001884e0','00187610','00069280','000852a0','0006b750')for c in t.pool_calls)
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--clock-replay',type=Path,default=CLOCK_FIXTURE);args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 replay=json.loads(args.clock_replay.read_text())
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n');t=execute(data,replay)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();proc.stderr.close()
 assert proc.returncode==0
 print(json.dumps(dict(stages=[dict(name=s['name'],completion=s['completion'],eip=s['stopped_eip'],invalid=s['invalid'])for s in t.pool_stages],map_callback_indices=[r['index']for r in t.map_callbacks])),flush=True)
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 def emit(name,ins):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in t.executed for i in ins))
 whole=emit('executed-original-instructions.asm',[t.code.at(a)for a in sorted(t.executed)])
 for e in ENTRIES:
  r=_trace(t.code,e,t.starts|set(ENTRIES));listings.append(dict(entry=f'{e:08x}',**emit(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)]),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call']))
 stores=emit('observed-map-pool-source-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for r in t.pool_stores+t.local_stores+t.input_stores+t.player_stores})])
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,stages=t.pool_stages,pool_calls=t.pool_calls,pool_stores=t.pool_stores,local_stores=t.local_stores,input_stores=t.input_stores,player_stores=t.player_stores,input_calls=t.input_calls,map_newmap_callbacks=t.map_callbacks,extra_clock_calls=t.extra_clock_calls,inherited_clock_calls=t.clock_calls,preparation=t.producers,services=t.events,physical_services=t.physical_services,platform=t.platform_input,compatibility_lock_calls=t.lock_calls,source_raw_impact_pool_allocations=t.os_pool_allocs,source_requested_host_storage=t.host_raw_storage,new_unprovided_kernel_requests=t.new_kernel_stops,removed_post_prepare_services=t.removed_post_prepare_services,instruction_listings=listings,instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),executed_original_listing=whole,observed_map_pool_store_listing=stores,executed_original_addresses_verified=len(t.executed),final_local_state=t.local_state(),final_pool_state=t.pool_state(),instruction_patches=[],new_supplied_gameplay_or_physics_results=[])
 out.update(summary=dict(complete_original_pool_constructor=1,complete_original_callback13_reset=1,first_pool_capacity=20,second_pool_capacity=50,pool_stride=8,source_first_pool_live_handles=15,source_second_pool_live_handles=0,completed_map_start=0,readiness_state4_or_local_bindings_supplied=0),original_static_allocator=dict(slot='0046fbf8',slot4_hex=t.image.read(0x46fbf8,4).hex(),object='00480d68',object10_hex=t.image.read(0x480d68,16).hex(),vtable='0045a480',vtable8_hex=t.image.read(0x45a480,8).hex()),boundaries=[
 'Every earlier frozen tool/doc/report and all50 preceding local-scheduling instruction files remain byte-identical. Complete compatibility KPCR/ETHREAD/host stack/process inputs and earlier raw-storage/cache/options delivery boundaries remain explicit.',
 'This extension reuses the frozen selected source player/world/control/view/camera/simulation preparation, executes original1479B0 RNG producer and inserts actual owning19F160 before complete190A30. This is a bounded root composition, not whole139390/full title/session initialization or authored allocation order.',
 'Original46FBF8 allocator pointer480D68/vtable45A480/target11E150 are unchanged static XBE data. Original19F160 requestsF0 and1E4, original11E150 supplies literal0/size/101000/4 to the fully executed33785C XAPI wrapper, which invokes ordinal184.',
 'Only that requested raw kernel storage boundary is supplied anew: real owned Linuxmmap RW pages are mirrored through Unicorn.mem_map_ptr at declared guestB200000/B201000. Measured host page size4096 determines returned raw mapped size. Original NTSTATUS0, base/size pointer outputs and flags101000 adaptation are explicit provider boundaries; Xbox kernel memory, MEM_NOZERO behavior and hardware address parity are not claimed. Initial zero bytes are actual host storage outputs, not manufactured game/pool fields or recovered Xbox defaults.',
 'The older Python33785C observers require prior fixed-size service returns. Only those observer layers are bypassed at this entry; ordinary journaling and unchanged wrapper/kernel caller bytes execute. All post-preparation gameplay/Havok service hooks are removed. No game, physics, membership, pool, profile or readiness result is supplied.',
 'Actual16D070/16D0D0 write both complete pool headers/name/capacity20or50/stride8/data/bitmap/allocator fields.19F160 binds4F5CBC/4F5CB8 and clears64tracking pairs. Source19F720 resets both pools then full19F6B0/19FC60/19FD70 execute; exact retained actual cache level inputs and source15 level-handle allocations are recorded. Cache loading/global binding remains the inherited authored-cache delivery boundary, not a complete tag loader proof.',
 'Ascending original139410 reset traversal admits targets at indices2/9/13/16; callback13 returns. Callback16/125370→2D7A1A reaches unresolved kernel182MmSetAddressProtect with82D47000/3BE000/4 and return125387. The previous raw protection provider declared only82D47000/3FE000/4; this extension retains the new request without applying permissions or returning void success.',
 'Full190A30 is interrupted before1395B0 membership,11D780 BSP startup and190A6E readiness. ActualGame+0 in-progress flag1, readiness+1120zero and source simulationkind1/state2 are retained. Ordinary local view/controller/member/profile/device scheduling, whole139750, upstream game clock and state4 remain unexecuted.',
 'Earlier simulation clocks and the new1479B0 real-host FILETIME/boot-ms measurements are replayed verbatim. No clock/RNG/game field is directly supplied; original32803Etime/rand/storage producers execute. Raw native mmap addresses and complete host context are retained separately because they may differ across independent runs.'])
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 (args.output.parent/'raw-host-allocator-storage.json').write_text(json.dumps(t.host_raw_inputs,indent=2,sort_keys=True)+'\n')
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),oracle_sha256=out['oracle_sha256'],executed=len(t.executed))),flush=True)
if __name__=='__main__':main()
