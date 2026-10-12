#!/usr/bin/env python3
"""Independent sourced critical-section initializer and original BSP producer.

Pinned compatibility implementation initializer expressions are compiled as
32-bit code and executed separately from original TU code. Current-thread,
contention, complete process startup and world stepping remain unavailable.
"""
import argparse,hashlib,json,re,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_ESP
from oracle_mp_spawn_process_heap_world import StartupTrace,KernelBoundary,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,ENTRIES as PRIOR_ENTRIES,digest
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace
from capstone import Cs,CS_ARCH_X86,CS_MODE_32

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
SOURCES_PIN='0813f92e273e295bfc7fa146555afcd6e752f73cc5295f305681dfd5bf54505e'
COMPILED=0x9000000
ENTRIES=PRIOR_ENTRIES+(0x12cb70,0x125280,0x217020,0x217090,0x1656a0,0x165700,
    0x2d79aa,0x2d7a1a,0x1c7480,0x1f2710,0x1d2ca0,0x2d6eba)
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_process_heap_world.py':'580df674c30e514c99a9334b89d541e97aca598790ff6e05434473d05a68013c',
 'analysis/multiplayer/integration/spawn-source/process-heap-world/report.json':'805c2ef2059202d24799af5dc86d241ebe8b6cc79ff66dd4105c1ffec02ababf'}


def compile_initializer():
    assert digest(OUT/'sources.json')==SOURCES_PIN
    sources=json.loads((OUT/'sources.json').read_text())
    for r in sources:assert digest(OUT/r['saved'])==r['sha256']
    rtl=(OUT/'Cxbx-Reloaded-EmuKrnlRtl.cpp').read_text()
    ke=(OUT/'Cxbx-Reloaded-EmuKrnlKe.cpp').read_text()
    types=(OUT/'Cxbx-Reloaded-types.h').read_text()
    # Exact primary initializer body; diagnostic logging is an empty macro.
    body=re.search(r'EXPORTNUM\(291\).*?RtlInitializeCriticalSection\s*\([^)]*\)\s*(\{.*?\n\})',rtl,re.S).group(1)
    event=re.search(r'// Setup the Xbox event struct\s*(Event->Header.Type.*?InitializeListHead\([^;]*;)',ke,re.S).group(1)
    declarations=[]
    for name in ('DISPATCHER_HEADER','KEVENT','RTL_CRITICAL_SECTION'):
        declarations.append(re.search(r'typedef struct _'+name+r'\s*\{.*?\}\s*(?://[^\n]*\n)?'+name+r'[^;]*;',types,re.S).group(0))
    nxdk=(OUT/'XboxDev-xboxkrnl.h').read_text()
    macro=next(x for x in nxdk.splitlines()if x.startswith('#define InitializeListHead('))
    source='''typedef unsigned char uchar_xt; typedef int long_xt; typedef long LONG; typedef void* HANDLE;
#define LOG_FUNC_ONE_ARG(x)
#define SynchronizationEvent 1
#define FALSE 0
'''+macro+'\n'+re.search(r'typedef struct _LIST_ENTRY\s*\{.*?\}\s*LIST_ENTRY[^;]*;',types,re.S).group(0)+'\n'+'\n'.join(declarations)+'''
_Static_assert(sizeof(KEVENT)==16,"event32");
_Static_assert(sizeof(RTL_CRITICAL_SECTION)==28,"critical_section32");
_Static_assert(__builtin_offsetof(RTL_CRITICAL_SECTION,LockCount)==16,"lockcount");
_Static_assert(__builtin_offsetof(RTL_CRITICAL_SECTION,RecursionCount)==20,"recursioncount");
_Static_assert(__builtin_offsetof(RTL_CRITICAL_SECTION,OwningThread)==24,"owningthread");
void __attribute__((stdcall)) KeInitializeEvent(PRKEVENT Event,int Type,unsigned char SignalState) {
'''+event+'''\n}
void __attribute__((stdcall)) RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION CriticalSection)
'''+body+'\n'
    OUT.mkdir(parents=True,exist_ok=True);p=OUT/'initializer.c';p.write_text(source)
    subprocess.run(['cc','-m32','-mno-sse','-mno-sse2','-O2','-ffreestanding','-fno-pie','-fno-stack-protector','-fno-asynchronous-unwind-tables','-c',str(p),'-o',str(OUT/'initializer.o')],check=True)
    subprocess.run(['ld','-m','elf_i386','-Ttext',f'0x{COMPILED:x}','-e','RtlInitializeCriticalSection',str(OUT/'initializer.o'),'-o',str(OUT/'initializer.elf')],check=True)
    subprocess.run(['objcopy','-O','binary','--only-section=.text',str(OUT/'initializer.elf'),str(OUT/'initializer.bin')],check=True)
    symbols=subprocess.check_output(['nm','-n',str(OUT/'initializer.elf')],text=True)
    address=int(next(x.split()[0]for x in symbols.splitlines()if x.endswith(' RtlInitializeCriticalSection')),16)
    return dict(source_manifest_sha256=SOURCES_PIN,primary_sources=sources,extracted_initializer_body=body,
        extracted_event_struct_initialization=event,extracted_list_macro=macro,
        generated_C_sha256=digest(p),compiled_text_sha256=digest(OUT/'initializer.bin'),
        compiled_text_hex=(OUT/'initializer.bin').read_bytes().hex(),entry=f'{address:08x}',
        excluded_compatibility_host_handle_guard=True,
        scope='Pinned compatibility ABI initializer expressions; not original Xbox kernel instructions/thread execution')


class SyncTrace(StartupTrace):
    def __init__(self,compiled,physical=False):
        self.compiled=compiled;self.compat_executed=set();self.compat_calls=[];self.compat_stores=[]
        self.physical=physical;self.physical_services=[]
        super().__init__(True)
        raw=bytes.fromhex(compiled['compiled_text_hex']);self.g.mem_map(COMPILED,0x1000);self.g.mem_write(COMPILED,raw)

    def hook(self,g,a,size,user):
        if COMPILED<=a<COMPILED+0x1000:
            self.compat_executed.add(a);return
        if a==0x80000123:
            sp=g.reg_read(UC_X86_REG_ESP);p=self.u(sp+4)
            self.compat_calls.append(dict(phase=self.active,kernel_ordinal=291,original_call_site=f'{self.recent[-1]:08x}',
                original_return_site=f'{self.u(sp):08x}',pointer=f'{p:08x}',before_hex=bytes(g.mem_read(p,28)).hex(),
                supplied_scope=self.compiled['scope']))
            g.reg_write(UC_X86_REG_EIP,int(self.compiled['entry'],16));return
        if self.physical and a==self.u(0x417ba0):
            sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(5)]
            assert args==[0x30e4000,0x61000,0x3144fff,0,4]
            start,end=0x80061000,0x83145000
            # Raw pages only. Retain inherited literal cache backing in the
            # overlapping emulator view; no claim of complete real RAM layout.
            for lo,hi,_ in list(g.mem_regions()):
                if lo<=start<=hi:start=hi+1
            if start<end:g.mem_map(start,end-start)
            self.physical_services.append(dict(kind='MmAllocateContiguousMemoryEx',ordinal=a&0x7fffffff,
                args=[f'{v:08x}'for v in args],raw_backing='80061000',raw_end_exclusive='83145000',
                original_return_site=f'{self.u(sp):08x}',scope='raw original-requested physical backing; inherited cache address mapping retained'))
            g.reg_write(UC_X86_REG_EAX,0x80061000);g.reg_write(UC_X86_REG_ESP,sp+24)
            g.reg_write(UC_X86_REG_EIP,self.u(sp));return
        if self.physical and a==self.u(0x417ba8):
            sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(3)]
            assert args==[0x82d47000,0x3fe000,4]
            self.physical_services.append(dict(kind='MmSetAddressProtect',ordinal=a&0x7fffffff,
                args=[f'{v:08x}'for v in args],original_return_site=f'{self.u(sp):08x}',scope='raw backing access protection only; void ABI, no object result'))
            g.reg_write(UC_X86_REG_ESP,sp+16);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
        if self.instrument and a in set(ENTRIES)-set(PRIOR_ENTRIES):
            sp=g.reg_read(UC_X86_REG_ESP);row=dict(entry=f'{a:08x}',phase=self.active,registers=self.registers(),
                return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
            self.new_calls.append(row);self.return_watch.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        pc=g.reg_read(UC_X86_REG_EIP)
        if COMPILED<=pc<COMPILED+0x1000:
            self.compat_stores.append(dict(instruction=f'{pc:08x}',address=f'{address:08x}',size=size,
                before=bytes(g.mem_read(address,size)).hex(),value=f'{value&((1<<(size*8))-1):0{size*2}x}'));return
        super().mem_write(g,access,address,size,value,user)

    def sync_snapshot(self):
        for c in self.compat_calls:c['after_hex']=bytes(self.g.mem_read(int(c['pointer'],16),28)).hex()
        table=self.u(0x52625c)
        body=self.u(0x486550);shape=self.u(0x526260);wrapper=self.u(0x526264)
        return dict(**self.startup_snapshot(),compatibility_initializer_calls=self.compat_calls,
            compatibility_initializer_stores=self.compat_stores,raw_physical_services=self.physical_services,
            source_game_arena=f'{self.u(0x4ed650):08x}',source_game_arena_cursor=self.u(0x4ed654),
            source_BSP_datum_table=f'{table:08x}',source_BSP_datum_table_sha256=hashlib.sha256(bytes(self.g.mem_read(table,8000))).hexdigest()if table else None,
            source_BSP_rigid_body=f'{body:08x}',source_BSP_body_hex=bytes(self.g.mem_read(body,0xa0)).hex()if body else None,
            source_BSP_shape=f'{shape:08x}',source_BSP_shape_hex=bytes(self.g.mem_read(shape,0x30)).hex()if shape else None,
            source_BSP_wrapper=f'{wrapper:08x}',source_BSP_wrapper_hex=bytes(self.g.mem_read(wrapper,0x18)).hex()if wrapper else None)


def cases(compiled):
    rows=[];traces=[]
    t=SyncTrace(compiled);p=STACK+0x300;t.g.mem_write(p,bytes([0xa5])*28)
    t.call('compatibility_initializer_dirty_platform_storage',int(compiled['entry'],16),(p,))
    actual=bytes(t.g.mem_read(p,28));assert actual[:4]==bytes([1,0xa5,4,0xa5])
    assert actual[4:]==struct.pack('<IIIIIi',0,p+8,p+8,0xffffffff,0,0)
    rows.append(dict(case='compatibility_initializer_dirty_platform_storage',supplied_platform_storage_hex='a5'*28,
        initializer_output_hex=actual.hex(),producers=t.producers,**t.sync_snapshot()));traces.append(t)
    for name,newmap in(('original_heap_writer_with_compatibility_initializer',False),('original_new_map_to_missing_current_thread_lock_entry',True)):
        t=SyncTrace(compiled);t.heap_prefix(None);assert t.u(0x566f08)==0x8000000
        if newmap:
            t.register();t.components();t.memory_backing();t.call(name,0x1c5ff0,blocked=0x80000115)
        rows.append(dict(case=name,producers=t.producers,original_calls=t.new_calls,**t.sync_snapshot()));traces.append(t)
    t=SyncTrace(compiled,True);t.call('original_physical_arena_root',0x12cb70)
    # Actual arena allocation/clear prefix; savegame resource/thread I/O tail
    # 1252BA.. is excluded, never replaced by successful I/O.
    t.active='original_game_arena_allocation_clear_prefix';t.g.reg_write(UC_X86_REG_ESP,STACK)
    t.g.emu_start(0x125280,0x1252ba,count=2000000)
    assert t.g.reg_read(UC_X86_REG_EIP)==0x1252ba
    t.producers.append(dict(name=t.active,original_fragment=['00125280','001252ba'],stop_is_exclusive=True,
        excluded_savegame_resource_thread_IO_tail=True,completion='complete_fragment'))
    assert t.u(0x4ed650)==0x82d47000
    t.call('original_BSP_datum_table_constructor',0x1f15b0)
    t.register();t.memory_backing();t.call('original_game_hkMemory',0x148190);t.call('original_Havok_singletons',0x2ded30)
    t.call('original_BSP_Havok_setup',0x1c6270)
    assert bytes(t.g.mem_read(t.u(0x52625c),8000))==b'\xff'*8000
    rows.append(dict(case='original_BSP_Havok_setup_from_source_arena_prefix',producers=t.producers,original_calls=t.new_calls,**t.sync_snapshot()));traces.append(t)
    return traces,rows


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    compiled=compile_initializer();traces,rows=cases(compiled);t=traces[0]
    args.output.parent.mkdir(parents=True,exist_ok=True);directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    executed=set().union(*(t.executed for t in traces));listings=[]
    raw=bytes.fromhex(compiled['compiled_text_hex']);compat_instructions=list(Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,COMPILED))
    compat_addresses=set().union(*(t.compat_executed for t in traces))
    assert compat_addresses<={i.address for i in compat_instructions}
    for trace in traces:assert bytes(trace.g.mem_read(COMPILED,len(raw)))==raw
    compat_listing=args.output.parent/'compatibility-instructions.asm'
    compat_listing.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in compat_instructions))
    compiled['compiled_listing_sha256']=digest(compat_listing)
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
    d=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
        instruction_manifest_sha256=manifest,instruction_listings=listings,executed_original_addresses_verified=len(executed),
        compiled_compatibility_initializer=compiled,compatibility_executed_addresses=[f'{a:08x}'for a in sorted(set().union(*(t.compat_executed for t in traces)))],
        instruction_patches=[],new_supplied_game_or_Havok_record_fields=[],cases=rows,
        summary=dict(cases=len(rows),source_heap_writer_completions_with_compatibility_platform_initializer=2,
            source_BSP_representation_completions=1,full_new_map_world_completions=0,ordinary_motion_or_contact_cases=0),
        boundaries=[
            'RtlInitializeCriticalSection uses exact pinned Cxbx primary initializer body and extracted Xbox event-struct stores, compiled32 with primary type layout/list macro. Diagnostic logging and host GetHandleInformation guard are excluded for declared raw guest storage.',
            'Generated compatibility instructions and their writes are audited separately; they are not original Xbox kernel bytes or hardware execution. The void ABI has no supplied NTSTATUS/success return.',
            'RtlEnterCriticalSection277 remains unavailable: its pinned primary implementation requires KeGetCurrentThread before lock ownership. No current-thread identity, thread fields, contention or synchronization success are supplied.',
            'Heap writer retains the frozen excluded shutdown/event/hardware startup and zero platform-flag boundary; it is not full XapiInitProcess execution.',
            'Original12CB70 hardcodes physical reservation61000..3145000 and originalXPhysicalAlloc requests30E4000 bytes. Raw OS backing/protection only are admitted; inherited cache mappings remain an explicitly separate emulator address view.',
            'Original125280..1252BA allocates/clears the actual3FE000 game arena at82D47000; original217090 savegame resource/thread IO tail and later startup allocations are excluded. No complete game-arena allocation-order claim.',
            'Original1F15B0 produces8000-byte BSP datum table from that source arena cursor; full1C6270 clears it, builds real Lockout geometry/wrapper/static rigid body and returns. No source world526234 is constructed in this separate BSP case.',
            'Original1C5FF0 with source heap writer and compatibility initializer still stops2DACAD at ordinal277 during its first C++ manager allocation, before full impact/BSP/world order.',
            'Prior raw object/resource/component/cache/context/FS boundaries persist. No biped world insertion, completed ordinary step/contact, native production change or prior frozen-suite edit.'])
    args.output.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(report=str(args.output),report_sha256=digest(args.output),oracle_sha256=d['oracle_sha256'],
        instruction_manifest_sha256=manifest,cases=len(rows))))


if __name__=='__main__':main()
