#!/usr/bin/env python3
"""Independent unchanged TU1.1 allocated-biped physics construction prefixes.

Real Lockout creation records, tags, BSP and animation resource bytes are used.
Only raw OS/object-heap backing and explicitly named resource delivery are
supplied. No physical component, rigid-body field, pose, contact or Havok return
is invented. Original Xbox FS/SEH context remains absent and blocks construction.
Earlier frozen tools/reports are read-only dependencies.
"""
import argparse
import hashlib
import json
from pathlib import Path

import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FS)
from oracle_mp_spawn_placement import PlacementTrace, PLACEMENT, digest
from oracle_mp_inventory_1_1 import ROOT, PIN, CACHE_PIN, POOL, HEADERS
from oracle_smg_firing_1_1 import TAGS, STACK, STOP, TIME, SEED
from functions import _trace

OUT = ROOT/'analysis/multiplayer/integration/spawn-source/biped-physics'
RAW_HAVOK, RAW_COMPONENTS, RESIDENT = 0x6000000, 0x6200000, 0x7000000
PRIOR = {
 'tools/oracle_mp_spawn_death_tail.py': 'e728636553ffb34db875c6a253326467b8db2a66a621035e1aaffdb180a2c553',
 'analysis/multiplayer/integration/spawn-source/death-tail/report.json': '7a467658d889a562b9d353f8378a7aba0f3bedff67f6f72b26075793248a4384',
 'tools/oracle_mp_spawn_placement.py': '29fe396eb70ae293a2c4a3cfd0469522e294b7d1524def43da6dec0f7bdd826f',
 'analysis/multiplayer/integration/spawn-source/placement-report.json': '11ae3316e030ce217f1aa73eaf59b74df483502aa4e6507697dba2bd51b02e34',
 'analysis/multiplayer/integration/spawn-source/owner-counter-report.json': 'e7492035911eb502c77b5154b3a35a9ad3c8decac254f3c5f18758d9a0d56dac',
 'analysis/multiplayer/integration/spawn-source/corpse-schedule-report.json': '649a0761e442ff5698476a18d4fcef36ebe89d4431ecb8619de497a46e357f6d',
 'analysis/multiplayer/integration/spawn-source/lethal-attribution-report.json': '46d65d088255fc1fc357a84aa6e8d17b6065387327be9c03ba9f392ce0b1c84d',
}
ENTRIES = (0x11de10, 0x1243c0, 0x1c5cf0, 0x1482a0, 0x148190,
    0x148b00, 0x2dd5b0, 0x2ded30, 0x1c5ff0, 0x1d2620, 0x11e150,
    0x16d070, 0x16d210, 0x16d220, 0x16d2c0, 0x22e4e0,
    0x148a10, 0x22e400, 0x22e1d0, 0x22e950, 0x318100, 0x312410,
    0xb8a30, 0xbd170, 0xbd270, 0x10cbd0, 0xc52c0, 0xddb40,
    0xdfaf0, 0xe6f60, 0x1ce8c0, 0x1ceac0, 0x1d0a60, 0x1cfde0,
    0x1cee00, 0x1e11b0, 0x124f30, 0x124dd0, 0x124bf0,
    0xbdf10, 0xbdf80, 0xbede0, 0x1ca950, 0x280030, 0x27fb10,
    0x11d4a0, 0x14b9d0, 0x14bd00, 0x1ce650, 0xb8180,
    0x1c7050, 0x1c6f50, 0x1c72e0, 0x1d2aa0, 0x1d3340,
    0x1d2b10, 0x1da570, 0x1da6f0, 0xbacc0, 0xe23a0, 0x1daac0,
    0x317890, 0x2e0860, 0x1dc750, 0x1d3a70, 0x1dacf0,
    0xb7e00, 0xb7f50, 0xb8040, 0xbd360, 0x10cdf0,
    0xc7e90, 0xde960, 0xddbc0, 0x1e9540, 0x1e9480, 0xe3b20,
    0x1c6270, 0x1c66a0, 0x1c6370, 0x1c7660, 0x317650,
    0x1d26f0, 0x139750)
WATCH_GLOBALS = {0x487638: 4, 0x480d90: 4, 0x480d98: 4,
    0x480d9c: 4, 0x480da0: 4, 0x526248: 4}


def file_sha(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        while raw := f.read(1024*1024):
            h.update(raw)
    return h.hexdigest()


class PhysicsTrace(PlacementTrace):
    def __init__(self, residency=False):
        self.new_calls, self.return_watch, self.producers = [], [], []
        self.new_stores, self.new_invalid = [], None
        self.resident, self.resource_requests = {}, []
        self.resource_records, self.raw_virtual_allocs = [], []
        self.instrument = False
        super().__init__(live=True)
        self.stop_at = None
        # The new allocated biped receives no reserved unit fields or poses.
        # Its runtime animation node is also allowed to execute unchanged.
        self.services.pop(0x1d0a60)
        self.g.mem_map(RAW_HAVOK, 0x200000)
        self.g.mem_map(RAW_COMPONENTS, 0x20000)
        self.g.mem_map(RESIDENT, 0x1000000)
        c = self.cache
        self.bipd = next(t for t in c.tags.values() if t['group']=='bipd'
            and t['name'].endswith('masterchief_mp'))
        self.model = c.tags[c.u(self.bipd['offset']+0x38)]
        self.graph = c.tags[c.u(self.model['offset']+0x14)]
        self.residency = residency
        at = RESIDENT
        with (ROOT/'assets/disc/maps/shared.map').open('rb') as shared:
            for n, off in enumerate(c.block(self.graph['offset']+0xac,20)):
                size, pointer = c.u(off+4), c.u(off+8)
                location, pos = pointer>>30, pointer&0x3fffffff
                # Only the same local/shared locations supported by the frozen
                # graph suites are admitted. Other locations are not guessed.
                if location not in (0,2):
                    continue
                if location == 2:
                    shared.seek(pos); raw = shared.read(size)
                else:
                    raw = bytes(c.data[pos:pos+size])
                assert len(raw)==size
                self.g.mem_write(at,raw)
                record = dict(index=n, descriptor=f'{off+c.magic:08x}',
                    descriptor_hex=bytes(c.data[off:off+20]).hex(), size=size,
                    location=location, file_offset=pos,
                    file='assets/disc/maps/shared.map' if location==2 else 'assets/disc/maps/lockout.map',
                    resident_pointer=f'{at:08x}', raw_sha256=hashlib.sha256(raw).hexdigest())
                self.resident[off+c.magic] = record
                self.resource_records.append(record)
                at += (size+4095)&~4095
        assert at <= RESIDENT+0x1000000
        self.events=[]; self.executed=set(); self.heap_requests=[]
        self.instrument=True

    def invalid(self,g,access,address,size,value,user):
        self.new_invalid = dict(access=access,address=f'{address:08x}',size=size,
            instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',phase=self.active,
            registers=self.registers(), fs_selector=g.reg_read(UC_X86_REG_FS),
            recent=[f'{a:08x}' for a in self.recent])
        return False

    def hook(self,g,a,size,user):
        if self.instrument:
            for ret,row in reversed(self.return_watch[:]):
                if a==ret:
                    row['returned_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
                    self.return_watch.remove((ret,row)); break
            sp=g.reg_read(UC_X86_REG_ESP)
            if a in ENTRIES:
                row=dict(entry=f'{a:08x}',phase=self.active,
                    registers=self.registers(),return_address=f'{self.u(sp):08x}',
                    stack=[f'{self.u(sp+4+i*4):08x}' for i in range(6)])
                if a==0x318100:
                    body=g.reg_read(UC_X86_REG_ECX);info=self.u(sp+4)
                    row.update(allocated_body=f'{body:08x}',
                        body_before_hex=bytes(g.mem_read(body,0xa0)).hex(),
                        construction_info=f'{info:08x}',
                        construction_info_hex=bytes(g.mem_read(info,0xc0)).hex())
                self.new_calls.append(row);self.return_watch.append((self.u(sp),row))
            if a==0x33785c:
                self.raw_virtual_allocs.append(dict(phase=self.active,
                    args=[f'{self.u(sp+4+i*4):08x}' for i in range(4)],
                    supplied_raw_backing=f'{self.services[a][1]:08x}'))
            if a==0x124f30 and self.residency:
                descriptor=g.reg_read(UC_X86_REG_EAX)
                assert descriptor in self.resident,('unsupported descriptor',hex(descriptor))
                record=self.resident[descriptor]
                self.resource_requests.append(dict(phase=self.active,
                    entry='00124f30', source_stack_argument=f'{self.u(sp+4):08x}',**record))
                self.executed.add(a)
                g.reg_write(UC_X86_REG_EAX,int(record['resident_pointer'],16))
                g.reg_write(UC_X86_REG_ESP,sp+8)
                g.reg_write(UC_X86_REG_EIP,self.u(sp));return
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if self.instrument:
            watched = any(a<=address<a+n for a,n in WATCH_GLOBALS.items())
            watched |= RAW_COMPONENTS<=address<RAW_COMPONENTS+0xf0
            if self.object_pointer:
                watched |= self.object_pointer<=address<self.object_pointer+0x478
            if watched:
                self.new_stores.append(dict(phase=self.active,
                    instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                    address=f'{address:08x}',size=size,
                    before=bytes(g.mem_read(address,size)).hex(),
                    value=f'{value&((1<<(size*8))-1):0{size*2}x}'))
        super().mem_write(g,access,address,size,value,user)

    def call(self,name,address,args=(),regs=(),blocked=None):
        self.active=name;start=len(self.events);self.new_invalid=None
        try:
            self.run(address,args,regs)
            completion='complete_return'
        except unicorn.UcError:
            assert blocked is not None and self.new_invalid is not None
            assert self.g.reg_read(UC_X86_REG_EIP)==blocked, self.new_invalid
            completion='blocked_original_instruction'
        if blocked is not None:
            assert completion=='blocked_original_instruction'
        self.producers.append(dict(name=name,entry=f'{address:08x}',
            arguments=[f'{v:08x}' for v in args],
            registers={str(r):f'{v:08x}' for r,v in regs},completion=completion,
            stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
            returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}' if completion=='complete_return' else None,
            invalid=self.new_invalid,services=self.events[start:]))

    def shape_records(self):
        c=self.cache;p=self.bipd['offset']
        return {f'{off:03x}':[dict(file_offset=q,address=f'{q+c.magic:08x}',
            record_hex=bytes(self.g.mem_read(q+c.magic,stride)).hex())
            for q in c.block(p+off,stride)]
            for off,stride in ((0x284,0x80),(0x28c,0x50),(0x294,0x80))}

    def fixup(self):
        c=self.cache;index=c.u(16);count=c.u(index+24)
        table=c.u(index+8)-(c.u(index)-(index+32))
        # Actual serialized instance records contain the salted datum at+4.
        # This cache-index/table view supplies the selected iterator context;
        # it does not assert complete tag-map loading or unrelated postprocess.
        self.g.mem_write(TAGS,bytes(c.data[table:table+count*16]))
        self.w(0x54fa50,index+c.magic)
        it=PLACEMENT+0x200;datum=int(self.bipd['datum'],16)
        self.w(it+12,datum&0xffff);self.w(it+16,0xffffffff)
        before=self.shape_records()
        self.call('original_selected_tag_iterator',0x1243c0,regs=((UC_X86_REG_EDI,it),))
        assert self.g.reg_read(UC_X86_REG_EAX)==datum
        dispatch=self.fragment('original_selected_bipd_dispatch',0x11de40,0x11ded1,
            ((UC_X86_REG_EAX,datum),))
        self.producers.append(dict(name='original_selected_bipd_dispatch',
            fragment=['0011de40','0011ded1'],stop_is_exclusive=True,
            completion='complete_bounded_fragment',registers=dict(eax=f'{datum:08x}')))
        after=self.shape_records()
        assert self.u(c.block(self.bipd['offset']+0x28c,0x50)[0]+c.magic+0x20)==0x45c6b0
        self.shape_fixup=dict(iterator_index=datum&0xffff,cache_tag_count=count,
            raw_instance_record_hex=bytes(c.data[table+(datum&0xffff)*16:table+(datum&0xffff)*16+16]).hex(),
            iterator_result=f'{datum:08x}',dispatch_start='0011de40',dispatch_stop_exclusive='0011ded1',
            full_shape_fixup_entry='001c5cf0',full_shape_fixup_edx=f'{self.bipd["offset"]+c.magic+0x264:08x}',
            before=before,after=after,complete_map_postprocess=False)

    def memory_backing(self):
        self.services[0x33785c]=('supplied raw OS VirtualAlloc backing only',RAW_HAVOK,16)
        self.call('original_physics_raw_os_allocation',0x1482a0)
        assert self.u(0x480d90)==RAW_HAVOK+0x1000

    def memory(self):
        self.memory_backing()
        self.call('original_game_havok_memory_construction',0x148190)
        self.fragment('original_hkMemory_singleton_install_slice',0x2ded5c,0x2ded67)
        self.producers.append(dict(name='original_hkMemory_singleton_install_slice',
            fragment=['002ded5c','002ded67'],stop_is_exclusive=True,
            completion='complete_bounded_fragment',source_previous_singleton='00000000',
            complete_havok_initialization=False))
        assert self.u(0x487638)==0x480db0
        assert self.u(RAW_HAVOK+0x1000)==0x45e77c
        assert self.u(self.u(0x480da0))==0x45e7a8

    def components(self):
        self.services[0x33785c]=('supplied raw OS component-pool backing only',RAW_COMPONENTS,16)
        self.call('original_havok_components_pool_constructor',0x1d2620)
        self.call('original_havok_components_pool_reset',0x16d210,
            regs=((UC_X86_REG_EAX,RAW_COMPONENTS),))
        assert self.u(0x526248)==RAW_COMPONENTS
        assert (self.u(RAW_COMPONENTS+0x20),self.u(RAW_COMPONENTS+0x24))==(512,0xa0)
        assert self.u(RAW_COMPONENTS+0x40,2)==0xe168
        self.pool_before_creation=bytes(self.g.mem_read(RAW_COMPONENTS,0x4c)).hex()

    def snapshot(self):
        memory=dict(singleton=f'{self.u(0x487638):08x}',
            static_game_memory_hex=bytes(self.g.mem_read(0x480db0,8)).hex(),
            raw_allocation_global=f'{self.u(0x480d90):08x}',
            game_memory_mode=self.u(0x480d98),fixed_memory=f'{self.u(0x480d9c):08x}',
            pool_memory=f'{self.u(0x480da0):08x}',
            fixed_header_hex=bytes(self.g.mem_read(RAW_HAVOK+0x1000,0x30)).hex(),
            aligned_allocation_prefix_hex=bytes(self.g.mem_read(RAW_HAVOK+0xff8,8)).hex())
        result=dict(memory=memory,source_component_pool=dict(address=f'{RAW_COMPONENTS:08x}',
            capacity=self.u(RAW_COMPONENTS+0x20),stride=self.u(RAW_COMPONENTS+0x24),
            before_creation_header_hex=getattr(self,'pool_before_creation',None),
            header_hex=bytes(self.g.mem_read(RAW_COMPONENTS,0x4c)).hex()),
            physics_live_flag=self.u(0x486560,1),source_time_tick=self.u(TIME+8),
            shared_rng=f'{self.u(SEED):08x}',local_rng=f'{self.u(SEED+4):08x}')
        if self.object_pointer:
            p=self.object_pointer;datum=self.u(HEADERS+24,2)<<16|2
            component=self.u(p+0xb4);m=p+self.u(p+0x12a,2)
            result.update(object=self.object_state(datum),unit_hex=bytes(self.g.mem_read(p,0x478)).hex(),
                biped_state=self.u(p+0x3dc,1),adjustment_count=self.u(p+0x3aa,1),
                unit_manager_offset=self.u(p+0x12a,2),unit_manager_hex=bytes(self.g.mem_read(m,0x90)).hex(),
                manager_graph=f'{self.u(m):08x}',manager_packed=f'{self.u(m+4):08x}',
                component_datum=f'{component:08x}',
                component_hex=bytes(self.g.mem_read(self.u(RAW_COMPONENTS+0x44)+(component&0xffff)*0xa0,0xa0)).hex()
                    if component!=0xffffffff else None)
        bodies=[r for r in self.new_calls if r['entry']=='00318100']
        if bodies:
            body=int(bodies[-1]['allocated_body'],16)
            result.update(rigid_body_allocation=f'{body:08x}',
                rigid_body_hex=bytes(self.g.mem_read(body,0xa0)).hex(),rigid_body_constructor_returned=False)
        return result


def cases():
    setup=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/placement-report.json').read_text())
    rows=[];traces=[]
    for name,index,residency,fixup,memory,block in (
        ('strict_resource_context_missing',0,False,True,True,0x124de8),
        ('serialized_shape_before_runtime_fixup',0,True,False,True,0x1da942),
        ('hkMemory_singleton_missing',0,True,True,False,0x1dac99),
        ('source_allocated_biped_start_0',0,True,True,True,0x318107),
        ('source_allocated_biped_start_32',32,True,True,True,0x318107),
        ('source_allocated_biped_start_65',65,True,True,True,0x318107)):
        t=PhysicsTrace(residency)
        if fixup:t.fixup()
        if memory:t.memory()
        t.components()
        row=next(r for r in setup['normal_setup']if r['version']=='tu11'and r['authored_index']==index)
        raw=bytes.fromhex(row['placement_hex']);t.g.mem_write(PLACEMENT,raw)
        t.call('original_actual_biped_creation',0xb8a30,(PLACEMENT,),blocked=block)
        state=t.snapshot()
        if block in(0x1dac99,0x318107,0x1da942):
            assert state['component_datum']=='e1680000'
            assert state['object']['position']['bits']==row['position']['bits']
            assert state['biped_state']==1
        if block==0x318107:
            assert state['rigid_body_allocation']=='06001030'
            assert state['rigid_body_hex'][8:12]=='a000'
            assert state['manager_graph']==t.graph['datum']
            assert not t.collision_queries
        rows.append(dict(case=name,authored_index=index,placement_producer_report='placement-report.json',
            placement_hex=raw.hex(),source_tag=t.bipd,source_model=t.model,source_graph=t.graph,
            resource_delivery_supplied=residency,resource_requests=t.resource_requests,
            runtime_shape_fixup=getattr(t,'shape_fixup',None),producers=t.producers,
            blocked_instruction=f'{block:08x}',retained=state,
            original_calls=t.new_calls,original_stores=t.new_stores,
            raw_virtual_allocs=t.raw_virtual_allocs,
            used_services=[e for e in t.events if 'service'in e],
            generic_object_heap_requests=t.heap_requests,root_writes=t.position_writes,
            collision_queries=t.collision_queries,complete_object_creation=False,
            ordinary_world_motion_or_contact_executed=False))
        traces.append(t)
    t=PhysicsTrace(False);t.memory_backing()
    t.call('original_complete_new_map_physics_initializer',0x1c5ff0,blocked=0x2ded37)
    rows.append(dict(case='complete_new_map_physics_initializer_missing_FS',
        producers=t.producers,blocked_instruction='002ded37',retained=t.snapshot(),
        original_calls=t.new_calls,original_stores=t.new_stores,
        raw_virtual_allocs=t.raw_virtual_allocs,
        used_services=[e for e in t.events if 'service'in e],complete_world_physics_initialization=False))
    traces.append(t)
    return traces,rows


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
    # Read-only pins are checked both before and after the new suite.
    for path,pin in PRIOR.items():assert digest(ROOT/path)==pin,path
    traces,rows=cases();t=traces[0]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
    executed=set().union(*(trace.executed for trace in traces));listings=[]
    for a in executed:
        i=t.code.at(a)
        assert t.image.read(a,len(i.bytes))==i.bytes
        assert all(bytes(trace.g.mem_read(a,len(i.bytes)))==i.bytes for trace in traces)
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES))
        ins=[t.code.at(a)for a in sorted(routine.code)]
        for i in ins:
            assert t.image.read(i.address,len(i.bytes))==i.bytes
            assert all(bytes(trace.g.mem_read(i.address,len(i.bytes)))==i.bytes for trace in traces)
        p=directory/f'{entry:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
        listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=digest(p),
            instructions=len(ins),executed=sum(i.address in executed for i in ins),
            direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
    manifest_hash=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    descriptor=t.u(0x46fad0);table=[]
    for n in range(4):
        p=t.u(descriptor+0x84+n*4)
        if not p:break
        assert t.image.read(p,0x84)==bytes(t.g.mem_read(p,0x84))
        table.append(dict(index=n,record=f'{p:08x}',record_hex=bytes(t.g.mem_read(p,0x84)).hex(),
            creation_callback_18=f'{t.u(p+0x18):08x}',update_callback_40=f'{t.u(p+0x40):08x}'))
    deps=('oracle_mp_spawn_placement.py','oracle_mp_spawn.py','oracle_mp_spawn_lifecycle.py',
        'oracle_mp_spawn_ownership.py','oracle_smg_firing_1_1.py','oracle_mp_inventory_1_1.py',
        'oracle_mp_inventory_netgame_1_1.py','oracle_mp_inventory_world_1_1.py')
    for path,pin in PRIOR.items():assert digest(ROOT/path)==pin,path
    report=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,
        shared_cache_sha256=file_sha(ROOT/'assets/disc/maps/shared.map'),
        oracle_sha256=digest(__file__),frozen_pins=PRIOR,
        dependencies={f'tools/{p}':digest(ROOT/'tools'/p)for p in deps},
        instruction_patches=[],new_direct_physical_component_body_pose_contact_writes=[],
        case_count=len(rows),cases=rows,instruction_listings=listings,
        instruction_manifest_sha256=manifest_hash,executed_original_addresses_verified=len(executed),
        original_biped_type_table=table,exact_resource_records=t.resource_records,
        summary=dict(source_created_biped_prefixes=6,
            source_fixed_shape_and_source_allocated_rigid_body_prefixes=3,
            source_complete_new_map_initializer_blocked_cases=1,
            completed_live_biped_constructors=0,ordinary_live_world_motion_cases=0,
            source_generated_live_contacts=0,original_instruction_bytes_verified=True),
        boundaries=[
            'The source-produced TU placement records come from the independently pinned earlier normal-setup suite; original creation/loadout leaves in that earlier suite remain explicit boundaries.',
            'Inherited64-header arena with two excluded entries, generic object-heap raw backing/growth, actual Lockout BSP loading and active cluster mask are supplied startup inputs; original startup datum order is unproved.',
            'The allocated biped body, extension/control/animation manager, current node matrices, location, component pool/record and hkMemory allocations are produced by original instructions in this new suite.',
            'Source physical-component pool1D2620/16D210 is512xA0 with source-derivedE168 first salt; the inherited64-slot physical pool is replaced through the original initializer.',
            'Live486560=1 remains an explicit requested-construction context input; complete1C5FF0 is executed separately and blocks before its source live-flag store.',
            'Selected1243C0 iterator uses literal cache index/table bytes and explicit starting index/groupNONE;11DE40..11DED1 dispatch and full1C5CF0 execute. Complete unrelated map postprocess is excluded.',
            'VirtualAlloc33785C receives source-owned original arguments and returns only raw mapped backing; original fixed-memory/pool constructors and allocation logic own Havok fields/results.',
            '2DED5C..2DED67 executes the original getter/setter singleton installation with487638 initiallyNULL; the remaining full Havok initialization remains unexecuted because original FS/SEH context is missing.',
            '124F30 exact local/shared animation resource delivery is supplied only in explicitly marked cases; full original resource retirement/cache/I-O pools4EB11C/4EB124 are uninitialized and independently shown to block the strict case.',
            'No Xbox FS/SEH thread record, Havok body/world return, physical component field, root/node/head pose or contact is supplied.',
            '318107 blocks before hkRigidBody initialization; allocated raw A0 storage with source allocation-size word is not a completed body. No source world insertion, ordinary biped update or live contact follows this partial constructor.',
            'Original object-update table/scheduler listings identify later consumers statically; listed routines with zero executed rows are not claimed as executed live motion.',
            'Earlier freezes and production are untouched.'])
    args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(rows),report=str(args.output),sha256=digest(args.output),
        oracle_sha256=report['oracle_sha256'],instruction_manifest_sha256=manifest_hash,
        executed_original_addresses_verified=len(executed),summary=report['summary'])))


if __name__=='__main__':main()
