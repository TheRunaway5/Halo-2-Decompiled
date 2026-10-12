#!/usr/bin/env python3
"""Independent original map-start with applied compatibility access protection."""
import argparse,ctypes,hashlib,json,os,re,subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_spawn_local_map_pools import MapTrace,FROZEN as OLDER,ENTRIES as PRIOR_ENTRIES,CLOCK_FIXTURE,OUT as PRIOR_OUT
from oracle_mp_spawn_player_visibility import prepare,ROOT,CONTEXT_OUT,digest
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-protection'
PRIMARY=OUT/'primary-source'
COMMIT='585c49a50af1255ab155099e06f24505f9c5a800'
SOURCES={
 'EmuKrnlMm.cpp':('src/core/kernel/exports/EmuKrnlMm.cpp','596efb2770c71f40a7f0773d35047f863e03c51feefa53d2e60f0394af109b99'),
 'VMManager.cpp':('src/core/kernel/memory-manager/VMManager.cpp','40a7ae0c8d00f198f353c017ff7a8024ea22e1b91e8b7fa4cffe6e9306f2220e'),
 'VMManager.h':('src/core/kernel/memory-manager/VMManager.h','c08d4092c5a93113858701a64f9f4abb40130855b021a1819c503f7e1400ee1c'),
 'PhysicalMemory.cpp':('src/core/kernel/memory-manager/PhysicalMemory.cpp','fea671559ec9ce38a0709a341bf710f9380a0804cc7a74014f0552c02fb238e5'),
 'PhysicalMemory.h':('src/core/kernel/memory-manager/PhysicalMemory.h','f53923836c247e9e5475e2effe21e450402616eb9d60fc2db6b89887286a2c3f'),
 'CxbxKrnl.h':('src/core/kernel/init/CxbxKrnl.h','7cebac11841c538365f6ee5b01144094f81bb12490a1b1d3c042b6311a8097f8'),
 'mm.h':('src/core/kernel/common/mm.h','e512c55b0b274fc37bd203904be85568b3f96c86f4d1d69701631ae22bb1872d'),
 'EmuKrnl.h':('src/core/kernel/exports/EmuKrnl.h','1682369c480a5a8751cc696eae8d600394390652b6bf2341d096b424ac804448')}
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_local_map_pools.py':'73eaaa882c57491583f952757c652bcd6dc36ac690d06e5b44c619f9fc33bb6f',
 'docs/MP_LOCAL_MAP_POOL_RESEARCH.md':'7d815accadb8b62f8b7156b88ed67e0159fe3dd0a2b3a19ba5d784cf9bc8086e',
 'analysis/multiplayer/integration/spawn-source/local-map-pools/report.json':'2a0fac629605e5166e8f8cbaf85f8c8723b21cfbe9b83d2f3a4e255ef03e76ab',
 'analysis/multiplayer/integration/spawn-source/local-map-pools/independent-freeze-verification.json':'a3e91e1d18d0f2b2fcca9a7af55f31e34f091865b3b170f2ea72001bc345ad22'}
_prior=json.loads((PRIOR_OUT/'report.json').read_text())
for _r in _prior['instruction_listings']+[_prior['executed_original_listing'],_prior['observed_map_pool_store_listing']]:
 FROZEN[str((PRIOR_OUT/'instructions'/_r['file']).relative_to(ROOT))]=_r['sha256']
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+(0x125280,0x217090,0x217020,0x12cb70,0x147cc0,0x147d20,0x1e2e50,0x1e2ea0)))

def function(text,signature):
 start=text.index(signature);brace=text.index('{',start);depth=1;end=brace+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]

def build_adapter():
 for name,(_,h)in SOURCES.items():assert digest(PRIMARY/name)==h,name
 phys=(PRIMARY/'PhysicalMemory.cpp').read_text();vm=(PRIMARY/'VMManager.cpp').read_text()
 fragments=[function(phys,'bool PhysicalMemory::ConvertXboxToSystemPtePermissions'),function(phys,'DWORD PhysicalMemory::PatchXboxPermissions'),function(phys,'DWORD PhysicalMemory::ConvertXboxToWinPermissions'),function(vm,'void VMManager::UpdateMemoryPermissions')]
 # Exact primary function bodies, unchanged. Only compilation types, constants,
 # logging and the real VirtualProtect transport are provided by this harness.
 definitions=[]
 for name in('CxbxKrnl.h','PhysicalMemory.h'):
  for line in(PRIMARY/name).read_text().splitlines():
   if re.match(r'#define (XBOX_PAGE_|PTE_(VALID|WRITE|DIRTY|ACCESS|CACHE_DISABLE|WRITE_THROUGH)_MASK|ValidKernelPteBits)',line):definitions.append(line)
 shim='''#include <cstdint>
#include <cstddef>
using DWORD=uint32_t; using ULONG=uint32_t; using VAddr=uint32_t;
namespace xbox { struct MMPTE { ULONG Default; }; using PMMPTE=MMPTE*; }
namespace LOG_LEVEL { constexpr int DEBUG=0; }
template<class... T> void EmuLog(T...) {}
using Provider=int(*)(VAddr,size_t,DWORD,DWORD*);
static Provider provider=nullptr; static int actual_result=0;
static int VirtualProtect(void* p,size_t n,DWORD perms,DWORD* old) {
 actual_result=provider(static_cast<VAddr>(reinterpret_cast<uintptr_t>(p)),n,perms,old);
 return actual_result;
}
static DWORD GetLastError() { return 1; }
class PhysicalMemory { public:
 bool ConvertXboxToSystemPtePermissions(DWORD,xbox::PMMPTE);
 DWORD PatchXboxPermissions(DWORD); DWORD ConvertXboxToWinPermissions(DWORD);
};
class VMManager: public PhysicalMemory { public: void UpdateMemoryPermissions(VAddr,size_t,DWORD); };
#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_GUARD 0x100
#define PAGE_NOCACHE 0x200
#define PAGE_WRITECOMBINE 0x400
'''
 wrapper='''
extern "C" int contract(DWORD p,DWORD* out) {
 PhysicalMemory m; xbox::MMPTE q{};
 if(!m.ConvertXboxToSystemPtePermissions(p,&q))return 0;
 out[0]=q.Default;out[1]=m.PatchXboxPermissions(p);
 out[2]=m.ConvertXboxToWinPermissions(out[1]);
 out[3]=out[2]&~(PAGE_WRITECOMBINE|PAGE_NOCACHE);return 1;
}
extern "C" int apply(VAddr a,size_t n,DWORD p,Provider cb) {
 provider=cb;actual_result=0;VMManager m;m.UpdateMemoryPermissions(a,n,p);return actual_result;
}
'''
 raw=(shim+'\n'+'\n'.join(definitions)+'\n\n'+'\n\n'.join(fragments)+wrapper).encode()
 cpp=OUT/'compiled-compatibility-access.cpp';so=OUT/'compiled-compatibility-access.so'
 if cpp.exists():assert cpp.read_bytes()==raw
 else:cpp.write_bytes(raw)
 subprocess.run(['g++','-std=c++17','-O2','-shared','-fPIC',str(cpp),'-o',str(so)],check=True)
 lib=ctypes.CDLL(str(so));cb_type=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32))
 lib.contract.argtypes=[ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)];lib.contract.restype=ctypes.c_int
 lib.apply.argtypes=[ctypes.c_uint32,ctypes.c_size_t,ctypes.c_uint32,cb_type];lib.apply.restype=ctypes.c_int
 return lib,cb_type,dict(source_sha256=digest(cpp),library_sha256=digest(so),source_functions=[dict(signature=f.split('\n',1)[0],unchanged_body_sha256=hashlib.sha256(f.encode()).hexdigest())for f in fragments],primary_commit=COMMIT)

class ProtectionTrace(MapTrace):
 def __init__(self,data,replay,adapter):
  self.access_calls=[];self.protection_stores=[];self.callback_states=[];self.callback_pending=None;self.adapter,self.cb_type,self.compiled_metadata=adapter
  super().__init__(data,replay)
 def callback_state(self):
  clock=self.u(0x518444);metadata=self.u(0x4ed664);owner=self.u(0x4fcdc4)
  return dict(local=self.local_state(),pool=self.pool_state(),arena_globals1c_hex=bytes(self.g.mem_read(0x4ed650,0x1c)).hex(),metadata_pointer=f'{metadata:08x}',metadata1288_hex=bytes(self.g.mem_read(metadata,0x1288)).hex()if metadata else None,clock_slot4_hex=bytes(self.g.mem_read(0x518444,4)).hex(),clock_pointer=f'{clock:08x}',inherited_clock24_hex=bytes(self.g.mem_read(clock,0x24)).hex()if clock else None,callback18_slot4_hex=bytes(self.g.mem_read(0x4fcdc4,4)).hex(),callback18_pointer=f'{owner:08x}',callback18_e4_hex=bytes(self.g.mem_read(owner,0xe4)).hex()if owner else None)
 def hook(self,g,a,size,user):
  if self.instrument and a==0x13944a:
   assert self.callback_pending is None
   row=dict(index=int(self.registers()['esi'],16)//36,target=self.registers()['eax'],phase=self.active,call_instruction='0013944a',before=self.callback_state(),registers_before=self.registers(),stack_pointer_before=f'{g.reg_read(UC_X86_REG_ESP):08x}')
   self.callback_states.append(row);self.callback_pending=row
  if self.instrument and a==0x13944c and self.callback_pending is not None:
   self.callback_pending.update(returned=True,after=self.callback_state(),registers_after=self.registers(),stack_pointer_after=f'{g.reg_read(UC_X86_REG_ESP):08x}');self.callback_pending=None
  if a==0x800000b6:
   sp=g.reg_read(UC_X86_REG_ESP);base,n,p=(self.u(sp+4+i*4)for i in range(3));ret=self.u(sp)
   assert base%4096==0 and n>0 and n%4096==0
   assert (base,n,p)in((0x82d47000,0x3fe000,4),(0x82d47000,0x3be000,4),(0x83105000,0x40000,0x404))
   out=(ctypes.c_uint32*4)();assert self.adapter.contract(p,out)==1
   before=bytes(g.mem_read(base,n));regs=self.registers();regions_before=[dict(start=f'{lo:08x}',end=f'{hi:08x}',permissions=perms)for lo,hi,perms in g.mem_regions()if lo<base+n and hi>=base]
   applied=[];errors=[]
   def apply(addr,bytes_count,windows,old):
    try:
     assert addr==base and bytes_count==n and windows==out[3]==0x40
     # The compiled pinned compatibility body has already converted the exact
     # PAGE mask. Apply its host-facing R/W/X access semantics to guest memory.
     permissions=7;g.mem_protect(addr,bytes_count,permissions)
     after=[dict(start=f'{lo:08x}',end=f'{hi:08x}',permissions=perms)for lo,hi,perms in g.mem_regions()if lo<base+n and hi>=base]
     assert all(r['permissions']==permissions for r in after)
     assert {r['permissions']for r in regions_before}=={7}
     old[0]=0x40;applied.append(dict(address=f'{addr:08x}',size=bytes_count,windows_access=f'{windows:08x}',unicorn_permissions=permissions,previous_windows_access=f'{old[0]:08x}',regions_after=after));return 1
    except Exception as e:errors.append(str(e));return 0
   cb=self.cb_type(apply);actual=self.adapter.apply(base,n,p,cb)
   assert actual==1 and len(applied)==1 and not errors,(actual,applied,errors)
   after=bytes(g.mem_read(base,n));assert after==before
   row=dict(phase=self.active,ordinal=182,kernel_entry='800000b6',original_return_site=f'{ret:08x}',stack12_hex=bytes(g.mem_read(sp+4,12)).hex(),registers_before=regs,base=f'{base:08x}',size=n,original_protection=f'{p:08x}',compatibility_derived_pte_mask=f'{out[0]:08x}',compatibility_patched_xbox=f'{out[1]:08x}',compatibility_windows_flags=f'{out[2]:08x}',compatibility_host_access=f'{out[3]:08x}',applied_access=applied[0],regions_before=regions_before,contents_before_sha256=hashlib.sha256(before).hexdigest(),contents_after_sha256=hashlib.sha256(after).hexdigest(),void_abi_eax_unmodified=True,actual_platform_application_completed=True,scope='compiled unchanged pinned compatibility conversion and host-facing UpdateMemoryPermissions; actual Unicorn.mem_protect; no Xbox cache/PTE/MM parity')
   self.access_calls.append(row)
   if self.post_prepare_source_only:self.new_kernel_stops=[]
   g.reg_write(UC_X86_REG_ESP,sp+16);g.reg_write(UC_X86_REG_EIP,ret)
   assert self.registers()==regs
   row['registers_after']=self.registers();row['returned_stack_pointer']=f'{sp+16:08x}'
   return
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.map_enabled and self.track_players and ((0x4ed650<=address<0x4ed66c)or(0x4eb130<=address<0x4ed648)):
   self.protection_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)

def execute(data,replay,adapter):
 t=prepare(data,trace_type=lambda d:ProtectionTrace(d,replay,adapter))
 t.removed_post_prepare_services={f'{a:08x}':dict(name=name,return_value=f'{r&0xffffffff:08x}'if r is not None else None,stack_cleanup=pop)for a,(name,r,pop)in t.services.items()};t.services.clear();t.post_prepare_source_only=True
 for name,entry in [('source_control_storage_record20',0x1874d0),('source_view_storage_record21',0x1ea3d0),('source_camera_storage_record35',0x156a30),('source_control_newmap_record20',0x187520),('source_view_newmap_record21',0x1ea410),('source_camera_newmap_record35',0x156ad0),('source_network_configuration_default_handler',0x663b0),('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0),('source_readiness_gated_simulation_scheduler',0x68300),('source_random_storage_record3',0x1479b0),('source_callback13_storage',0x19f160)]:
  if not t.pool_action(name,entry):return t
 t.pool_action('source_complete_map_ready_owner',0x190a30,regs=((UC_X86_REG_EAX,t.u(0x4edf18)+8),))
 assert [r['original_protection']for r in t.access_calls]==['00000004','00000004','00000404']
 assert [c['index']for c in t.map_callbacks]==[2,9,13,16,17,18]
 assert [c['index']for c in t.callback_states if c.get('returned')]==[2,9,13,16,17]
 assert t.pool_stages[-1]['completion']=='blocked_original_instruction'and t.pool_stages[-1]['stopped_eip']=='001e2eb2'and t.new_invalid['address']=='00000000'
 assert t.u(0x4fcdc4)==0
 assert t.u(0x4ed664)==0x82d47000 and t.u(t.u(0x4edf18)+0x1120,1)==0
 assert t.u(t.u(0x4d6d44)+0x18)==2
 assert t.extra_replay_cursor==len(t.extra_replay)
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 adapter=build_adapter();replay=json.loads(CLOCK_FIXTURE.read_text())
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n');t=execute(data,replay,adapter)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();proc.stderr.close()
 assert proc.returncode==0
 print(json.dumps(dict(applied_protections=t.access_calls,callback_indices=[r['index']for r in t.map_callbacks],stop=t.pool_stages[-1]['invalid'])),flush=True)
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 def emit(name,ins):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in t.executed for i in ins))
 whole=emit('executed-original-instructions.asm',[t.code.at(a)for a in sorted(t.executed)])
 for e in ENTRIES:
  r=_trace(t.code,e,t.starts|set(ENTRIES));listings.append(dict(entry=f'{e:08x}',**emit(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)]),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call']))
 stores=emit('observed-map-protection-source-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for r in t.pool_stores+t.local_stores+t.input_stores+t.player_stores+t.protection_stores})])
 primary={name:dict(path=rel,sha256=h,url=f'https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/blob/{COMMIT}/{rel}')for name,(rel,h)in SOURCES.items()}
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,primary_sources=primary,compiled_compatibility_access=t.compiled_metadata,stages=t.pool_stages,pool_calls=t.pool_calls,pool_stores=t.pool_stores,local_stores=t.local_stores,input_stores=t.input_stores,player_stores=t.player_stores,protection_stores=t.protection_stores,input_calls=t.input_calls,map_newmap_callbacks=t.map_callbacks,callback_before_after_states=t.callback_states,extra_clock_calls=t.extra_clock_calls,inherited_clock_calls=t.clock_calls,preparation=t.producers,services=t.events,physical_services=t.physical_services,platform=t.platform_input,compatibility_lock_calls=t.lock_calls,source_raw_impact_pool_allocations=t.os_pool_allocs,source_requested_host_storage=t.host_raw_storage,applied_memory_protection=t.access_calls,removed_post_prepare_services=t.removed_post_prepare_services,instruction_listings=listings,instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),executed_original_listing=whole,observed_map_protection_store_listing=stores,executed_original_addresses_verified=len(t.executed),final_local_state=t.local_state(),final_pool_state=t.pool_state(),final_callback16_globals=dict(arena_globals1c_hex=bytes(t.g.mem_read(0x4ed650,0x1c)).hex(),metadata1288_hex=bytes(t.g.mem_read(t.u(0x4ed664),0x1288)).hex(),callback_globals10_hex=bytes(t.g.mem_read(0x4eb130,0x10)).hex(),cleared_callback_data2510_sha256=hashlib.sha256(bytes(t.g.mem_read(0x4eb138,0x2510))).hexdigest()),instruction_patches=[],new_supplied_gameplay_or_physics_results=[],summary=dict(cases=1,applied_compatibility_protection_requests=3,complete_callback16=1,complete_callback17_on_inherited_clock_backing=1,map_ready=0,source_local_bindings=0,exact_dependency_stop=t.pool_stages[-1]['invalid']),boundaries=[
 'All earlier freezes remain byte-identical. Existing selected source preparation/cache/options delivery, real host complete compatibility context and raw allocation boundaries are inherited explicitly; this is not whole-title startup.',
 'Original import417BA8 contains encoded ordinal182. Original2D7A1A tail-jumps with BaseAddress/NumberOfBytes/NewProtect and ret0Ch on zero size. Actual125370 caller requests3BE000 at82D47000 protection4 then40000 at83105000 protection404. Void ABI is independently established by pinned primary kernel declaration and original caller; no EAX status is supplied.',
 'The adapter compiles unchanged primary compatibility ConvertXboxToSystemPtePermissions, PatchXboxPermissions, ConvertXboxToWinPermissions and UpdateMemoryPermissions bodies. Only compilation types/constants/logging and VirtualProtect transport are harness boundaries. The compiled request validator outputs original compatibility-derived PTE masks for evidence, without writing guest PTE records. Full VMManager::Protect/virtual memory manager state is not executed.',
 'Pinned compatibility semantics add execute access and deliberately omit cache modifiers from host access protection. For4: derivedPTE63/patched40/Windows40/access40. For404: derivedPTE6B/patched440/Windows440/access40. The real VirtualProtect transport applies Unicorn.mem_protect permissions7 to existing pages and checks all overlapping region permissions plus unchanged complete content hashes before completing the void call.',
 'Applied access protection is a declared emulator platform service, not original Xbox cache, hardware page tables, OS virtual-memory state or exact cache/write-combine behavior. No page table, game field, object, pose, clock, membership, profile, local binding, readiness or successful gameplay result is supplied. The compatibility-provided access semantics are sufficient for this bounded unchanged caller only.',
 'The source-owned pre-existing game arena has exactly3FE000 bytes. No storage enlargement or relocation occurs; both map-reset ranges partition that retained arena. Preparation protection also uses the applied adapter rather than its inherited return-only observer. Host addresses and context records remain separately retained and may vary; deterministic source/request reports do not include those addresses.',
 'Full190A30 advances beyond both actual protection calls and the original callback16 flag/data clearing. Earlier actual125280 tail owns4ED664 metadata pointer82D47000 and1288-byte reservation; original125370 clears and fills that complete record. The excluded217090 savegame/thread I/O remains an inherited boundary; no I/O success is supplied.',
 'The unchanged ascending reset sequence is2,9,13,16,17,18. Callback17/147D20 executes on the inherited declared legacy clock backing01016000, whose complete24-byte before/after records are captured; its147CC0 owner is statically listed but not added here. This does not close upstream game-clock ownership or manufacture a clock value. Existing measured platform clock fixtures replay verbatim; there are no new clock inputs.',
 'Callback18/1E2EA0 reaches1E2EB2 repstosd toNULL because4FCDC4 isNULL. Actual owning callback18 constructor1E2E50 is statically listed: reserveE4 from the existing original game arena, checksum1656A0, clear39dwords and bind4FCDC4 at1E2E92. It is not executed in this freeze; no record pointer or admitted reset result is supplied.',
 '1395B0 membership,11D780 BSP readiness,190A6E ready writer and ordinary local control/device scheduling remain unexecuted. The actual source player pool remains empty; this selected fixture does not establish lobby/menu/countdown flow or actual local launch participant count. No ownership/table/readiness/admitted packet is supplied.'])
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 (args.output.parent/'raw-host-allocator-storage.json').write_text(json.dumps(t.host_raw_inputs,indent=2,sort_keys=True)+'\n')
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),oracle_sha256=out['oracle_sha256'],executed=len(t.executed),manifest=out['instruction_manifest_sha256'])),flush=True)
if __name__=='__main__':main()
