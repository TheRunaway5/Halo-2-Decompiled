#!/usr/bin/env python3
"""Separate complete pinned compatibility KPCR producer with real Wine32 OS.

The actual host-thread template branch is extracted unchanged; primary startup
calls it before KiInitSystem, with the source static KiUniqueProcess. No partial
Prcb+250 NULL fixture is supplied. Platform snapshots remain compatibility ABI.
"""
import argparse,hashlib,json,re,subprocess,os,struct
from pathlib import Path
from oracle_mp_spawn_host_lock_order import ROOT,OUT as LOCK_OUT,FROZEN as OLDER,digest,HostLockTrace,ENTRIES,PIN,CACHE_PIN
from oracle_mp_spawn_biped_platform import GDT,segment
from oracle_smg_firing_1_1 import STACK
from unicorn.x86_const import UC_X86_REG_FS,UC_X86_REG_FS_BASE,UC_X86_REG_FPCW
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr'
S=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_host_lock_order.py':'ae50e2c243d1a534008fd187f62ad8eaea3e0503fc7048b9708866d133411500',
 'docs/MP_HOST_THREAD_LOCK_ORDER_RESEARCH.md':'6fabc0d6e2d387cbc536864debf8b340a6e8914d2cce1569995167740e640396',
 'analysis/multiplayer/integration/spawn-source/host-lock-new-map-order/report.json':'4cff6712198ba763550115d184030d25f69eeb46b414b2a5e73f229e05d2a7f8'}
SOURCE_PINS={
 'EmuFS.h':'3bc888a0a8bdd2376f12d0fa22b3612c1ba4c261cd8ad91e18f365b7bfdf53ab',
 'EmuKrnlKi.h':'fe59f62b389a9c3cb3a283508d51a75e1644adfe9f4b1a7a2027db9d87c605a8',
 'EmuKrnlKe.h':'74ddc31cc3f2b945d2cf5a0fcff94fd08835af009c59cfb870b4150cb4bfc034',
 'Xbe.h':'c4ea8f5e113b1bc3fef8d2c7df5dad53f9df7d39b8a577e93346061eddd069ba',
 'EmuKrnl.h':'1682369c480a5a8751cc696eae8d600394390652b6bf2341d096b424ac804448',
 'NativeHandle.h':'e31db3a884ed30259f6e96e3c74e2bd36f01de0d8d4eac17f8b4ea900ae2444f',
 'CxbxKrnl.h':'7cebac11841c538365f6ee5b01144094f81bb12490a1b1d3c042b6311a8097f8',
 'CxbxUtil.h':'fd682d8bb1d57c1af17caaa6f1180cbb39711ba0c181132f9f297bbed19c3b71',
 'CxbxKrnl.cpp':'998667dfdec7258e7a40d17eda49dca24d904cb85b8171bf30d409a81e15e3ea'}

def body(text,pattern):
 m=re.search(pattern,text,re.S);assert m,pattern
 begin=text.index('{',m.end());depth=1;end=begin+1
 while depth:
  if text[end]=='{':depth+=1
  elif text[end]=='}':depth-=1
  end+=1
 return text[begin:end]

def build():
 for p,h in SOURCE_PINS.items():assert digest(OUT/p)==h,p
 old=json.loads((LOCK_OUT/'report.json').read_text())
 for r in old['compiled_compatibility_locks']['primary_sources']:assert digest(S/r['saved'])==r['sha256']
 for r in old['inspected_primary_context_sources']:assert digest(LOCK_OUT/r['saved'])==r['sha256']
 types=(S/'Cxbx-Reloaded-types.h').read_text();fs=(LOCK_OUT/'EmuFS.cpp').read_text()
 ke=(S/'Cxbx-Reloaded-EmuKrnlKe.cpp').read_text();ki=(S/'Cxbx-Reloaded-EmuKrnlKi.cpp').read_text();rtl=(S/'Cxbx-Reloaded-EmuKrnlRtl.cpp').read_text()
 startup=(OUT/'CxbxKrnl.cpp').read_text();assert startup.index('EmuGenerateFS<true>')<startup.index('xbox::KiInitSystem();')
 decl=[]
 def struct_type(name):
  return re.search(r'typedef (?:struct|union) _'+name+r'\s*\{.*?\}\s*(?://[^\n]*\n)?'+name+r'[^;]*;',types,re.S).group(0)
 def typedef(name):
  return re.search(r'typedef void_xt \(NTAPI \*'+name+r'\)\s*\(.*?\);',types,re.S).group(0)
 for name in('LIST_ENTRY','DISPATCHER_HEADER','KSEMAPHORE','LARGE_INTEGER','ULARGE_INTEGER','NT_TIB','KPROCESS','KAPC_STATE','KTIMER'):decl.append(struct_type(name))
 decl.append('typedef struct _KTHREAD KTHREAD,*PKTHREAD;')
 for name in('PKSTART_ROUTINE','PKSYSTEM_ROUTINE','PKNORMAL_ROUTINE','PKKERNEL_ROUTINE','PKRUNDOWN_ROUTINE'):decl.append(typedef(name))
 for name in('KSTART_FRAME','KSWITCHFRAME','KWAIT_BLOCK','KAPC','KTHREAD','ETHREAD','KPRCB','KPCR'):decl.append(struct_type(name))
 decl.append('#pragma pack(push,1)\n'+struct_type('FLOATING_SAVE_AREA')+'\n#pragma pack(pop)\n'+struct_type('FX_SAVE_AREA'))
 for name in('MODE','WAIT_TYPE','TIMER_TYPE','KTHREAD_STATE','KOBJECTS'):
  decl.append(re.search(r'typedef enum _'+name+r'\s*\{.*?\}\s*'+name+r'[^;]*;',types,re.S).group(0))
 macros=[]
 macro_source=types+'\n'+(OUT/'CxbxKrnl.h').read_text()
 for name in('ALIGN_DOWN','ALIGN_UP','NPX_STATE_NOT_LOADED','X_EXCEPTION_CHAIN_END','PASSIVE_LEVEL','X_STATUS_TIMEOUT','XBOX_MEM_RESERVE','XBOX_MEM_COMMIT','XBOX_PAGE_READWRITE'):
  macros.append(next(x for x in macro_source.splitlines()if x.startswith('#define '+name+' ' )or x.startswith('#define '+name+'(')))
 macro=next(x for x in (S/'XboxDev-xboxkrnl.h').read_text().splitlines()if x.startswith('#define InitializeListHead('))
 source='''#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <malloc.h>
#include <float.h>
#undef RtlZeroMemory
#define LOG_FUNC_ONE_ARG(x)
#define LOG_FUNC_ONE_ARG_OUT(x)
#define LOG_FUNC_BEGIN
#define LOG_FUNC_ARG(x)
#define LOG_FUNC_ARG_OUT(x)
#define LOG_FUNC_END
#define LOG_FORWARD(x)
#define EmuLog(...)
#define RESTRICTED_POINTER
'''+macro+'\n'+'\n'.join(macros)+'''
namespace xbox {
typedef void void_xt; typedef unsigned char uchar_xt,boolean_xt,byte_xt;
typedef char char_xt,cchar_xt; typedef short cshort_xt; typedef unsigned short ushort_xt,word_xt;
typedef long long longlong_xt; typedef unsigned long long ulonglong_xt;
typedef int long_xt,ntstatus_xt; typedef unsigned int ulong_xt,addr_xt,dword_xt,ulong_ptr_xt;
typedef void* PVOID; typedef byte_xt* PBYTE; typedef cchar_xt KPROCESSOR_MODE;
constexpr ulong_xt zero=0; constexpr auto zeroptr=nullptr;
'''+ '\n'.join(decl)+'''
static_assert(sizeof(KTHREAD)==0x110,"source KTHREAD");
static_assert(sizeof(ETHREAD)==0x140,"source ETHREAD");
static_assert(sizeof(KPRCB)==0x258,"source KPRCB");
static_assert(sizeof(KPCR)==0x280,"source KPCR");
static_assert(sizeof(FX_SAVE_AREA)==0x210,"source FX_SAVE_AREA");
static_assert(sizeof(KSTART_FRAME)==12 && sizeof(KSWITCHFRAME)==12,"source frames");
static_assert(offsetof(KPCR,Prcb)==0x20 && offsetof(KPCR,PrcbData)==0x28,"source PRCB routing");
static_assert(sizeof(::NT_TIB)==sizeof(NT_TIB),"actual 32-bit host TIB copy size");
void NTAPI RtlZeroMemory(void* Destination,size_t Length);
ntstatus_xt NtAllocateVirtualMemory(void**,ulong_xt,ulong_xt*,ulong_xt,ulong_xt);
void NTAPI KeInitializeApc(PKAPC,PKTHREAD,PKKERNEL_ROUTINE,PKRUNDOWN_ROUTINE,PKNORMAL_ROUTINE,KPROCESSOR_MODE,PVOID);
void NTAPI KeInitializeSemaphore(KSEMAPHORE*,long_xt,long_xt);
void NTAPI KeInitializeTimerEx(PKTIMER,TIMER_TYPE);
void NTAPI KeInitializeTimer(PKTIMER);
void KiInitializeContextThread(PKTHREAD,ulong_xt,PKSYSTEM_ROUTINE,PKSTART_ROUTINE,PVOID);
void NTAPI KiSuspendNop(PKAPC,PKNORMAL_ROUTINE*,PVOID*,PVOID*,PVOID*);
void NTAPI KiSuspendThread(PVOID,PVOID,PVOID);
void NTAPI KiThreadStartup();
template<bool IsHostThread> void KeInitializeThread(PKTHREAD,PVOID,ulong_xt,ulong_xt,PKSYSTEM_ROUTINE,PKSTART_ROUTINE,PVOID,PKPROCESS);
void MmDeleteKernelStack(PVOID,PVOID);
UCHAR KeRaiseIrqlToDpcLevel(); void KfLowerIrql(UCHAR);
void InsertTailList(LIST_ENTRY*,LIST_ENTRY*);
}
// Exact primary global declaration; static storage is zero-initialized.
xbox::KPROCESS KiUniqueProcess;
namespace Xbe {
'''+re.search(r'struct TLS\s*\{.*?\}',(OUT/'Xbe.h').read_text(),re.S).group(0)+''';
}
// Unused host-branch TLS expression requires this declaration. No XBE/global
// instance or TLS default is supplied: source context producer sets TlsDataNULL.
struct XbeHostView { struct { uint32_t dwTLSAddr; } m_Header; };
XbeHostView* CxbxKrnl_Xbe=nullptr;
#define TIB_LinearSelfAddress 0x18
#define TIB_ArbitraryDataSlot 0x14
'''
 bodies={}
 entries=[('xbox::RtlZeroMemory',rtl,r'EXPORTNUM\(320\).*?RtlZeroMemory\s*\([^)]*\)', 'void NTAPI xbox::RtlZeroMemory(void* Destination,size_t Length)'),
 ('xbox::KeInitializeApc',ke,r'EXPORTNUM\(105\).*?KeInitializeApc\s*\([^)]*\)','void NTAPI xbox::KeInitializeApc(PKAPC Apc,PKTHREAD Thread,PKKERNEL_ROUTINE KernelRoutine,PKRUNDOWN_ROUTINE RundownRoutine,PKNORMAL_ROUTINE NormalRoutine,KPROCESSOR_MODE ApcMode,PVOID NormalContext)'),
 ('xbox::KeInitializeSemaphore',ke,r'EXPORTNUM\(112\).*?KeInitializeSemaphore\s*\([^)]*\)','void NTAPI xbox::KeInitializeSemaphore(KSEMAPHORE* Semaphore,long_xt Count,long_xt Limit)'),
 ('xbox::KeInitializeTimerEx',ke,r'EXPORTNUM\(113\).*?KeInitializeTimerEx\s*\([^)]*\)','void NTAPI xbox::KeInitializeTimerEx(PKTIMER Timer,TIMER_TYPE Type)'),
 ('xbox::KeInitializeTimer',ke,r'xbox::void_xt NTAPI xbox::KeInitializeTimer\s*\([^)]*\)','void NTAPI xbox::KeInitializeTimer(PKTIMER Timer)'),
 ('xbox::KiInitializeContextThread',ki,r'xbox::void_xt xbox::KiInitializeContextThread\s*\([^)]*\)','void xbox::KiInitializeContextThread(PKTHREAD Thread,ulong_xt TlsDataSize,PKSYSTEM_ROUTINE SystemRoutine,PKSTART_ROUTINE StartRoutine,PVOID StartContext)'),
 ('xbox::KeInitializeThread',ke,r'xbox::void_xt xbox::KeInitializeThread\s*\([^)]*\)','template<bool IsHostThread> void xbox::KeInitializeThread(PKTHREAD Thread,PVOID KernelStack,ulong_xt KernelStackSize,ulong_xt TlsDataSize,PKSYSTEM_ROUTINE SystemRoutine,PKSTART_ROUTINE StartRoutine,PVOID StartContext,PKPROCESS Process)'),
 ('GetNtTib',fs,r'NT_TIB \*GetNtTib\s*\(\)','NT_TIB* GetNtTib()'),
 ('EmuKeSetPcr',fs,r'void EmuKeSetPcr\s*\([^)]*\)','void EmuKeSetPcr(xbox::KPCR* Pcr)'),
 ('EmuGetTIBStackBase',fs,r'xbox::PVOID EmuGetTIBStackBase\s*\([^)]*\)','xbox::PVOID EmuGetTIBStackBase(xbox::PVOID ThreadStackBase)'),
 ('EmuGenerateStackSize',fs,r'xbox::dword_xt EmuGenerateStackSize\s*\([^)]*\)','xbox::dword_xt EmuGenerateStackSize(xbox::addr_xt& espBaseAddress,xbox::ulong_xt TlsDataSize)'),
 ('EmuGenerateFS',fs,r'void EmuGenerateFS\s*\([^)]*\)','template<bool IsHostThread> void EmuGenerateFS(xbox::PETHREAD Ethread,unsigned Host2XbStackBaseReserved,unsigned Host2XbStackSizeReserved)')]
 for name,text,pattern,signature in entries:
  value=body(text,pattern);bodies[name]=value;source+=signature+'\n'+value+'\n'
 source+='''
// These entry points are stored as pointers by the complete producer, never
// invoked in this bounded case. Stop rather than supply scheduler behavior.
void NTAPI xbox::KiSuspendNop(PKAPC,PKNORMAL_ROUTINE*,PVOID*,PVOID*,PVOID*){abort();}
void NTAPI xbox::KiSuspendThread(PVOID,PVOID,PVOID){abort();}
void NTAPI xbox::KiThreadStartup(){abort();}
void xbox::MmDeleteKernelStack(PVOID,PVOID){abort();}
struct Allocation { void* base;unsigned requested,size,flags,protect;LONG status; } allocations[2];
unsigned allocation_count=0;
xbox::ntstatus_xt xbox::NtAllocateVirtualMemory(void** base,ulong_xt zeroBits,ulong_xt* size,ulong_xt flags,ulong_xt protect) {
 typedef LONG (NTAPI *Native)(HANDLE,PVOID*,ULONG,ULONG*,ULONG,ULONG);
 auto native=(Native)GetProcAddress(GetModuleHandleA("ntdll.dll"),"NtAllocateVirtualMemory");
 assert(native && allocation_count<2);
 static_assert(sizeof(ULONG)==sizeof(ulong_xt),"native 32-bit size ABI");
 unsigned requested=*size;LONG status=native((HANDLE)-1,base,zeroBits,reinterpret_cast<ULONG*>(size),flags,protect);
 allocations[allocation_count++]={*base,requested,*size,flags,protect,status};
 assert(status>=0);return status;
}
void hex(const void* ptr,unsigned n) {auto p=(const unsigned char*)ptr;for(unsigned i=0;i<n;++i)printf("%02x",p[i]);}
DWORD WINAPI ContextHost(void*) {
 // Backed host stack reservation follows the actual startup's size producer.
 unsigned captured=0;__asm__ __volatile__("movl %%esp,%0":"=r"(captured));
 unsigned base=captured;unsigned amount=EmuGenerateStackSize(base,0);
 void* reserve=_alloca(amount+64);assert((unsigned)reserve<=base-amount);
 auto hTib=GetNtTib();unsigned original_slot=__readfsdword(TIB_ArbitraryDataSlot);
 unsigned char before_tib[sizeof(NT_TIB)];memcpy(before_tib,hTib,sizeof(NT_TIB));
 EmuGenerateFS<true>(xbox::zeroptr,base,amount);
 auto pcr=(xbox::KPCR*)__readfsdword(TIB_ArbitraryDataSlot);auto thread=(xbox::PETHREAD)pcr->PrcbData.CurrentThread;
 unsigned short cw;__asm__ __volatile__("fnstcw %0":"=m"(cw));
 unsigned stack_size=(unsigned)hTib->StackBase-(unsigned)hTib->StackLimit;
 assert(stack_size && stack_size<0x400000);
 unsigned char* backed_stack=(unsigned char*)malloc(stack_size);assert(backed_stack);
 memcpy(backed_stack,hTib->StackLimit,stack_size);
 printf("{\\"native_thread_id\\":%lu,\\"host_tib_address\\":%u,\\"host_tib_before_hex\\":\\"",GetCurrentThreadId(),(unsigned)hTib);hex(before_tib,sizeof(NT_TIB));
 printf("\\",\\"captured_esp\\":%u,\\"aligned_stack_base\\":%u,\\"reserved_source_size\\":%u,\\"alloca_backing\\":%u,\\"kpcr_address\\":%u,\\"kpcr_hex\\":\\"",captured,base,amount,(unsigned)reserve,(unsigned)pcr);hex(pcr,sizeof(*pcr));
 printf("\\",\\"ethread_address\\":%u,\\"ethread_hex\\":\\"",(unsigned)thread);hex(thread,sizeof(*thread));
 printf("\\",\\"produced_stack_address\\":%u,\\"produced_stack_hex\\":\\"",(unsigned)thread->Tcb.KernelStack);hex(thread->Tcb.KernelStack,base-(unsigned)thread->Tcb.KernelStack);
 printf("\\",\\"source_static_process_address\\":%u,\\"source_static_process_hex\\":\\"",(unsigned)&KiUniqueProcess);hex(&KiUniqueProcess,sizeof(KiUniqueProcess));
 printf("\\",\\"host_backed_stack_address\\":%u,\\"host_backed_stack_hex\\":\\"",(unsigned)hTib->StackLimit);hex(backed_stack,stack_size);free(backed_stack);
 printf("\\",\\"fpu_control_word\\":%u,\\"allocations\\":[",cw);
 for(unsigned i=0;i<allocation_count;++i) {auto a=allocations[i];printf("%s{\\"base\\":%u,\\"requested\\":%u,\\"result_size\\":%u,\\"flags\\":%u,\\"protect\\":%u,\\"actual_ntstatus\\":%ld}",i?",":"",(unsigned)a.base,a.requested,a.size,a.flags,a.protect,a.status);}
 printf("]}\\n");fflush(stdout);
 // Keep real host thread and allocations alive until consumer completes.
 char byte;DWORD read;ReadFile(GetStdHandle(STD_INPUT_HANDLE),&byte,1,&read,nullptr);
 __writefsdword(TIB_ArbitraryDataSlot,original_slot);return 0;
}
int main() {
 // A real OS-created worker supplies its own native TIB and stack. The complete
 // primary generator's host-thread branch does not require a guessed layout.
 HANDLE thread=CreateThread(nullptr,0,ContextHost,nullptr,0,nullptr);assert(thread);
 DWORD wait=WaitForSingleObject(thread,INFINITE);assert(wait==WAIT_OBJECT_0);
 DWORD code;GetExitCodeThread(thread,&code);CloseHandle(thread);return code;
}
'''
 OUT.mkdir(exist_ok=True);p=OUT/'generator.cpp';p.write_text(source)
 command=['i686-w64-mingw32-g++','-std=c++17','-O2','-fpermissive','-static','-Wl,--image-base,0x10000000','-Wl,--no-insert-timestamp',str(p),'-o',str(OUT/'generator.exe')]
 result=subprocess.run(command,text=True,capture_output=True);(OUT/'compiler.txt').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stderr
 listing=OUT/'native-compatibility-instructions.asm'
 listing.write_text(subprocess.check_output(['i686-w64-mingw32-objdump','-d',str(OUT/'generator.exe')],text=True))
 return dict(extracted_bodies=bodies,generated_source_sha256=digest(p),compiled_PE_sha256=digest(OUT/'generator.exe'),
  compiled_PE_listing_sha256=digest(listing),build_command=command,compiler_version=subprocess.check_output(['i686-w64-mingw32-g++','--version'],text=True).splitlines()[0],
  compiler_diagnostic_sha256=digest(OUT/'compiler.txt'),
  exact_source_operation_bodies=sorted(bodies),host_template_branch=True,
  stored_callback_endpoints_unprovided=['KiSuspendNop','KiSuspendThread','KiThreadStartup'],
  unused_non_host_stack_service_unprovided='MmDeleteKernelStack',
  source_static_KiUniqueProcess_used_before_KiInitSystem=True)

class ContextTrace(HostLockTrace):
 def __init__(self,data,physical=False):
  old=json.loads((LOCK_OUT/'report.json').read_text());initial=json.loads((S/'report.json').read_text())['compiled_compatibility_initializer']
  self.context=data;self.context_regions=[];self.context_original_stores=[];super().__init__(initial,old['compiled_compatibility_locks'],physical)
  for name,addr_key,hex_key in [('complete_native_produced_KPCR','kpcr_address','kpcr_hex'),
    ('complete_native_produced_ETHREAD','ethread_address','ethread_hex'),
    ('real_native_host_stack_snapshot','host_backed_stack_address','host_backed_stack_hex'),
    ('source_BSS_KiUniqueProcess','source_static_process_address','source_static_process_hex')]:
   addr=data[addr_key];raw=bytes.fromhex(data[hex_key]);lo=addr&~4095;hi=(addr+len(raw)+4095)&~4095
   assert all(hi<=a or lo>b for a,b,_ in self.g.mem_regions()),(name,hex(lo),hex(hi))
   self.g.mem_map(lo,hi-lo);self.g.mem_write(addr,raw)
   self.context_regions.append(dict(kind=name,address=f'{addr:08x}',bytes=len(raw),mapped_lo=f'{lo:08x}',mapped_hi_exclusive=f'{hi:08x}'))
  p=data['kpcr_address'];self.g.mem_write(GDT+24,segment(p,0x93));self.g.reg_write(UC_X86_REG_FS,24)
  self.g.reg_write(UC_X86_REG_FPCW,data['fpu_control_word'])
  assert self.g.reg_read(UC_X86_REG_FS_BASE)==p
  assert self.u(p+0x20)==p+0x28 and self.u(p+0x28)==data['ethread_address'] and self.u(p+0x278)==0
  self.platform_input=dict(scope='Entire pinned compatibility producer output transferred with actual native allocations/stack; not Xbox hardware or ordinary XAPI thread context',
   generator_host_branch=True,kpcr_address=f'{p:08x}',kpcr_complete_bytes=0x280,ethread_complete_bytes=0x140,
   native_real_host_thread_kept_alive=True,source_generator_before_KiInitSystem=True,
   native_current_thread_id_recorded_separately=True,provided_by_whole_source_producer=True,
   compatibility_produced_PRCB_callback_zero=True,
   context_regions=self.context_regions)

 def token(self):
  value=self.u(self.u(self.context['kpcr_address']+0x20))
  assert value==self.context['ethread_address']
  if not self.host_rows:self.host_rows.append(dict(ordinal=0,native_thread_id=self.context['native_thread_id'],token=f'{value:08x}'))
  return value

 def mem_write(self,g,access,address,size,value,user):
  p=self.context['kpcr_address']
  if p<=address<p+0x280:
   from unicorn.x86_const import UC_X86_REG_EIP
   self.context_original_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',offset=f'{address-p:03x}',size=size,
    before=bytes(g.mem_read(address,size)).hex(),value=f'{value&((1<<(size*8))-1):0{size*2}x}'))
  super().mem_write(g,access,address,size,value,user)

 def context_snapshot(self):
  result=self.sync_snapshot();result['retained']['fs_head_after']=f'{self.u(self.context["kpcr_address"]):08x}'
  return dict(**result,compatibility_context_lock_calls=self.lock_calls,compatibility_context_lock_stores=self.lock_stores,
   compatibility_current_thread_helpers=self.helper_calls,raw_impact_pool_allocations=self.os_pool_allocs,
   whole_compatibility_KPCR_producer=True,original_stores_into_produced_KPCR=self.context_original_stores,
   new_supplied_game_or_Havok_fields=[])

def source_cases(data):
 traces=[];rows=[]
 for ready,block in((False,0x1c6058),(True,0x2dd621)):
  t=ContextTrace(data,ready);t.heap_prefix(None);t.register();t.components()
  if ready:t.arena();t.call('original_impact_pool_initializer',0x2284c0)
  t.memory_backing();t.call('original_full_new_map_order',0x1c5ff0,blocked=block)
  assert t.u(0x526538)==0x8000690 and t.u(0x52653c)==0x80006d0
  assert t.u(0x80006d0)==0x461b9c
  assert [r['ordinal']for r in t.lock_calls]==[277,294,277,294]
  assert all(int(r['after_hex'][-8:],16)==0 for r in t.lock_calls if r['ordinal']==294)
  core=[r for r in t.new_calls if r['entry']=='00316710']
  if ready:
   assert t.u(0x526234)==0x6005470 and core[-1]['returned_eax']=='06005470'
   failed=[r for r in t.new_calls if r['entry']=='0022e1d0'][-1]
   assert failed['stack'][:2]==['00000010','00002010'] and failed['returned_eax']=='00000000'
   assert t.u(0x526260)==0x60051f0 and t.u(0x526264)==0x6005220 and t.u(0x486550)==0x6005250
   assert t.u(0x480d98)==1
   for p in(0x52648c,0x526490,0x526248):assert t.u(t.u(p)+0x29,1)==1
  else:assert not core and t.u(0x526234)==0 and t.u(0x52648c)==0
  rows.append(dict(case='original_full_new_map_with_whole_compatibility_context_and_source_preparation'if ready else'original_full_new_map_to_missing_impact_pool_with_whole_compatibility_context',
   producers=t.producers,original_calls=t.new_calls,**t.context_snapshot()));traces.append(t)
 return traces,rows

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 metadata=build();env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(OUT/'wine-prefix'))
 process=subprocess.Popen(['wine',str(OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
 try:
  line=process.stdout.readline();assert line,line
  data=json.loads(line);args.output.parent.mkdir(exist_ok=True,parents=True)
  kpcr=bytes.fromhex(data['kpcr_hex']);ethread=bytes.fromhex(data['ethread_hex'])
  assert len(kpcr)==0x280 and len(ethread)==0x140 and kpcr[0x5c:]==bytes(0x224)
  assert bytes.fromhex(data['source_static_process_hex'])==bytes(28)
  assert struct.unpack_from('<I',ethread,0x28)[0]==0
  assert [r['requested']for r in data['allocations']]==[0x280,0x140]
  assert all(r['flags']==0x3000 and r['protect']==4 and r['actual_ntstatus']==0 and r['result_size']==4096 for r in data['allocations'])
  assert bytes.fromhex(data['produced_stack_hex'])[:8]==struct.pack('<II',0xffffffff,0x200)
  assert data['fpu_control_word']==0x27f
  (args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
  traces,rows=source_cases(data)
 finally:
  stdout,stderr=process.communicate('x',timeout=60);(OUT/'runtime-stderr.txt').write_text(stderr)
 assert process.returncode==0,(process.returncode,stderr)
 (args.output.parent/'producer-metadata.json').write_text(json.dumps(metadata,indent=2,sort_keys=True)+'\n')
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);t=traces[0]
 executed=set().union(*(x.executed for x in traces));listings=[]
 for a in executed:
  i=t.code.at(a);assert t.image.read(a,len(i.bytes))==i.bytes
  assert all(bytes(x.g.mem_read(a,len(i.bytes)))==i.bytes for x in traces)
 for entry in dict.fromkeys(ENTRIES):
  routine=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(routine.code)]
  for i in ins:
   assert t.image.read(i.address,len(i.bytes))==i.bytes
   assert all(bytes(x.g.mem_read(i.address,len(i.bytes)))==i.bytes for x in traces)
  p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=digest(p),instructions=len(ins),executed=sum(i.address in executed for i in ins),
   direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 manifest=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 report=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
  inspected_additional_primary_source_pins=SOURCE_PINS,primary_compatibility_commit='585c49a50af1255ab155099e06f24505f9c5a800',
  producer=metadata,real_native_runtime=subprocess.check_output(['wine','--version'],text=True).strip(),
  native_context_snapshot_file='raw-host-context.json',native_snapshot_scope='Real per-run OS thread ID/stack contents stored separately; entire produced KPCR/ETHREAD and backed stack transferred without relocating any source pointer',
  complete_native_compatibility_producer_observed=dict(kpcr_bytes=0x280,ethread_bytes=0x140,
   actual_NtAllocateVirtualMemory_requests_and_results=data['allocations'],entire_PRCB_unknown_224_bytes_compatibility_produced_zero=True,
   source_static_KiUniqueProcess_BSS_28_bytes_zero_before_KiInitSystem=True,
   source_host_branch_TlsData_produced_NULL=True,actual_fpu_control_word='027f',
   entire_host_backed_stack_snapshot_bytes=len(data['host_backed_stack_hex'])//2),
  instruction_manifest_sha256=manifest,instruction_listings=listings,executed_original_addresses_verified=len(executed),instruction_patches=[],cases=rows,
  summary=dict(cases=len(rows),source_manager_constructor_completions=2,source_full_order_BSP_constructor_completions=1,source_full_order_core_world_returns=1,
   complete_new_map_world_initializations=0,ordinary_motion_or_contact_cases=0),
  boundaries=[
   'Complete pinned primary EmuGenerateFS<true> body and all called construction bodies are extracted unchanged. Native32 C++ executes them with actual Wine32 TIB, a real OS-created worker, actual backed host stack, real NtAllocateVirtualMemory reserve/commit and actual CRT controlfp. Compiler fpermissive allows the primary function-pointer-to-PVOID assignment without altering its expression.',
   'Actual CxbxKrnl startup calls this host branch before Ob/Ps/Ki/RtlInitSystem; exact primary KiUniqueProcess global declaration therefore begins as actual static BSS zero. No process quantum or thread record fields are independently supplied.',
   'The host-thread branch is complete, including whole KPCR/ETHREAD zeroing, APC/semaphore/timer/context construction, host TIB copy, self/Prcb/DPC/current-thread assignments, pointer installation and FPU control. Unknown PRCB bytes, including+250, are compatibility-produced zeros only; not recovered ordinary original Xbox defaults.',
   'The actual generator host branch sets TlsDataNULL through its complete context producer; the conditional XBE TLS read is not reached. The title-thread false branch, PsCreateSystemThreadEx/ObjectManager and whole original XAPI startup remain unproved.',
   'Host startup capture uses actual ESP and exact source EmuGenerateStackSize; GCC alloca supplies backed reserved host stack with an extra64-byte wrapper margin and verifies full source range is inside it. This is declared host backing, not original Xbox stack allocation.',
   'Entire640-byte produced KPCR,320-byte ETHREAD, real backed host stack snapshot and source static process object transfer at their actual addresses into an otherwise separate original-code emulator. Overlap with every existing source/cache/resource mapping is rejected. Wine worker remains alive until source consumers finish; native OS IDs and stack bytes are reported separately.',
   'Current-thread equality helper reads the complete produced PRCB.CurrentThread pointer. Pinned compatibility lock bodies/serialized atomics remain explicit; contended waits/wakes and stored KiSuspendNop/KiSuspendThread/KiThreadStartup endpoint behavior are unavailable and would stop. No scheduler or exception delivery result is supplied.',
   'Original full1C5FF0 reaches actual heap-created manager08000690 and callback080006D0. Missing impact-pool case stops1C6058, retaining those objects. Prepared case executes original pool enable/reset, complete1C6270 then1C66A0 and core316710 returns world06005470.',
   'Prepared full order stops original2DD621 after original fixed-memory mode1 allocator returnsNULL for2010 bytes. Source mode2 assignment1C60A1 occurs only after1C66A0 returns and is not reached. No expansion mode/free-list/memory object field, success return or allocation is manufactured.',
   'Prior raw cache/object/resource/component/heap/physical backing and excluded savegame/XAPI intervals remain declared; actual source preparation runs selected constructors but does not prove whole authored startup allocation order. No complete map-world initialization, body insertion, ordinary motion/contact, production changes or prior frozen edits.'])
 args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 native_pins={p:digest(p)for p in('/usr/bin/wine','/usr/lib/wine/i386-windows/ntdll.dll','/usr/lib/wine/i386-windows/kernel32.dll')}
 native_pins.update({name:digest(args.output.parent/name)for name in('raw-host-context.json','producer-metadata.json')})
 (args.output.parent/'run-native-evidence-pins.json').write_text(json.dumps(native_pins,indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(report=str(args.output),report_sha256=digest(args.output),kpcr_address=hex(data['kpcr_address']),ethread_address=hex(data['ethread_address']))))

if __name__=='__main__':main()
