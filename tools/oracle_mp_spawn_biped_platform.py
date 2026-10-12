#!/usr/bin/env python3
"""Separate sourced compatibility-ABI context extension of the frozen prefix.

Only FS exception-chain backing is supplied from pinned primary Xbox ABI
implementation evidence. Original rigid-body/motion, factory registration and
Havok initialization execute unchanged. World/process-heap context remains an
explicit blocker. This does not establish original kernel or Xbox execution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_ECX,
    UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_GDTR,UC_X86_REG_CS,
    UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_FS,UC_X86_REG_FS_BASE)
from oracle_mp_spawn_biped_physics import (PhysicsTrace,ROOT,PIN,CACHE_PIN,
    PRIOR,PLACEMENT,ENTRIES as PREFIX_ENTRIES,file_sha,digest)
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/biped-platform-context'
GDT,TIB=0x6300000,0x6400000
CONSTRUCTORS={0x37f110,0x37f130,0x37f150,0x37f1e0,0x37f240,
    0x37fc70,0x37fc90,0x37fcb0}
ENTRIES=PREFIX_ENTRIES+(0x2d856e,0x2d8606,0x326b74,0x2d80fd,
    *sorted(CONSTRUCTORS),0x2de250,0x2de4b0,0x2de4f0,
    0x2ddab0,0x2ddb20,0x2dda40,0x2dd9b0,
    0x317ea0,0x320cc0,0x317bf0,0x1dc0c0,0x1d4f30,
    0x1c7320,0x315820,0x32684b,0x3293a6,0x2dac5e)
FROZEN={**PRIOR,
    'tools/oracle_mp_spawn_biped_physics.py':'a2afef3a4762b991bf1cb974042b858b952b7d712583afd44960f766532abbcd',
    'analysis/multiplayer/integration/spawn-source/biped-physics/report.json':'928cf90743fd274461c594c7470708f5505b57fbb65b29c23b4bb69a597fea1c'}
SOURCES_PIN='02e1f9ab338ca3171d3772132b46054b8fc8e1fdbb3b15086f824d2192ad8675'


def segment(base,access):
    # Emulator routing descriptors: flat32 code/data and FS-base routing.
    # Their addresses/selectors are declared fixture choices, not Xbox dumps.
    limit=0xfffff
    return struct.pack('<II',(limit&0xffff)|((base&0xffff)<<16),
        ((base>>16)&255)|(access<<8)|(limit&0xf0000)|(0xc<<20)|(base&0xff000000))


class PlatformTrace(PhysicsTrace):
    def __init__(self,residency=True):
        self.platform_stores=[]
        super().__init__(residency)
        self.g.mem_map(GDT,0x1000);self.g.mem_map(TIB,0x1000)
        # Primary pinned compatibility implementation defines offset0 and
        # explicitly initializes X_EXCEPTION_CHAIN_END=FFFFFFFF. No other
        # thread/TLS/kernel fields are supplied or asserted initialized.
        self.w(TIB,0xffffffff)
        self.g.mem_write(GDT+8,segment(0,0x9b)+segment(0,0x93)+segment(TIB,0x93))
        self.g.reg_write(UC_X86_REG_GDTR,(0,GDT,0xfff,0))
        for reg,value in((UC_X86_REG_CS,8),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),
                (UC_X86_REG_SS,16),(UC_X86_REG_FS,24)):
            self.g.reg_write(reg,value)
        assert self.g.reg_read(UC_X86_REG_FS_BASE)==TIB
        self.platform_input=dict(evidence='pinned Cxbx-Reloaded compatibility ABI; not original kernel/Xbox execution',
            backing_address=f'{TIB:08x}',backing_capacity=0x1000,
            provided_fields={'000':'ffffffff'},gdt_address=f'{GDT:08x}',
            gdt_descriptor_hex=bytes(self.g.mem_read(GDT+8,24)).hex(),
            selectors=dict(cs=8,ds=16,es=16,ss=16,fs=24),
            selector_and_address_scope='fixture routing only; no actual Xbox selector/address claim',
            unprovided_thread_tls_fields=True)

    def hook(self,g,a,size,user):
        if self.instrument and a in set(ENTRIES)-set(PREFIX_ENTRIES):
            sp=g.reg_read(UC_X86_REG_ESP)
            row=dict(entry=f'{a:08x}',phase=self.active,registers=self.registers(),
                return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
            self.new_calls.append(row);self.return_watch.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if self.instrument and TIB<=address<TIB+0x1000:
            self.platform_stores.append(dict(phase=self.active,
                instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',offset=f'{address-TIB:03x}',
                size=size,before=bytes(g.mem_read(address,size)).hex(),
                value=f'{value&((1<<(size*8))-1):0{size*2}x}'))
        super().mem_write(g,access,address,size,value,user)

    def register(self):
        self.registration=[]
        for at in range(0x468630,0x468cb8,4):
            target=self.u(at)
            if target not in CONSTRUCTORS:continue
            before=self.u(0x487694)
            self.call('original_havok_static_registration',target)
            record=self.u(0x487694)
            assert self.u(record+4)==before
            self.registration.append(dict(original_crt_table_word=f'{at:08x}',
                original_table_hex=bytes(self.g.mem_read(at,4)).hex(),entry=f'{target:08x}',
                previous_head=f'{before:08x}',record=f'{record:08x}',
                record_hex=bytes(self.g.mem_read(record,12)).hex(),
                factory=f'{self.u(record):08x}',target_global=f'{self.u(record+8):08x}'))
        assert len(self.registration)==8

    def snapshot(self):
        result=super().snapshot()
        result.update(platform_input=self.platform_input,
            fs_head_after=f'{self.u(TIB):08x}',original_fs_stores=self.platform_stores,
            factory_registration=getattr(self,'registration',None),
            havok_init_flag=self.u(0x48766c,1),world=f'{self.u(0x526234):08x}',
            physics_manager=f'{self.u(0x526538):08x}',
            global_objects={f'{a:08x}':f'{self.u(a):08x}'for a in
                (0x487638,0x487668,0x487680,0x487684,0x487688,0x487694,0x487698,0x48769c,0x4876a0)})
        constructors=[r for r in self.new_calls if r['entry']=='00318100']
        if constructors:
            returned='returned_eax'in constructors[-1]
            result['rigid_body_constructor_returned']=returned
            if returned:
                body=int(constructors[-1]['returned_eax'],16)
                assert body==int(constructors[-1]['allocated_body'],16)
                motion=self.u(body+0x3c)
                result.update(rigid_body_vtable=f'{self.u(body):08x}',
                    motion_pointer=f'{motion:08x}',motion_prefix_90_hex=bytes(self.g.mem_read(motion,0x90)).hex(),
                    body_transform_hex=bytes(self.g.mem_read(self.u(body+0x14),0x30)).hex())
        if result.get('component_datum')not in(None,'ffffffff'):
            pool=self.u(0x526248);c=self.u(pool+0x44)+(int(result['component_datum'],16)&0xffff)*0xa0
            n=self.u(c+0x74);p=self.u(c+0x70)
            result.update(body_slot_count=n,body_slot_array=f'{p:08x}',
                source_body_slot_array_hex=bytes(self.g.mem_read(p,n*0x60)).hex()if n else None)
        globals_rows=[]
        for a in(0x487668,0x487680,0x487684,0x487688,0x487698,0x48769c,0x4876a0):
            p=self.u(a)
            if p:
                globals_rows.append(dict(global_address=f'{a:08x}',object=f'{p:08x}',
                    header_20_hex=bytes(self.g.mem_read(p,0x20)).hex()))
        result['source_havok_singleton_headers']=globals_rows
        return result


def cases():
    setup=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/placement-report.json').read_text())
    rows=[];traces=[]
    for index in(0,32,65):
        t=PlatformTrace();t.fixup();t.memory();t.components()
        r=next(r for r in setup['normal_setup']if r['version']=='tu11'and r['authored_index']==index)
        t.g.mem_write(PLACEMENT,bytes.fromhex(r['placement_hex']))
        t.call('original_biped_creation_to_missing_world',0xb8a30,(PLACEMENT,),blocked=0x315824)
        retained=t.snapshot()
        assert retained['rigid_body_constructor_returned']
        assert retained['rigid_body_vtable']=='0041a950'
        assert retained['body_slot_count']==1 and retained['fs_head_after']=='ffffffff'
        assert retained['world']=='00000000'and not t.collision_queries
        rows.append(dict(case=f'original_rigid_body_start_{index}',authored_index=index,
            placement_hex=r['placement_hex'],producers=t.producers,retained=retained,
            original_calls=t.new_calls,original_stores=t.new_stores,
            resource_requests=t.resource_requests,used_services=[e for e in t.events if 'service'in e],
            generic_object_heap_requests=t.heap_requests,
            complete_rigid_body_constructor=True,complete_biped_creation=False,
            ordinary_world_motion_or_contact_executed=False))
        traces.append(t)
    for name,registered,entry,block in(
        ('havok_init_missing_static_registration',False,0x2ded30,0x2dee15),
        ('complete_havok_init_with_original_static_registration',True,0x2ded30,None),
        ('new_map_physics_missing_process_heap',True,0x1c5ff0,0x2dac7f)):
        t=PlatformTrace()
        if registered:t.register()
        if entry==0x1c5ff0:t.components()
        t.memory_backing()
        if entry==0x2ded30:t.call('original_game_havok_memory_construction',0x148190)
        t.call(name,entry,blocked=block)
        retained=t.snapshot()
        if block is None:
            assert retained['havok_init_flag']==1 and retained['fs_head_after']=='ffffffff'
            assert retained['global_objects']['00487668']=='06005140'
        rows.append(dict(case=name,producers=t.producers,retained=retained,
            original_calls=t.new_calls,original_stores=t.new_stores,
            raw_virtual_allocs=t.raw_virtual_allocs,used_services=[e for e in t.events if 'service'in e],
            complete_havok_singleton_initialization=block is None,
            complete_world_initialization=False))
        traces.append(t)
    t=PlatformTrace(False)
    # Actual XAPI/SEH original instructions, stopping before thread/TLS access.
    # No start-routine call or invented current-thread structure is supplied.
    t.fragment('original_XapiThreadStartup_SEH_prefix',0x2d856e,0x2d857a)
    head=t.u(TIB);assert t.u(head)==0xffffffff
    rows.append(dict(case='original_XapiThreadStartup_SEH_registration_prefix',
        original_fragment=['002d856e','002d857a'],stop_is_exclusive=True,
        source_seh_head=f'{head:08x}',source_seh_frame_hex=bytes(t.g.mem_read(head,24)).hex(),
        retained=t.snapshot(),original_calls=t.new_calls,complete_thread_startup=False))
    traces.append(t)
    return traces,rows


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    assert digest(OUT/'sources.json')==SOURCES_PIN
    sources=json.loads((OUT/'sources.json').read_text())
    for r in sources:assert digest(OUT/r['saved'])==r['sha256']
    types=(OUT/'Cxbx-Reloaded-types.h').read_text()
    ki=(OUT/'Cxbx-Reloaded-EmuKrnlKi.cpp').read_text()
    assert '#define X_EXCEPTION_CHAIN_END       0xFFFFFFFF'in types
    assert '*ExceptionList;          // 0x00'in types
    assert 'CtxSwitchFrame->ExceptionList = reinterpret_cast<PVOID>(X_EXCEPTION_CHAIN_END);'in ki
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
            executed=sum(i.address in executed for i in ins),
            direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
    manifest_hash=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
    report=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),
        frozen_pins=FROZEN,compatibility_abi_source_manifest_sha256=SOURCES_PIN,
        compatibility_abi_primary_sources=sources,
        instruction_patches=[],new_supplied_physical_body_pose_contact_fields=[],
        instruction_manifest_sha256=manifest_hash,instruction_listings=listings,
        executed_original_addresses_verified=len(executed),case_count=len(rows),cases=rows,
        summary=dict(source_rigid_body_constructors_completed=3,
            source_havok_singleton_initializers_completed=1,
            source_Xapi_SEH_prefixes=1,completed_live_biped_creations=0,
            completed_world_constructors=0,ordinary_live_motion_or_contact_cases=0),
        boundaries=[
            'FS offset0/FFFFFFFF empty-chain sentinel comes from pinned primary Cxbx-Reloaded Xbox compatibility ABI implementation; original kernel bytes and hardware execution are unavailable.',
            'The GDT/segment descriptors and backing addresses are emulator routing inputs; no actual Xbox selector, address, whole kernel thread/TLS state or exception dispatch is claimed.',
            'Original XAPI2D856E/326B74 registers an outer SEH frame in a bounded prefix; no actual current-thread/TLS fields or start routine are supplied.',
            'Every source rigid-body/motion/component/body-slot/animation/pose field is original-instruction output; no Havok return values are supplied.',
            'Rigid-body cases inherit the frozen source hkMemory singleton-install slice, with mode1 and raw OS backing; full Havok initialization is executed in separate cases.',
            'Only the eight relevant static registration callbacks from the actual CRT table execute, in original table order. Complete process-wide_cinit/XAPI initialization is excluded.',
            'Full2DED30 returns with original factory-produced singleton487668 and source initialized flag1; this is not the complete1C5FF0 world initializer.',
            'B8A30 stops315824 on NULL source world526234; complete1C5FF0 stops2DAC7F at missing original process-heap receiver+18, before world construction.',
            'Raw OS/object-heap backing, resource delivery and earlier allocation/BSP/cluster/context fixture boundaries remain declared. No completed live world step, collision admission, contacts or Xbox float parity claim.',
            'Frozen7-case prefix, earlier suites and production remain untouched.'])
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(report=str(args.output),report_sha256=digest(args.output),oracle_sha256=report['oracle_sha256'],
        instruction_manifest_sha256=manifest_hash,cases=len(rows),summary=report['summary'])))


if __name__=='__main__':main()
