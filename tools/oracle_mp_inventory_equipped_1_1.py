#!/usr/bin/env python3
"""Separate unchanged TU1.1 equipped Spartan pickup admission execution.

Exact cache animation bytes, hardware packets, allocated player/unit arena and
world heap backing storage are explicit boundaries. The capability, authored
world target allocation, spatial enumeration, terrain visibility, held-input
action request and inventory/equipped animation transitions execute unchanged.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_spartan_equipped_animation_1_1 import EquippedTrace, MANAGER_OFFSET, REQUEST
from oracle_mp_inventory_controls_1_1 import ControlsMixin
from oracle_mp_inventory_world_1_1 import WorldTrace
from oracle_mp_inventory_spatial_1_1 import SpatialTrace
from oracle_mp_inventory_1_1 import (
    InventoryTrace, ROOT, PIN, CACHE_PIN, Cache, SCRATCH, POOL, HEADERS,
    UNIT, UI, WEAPON, WI, GAME, TIME, SEED, STOP, _trace,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
)
from oracle_smg_firing_1_1 import TAGS, PLAYER_DATA

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/equipped-pickup'
ENTRIES = (0x14e240, 0xcd3b0, 0xc52c0, 0x101bb0, 0x15c110, 0xb8a30,
    0x16d2c0, 0x18fce0, 0x18fd70, 0xbdf10, 0xbdf80, 0xbede0,
    0x11d4a0, 0xbbf40, 0xb94f0, 0xb96a0, 0xcfed0, 0x1527c0, 0x152e90, 0xc57a0, 0x16b240,
    0xceaf0, 0xcec00, 0x111490, 0x1cfde0, 0x1cfce0, 0x1deb60,
    0x126350, 0x219d50, 0x187610, 0x1884e0, 0x14e7e0, 0x150dd0,
    0x150f00, 0x151c90, 0x1533e0, 0x153090, 0xe7f30, 0xe7ef0,
    0xea660, 0xce510, 0xe9fe0, 0xe9b40, 0xe9a90, 0xe9d50, 0xe9c50,
    0x111500, 0x115650, 0xc9a40, 0x1042e0, 0x1040f0, 0x1021d0,
    0x1d0a60, 0x1d0ed0, 0xc6c50, 0xe7e60, 0x112e10)


class EquippedPickupTrace(ControlsMixin, EquippedTrace):
    action = InventoryTrace.action
    cluster_chain = SpatialTrace.cluster_chain
    registry = SpatialTrace.registry

    def object_state(self,datum):
        result=WorldTrace.object_state(self,datum)
        p=int(result['address'],16)
        result['weapon_ready_ticks']=self.u(p+0x17c,2)
        result['weapon_animation_index']=self.u(p+0x17e,2)
        result['weapon_flags']=f'{self.u(p+0x16c,2):04x}'
        result['weapon_input_flags']=f'{self.u(p+0x16e,2):04x}'
        return result

    def inventory_state(self):
        # Resolve actual allocator-owned header salt and address, including the
        # explicitly preallocated starting weapon datum in slot0.
        result = InventoryTrace.inventory_state(self)
        result['weapons'] = []
        for n in range(self.u(POOL+0x38)):
            h = HEADERS+n*12
            if self.u(h+3, 1) == 2 and self.u(h+8):
                result['weapons'].append(self.object_state(self.u(h,2)<<16 | n))
        result['manager'] = self.manager_state()
        return result

    def manager_state(self):
        m = UNIT+MANAGER_OFFSET
        packed = self.u(m+4)
        index = packed>>16
        clip = self.live_metadata['animations'][index] if index<len(self.live_metadata['animations']) else None
        secondary_at=UNIT+self.u(UNIT+0x33e,2)+0x9c
        secondary_index=self.u(secondary_at+6,2)
        return dict(packed=f'{packed:08x}', clip=None if clip is None else dict(index=index,name=clip['name']),
            frame_bits=f'{self.u(m+0x1c):08x}', flags=f'{self.u(m+0x12,2):04x}',
            mode=f'{self.u(m+0x70):08x}', weapon_class=f'{self.u(m+0x74):08x}',
            weapon_name=f'{self.u(m+0x78):08x}', activity=f'{self.u(m+0x7c):08x}',
            manager_hex=bytes(self.g.mem_read(m,0x90)).hex(),
            secondary_channel_hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+0x33e,2)+0x9c,32)).hex(),
            secondary_clip=None if secondary_index>=len(self.live_metadata['animations']) else dict(
                index=secondary_index,name=self.live_metadata['animations'][secondary_index]['name']),
            secondary_frame_bits=f'{self.u(secondary_at+0x1c):08x}',
            controlled_unit_flags=f'{self.u(UNIT+0x134):08x}',
            unit_action_hex=bytes(self.g.mem_read(UNIT+0x704,12)).hex(),
            unit_control_hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+0x346,2),0x44)).hex())

    def selected(self):
        result = self.manager_state()
        result['equipped_indices_hex'] = bytes(self.g.mem_read(UNIT+0x212,6)).hex()
        return result

    def __init__(self, row=1, near=True):
        self.controls_ready=False
        self.executed=set()
        super().__init__()
        self.inventory_rows=[]
        self.source_calls=[]
        self.source_returns=[]
        self.heap_requests=[]
        self.collision_queries=[]
        self.collision_returns=[]
        self.control_gate_writes=[]
        self.cache=Cache(ROOT/'assets/disc/maps/lockout.map')
        self.w(0x4e7920,self.cache.scenario+self.cache.magic)
        self.w(0x4e791c,next(t for t in self.cache.tags.values() if t['group']=='matg')['offset']+self.cache.magic)
        self.w(0x566084,SCRATCH);self.w(SCRATCH,0x463b48)
        self.g.mem_write(GAME+12,b'\x01')
        self.weapon_resources=[]
        self.g.mem_map(0x7100000,0x400000);resident_at=0x7100000
        with (ROOT/'assets/disc/maps/shared.map').open('rb') as shared:
            for tag in self.cache.tags.values():
                if tag['group']!='jmad' or tag['name'] not in (
                    'objects\\weapons\\rifle\\smg\\smg',
                    'objects\\weapons\\rifle\\battle_rifle\\battle_rifle'):continue
                for i,off in enumerate(self.cache.block(tag['offset']+0xac,20)):
                    size,pointer=self.cache.u(off+4),self.cache.u(off+8)
                    location=pointer>>30;file_offset=pointer&0x3fffffff
                    assert location in (0,2)
                    if location==2:shared.seek(file_offset);raw=shared.read(size)
                    else:raw=bytes(self.cache.data[file_offset:file_offset+size])
                    assert len(raw)==size
                    self.g.mem_write(resident_at,raw)
                    resource=dict(resident_pointer=resident_at,index=i,size=size,
                        file_offset=file_offset,file='assets/disc/maps/shared.map' if location==2 else 'assets/disc/maps/lockout.map',
                        location=location,descriptor_hex=bytes(self.cache.data[off:off+20]).hex(),raw_sha256=hashlib.sha256(raw).hexdigest())
                    self.resident[off+self.cache.magic]=resource
                    self.weapon_resources.append(dict(graph=tag,descriptor_pointer=f'{off+self.cache.magic:08x}',**resource))
                    resident_at+=(size+4095)&~4095
        assert resident_at<=0x7500000
        # Undo only the inherited cloned SMG model; use every exact cache tag
        # address and group for original allocation and metadata lookup.
        for datum,tag in self.cache.tags.items():
            if tag['offset'] is not None:
                self.w(TAGS+(datum&0xffff)*16,int.from_bytes(tag['group'].encode('ascii'),'big'))
                self.w(TAGS+(datum&0xffff)*16+8,tag['offset']+self.cache.magic)
        self.w(POOL+0x20,64);self.w(POOL+0x34,2);self.w(POOL+0x38,2)
        self.w(POOL+0x3c,2);self.sh(POOL+0x40,1)
        self.g.mem_map(0x4000000,0x20000);self.heap_next=0x4000000
        self.services.pop(0xb8a30,None)
        self.services.pop(0x1e20c0,None)
        self.services[0x18fce0]=('supplied object heap backing storage; original datum allocator executes',1,12)
        self.services[0xc10e0]=('supplied looping sound source unavailable',0xffffffff,20)
        self.services[0x170060]=('supplied audio playback boundary',0,4)
        self.services[0x246a50]=('supplied BSP Havok shape test unavailable; terrain plane/surface path executes',0,12)
        self.w(0x526254,SCRATCH+0x600)
        self.action('original_physics_constants',0x1eefe0)
        self.setup_world_storage()
        self.g.mem_map(0x4030000,0x5000);self.w(0x4f4850,0x4030000)
        self.fragment('original_reset_intact_breakable_bits',0x15b670,0x15b680,((UC_X86_REG_EBX,0xffffffff),))
        startup=self.initialize();self.equipped_active=True
        self.w(PLAYER_DATA+0x2c,-1);self.w(UNIT+0xd4,-1)
        self.action('original_player_unit_assignment',0x14e240,(0,),((UC_X86_REG_EBX,UI),))
        assigned=self.selected()
        self.w(SCRATCH+0x200,2);self.w(SCRATCH+0x204,0)
        self.action('original_default_ffa_initial_loadout',0x1a28a0,(UI,SCRATCH+0x200,SCRATCH+0x204))
        self.initial_datum=self.u(UNIT+0x218)
        self.initial_pointer=self.u(HEADERS+(self.initial_datum&0xffff)*12+8)
        self.initial_equipped=dict(startup=startup,assigned=assigned,equipped=self.selected(),
            initial_weapon=self.object_state(self.initial_datum))
        self.row=row
        self.entries=self.u(0x4f10b8)+0x7dc
        self.scenario=self.u(0x4e7920)
        self.record=self.u(self.scenario+0x124)+row*0x90
        self.action('original_equipment_entries_initialize',0x15c320,registers=((UC_X86_REG_ESI,self.entries),))
        self.action('original_equipment_entries_reset',0x15c370,registers=((UC_X86_REG_EDI,self.entries),))
        self.tick=row;self.w(TIME+8,row)
        self.action('original_authored_target_allocate',0x15c110,(self.entries,row))
        self.target_datum=self.u(self.entries+row*8)
        self.target_pointer=self.u(HEADERS+(self.target_datum&0xffff)*12+8)
        self.target_initial=self.object_state(self.target_datum)
        self.place_player(near)
        self.controlled_unit_gate_before=self.u(UNIT+0x134)
        self.setup_controls()
        self.controlled_unit_gate_after=self.u(UNIT+0x134)
        self.start_inventory=self.inventory_state()
        self.start_registry=self.registry()

    def setup_world_storage(self):
        c=self.cache;bsp_row=c.block(c.scenario+0x210,0x44)[0]
        off,size,base=(c.u(bsp_row+n) for n in (0,4,8))
        self.g.mem_write(base,bytes(c.data[off:off+size]))
        root=c.u(off+4)
        self.w(0x4e7918,root);self.w(0x4e7910,self.u(root+0x18));self.w(0x4e790c,self.u(root+0x18));self.sh(0x46fb64,0)
        self.g.mem_write(self.u(0x4e58c4),b'\x01')
        self.g.mem_write(GAME+0x11b8,b'\xff'*((self.u(root+0x9c)+31)//32*4))
        for i,globalp in enumerate((0x4e58b4,0x4e58b8,0x4e58a8,0x4e58ac)):
            p=0x10b0000+i*0x3000;stride=32 if i%2==0 else 12
            self.w(globalp,p);self.w(p+0x20,64);self.w(p+0x24,stride)
            self.w(p+0x44,p+0x1000);self.w(p+0x48,p+0x2000);self.sh(p+0x40,1)
        for i,globalp in enumerate((0x4e58b0,0x4e58a4)):
            p=0x10c8000+i*0x1000;self.w(globalp,p);self.g.mem_write(p,b'\xff'*1024)
        self.w(0x517fe0,0x10cc000);self.w(0x4e58bc,0x10ce000);self.w(0x10ce000+0x2c,0x20000)
        # The player and carried starting SMG are deliberately excluded from
        # the world registry fixture. Only the original allocated target is
        # registered. Full unit/world registration is outside this execution.
        self.g.mem_write(HEADERS+2,b'\x00');self.g.mem_write(HEADERS+14,b'\x00')

    def place_player(self,near):
        start=self.cache.block(self.cache.scenario+0x100,0x34)[0]
        position=[self.fv(self.target_pointer+0x30+n*4)+(0.05 if n==0 else 0) for n in range(3)] if near else [self.cache.f(start+n*4) for n in range(3)]
        self.g.mem_write(UNIT+0x64,struct.pack('<3f',*position))
        self.g.mem_write(SCRATCH+0x700,struct.pack('<3f',self.cache.f(start+12),0,0))
        self.action('original_authored_start_facing_basis',0x11f680,registers=((UC_X86_REG_EAX,SCRATCH+0x700),(UC_X86_REG_EDX,UNIT+0x70),(UC_X86_REG_ECX,UNIT+0x7c)))
        self.action('original_equipped_live_graph_pose',0xbdf80,(UI,))
        self.action('original_equipped_unit_bounds',0xbede0,(UI,))
        self.action('original_unit_current_bsp_location',0x11d4a0,registers=((UC_X86_REG_EDX,UNIT+0x30),(UC_X86_REG_ESI,UNIT+0x28)))
        self.player_pose=dict(authored_start=0,near_target=near,supplied_root=self.vec(UNIT+0x64),
            bound_center=self.vec(UNIT+0x30),bound_radius_bits=f'{self.u(UNIT+0x3c):08x}',
            location_hex=bytes(self.g.mem_read(UNIT+0x28,8)).hex(),
            current_pose_hex=bytes(self.g.mem_read(UNIT+self.u(UNIT+0x116,2),33*52)).hex())

    def finish_source_returns(self,a):
        for ret,row in self.source_returns[:]:
            if ret==a:
                row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                row['after']=self.manager_state()
                if '_out' in row:
                    out,length=row.pop('_out');row['output_hex']=bytes(self.g.mem_read(out,length)).hex()
                self.source_returns.remove((ret,row))
        for ret,out,row in self.collision_returns[:]:
            if ret==a:
                row.update(return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',result_hex=bytes(self.g.mem_read(out,0x5c)).hex())
                self.collision_returns.remove((ret,out,row))

    def run(self,address,argv=(),regs=()):
        super().run(address,argv,regs)
        if hasattr(self,'source_returns'):self.finish_source_returns(STOP)

    def mem_write(self,g,access,address,size,value,user):
        from oracle_mp_inventory_1_1 import UC_X86_REG_EIP
        if hasattr(self,'control_gate_writes') and address<=UNIT+0x134<address+size:
            self.control_gate_writes.append(dict(phase=self.active,
                instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',
                size=size,before=f'{self.u(UNIT+0x134):08x}',value=f'{value:08x}'))
        super().mem_write(g,access,address,size,value,user)

    def fragment(self,name,start,stop,registers=()):
        from oracle_mp_inventory_1_1 import STACK, UC_X86_REG_EIP
        self.active=name;before=self.inventory_state();event_start=len(self.events)
        self.count=0;self.g.reg_write(UC_X86_REG_ESP,STACK)
        for reg,value in registers:self.g.reg_write(reg,value)
        self.g.emu_start(start,stop,count=1000000)
        assert self.g.reg_read(UC_X86_REG_EIP)==stop
        row=dict(name=name,original_fragment=[f'{start:08x}',f'{stop:08x}'],stop_is_exclusive=True,
            before=before,after=self.inventory_state(),event_range=[event_start,len(self.events)])
        self.inventory_rows.append(row);return row

    def hook(self,g,a,size,user):
        self.executed.add(a)
        if hasattr(self,'source_calls'):
            self.finish_source_returns(a);sp=g.reg_read(UC_X86_REG_ESP)
            if a==0x18fce0:
                address=self.heap_next;self.heap_next+=0x2000
                assert self.heap_next<=0x4020000
                self.w(self.u(sp+4),address)
                self.heap_requests.append(dict(requested_bytes=self.u(sp+8),supplied_address=f'{address:08x}',capacity=0x2000))
            if a==0x16b240:
                out=self.u(sp+24)
                row=dict(phase=self.active,mask=f'{self.u(sp+4):08x}',start=self.vec(self.u(sp+8)),delta=self.vec(self.u(sp+12)))
                self.collision_queries.append(row);self.collision_returns.append((self.u(sp),out,row))
            if a in ENTRIES:
                row=dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),
                    stack=[f'{self.u(sp+4+n*4):08x}' for n in range(4)],before=self.manager_state())
                if a==0xcec00:row['_out']=(self.u(sp+12),4)
                if a==0xb8a30:row['object_descriptor_hex']=bytes(g.mem_read(self.u(sp+4),0xc4)).hex()
                if a==0x111490:
                    row['unit_datum']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
                    row['mode_class_weapon']=[f'{self.u(sp+4+n*4):08x}'for n in range(3)]
                if a==0x1cfde0:
                    row['_out']=(self.u(sp+24),4)
                    row['graph_datum']=f'{self.u(g.reg_read(UC_X86_REG_EAX)+0x68):08x}'
                if a==0xbbf40:
                    row['_out']=(self.u(sp+24),min(self.u(sp+28,2),64)*4)
                    row['maximum']=self.u(sp+28,2)
                if a==0xe7f30:row['request_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),32)).hex()
                self.source_calls.append(row);self.source_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)


def listing_manifest(traces,output):
    output.mkdir(parents=True,exist_ok=True);t=traces[0]
    executed=set().union(*(x.executed for x in traces));result=[]
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES))
        ins=[t.code.at(a)for a in sorted(routine.code)]
        path=output/f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in ins)+'\n')
        result.append(dict(entry=f'{entry:08x}',listing=str(path.relative_to(ROOT)),
            sha256=hashlib.sha256(path.read_bytes()).hexdigest(),instruction_count=len(ins),
            executed_instruction_count=sum(i.address in executed for i in ins),
            direct_calls=[dict(site=f'{s:08x}',target=f'{d:08x}')for s,d,k in routine.sites if k=='call']))
    return result


def continue_graph(t,ticks=3,weapon_ticks=False):
    result=[]
    for n in range(ticks):
        t.tick=t.row+1+n;t.w(TIME+8,t.tick)
        before=t.inventory_state()
        # Bounded original routine roots; this is not a full C7660 physics tick.
        t.action('continuation_manager_blend',0x1d0ed0,registers=((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),))
        t.action('continuation_unit_control',0xc6c50,(UI,))
        if weapon_ticks:
            t.action('continuation_initial_weapon_tick',0x1021d0,(t.initial_datum,))
            t.action('continuation_target_weapon_tick',0x1021d0,(t.target_datum,))
        t.action('continuation_action_dispatch',0xe7e60,(UI,))
        t.action('continuation_animation_update',0x112e10,(UI,REQUEST))
        t.action('continuation_live_graph_pose',0xbdf80,(UI,))
        result.append(dict(tick=t.tick,before=before,after=t.inventory_state()))
    return result


def saved_case(t,name,**fields):
    assert not t.control_returns and not t.source_returns and not t.collision_returns
    assert t.u(SEED)==0x12345678
    assert t.controlled_unit_gate_before==t.controlled_unit_gate_after==3
    assert t.initial_datum==0x10002 and t.target_datum==0x20003
    assert t.initial_equipped['equipped']['clip']['index']==185
    return dict(name=name,**fields,initial=t.initial_equipped,pose=t.player_pose,
        authored_animation_records=[t.live_metadata['animations'][i]for i in (54,185,206,207)],
        authored_row=t.row,target=t.target_initial,initial_weapon_resources=t.weapon_resources,
        controlled_unit_gate=[f'{t.controlled_unit_gate_before:08x}',f'{t.controlled_unit_gate_after:08x}'],
        initial_inventory=t.start_inventory,final=t.inventory_state(),samples=t.samples,
        initial_registry=t.start_registry,final_registry=t.registry(),
        control_gate_writes=t.control_gate_writes,
        map_row_entry_hex=bytes(t.g.mem_read(t.entries+t.row*8,8)).hex(),
        calls=t.source_calls,control_calls=t.control_calls,rows=t.inventory_rows,
        collision_queries=t.collision_queries,heap_requests=t.heap_requests,
        events=[{k:v for k,v in e.items() if k!='state'}for e in t.events],
        equipped_events=t.equipped_events,live_events=t.live_events,
        supplied_services={f'{a:08x}':dict(description=n,return_eax=f'{r:08x}',cleanup=p)
                           for a,(n,r,p)in sorted(t.services.items())})


def run_suite():
    traces=[];cases=[]
    for row,button,ms,near in ((0,2,233,True),(1,2,232,True),(1,2,233,True),
        (1,2,233,False),(12,2,233,True),(12,3,232,True),(12,3,233,True)):
        t=EquippedPickupTrace(row,near);t.seeded_duration(button,ms-1)
        r=t.poll('exact_duration_boundary',{button:255},milliseconds=1)
        accepted=near and ms==233 and ((row in (0,1) and button==2) or (row==12 and button==3))
        requests=[c for c in t.control_calls if c.get('type')==20]
        assert bool(requests)==accepted
        if accepted:
            assert t.u(UNIT+0x21c)==t.target_datum
            assert t.u(t.target_pointer+0x154)==UI
            assert (t.u(t.target_pointer+0x22a,2),t.u(t.target_pointer+0x22c,2))==((120,60)if row==12 else(72,36))
            assert t.u(t.target_pointer+0x184)==0
            assert t.manager_state()['clip']['index']==(54 if button==3 else 185)
        post=continue_graph(t)
        cases.append(saved_case(t,f'row{row}_button{button}_{ms}ms_near{int(near)}',
            button=button,duration_ms=ms,near=near,accepted=accepted,continuation=post))
        traces.append(t)
    t=EquippedPickupTrace(1,True)
    # Original polling tests the previously stored analog value before copying
    # the new packet; eight 33ms polls produce a source hold timer of231.
    for n in range(8):
        r=t.poll(f'continuous_hardware_hold_{n}',{2:255},milliseconds=33)
        assert not any(c.get('type')==20 for c in t.control_calls)
    t.poll('continuous_hardware_hold_233ms',{2:255},milliseconds=2)
    assert len([c for c in t.control_calls if c.get('type')==20])==1
    t.poll('continued_hold_is_consumed',{2:255},milliseconds=33)
    assert len([c for c in t.control_calls if c.get('type')==20])==1
    t.poll('original_hardware_release',milliseconds=33)
    post=continue_graph(t)
    cases.append(saved_case(t,'continuous_primary_br_hold_release',accepted=True,continuation=post))
    traces.append(t)
    t=EquippedPickupTrace(1,True);t.seeded_duration(2,232)
    t.poll('pickup_before_switch',{2:255},milliseconds=1)
    for name,buttons in (('release_primary_1',{}),('release_primary_2',{}),
        ('short_other_press_1',{3:255}),('short_other_press_2',{3:255}),
        ('short_other_release_1',{}),('short_other_release_2',{})):
        t.poll(name,buttons,milliseconds=33)
    requested=t.inventory_state()
    assert requested['current']==[1,-1] and requested['requested']==[0,-1]
    post=continue_graph(t,10,True)
    assert [r['after']['current'][0]for r in post]==[1,1,1,1,0,0,0,0,0,0]
    assert post[0]['after']['manager']['secondary_clip']['index']==206
    assert post[4]['after']['manager']['secondary_clip']['index']==207
    cases.append(saved_case(t,'allocated_br_short_other_button_switch_to_smg',accepted=True,
        after_short_release=requested,continuation=post,continuation_weapon_ticks=True))
    traces.append(t)
    return traces,cases


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');args=p.parse_args()
    traces,cases=run_suite()
    dependencies=[Path(__file__),ROOT/'tools/oracle_spartan_equipped_animation_1_1.py',
        ROOT/'tools/oracle_spartan_live_pose_1_1.py',ROOT/'tools/oracle_spartan_default_pose_1_1.py',
        ROOT/'tools/oracle_mp_inventory_controls_1_1.py',ROOT/'tools/oracle_mp_inventory_world_1_1.py',
        ROOT/'tools/oracle_mp_inventory_spatial_1_1.py',ROOT/'tools/oracle_mp_inventory_1_1.py',ROOT/'tools/oracle_smg_firing_1_1.py']
    report=dict(xbe_sha256=PIN,cache_sha256=CACHE_PIN,
        shared_cache_sha256=traces[0].equipped_metadata['shared_cache_sha256'],
        tool_dependencies=[dict(path=str(p.relative_to(ROOT)),sha256=hashlib.sha256(p.read_bytes()).hexdigest())for p in dependencies],
        cases=cases,instructions=listing_manifest(traces,args.output.parent/'instructions'),
        scope='Bounded original FFA equipped Spartan capability, allocated SMG/BR identity, real registry/BSP visibility and held-input action. Marker attachment, Havok, full object lifecycle and engine ticks remain explicit boundaries.')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),accepted=sum(c['accepted']for c in cases),
        rows=sum(len(c['rows'])for c in cases),calls=sum(len(c['calls'])for c in cases),
        terrain_segments=sum(len(c['collision_queries'])for c in cases),
        report_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())))

if __name__=='__main__':main()
