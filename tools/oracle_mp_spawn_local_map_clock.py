#!/usr/bin/env python3
"""Unchanged compatibility timer producer at the next original map clock read."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP
from oracle_mp_spawn_local_map_storage import StorageTrace,FROZEN as OLDER,ENTRIES as PRIOR_ENTRIES,OUT as STORAGE_OUT
from oracle_mp_spawn_local_map_protection import build_adapter
from oracle_mp_spawn_local_map_pools import CLOCK_FIXTURE
from oracle_mp_spawn_player_visibility import prepare,ROOT,CONTEXT_OUT,digest
from oracle_mp_spawn_complete_kpcr import body,S
from oracle_smg_firing_1_1 import PIN,CACHE_PIN,Trace as SourceTrace
from functions import _trace
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/local-map-clock'
PRIMARY=OUT/'primary-source'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_local_map_storage.py':'3850f43af06128dc8e5761928eb195c86497adaf31cd211b70588b519bc851db',
 'docs/MP_LOCAL_MAP_STORAGE_RESEARCH.md':'4bb4f46ba1c5df9f45cfc229ad6a38dfa14557184d148a2749fc70e3106faead',
 'analysis/multiplayer/integration/spawn-source/local-map-storage/report.json':'cec8b8aa5b8a5c78ecbb09479247e0779b5021dfecf80f24816f743c650c4524',
 'analysis/multiplayer/integration/spawn-source/local-map-storage/independent-freeze-verification.json':'9ba694c944bd4e5d1b9ae509310f9f6e86cad73576d7989601339d440370c967',
 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h':'7223ea0a9fb7e31e804008d5b0b1b3167908ab9c16d43fc2c8a67375c0f58f88',
 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h':'988e679c3e042bb9f0c3836a77d3aee24117e83683da9cf839ba2a84c7fe32fb'}
_prior=json.loads((STORAGE_OUT/'report.json').read_text())
for _r in _prior['instruction_listings']+[_prior['executed_original_listing'],_prior['observed_map_storage_store_listing']]:FROZEN[str((STORAGE_OUT/'instructions'/_r['file']).relative_to(ROOT))]=_r['sha256']
SOURCE_PINS={
 'EmuKrnlKe.cpp':'3cfc332ef4f9bd2e38214af13c8a91a778bf77aba6884ef3a257b28f51b1e18c',
 'EmuKrnlKi.cpp':'cc3144046c4741ac531d0f79b075d887ffe87abcb38a99c581e9ab5aa2482960',
 'EmuKrnlKe.h':'74ddc31cc3f2b945d2cf5a0fcff94fd08835af009c59cfb870b4150cb4bfc034',
 'EmuKrnlKi.h':'fe59f62b389a9c3cb3a283508d51a75e1644adfe9f4b1a7a2027db9d87c605a8',
 'Timer.cpp':'b86dd9bdd1d1775386a070ed8efd5da624188f5907e8951df3d267e7572e684b',
 'Timer.h':'645d25d68995be453574b9cc57ff234b0989266ba28f266f62bf61ebb58b05a6',
 'ps.h':'c1c9eea185c798d398527c8df2bded16b996341d77537ac95cf7e2730f6dd74f',
 'kernel.h':'8657b9dc02fe483512fca873db7d874631778e7d017b6da05d6c2992310bbd7c',
 'xboxkrnl.h':'76af0b53f984acff8f6aedd912b9fe77c74817096dd2ed76d48269529e647d0b',
 'xbox.h':'38fdfb3d08a395beec78d6bc66006a47b1a2251ed0a53cdee01864e2f48cfdfc',
 'EmuKrnlPs.hpp':'990a6a4d577590fbf0c895b1133d0b5ce39bb5598dfef9ab03c7de01503d04a4'}
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+(0x139880,0x7b540,0x337a47,0x68e87,0x123b0,0x169d0,0x3368a0,0x3368c9,0x190ba0,0x190bf0,0x23855a,0x6f190)))

def build_timer():
 for p,h in SOURCE_PINS.items():assert digest(PRIMARY/p)==h,p
 types=(S/'Cxbx-Reloaded-types.h').read_text();ki=(PRIMARY/'EmuKrnlKi.cpp').read_text();ke=(PRIMARY/'EmuKrnlKe.cpp').read_text();timer=(PRIMARY/'Timer.cpp').read_text();kih=(PRIMARY/'EmuKrnlKi.h').read_text()
 decl=[]
 def st(name):
  m=re.search(r'typedef (?:struct|union) _'+name+r'\s*\{.*?\}\s*(?://[^\n]*\n)?'+name+r'[^;]*;',types,re.S);assert m,name;return m.group(0)
 def td(name):
  m=re.search(r'typedef (?:void_xt|boolean_xt)\s*\((?:NTAPI )?\*'+name+r'\)\s*\(.*?\);',types,re.S);assert m,name;return m.group(0)
 for n in('LIST_ENTRY','DISPATCHER_HEADER','LARGE_INTEGER','ULARGE_INTEGER','KPROCESS','KSYSTEM_TIME','KTIMER','KDEVICE_QUEUE'):decl.append(st(n))
 decl.append(td('PKDEFERRED_ROUTINE'));decl.append(st('KDPC'));decl.append(td('PKSERVICE_ROUTINE'))
 decl.append(re.search(r'typedef enum _KINTERRUPT_MODE\s*\{.*?\}\s*KINTERRUPT_MODE;',types,re.S).group(0))
 decl.append(st('KINTERRUPT'))
 for n in('PIDE_INTERRUPT_ROUTINE','PIDE_FINISHIO_ROUTINE','PIDE_POLL_RESET_COMPLETE_ROUTINE','PIDE_TIMEOUT_EXPIRED_ROUTINE','PIDE_START_PACKET_ROUTINE','PIDE_START_NEXT_PACKET_ROUTINE'):decl.append(td(n))
 decl.append(st('IDE_CHANNEL_OBJECT'))
 for n in('KTIMER_TABLE_ENTRY','KI_TIMER_LOCK'):
  decl.append(re.search(r'typedef struct _'+n+r'\s*\{.*?\}\s*'+n+r'[^;]*;',kih,re.S).group(0))
 macros=[]
 for name,src in [('X_THREAD_QUANTUM',(PRIMARY/'ps.h').read_text()),('TIMER_TABLE_SIZE',kih),('DISPATCH_SIZE',types)]:macros.append(next(x for x in src.splitlines()if x.startswith('#define '+name+' ')))
 macro=next(x for x in (S/'XboxDev-xboxkrnl.h').read_text().splitlines()if x.startswith('#define InitializeListHead('))
 dpcenum=re.search(r'typedef enum _KOBJECTS\s*\{.*?\}\s*KOBJECTS[^;]*;',types,re.S).group(0)
 source='''#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <vector>
#include <atomic>
#include <mutex>
#define LOG_FUNC_BEGIN
#define LOG_FUNC_ARG(x)
#define LOG_FUNC_END
#define RESTRICTED_POINTER
'''+macro+'\n'+'\n'.join(macros)+'''
namespace xbox {
typedef void void_xt; typedef unsigned char uchar_xt,boolean_xt,byte_xt,KIRQL;
typedef char char_xt; typedef short cshort_xt; typedef int long_xt;
typedef unsigned int ulong_xt,dword_xt; typedef long long longlong_xt;
typedef unsigned long long ulonglong_xt; typedef void* PVOID;
struct _DEVICE_OBJECT; typedef _DEVICE_OBJECT* PDEVICE_OBJECT;
'''+ '\n'.join(decl)+'\n'+dpcenum+'''
static_assert(sizeof(void*)==4 && sizeof(KPROCESS)==0x1c && sizeof(KDPC)==0x1c,"original private platform types");
static_assert(sizeof(KSYSTEM_TIME)==12 && sizeof(KTIMER_TABLE_ENTRY)==16,"original private clock/table types");
void NTAPI KiTimerExpiration(PKDPC,PVOID,PVOID,PVOID) { std::abort(); }
void NTAPI KeSetSystemTime(PLARGE_INTEGER,PLARGE_INTEGER) { std::abort(); }
boolean_xt NTAPI KeInsertQueueDpc(PKDPC,PVOID,PVOID) { std::abort(); }
void KiInitSystem();
void KiClockIsr(ulonglong_xt);
void NTAPI KeInitializeDpc(KDPC*,PKDEFERRED_ROUTINE,PVOID);
std::atomic_flag KeSystemTimeChanged;
KSYSTEM_TIME KeInterruptTime = {0,0,0};
KSYSTEM_TIME KeSystemTime = {0,0,0};
volatile dword_xt KeTickCount = 0;
KPROCESS KiUniqueProcess;
const ulong_xt CLOCK_TIME_INCREMENT = 0x2710;
KDPC KiTimerExpireDpc;
KI_TIMER_LOCK KiTimerMtx;
KTIMER_TABLE_ENTRY KiTimerTableListHead[TIMER_TABLE_SIZE];
LIST_ENTRY KiWaitInListHead;
IDE_CHANNEL_OBJECT IdexChannelObject;
}
struct Observation { unsigned kind; long long value; };
static std::vector<Observation> observed,replay;
static size_t replay_at; static bool replay_mode;
static void Observe(unsigned kind,long long* value) {
 if(replay_mode) { assert(replay_at<replay.size()); auto row=replay[replay_at++];assert(row.kind==kind);*value=row.value; }
 observed.push_back({kind,*value});
}
static BOOL ObservedQPF(::LARGE_INTEGER* out) {
 if(!replay_mode && !::QueryPerformanceFrequency(out)) std::abort(); Observe(1,&out->QuadPart); return TRUE;
}
static BOOL ObservedQPC(::LARGE_INTEGER* out) {
 if(!replay_mode && !::QueryPerformanceCounter(out)) std::abort(); Observe(2,&out->QuadPart); return TRUE;
}
static void ObservedFileTime(LPFILETIME out) {
 long long value=0;if(!replay_mode) {::GetSystemTimeAsFileTime(out);std::memcpy(&value,out,8);}Observe(3,&value);std::memcpy(out,&value,8);
}
#define QueryPerformanceFrequency ObservedQPF
#define QueryPerformanceCounter ObservedQPC
#define GetSystemTimeAsFileTime ObservedFileTime
static std::atomic_uint64_t last_qpc;
static std::atomic_uint64_t exec_time;
static uint64_t pit_last;
static uint64_t pit_last_qpc;
int64_t HostQPCFrequency,HostQPCStartTime;
uint64_t get_now();
'''
 for sig,src,pattern in [
 ('void timer_init()',timer,r'void timer_init\(\)'),
 ('uint64_t get_now()',timer,r'uint64_t get_now\(\)'),
 ('static uint64_t pit_next(uint64_t now)',timer,r'static uint64_t pit_next\(uint64_t now\)'),
 ('xbox::void_xt xbox::KiInitSystem()',ki,r'xbox::void_xt xbox::KiInitSystem\(\)'),
 ('xbox::void_xt NTAPI xbox::KeInitializeDpc(xbox::KDPC* Dpc,xbox::PKDEFERRED_ROUTINE DeferredRoutine,xbox::PVOID DeferredContext)',ke,r'xbox::KeInitializeDpc\s*\([^)]*\)'),
 ('xbox::void_xt xbox::KiClockIsr(xbox::ulonglong_xt TotalUs)',ki,r'xbox::void_xt xbox::KiClockIsr\(ulonglong_xt TotalUs\)')]:source+=sig+'\n'+body(src,pattern)+'\n'
 source+='''
static void Hex(const void* p,size_t n) { const auto* b=(const unsigned char*)p;for(size_t i=0;i<n;++i)std::printf("%02x",b[i]); }
static void Report(const char* phase,uint64_t now,uint64_t next) {
 std::printf("{\\\"phase\\\":\\\"%s\\\",\\\"now_us\\\":%llu,\\\"next_us\\\":%llu,\\\"clock4_hex\\\":\\\"",phase,(unsigned long long)now,(unsigned long long)next);Hex((const void*)&xbox::KeTickCount,4);
 std::printf("\\\",\\\"interrupt12_hex\\\":\\\"");Hex(&xbox::KeInterruptTime,12);std::printf("\\\",\\\"system12_hex\\\":\\\"");Hex(&xbox::KeSystemTime,12);
 std::printf("\\\",\\\"last_qpc\\\":%llu,\\\"exec_time_us\\\":%llu,\\\"pit_last_us\\\":%llu,\\\"pit_last_qpc\\\":%llu,\\\"qpc_frequency\\\":%lld,\\\"qpc_start\\\":%lld,\\\"timer_lock_acquired\\\":%d,\\\"system_time_changed\\\":%s,\\\"process_quantum\\\":%u,\\\"process_stack_count\\\":%u,\\\"timer_table\\\":[",(unsigned long long)last_qpc.load(),(unsigned long long)exec_time.load(),(unsigned long long)pit_last,(unsigned long long)pit_last_qpc,(long long)HostQPCFrequency,(long long)HostQPCStartTime,xbox::KiTimerMtx.Acquired,xbox::KeSystemTimeChanged.test()?"true":"false",xbox::KiUniqueProcess.ThreadQuantum,xbox::KiUniqueProcess.StackCount);
 for(unsigned i=0;i<TIMER_TABLE_SIZE;++i) {auto& t=xbox::KiTimerTableListHead[i];std::printf("%s{\\\"index\\\":%u,\\\"forward_self\\\":%s,\\\"reverse_self\\\":%s,\\\"time8_hex\\\":\\\"",i?",":"",i,t.Entry.Flink==&t.Entry?"true":"false",t.Entry.Blink==&t.Entry?"true":"false");Hex(&t.Time,8);std::printf("\\\"}");}
 std::printf("],\\\"observations\\\":[");for(size_t i=0;i<observed.size();++i)std::printf("%s{\\\"kind\\\":%u,\\\"value\\\":%lld}",i?",":"",observed[i].kind,observed[i].value);
 std::printf("],\\\"private_native_records\\\":{\\\"process1c_hex\\\":\\\"");Hex(&xbox::KiUniqueProcess,sizeof(xbox::KiUniqueProcess));
 std::printf("\\\",\\\"timer_dpc1c_hex\\\":\\\"");Hex(&xbox::KiTimerExpireDpc,sizeof(xbox::KiTimerExpireDpc));
 std::printf("\\\",\\\"wait_head8_hex\\\":\\\"");Hex(&xbox::KiWaitInListHead,sizeof(xbox::KiWaitInListHead));
 std::printf("\\\",\\\"ide_channel_hex\\\":\\\"");Hex(&xbox::IdexChannelObject,sizeof(xbox::IdexChannelObject));
 std::printf("\\\",\\\"timer_table200_hex\\\":\\\"");Hex(&xbox::KiTimerTableListHead,sizeof(xbox::KiTimerTableListHead));
 std::printf("\\\",\\\"host_cpp_timer_mutex_hex\\\":\\\"");Hex(&xbox::KiTimerMtx,sizeof(xbox::KiTimerMtx));
 std::printf("\\\"}}\\n");std::fflush(stdout);observed.clear();
}
int main() {
 char op;while(std::scanf(" %c",&op)==1) {
  if(op=='x')return 0;
  if(op=='r') {unsigned n;assert(std::scanf("%u",&n)==1);replay.clear();replay_at=0;for(unsigned i=0;i<n;++i){Observation o;assert(std::scanf("%u%lld",&o.kind,&o.value)==2);replay.push_back(o);}replay_mode=true;assert(std::scanf(" %c",&op)==1);}else replay_mode=false;
  if(op=='i') {timer_init();xbox::KiInitSystem();Report("initialize",0,0);}
  else if(op=='p') {auto now=get_now();auto next=pit_next(now);Report("update",now,next);}
  else std::abort();
  assert(!replay_mode || replay_at==replay.size());
 }
}
'''
 p=OUT/'compiled-timer.cpp';p.write_text(source);exe=OUT/'compiled-timer.exe'
 cmd=['i686-w64-mingw32-g++','-std=c++20','-O0','-static','-Wl,--no-insert-timestamp',str(p),'-o',str(exe)]
 result=subprocess.run(cmd,capture_output=True,text=True);(OUT/'timer-compiler.txt').write_text(result.stderr);assert result.returncode==0,result.stderr
 return dict(source_sha256=digest(p),executable_sha256=digest(exe),primary_sources=SOURCE_PINS,private_platform_abi='actual32-bit Wine C++ types and synchronization; no game record layout supplied',unavailable_branches_abort=['KiTimerExpiration','KeSetSystemTime','KeInsertQueueDpc'],original_bodies=['timer_init','get_now','pit_next','KiInitSystem','KeInitializeDpc','KiClockIsr'])

class NativeTimer:
 def __init__(self,replay=None):
  self.replay=replay;self.rows=[];self.native_raw_records=[];self.p=subprocess.Popen(['wine',str(OUT/'compiled-timer.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=(OUT/'timer-runtime-stderr.txt').open('w'),text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
  self.request('i')
 def request(self,op):
  if self.replay is None:self.p.stdin.write(op+'\n')
  else:
   old=self.replay[len(self.rows)];assert old['phase']==('initialize'if op=='i'else'update');obs=old['observations'];self.p.stdin.write('r '+str(len(obs))+'\n'+''.join(f"{r['kind']} {r['value']}\n"for r in obs)+op+'\n')
  self.p.stdin.flush();line=self.p.stdout.readline();assert line,line;r=json.loads(line);self.native_raw_records.append(dict(phase=r['phase'],**r.pop('private_native_records')));self.rows.append(r)
  if self.replay is not None:assert r==old
  return r
 def close(self):
  self.p.stdin.write('x\n');self.p.stdin.flush();self.p.stdin.close();self.p.wait(timeout=60);self.p.stdout.close();assert self.p.returncode==0

class ClockTrace(StorageTrace):
 def __init__(self,data,replay,adapter,timer):
  self.native_timer=timer;self.new_clock_calls=[];self.new_clock_returns=[]
  super().__init__(data,replay,adapter)
 def hook(self,g,a,size,user):
  if self.post_prepare_source_only and self.active=='source_complete_map_ready_owner':
   if a==0x337a40:
    self.executed.add(a);SourceTrace.hook(self,g,a,size,user);return
   if a==0x337a45:
    sp=g.reg_read(UC_X86_REG_ESP);p=g.reg_read(UC_X86_REG_EAX);r=self.native_timer.request('p');before=bytes(g.mem_read(p,4)).hex();raw=bytes.fromhex(r['clock4_hex']);g.mem_write(p,raw)
    self.new_clock_calls.append(dict(phase=self.active,instruction=f'{a:08x}',return_address=f'{self.u(sp):08x}',registers_before=self.registers(),stack40_hex=bytes(g.mem_read(sp,0x40)).hex(),source_import4_hex=self.image.read(0x417cd4,4).hex(),runtime_import4_hex=bytes(g.mem_read(0x417cd4,4)).hex(),provider_backing=f'{p:08x}',backing4_before_hex=before,backing4_after_hex=raw.hex(),native_timer_row=len(self.native_timer.rows)-1,body_owned_time=True))
    self.executed.add(a);SourceTrace.hook(self,g,a,size,user);return
   if a==0x337a47:
    self.new_clock_returns.append(dict(instruction=f'{a:08x}',registers=self.registers(),eax4_hex=g.reg_read(UC_X86_REG_EAX).to_bytes(4,'little').hex()))
  super().hook(g,a,size,user)

def execute(data,replay,adapter,timer):
 t=prepare(data,trace_type=lambda d:ClockTrace(d,replay,adapter,timer))
 t.removed_post_prepare_services={f'{a:08x}':dict(name=name,return_value=f'{r&0xffffffff:08x}'if r is not None else None,stack_cleanup=pop)for a,(name,r,pop)in t.services.items()};t.services.clear();t.post_prepare_source_only=True
 for name,entry in [('source_control_storage_record20',0x1874d0),('source_view_storage_record21',0x1ea3d0),('source_camera_storage_record35',0x156a30),('source_control_newmap_record20',0x187520),('source_view_newmap_record21',0x1ea410),('source_camera_newmap_record35',0x156ad0),('source_network_configuration_default_handler',0x663b0),('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0),('source_readiness_gated_simulation_scheduler',0x68300),('source_random_storage_record3',0x1479b0),('source_callback13_storage',0x19f160),('source_clock_storage_record17',0x147cc0),('source_callback18_storage',0x1e2e50)]:
  if not t.storage_action(name,entry):return t
 t.storage_action('source_complete_map_ready_owner',0x190a30,regs=((UC_X86_REG_EAX,t.u(0x4edf18)+8),))
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--clock-replay',type=Path);args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 meta=build_timer();adapter=build_adapter();replay=json.loads(CLOCK_FIXTURE.read_text());timer=NativeTimer(json.loads(args.clock_replay.read_text())if args.clock_replay else None)
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=(args.output.parent/'context-stderr.txt').open('w'),text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n');t=execute(data,replay,adapter,timer)
 finally:
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60);proc.stdout.close();timer.close()
 assert proc.returncode==0
 print(json.dumps(dict(callbacks=[dict(index=r['index'],target=r['target'])for r in t.map_callbacks],new_clock_calls=t.new_clock_calls,stop={k:v for k,v in t.pool_stages[-1]['invalid'].items()if k!='current_storage_state'})),flush=True)
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 def emit(name,ins):
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes and bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in t.executed for i in ins))
 whole=emit('executed-original-instructions.asm',[t.code.at(a)for a in sorted(t.executed)])
 for e in ENTRIES:
  r=_trace(t.code,e,t.starts|set(ENTRIES));listings.append(dict(entry=f'{e:08x}',**emit(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)]),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call']))
 stores=emit('observed-map-clock-source-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for r in t.storage_stores+t.pool_stores+t.local_stores+t.input_stores+t.player_stores+t.protection_stores})])
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,tool_sha256=digest(__file__),frozen_pins=FROZEN,compiled_compatibility_timer=meta,native_timer_records=timer.rows,new_clock_calls=t.new_clock_calls,new_clock_returns=t.new_clock_returns,stages=t.storage_stages,actual_source_owned_allocations=t.allocated_owned_records,storage_calls=t.storage_calls,storage_stores=t.storage_stores,pool_calls=t.pool_calls,pool_stores=t.pool_stores,local_stores=t.local_stores,input_stores=t.input_stores,player_stores=t.player_stores,protection_stores=t.protection_stores,map_newmap_callbacks=t.map_callbacks,callback_before_after_states=t.callback_states,applied_memory_protection=t.access_calls,owner_dependency_stops=t.owner_stops,extra_clock_calls=t.extra_clock_calls,inherited_clock_calls=t.clock_calls,preparation=t.producers,services=t.events,physical_services=t.physical_services,platform=t.platform_input,compatibility_lock_calls=t.lock_calls,post_prepare_platform_storage_stores=t.platform_storage_stores,source_raw_impact_pool_allocations=t.os_pool_allocs,source_requested_host_storage=t.host_raw_storage,removed_post_prepare_services=t.removed_post_prepare_services,final_owning_state=t.callback_state(),instruction_listings=listings,instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),executed_original_listing=whole,observed_map_clock_store_listing=stores,executed_original_addresses_verified=len(t.executed),instruction_patches=[],new_supplied_gameplay_or_physics_results=[],boundaries=[
 'All prior pins remain unchanged. Exact pinned primary compatibility timer initialization/update bodies execute as a separate actual32-bit Wine process with its genuine host QueryPerformanceFrequency, QueryPerformanceCounter, GetSystemTimeAsFileTime and C++ synchronization. Actual requested raw API observations are retained and may replay verbatim; no timestamp is chosen.',
 'Source static definitions initializeKeTickCount/KeInterruptTime/KeSystemTime tozero; whole timer_init owns QPC frequency/start/last/pit/system-time state, then whole KiInitSystem/KeInitializeDpc own every private timer-table list/time/process/queue record. Whole get_now/pit_next/KiClockIsr update the clock from genuinely observed elapsed performance-counter time. All32 table entries remain source-empty; any unimplemented time-change/DPC-expiry branch aborts instead of providing success.',
 'The produced literal4-byte KeTickCount is transported into the already declared ordinal156 guest backing only at the new map-reset read. The original337A40/337A45 load and337A47return execute unchanged and retain exact input/outputregisters,stack and buffer. This is a declared compatibility data transport, not Xboxhardware clocks or full Cxbxsystem_events scheduling.',
 'Earlier prepared raw clock providers retain their prior measured replay boundary and timebases. This new compatibility timer begins with its source staticzero; complete title clock continuity or all ordinary-clock consumers are not claimed. Source options/Game+0E remains its original inheritedzero; no30Hz or readiness is assigned.',
 'Original139880 options-initializer and7B540 random-ID consumer are statically listed for the nonzero-rate owner; these are not direct-called to overwrite a multiplayer/session record. Their complete owning lobby/options/randomness delivery remains separate.',
 'Full190A30 preserves original callback order and stops at the first next unresolved source dependency. No local identity/profile/member/user binding, sessionstate4, admitted command, selectedcontact/pose or successfulworld result is supplied. Complete before/after state and actual callback returns remain in the report.'])
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 (args.output.parent/'measured-native-clock-inputs.json').write_text(json.dumps(timer.rows,indent=2,sort_keys=True)+'\n')
 (args.output.parent/'raw-native-private-clock-records.json').write_text(json.dumps(timer.native_raw_records,indent=2,sort_keys=True)+'\n')
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),tool_sha256=out['tool_sha256'],manifest=out['instruction_manifest_sha256'],executed=len(t.executed))),flush=True)
if __name__=='__main__':main()
