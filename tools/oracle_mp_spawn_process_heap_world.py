#!/usr/bin/env python3
"""Separate unchanged XAPI heap and real Lockout world constructor investigation.

Literal XBE header inputs and original producer instructions execute. Only raw
NtAllocateVirtualMemory backing may be provided under its source request ABI.
Unavailable kernel synchronization and whole startup remain explicit gates.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EBP
from oracle_mp_spawn_biped_platform import PlatformTrace,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,ENTRIES as PRIOR_ENTRIES,digest
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/process-heap-world'
RAW_HEAP=0x8000000
ENTRIES=PRIOR_ENTRIES+(0x2d7f8e,0x2dc5ff,0x2da83a,0x2da2eb,0x2d9824,0x2d9d19,
    0x2d78e7,0x27dae0,0x1c66a0,0x317720,0x316710,
    0x2284c0,0x1c6270,0x2dd5f0,0x2df710,0x22e430,0x1f15b0,0x1f1600,0x1c74d0,0x2d6f2e)
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_biped_platform.py':'709a0621c00c48146a7a8b143f2210671f20814e0a8a76b7c95e05a9b0be1647',
 'analysis/multiplayer/integration/spawn-source/biped-platform-context/report.json':'02a3e800af3c579d4635b159f6ce29d5bd0a3dc57e6877ed70bb70d5b0033633'}


class KernelBoundary(Exception):pass


class StartupTrace(PlatformTrace):
    def __init__(self,raw_heap=False):
        self.os_allocations=[];self.heap_stores=[];self.raw_heap=raw_heap;self.heap_reserved=False
        super().__init__(True)
        size=struct.unpack_from('<I',self.image.data,0x108)[0]
        assert size==0x2000 and self.image.base==0x10000
        self.g.mem_write(self.image.base,self.image.data[:size])
        self.literal_header=dict(base=f'{self.image.base:08x}',size=size,
            bytes_sha256=hashlib.sha256(self.image.data[:size]).hexdigest(),
            init_flags=f'{self.u(0x10124):08x}',heap_reserve=f'{self.u(0x10134):08x}',
            heap_commit=f'{self.u(0x10138):08x}')

    def hook(self,g,a,size,user):
        if 0x80000000<=a<0x80001000 and not(self.raw_heap and a==0x800000b8):
            sp=g.reg_read(UC_X86_REG_ESP)
            self.new_invalid=dict(boundary='unresolved original kernel import ordinal; target is never executed',
                kernel_target=f'{a:08x}',kernel_ordinal=a&0x7fffffff,
                original_return_site=f'{self.u(sp):08x}',original_call_site=f'{self.recent[-1]:08x}',
                stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],registers=self.registers())
            raise KernelBoundary()
        if self.instrument and a in set(ENTRIES)-set(PRIOR_ENTRIES):
            sp=g.reg_read(UC_X86_REG_ESP)
            row=dict(entry=f'{a:08x}',phase=self.active,registers=self.registers(),
                return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
            self.new_calls.append(row);self.return_watch.append((self.u(sp),row))
        if self.raw_heap and a==0x800000b8:
            sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(5)]
            baseptr,zerobits,sizeptr,kind,protect=args
            requested_base,requested_size=self.u(baseptr),self.u(sizeptr)
            assert zerobits==0 and protect==4
            if kind==0x2000:
                assert not self.heap_reserved and requested_base==0 and requested_size==0x100000
                self.g.mem_map(RAW_HEAP,requested_size)
                self.w(baseptr,RAW_HEAP);self.heap_reserved=True
            else:
                assert kind==0x1000 and self.heap_reserved and requested_base==RAW_HEAP and requested_size==0x1000
            self.os_allocations.append(dict(phase=self.active,original_return_site=f'{self.u(sp):08x}',
                kernel_ordinal=184,args=[f'{v:08x}'for v in args],
                requested_base=f'{requested_base:08x}',requested_size=requested_size,
                resulting_base=f'{self.u(baseptr):08x}',resulting_size=self.u(sizeptr),
                supplied_ntstatus='00000000',scope='raw reserve/commit backing; no heap/game/Havok object fields'))
            g.reg_write(UC_X86_REG_EAX,0);g.reg_write(UC_X86_REG_ESP,sp+24)
            g.reg_write(UC_X86_REG_EIP,self.u(sp));return
        super().hook(g,a,size,user)

    def call(self,name,address,args=(),regs=(),blocked=None):
        try:super().call(name,address,args,regs,blocked)
        except KernelBoundary:
            assert self.g.reg_read(UC_X86_REG_EIP)==blocked,self.new_invalid
            self.producers.append(dict(name=name,entry=f'{address:08x}',arguments=[f'{v:08x}'for v in args],
                completion='blocked_kernel_import',invalid=self.new_invalid,stopped_eip=f'{blocked:08x}'))

    def mem_write(self,g,access,address,size,value,user):
        if self.instrument and (RAW_HEAP<=address<RAW_HEAP+0x100000 or address==0x566f08 or address==0x526234):
            self.heap_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                address=f'{address:08x}',size=size,before=bytes(g.mem_read(address,size)).hex(),
                value=f'{value&((1<<(size*8))-1):0{size*2}x}'))
        super().mem_write(g,access,address,size,value,user)

    def heap_prefix(self,blocked):
        # Original XAPI prologue, exclusive stop immediately before the first
        # unavailable kernel service. Keep that actual source-created frame.
        self.fragment('original_XapiInitProcess_prologue',0x2d7f8e,0x2d7fa5)
        self.active='original_XapiInitProcess_heap_writer_fragment';self.new_invalid=None
        # Kernel shutdown registration and XAPI hardware/event initialization
        # 2D7FA5..2D7FB0 are excluded, not replaced with success returns.
        try:self.g.emu_start(0x2d7fb0,0x2d7fe3,count=1000000)
        except (unicorn.UcError,KernelBoundary):
            assert self.new_invalid and self.g.reg_read(UC_X86_REG_EIP)==blocked,self.new_invalid
        else:assert blocked is None
        self.producers.append(dict(name=self.active,original_fragment=['002d7fb0','002d7fe3'],
            excluded_platform_interval=['002d7fa5','002d7fb0'],stop_is_exclusive=True,
            completion='blocked_original_instruction'if blocked else'complete_fragment',invalid=self.new_invalid,
            stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}'))

    def startup_snapshot(self):
        world=self.u(0x526234)
        return dict(literal_header=self.literal_header,original_heap_global=f'{self.u(0x566f08):08x}',
            raw_heap_prefix_hex=bytes(self.g.mem_read(RAW_HEAP,0x650)).hex()if self.heap_reserved else None,
            original_heap_stores=self.heap_stores,raw_platform_allocations=self.os_allocations,
            xapi_global_flags=f'{self.u(0x566ef0):08x}',retained=self.snapshot(),
            original_world_hex=bytes(self.g.mem_read(world,0x260)).hex()if world else None,
            fixed_memory_record_hex=bytes(self.g.mem_read(self.u(0x480d9c),0x30)).hex()if self.u(0x480d9c)else None,
            original_static_bsp=f'{self.u(0x4e7918):08x}',
            original_bsp_world_bounds_hex=bytes(self.g.mem_read(self.u(0x4e7918)+0x1ec,0x18)).hex())


def cases():
    rows=[];traces=[]
    t=StartupTrace();t.call('full_XapiInitProcess_missing_shutdown_kernel_service',0x2d7f8e,blocked=0x8000002f)
    rows.append(dict(case='full_XapiInitProcess_missing_shutdown_service',producers=t.producers,
        original_calls=t.new_calls,**t.startup_snapshot()));traces.append(t)
    for supplied,block in((False,0x800000b8),(True,0x80000123)):
        t=StartupTrace(supplied);t.heap_prefix(block)
        rows.append(dict(case='heap_writer_raw_OS_backing_to_missing_critical_section'if supplied else'heap_writer_missing_NtAllocateVirtualMemory',
            producers=t.producers,original_calls=t.new_calls,**t.startup_snapshot()));traces.append(t)
    t=StartupTrace();t.register();t.memory_backing();t.call('original_game_havok_memory',0x148190)
    t.call('original_Havok_singletons',0x2ded30)
    t.call('direct_original_world_constructor',0x1c66a0,blocked=0x2dd621)
    core=next(r for r in t.new_calls if r['entry']=='00316710')
    assert core['returned_eax']==f'{t.u(0x526234):08x}'=='06005290'
    failed=[r for r in t.new_calls if r['entry']=='0022e1d0'][-1]
    assert failed['stack'][:2]==['00000010','00002010']and failed['returned_eax']=='00000000'
    rows.append(dict(case='direct_world_constructor_to_original_fixed_memory_exhaustion',producers=t.producers,
        original_calls=t.new_calls,direct_entry_excludes_full_new_map_manager_and_BSP_shape_setup=True,
        core_world_constructor_returned=True,complete_map_world_initialization=False,
        **t.startup_snapshot()));traces.append(t)
    return traces,rows


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    traces,rows=cases();t=traces[0]
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
        listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=digest(p),instructions=len(ins),
            executed=sum(i.address in executed for i in ins),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
    manifest=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    d=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
        instruction_manifest_sha256=manifest,instruction_listings=listings,executed_original_addresses_verified=len(executed),
        instruction_patches=[],new_supplied_game_heap_physical_body_pose_contact_fields=[],cases=rows,
        summary=dict(cases=len(rows),full_XapiInitProcess_completions=0,full_process_heap_completions=0,
            raw_NtAllocateVirtualMemory_cases=1,core_hkWorld_constructor_returns=1,
            full_new_map_world_completions=0,live_world_motion_or_contact_cases=0),
        boundaries=[
            'The frozen compatibility ABI supplies FS offset0/FFFFFFFF only; all previous body/resource/BSP/context boundaries persist.',
            'Literal original TU XBE header bytes are newly installed unchanged, including reserve100000/commit1000/init80000005. This is executable data, not a guessed heap default.',
            'Full original XapiInitProcess stops at its first unavailable kernel import HalRegisterShutdownNotification ordinal47; no shutdown success or platform event/hardware fields are supplied.',
            'The separate heap writer executes the actual XAPI prologue and2D7FB0..2D7FE3, excluding kernel shutdown and hardware/event setup2D7FA5..2D7FB0. Zero loaded XAPI flags are retained but not claimed ordinary platform-produced state.',
            'The raw OS case supplies only the source NtAllocateVirtualMemory five-argument reserve/commit protocol and raw zero backing, with declared NTSTATUS0; it constructs no heap/synchronization/game/Havok fields.',
            'Original2DA83A produces heap metadata and free-list heads, then stops at RtlInitializeCriticalSection ordinal291 before heap constructor return/global566F08 installation. No synchronization return/fields are supplied.',
            'Kernel import ordinal targets are intercepted before execution; overlapping cache-address backing is never interpreted as kernel instructions.',
            'Separate direct1C66A0 executes actual Lockout bounds/config and core316710 return/global526234 store, but excludes full new-map manager and BSP shape construction.',
            'Direct1C66A0 later original temporary2010-byte allocation returnsNULL in source mode1 fixed allocator;2DD621 writesNULL. No expanded arena, alternative allocation mode or success return is supplied.',
            'A core world constructor return is distinct from completed map-world initialization, body insertion, ordinary step/contact or original Xbox execution.',
            'Prior frozen suites and production remain untouched.'])
    args.output.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(report=str(args.output),report_sha256=digest(args.output),oracle_sha256=d['oracle_sha256'],
        instruction_manifest_sha256=manifest,cases=len(rows))))


if __name__=='__main__':main()
