#!/usr/bin/env python3
"""Separate host-identity compatibility lock provider and original new-map order.

Only opaque real host-thread identity and host-serialized32-bit atomics are
provided. Exact pinned compatibility enter/leave bodies run as compiled guest
code. Thread records, contended waits/wakes and game/Havok fields are absent.
"""
import argparse,hashlib,json,re,struct,subprocess,threading
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ESP
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_mp_spawn_kernel_sync_world import SyncTrace,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,ENTRIES as PRIOR_ENTRIES,digest
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/host-lock-new-map-order'
SOURCE=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
LOCKCODE,HELPERS,TOKENS=0x9400000,0x9500000,0x96f0000
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_kernel_sync_world.py':'ad54453224f4c770be75cf71c6c17d5b06dd957ada8db293ebb370a6c6295c87',
 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/report.json':'ad0fb0b253d9fbf5b8c40b79ec2a4c142271be284c3570656b8325a7e9b92c54',
 'docs/MP_BIPED_PHYSICS_CONSTRUCTION_RESEARCH.md':'3ad01e5ebc36811f5bfb2b76fff99b9c62ce586a8c338f1d489b7a73f88a783b',
 'docs/MP_BIPED_PLATFORM_CONTEXT_RESEARCH.md':'1545bf7e458b2e34257982aae0b3591cef4e47fd57f7f45ef813cbc381bade9e',
 'docs/MP_PROCESS_HEAP_WORLD_STARTUP_RESEARCH.md':'9c2f4c16b49de2e34affcf23a8b6696b803e7c350bdede143cf479bf0b7a5f72',
 'docs/MP_KERNEL_SYNC_BSP_PRODUCER_RESEARCH.md':'d2134d8ef9e77006364170914bfed49a234dae74ad02808b99f819a2d0da6697'}
EXTRA_SOURCES=[dict(url='https://raw.githubusercontent.com/Cxbx-Reloaded/Cxbx-Reloaded/585c49a50af1255ab155099e06f24505f9c5a800/'+path,
    saved=name,sha256=sha,scope='Pinned primary compatibility context producer; inspected, not executed or supplied')
    for path,name,sha in(
    ('src/core/kernel/support/EmuFS.cpp','EmuFS.cpp','b228854d936ed37b6b1a511924d8bf9397106f530b7d4fdf336cca2235fef3d5'),
    ('src/core/kernel/exports/EmuKrnlPs.cpp','EmuKrnlPs.cpp','bca7c1fbd1f1c26b8ec5650eed85ed08893389e68cfc52d6c639b27eb726591c'))]
ENTRIES=PRIOR_ENTRIES+(0x2284c0,0x27dae0,0x1482f0,0x1c6370,0x3123a0,
    0x312fa0,0x317270,0x27e770,0x2dd5f0,0x2d82b4,0x2d8250,
    0x2d8532,0x2db3c6,0x2db3d9,0x2db5cd,0x2d755e,0x2d6db9,
    0x329353,0x32937a,0x2d7969)


def compile_locks():
    previous=json.loads((SOURCE/'report.json').read_text())['compiled_compatibility_initializer']
    assert digest(SOURCE/'sources.json')==previous['source_manifest_sha256']
    for r in previous['primary_sources']:assert digest(SOURCE/r['saved'])==r['sha256']
    for r in EXTRA_SOURCES:assert digest(OUT/r['saved'])==r['sha256']
    rtl=(SOURCE/'Cxbx-Reloaded-EmuKrnlRtl.cpp').read_text();bodies={}
    for ordinal,name in((277,'RtlEnterCriticalSection'),(294,'RtlLeaveCriticalSection')):
        bodies[name]=re.search(r'EXPORTNUM\('+str(ordinal)+r'\).*?'+name+r'\s*\([^)]*\)\s*(\{.*?\n\})',rtl,re.S).group(1)
    header=(SOURCE/'initializer.c').read_text().split('void __attribute__((stdcall)) KeInitializeEvent')[0]
    assert digest(SOURCE/'initializer.c')==previous['generated_C_sha256']
    # The preceding frozen header is C; only its assertion spelling changes
    # for this C++ wrapper. Extracted primary operation bodies stay literal.
    code=header.replace('_Static_assert','static_assert')+'''
namespace xbox { typedef int ntstatus_xt; }
#define X_NT_SUCCESS(Status) (((xbox::ntstatus_xt)(Status)) >= 0)
typedef void* PKTHREAD; typedef void* PVOID; typedef int NTSTATUS;
typedef int KWAIT_REASON; typedef int KPROCESSOR_MODE; typedef unsigned char boolean_xt;
typedef void* PLARGE_INTEGER; typedef int KPRIORITY;
extern "C" {
PKTHREAD __attribute__((stdcall)) KeGetCurrentThread();
LONG __attribute__((stdcall)) InterlockedIncrement(LONG*);
LONG __attribute__((stdcall)) InterlockedDecrement(LONG*);
NTSTATUS __attribute__((stdcall)) KeWaitForSingleObject(PVOID,KWAIT_REASON,KPROCESSOR_MODE,boolean_xt,PLARGE_INTEGER);
LONG __attribute__((stdcall)) KeSetEvent(PRKEVENT,KPRIORITY,boolean_xt);
void CxbxrAbort(const char*,...);
}
'''
    for name,body in bodies.items():code+='extern "C" void __attribute__((stdcall)) '+name+'(PRTL_CRITICAL_SECTION CriticalSection)\n'+body+'\n'
    OUT.mkdir(parents=True,exist_ok=True);p=OUT/'locks.cpp';p.write_text(code)
    subprocess.run(['c++','-m32','-mno-sse','-mno-sse2','-O2','-ffreestanding','-fno-pie','-fno-exceptions','-fno-rtti','-fno-stack-protector','-fno-asynchronous-unwind-tables','-c',str(p),'-o',str(OUT/'locks.o')],check=True)
    names=('KeGetCurrentThread','InterlockedIncrement','InterlockedDecrement','KeWaitForSingleObject','KeSetEvent','CxbxrAbort')
    defs=[x for n,name in enumerate(names)for x in('--defsym',f'{name}=0x{HELPERS+n*16:x}')]
    subprocess.run(['ld','-m','elf_i386','-Ttext',f'0x{LOCKCODE:x}','-e','RtlEnterCriticalSection',*defs,str(OUT/'locks.o'),'-o',str(OUT/'locks.elf')],check=True)
    subprocess.run(['objcopy','-O','binary','--only-section=.text',str(OUT/'locks.elf'),str(OUT/'locks.bin')],check=True)
    symbols=subprocess.check_output(['nm','-n',str(OUT/'locks.elf')],text=True)
    entries={name:int(next(x.split()[0]for x in symbols.splitlines()if x.endswith(' '+name)),16)for name in bodies}
    return previous,dict(source_manifest_sha256=previous['source_manifest_sha256'],primary_sources=previous['primary_sources'],
        extracted_primary_bodies=bodies,generated_Cpp_sha256=digest(p),compiled_text_sha256=digest(OUT/'locks.bin'),
        compiled_text_hex=(OUT/'locks.bin').read_bytes().hex(),entries={n:f'{a:08x}'for n,a in entries.items()},
        helpers={name:f'{HELPERS+n*16:08x}'for n,name in enumerate(names)},
        scope='Exact pinned compatibility enter/leave bodies; opaque real host-thread token provider and real host-serialized atomics. Not original Xbox kernel bytes.')


class HostLockTrace(SyncTrace):
    def __init__(self,initial,locks,physical=False):
        self.locks=locks;self.lock_executed=set();self.lock_stores=[];self.lock_calls=[];self.lock_returns=[]
        self.host_guard=threading.Lock();self.host_ids={};self.host_handles={};self.host_rows=[];self.helper_calls=[];self.os_pool_allocs=[]
        self.pool_next=0x9300000
        super().__init__(initial,physical)
        raw=bytes.fromhex(locks['compiled_text_hex']);self.g.mem_map(LOCKCODE,0x1000);self.g.mem_write(LOCKCODE,raw)
        self.g.mem_map(HELPERS,0x1000)

    def token(self):
        identity=threading.get_native_id();thread=threading.current_thread()
        with self.host_guard:
            if thread not in self.host_handles:
                value=TOKENS+len(self.host_handles)*4
                self.host_handles[thread]=value;self.host_ids[thread]=identity
                self.host_rows.append(dict(ordinal=len(self.host_handles)-1,native_thread_id=identity,token=f'{value:08x}'))
            assert self.host_ids[thread]==identity
            return self.host_handles[thread]

    def hook(self,g,a,size,user):
        for ret,row in self.lock_returns[:]:
            if a==ret:
                row['after_hex']=bytes(g.mem_read(int(row['pointer'],16),28)).hex()
                self.lock_returns.remove((ret,row))
        if LOCKCODE<=a<LOCKCODE+0x1000:
            self.lock_executed.add(a);return
        if HELPERS<=a<HELPERS+0x1000:
            sp=g.reg_read(UC_X86_REG_ESP);helper=(a-HELPERS)//16;ret=self.u(sp)
            if helper==0:
                token=self.token();result=token;cleanup=0
                row=dict(kind='opaque_current_real_host_thread',token=f'{token:08x}',compiled_return_site=f'{ret:08x}',thread_record_supplied=False)
            elif helper in(1,2):
                p=self.u(sp+4)
                with self.host_guard:
                    old=self.u(p);value=(old+(1 if helper==1 else -1))&0xffffffff;self.w(p,value)
                row=dict(kind='host_serialized_atomic_increment'if helper==1 else'host_serialized_atomic_decrement',
                    pointer=f'{p:08x}',before=f'{old:08x}',after=f'{value:08x}',compiled_return_site=f'{ret:08x}')
                result=value;cleanup=4
            else:
                name=next(name for name,addr in self.locks['helpers'].items()if int(addr,16)==a)
                self.new_invalid=dict(boundary='unprovided contended wait/wake/abort service',helper=name,
                    instruction=f'{a:08x}',compiled_return_site=f'{ret:08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(5)])
                raise KernelBoundary()
            self.helper_calls.append(row);g.reg_write(UC_X86_REG_EAX,result);g.reg_write(UC_X86_REG_ESP,sp+4+cleanup);g.reg_write(UC_X86_REG_EIP,ret);return
        if a in(0x80000115,0x80000126):
            sp=g.reg_read(UC_X86_REG_ESP);p=self.u(sp+4);ret=self.u(sp)
            name='RtlEnterCriticalSection'if a==0x80000115 else'RtlLeaveCriticalSection'
            row=dict(phase=self.active,original_call_site=f'{self.recent[-1]:08x}',original_return_site=f'{ret:08x}',
                ordinal=a&0x7fffffff,pointer=f'{p:08x}',before_hex=bytes(g.mem_read(p,28)).hex())
            self.lock_calls.append(row);self.lock_returns.append((ret,row))
            g.reg_write(UC_X86_REG_EIP,int(self.locks['entries'][name],16));return
        if a==0x33785c and self.active=='original_impact_pool_initializer':
            sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
            assert args[0]==0 and args[1]in(0x1450,0x850)and args[2:]==[0x101000,4]
            cap=(args[1]+4095)&~4095;p=self.pool_next;self.pool_next+=cap;g.mem_map(p,cap)
            self.os_pool_allocs.append(dict(original_return_site=f'{self.u(sp):08x}',args=[f'{v:08x}'for v in args],
                supplied_raw_backing=f'{p:08x}',mapped_capacity=cap,scope='raw source-requested OS memory only'))
            self.executed.add(a);g.reg_write(UC_X86_REG_EAX,p);g.reg_write(UC_X86_REG_ESP,sp+20);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
        if self.instrument and a in set(ENTRIES)-set(PRIOR_ENTRIES):
            sp=g.reg_read(UC_X86_REG_ESP);row=dict(entry=f'{a:08x}',phase=self.active,registers=self.registers(),
                return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
            self.new_calls.append(row);self.return_watch.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        pc=g.reg_read(UC_X86_REG_EIP)
        if LOCKCODE<=pc<LOCKCODE+0x1000:
            self.lock_stores.append(dict(instruction=f'{pc:08x}',address=f'{address:08x}',size=size,
                before=bytes(g.mem_read(address,size)).hex(),value=f'{value&((1<<(size*8))-1):0{size*2}x}'));return
        super().mem_write(g,access,address,size,value,user)

    def host_snapshot(self):
        assert all(not lo<=TOKENS<=hi for lo,hi,_ in self.g.mem_regions())
        pools={}
        for a in(0x52648c,0x526490):
            p=self.u(a)
            pools[f'{a:08x}']=dict(pointer=f'{p:08x}',header_50_hex=bytes(self.g.mem_read(p,0x50)).hex()if p else None)
        return dict(**self.sync_snapshot(),host_identity_scope='Current real host threads mapped to stable opaque32-bit guest equality tokens; no KTHREAD/TLS/FS28 record supplied or dereferenced',
            normalized_host_threads=[dict(ordinal=r['ordinal'],token=r['token'],real_native_identity_verified=True)for r in self.host_rows],
            compatibility_lock_calls=self.lock_calls,compatibility_lock_stores=self.lock_stores,host_helper_calls=self.helper_calls,
            raw_impact_pool_allocations=self.os_pool_allocs,source_impact_pools=pools,
            new_supplied_game_Havok_thread_record_fields=[])

    def arena(self):
        self.call('original_physical_arena_root',0x12cb70)
        self.active='original_game_arena_allocation_clear_prefix';self.g.reg_write(UC_X86_REG_ESP,STACK)
        self.g.emu_start(0x125280,0x1252ba,count=2000000);assert self.g.reg_read(UC_X86_REG_EIP)==0x1252ba
        self.producers.append(dict(name=self.active,original_fragment=['00125280','001252ba'],stop_is_exclusive=True,
            excluded_savegame_resource_thread_IO_tail=True,completion='complete_fragment'))
        self.call('original_BSP_datum_table_constructor',0x1f15b0)


def cases(initial,locks):
    rows=[];traces=[]
    t=HostLockTrace(initial,locks);p=STACK+0x300;t.call('compatibility_platform_initializer',int(initial['entry'],16),(p,))
    stages=[]
    for name in('RtlEnterCriticalSection','RtlEnterCriticalSection','RtlLeaveCriticalSection','RtlLeaveCriticalSection'):
        t.call(name,int(locks['entries'][name],16),(p,));stages.append(dict(operation=name,state_hex=bytes(t.g.mem_read(p,28)).hex()))
    errors=[]
    def worker():
        try:
            for name in('RtlEnterCriticalSection','RtlLeaveCriticalSection'):
                t.call(name,int(locks['entries'][name],16),(p,));stages.append(dict(operation='second_host_thread_'+name,state_hex=bytes(t.g.mem_read(p,28)).hex()))
        except Exception as e:errors.append(repr(e))
    thread=threading.Thread(target=worker);thread.start();thread.join();assert not errors
    assert len(t.host_rows)==2 and t.host_rows[0]['native_thread_id']!=t.host_rows[1]['native_thread_id']
    assert t.u(p+0x10)==0xffffffff and t.u(p+0x14)==t.u(p+0x18)==0
    rows.append(dict(case='real_host_identity_uncontended_recursive_release_two_threads',stages=stages,producers=t.producers,**t.host_snapshot()));traces.append(t)
    t=HostLockTrace(initial,locks);p=STACK+0x300;t.call('compatibility_initializer',int(initial['entry'],16),(p,))
    t.call('first_host_thread_enter',int(locks['entries']['RtlEnterCriticalSection'],16),(p,))
    errors=[]
    def blocked_worker():
        try:t.call('second_host_thread_contended_enter',int(locks['entries']['RtlEnterCriticalSection'],16),(p,),blocked=HELPERS+0x30)
        except Exception as e:errors.append(repr(e))
    thread=threading.Thread(target=blocked_worker);thread.start();thread.join();assert not errors
    assert t.u(p+0x10)==1 and t.u(p+0x18)==TOKENS
    rows.append(dict(case='real_second_host_thread_stops_at_unprovided_wait',producers=t.producers,
        critical_section_hex=bytes(t.g.mem_read(p,28)).hex(),**t.host_snapshot()));traces.append(t)
    for ready,block in((False,0x2db3a0),(True,0x2db3a0)):
        t=HostLockTrace(initial,locks,physical=ready);t.heap_prefix(None);t.register();t.components()
        if ready:
            t.arena();t.call('original_impact_pool_initializer',0x2284c0)
        t.memory_backing();t.call('original_full_new_map_order',0x1c5ff0,blocked=block)
        assert t.new_invalid['address']=='00000250' and t.u(0x6400020)==0
        assert t.u(0x526538)==t.u(0x52653c)==t.u(0x526234)==0
        assert len(t.lock_calls)==2 and [r['ordinal']for r in t.lock_calls]==[277,294]
        assert t.u(0x8000620)==0xffffffff and t.u(0x8000624)==t.u(0x8000628)==0
        allocation=int(t.new_invalid['registers']['esi'],16);assert allocation==0x8000690
        rows.append(dict(case='full_new_map_with_original_arena_and_impact_producers_to_unprovided_PRCB'if ready else'full_new_map_without_impact_pool_initialization_to_unprovided_PRCB',
            producers=t.producers,original_calls=t.new_calls,
            retained_original_manager_allocation=dict(pointer=f'{allocation:08x}',cpp_new_requested_size=0x28,
                original_CRT_rounded_heap_request=0x30,
                notification_words=[f'{t.u(int(t.new_invalid["registers"]["ebp"],16)-0x5c+i*4):08x}'for i in range(3)],
                payload_28_hex=bytes(t.g.mem_read(allocation,0x28)).hex(),
                constructor_27DAE0_reached=False,manager_global_526538='00000000'),
            complete_new_map_world_initialization=False,**t.host_snapshot()));traces.append(t)
    return traces,rows


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    initial,locks=compile_locks();traces,rows=cases(initial,locks);t=traces[0]
    args.output.parent.mkdir(parents=True,exist_ok=True);directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    executed=set().union(*(t.executed for t in traces));listings=[]
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
    raw=bytes.fromhex(locks['compiled_text_hex']);code=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,LOCKCODE))
    lock_executed=set().union(*(t.lock_executed for t in traces));assert lock_executed<={i.address for i in code}
    for trace in traces:assert bytes(trace.g.mem_read(LOCKCODE,len(raw)))==raw
    p=args.output.parent/'compatibility-lock-instructions.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in code));locks['compiled_listing_sha256']=digest(p)
    manifest=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    d=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
        instruction_manifest_sha256=manifest,instruction_listings=listings,executed_original_addresses_verified=len(executed),
        compiled_compatibility_locks=locks,inspected_primary_context_sources=EXTRA_SOURCES,
        compatibility_lock_executed_addresses=[f'{a:08x}'for a in sorted(lock_executed)],
        instruction_patches=[],cases=rows,
        summary=dict(cases=len(rows),original_full_new_map_attempts=2,complete_new_map_world_initializations=0,
            compatibility_uncontended_original_heap_lock_cycles=2,
            source_manager_allocations_retained_before_unprovided_PRCB_notification=2,
            source_impact_pool_constructor_completions=1,ordinary_motion_or_contact_cases=0),
        boundaries=[
            'Pinned primary Cxbx enter/leave operation bodies execute as compiled32 compatibility code. Primary struct declarations are unchanged; only C wrapper _Static_assert spelling becomes C++ static_assert, and diagnostic logging is empty. Not original Xbox kernel instruction bytes.',
            'KeGetCurrentThread is a declared real-host provider: Python current_thread objects paired with kernel get_native_id values map to stable unmapped opaque32 guest tokens. Actual native IDs are recorded separately and intentionally excluded from reproducible semantic report. No KTHREAD/ETHREAD/TLS/FS28 record is supplied.',
            'Host threading.Lock serializes the32-bit guest increment/decrement helpers and returns modulo2^32 results. This supplies the atomic primitive, not a game field. Helper calls use a private compiled-wrapper stdcall protocol, separately from original Xbox FASTCALL import ABI.',
            'Contended wait/wake/abort services remain unavailable. Second actual host-thread acquisition stops before KeWaitForSingleObject; it receives no success. Unicorn execution is sequential across real host threads, with no claim of concurrent Xbox scheduling.',
            'Source original heap acquires at2DACAD and releases at2DB25E, then retains actual allocation08000690 (size28), before original2DB3A0 reads [FS20+250]. FS20 is unprovided and no PRCB callback value is supplied. Constructor27DAE0, manager globals, impact reset,1C6270/1C66A0 in full scheduling and world insertion are not reached.',
            'Pinned compatibility types define FS20 as KPCR.Prcb and Prcb.CurrentThread at+0, but Prcb+250 is in declared Unknown bytes. Primary EmuGenerateFS zeroes sizeofKPCR before pointer/DPC/thread/TIB setup. That compatibility producer is inspected only; it is not original Xbox producer proof and is not executed or partially manufactured here.',
            'Source original callers use Prcb+250 as optional callback record (heap op10, free op11, unhandled exception op12, reboot op2). Actual field owner/ordinary-original initialization stays unresolved, not equated to a thread ID.',
            'Original2284C0 creates32-entry impact and impact-array pools with stridesA0/40, from source VirtualAlloc requests1450/850. Only raw requested OS memory is supplied; original pool headers are produced. They remain inactive because full new-map stops before source+29 enable/reset.',
            'All earlier raw cache/object/resource/component/SEH/heap/physical backing and excluded XAPI/savegame startup boundaries persist. Separate prior direct BSP completion is not complete original1C5FF0 scheduling. No production files or earlier frozen suites edited.'])
    args.output.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    # Actual kernel IDs vary per process; keep them separate from normalized
    # reproducible semantic evidence, explicitly declared in the report/docs.
    (args.output.parent/'run-host-identities.json').write_text(json.dumps([t.host_rows for t in traces],indent=2)+'\n')
    print(json.dumps(dict(report=str(args.output),report_sha256=digest(args.output),oracle_sha256=d['oracle_sha256'],instruction_manifest_sha256=manifest,cases=len(rows))))


if __name__=='__main__':main()
