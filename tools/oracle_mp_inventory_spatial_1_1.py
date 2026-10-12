#!/usr/bin/env python3
"""Original TU1.1 pickup spatial enumeration and Lockout terrain visibility.

The original allocator and cluster registry own every weapon datum and link.
Only heap backing storage, unit root poses, active clusters and the named lower
services are supplied. Neither BBF40 nor any terrain collision result is supplied.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_inventory_world_1_1 import WorldTrace
from oracle_mp_inventory_1_1 import (
    InventoryTrace, ROOT, PIN, CACHE_PIN, HEADERS, POOL, UNIT, UI,
    SCRATCH, SEED, TIME, STACK, STOP, _trace,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
)
from unicorn.x86_const import UC_X86_REG_XMM0

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/spatial'
BEST = SCRATCH + 0x300
QUERY_OUT = SCRATCH + 0x800
ENTRIES = (0x1a28a0, 0x15c110, 0xb8a30, 0x16d2c0, 0x16d4c0,
           0x18fce0, 0x18fd70, 0xbdf10, 0xbdf80, 0xbede0, 0xb94f0,
           0xb97d0, 0xb96a0, 0x1ce650, 0x1ce7e0, 0x1ce800, 0x1ce830,
           0xbbf40, 0xb9830, 0xb9900, 0x14bd00, 0x14bac0,
           0x121f70, 0x23c260, 0x11d4a0, 0x14b9d0, 0x11f680, 0x15b460,
           0xbeb30, 0x1527c0, 0x152e90, 0x152a30, 0xcec00,
           0xc57a0, 0xe0c20, 0xfbfa0, 0xbacc0, 0x16bac0,
           0x16b240, 0x16a560, 0x16a5c0, 0x1e2020, 0x1e20c0,
           0x1e23f0, 0x1e25c0, 0x29c7d0, 0x246a50,
           0xb9c20, 0x14ba50, 0x30bd0, 0xce510, 0xc52c0)


def signed(value, bits):
    return value - (1 << bits) if value & (1 << (bits-1)) else value


class SpatialTrace(WorldTrace):
    def __init__(self):
        self.spatial_calls = []
        self.return_watch = []
        self.heap_backing_ready = False
        super().__init__(0, True)
        # Larger backing storage is a heap leaf input. Original object and
        # registry allocators retain their unmodified datum/salt/bitmap logic.
        self.g.mem_map(0x4000000, 0x20000)
        self.spatial_heap_next = 0x4000000
        self.heap_backing_ready = True
        # Supply storage, then execute the original full-reset fill. The
        # surrounding full-reset engine/lifecycle branch remains excluded.
        self.g.mem_map(0x4030000,0x5000)
        self.w(0x4f4850,0x4030000)
        self.fragment('original_full_reset_intact_breakable_bits',0x15b670,0x15b680,
                      ((UC_X86_REG_EBX,0xffffffff),))
        assert bytes(self.g.mem_read(0x4030000,0x4201))==b'\xff'*0x4201
        self.w(SCRATCH, 0x463b48)  # Original mode-2 Slayer vtable.
        self.w(UNIT + 0x13c, 0)
        self.w(STACK, STOP)
        self.w(STACK+4, UI)
        self.fragment('original_unit_inventory_constructor_prefix', 0xc52c0, 0xc5359)
        self.starts_authored = self.cache.block(self.cache.scenario+0x100, 0x34)
        self.unit_pose(0)
        self.w(SCRATCH+0x200, 2)
        self.w(SCRATCH+0x204, 0)
        self.action('original_default_ffa_loadout', 0x1a28a0,
                    (UI, SCRATCH+0x200, SCRATCH+0x204))
        self.initial_weapon = self.u(UNIT+0x218)
        assert self.initial_weapon == 0x10002
        self.world_datums = {}
        for row in (0, 1, 12):
            self.tick = row
            self.w(TIME+8, row)
            self.action(f'original_authored_row_{row}_allocation', 0x15c110,
                        (self.entries, row))
            self.world_datums[row] = self.u(self.entries+row*8)
        assert self.world_datums == {0: 0x20003, 1: 0x30004, 12: 0x40005}
        assert self.u(SEED) == 0x12345678

    def inventory_state(self):
        result = InventoryTrace.inventory_state(self)
        result['weapons'] = []
        for index in range(2, self.u(POOL+0x38)):
            h = HEADERS+index*12
            if self.u(h+3, 1) != 2:
                continue
            p = self.u(h+8)
            datum = index | self.u(h, 2) << 16
            result['weapons'].append(dict(
                datum=f'{datum:08x}', definition=f'{self.u(p):08x}',
                owner=f'{self.u(p+0x154):08x}', item_flags=f'{self.u(p+0x12c,2):04x}',
                loaded=self.u(p+0x22c,2), reserve=self.u(p+0x22a,2)))
        return result

    def complete_returns(self, a):
        g = self.g
        for ret, row, kind, values in self.return_watch[:]:
            if a != ret:
                continue
            row['return_eax'] = f'{g.reg_read(UC_X86_REG_EAX):08x}'
            if kind == 'enumeration':
                out, maximum = values
                count = signed(g.reg_read(UC_X86_REG_EAX)&0xffff,16)
                assert 0 <= count <= maximum
                row['returned'] = [f'{self.u(out+n*4):08x}' for n in range(count)]
            elif kind == 'clusters':
                out, maximum = values
                count = signed(g.reg_read(UC_X86_REG_EAX)&0xffff,16)
                row['returned_clusters'] = [self.u(out+n*2,2) for n in range(min(count,maximum))]
            elif kind == 'capsule':
                bottom, extent, radius = values
                row.update(bottom=self.vec(bottom), extent_bits=f'{self.u(extent):08x}',
                           radius_bits=f'{self.u(radius):08x}')
            elif kind == 'point':
                row['point'] = self.vec(values[0])
            elif kind == 'visibility':
                row['visible'] = bool(g.reg_read(UC_X86_REG_EAX)&255)
                row['collision_query_range'][1] = len(self.collision_queries)
            elif kind == 'admission':
                row['accepted'] = bool(g.reg_read(UC_X86_REG_EAX)&255)
                row['modes'] = list(g.mem_read(values[0],4))
            elif kind == 'lower':
                descriptor,output=values
                row['descriptor_after_hex']=bytes(g.mem_read(descriptor,0x28)).hex()
                row['result_hex']=bytes(g.mem_read(output,0x420)).hex()
                row['visited_count']=self.u(output+0x18)
                row['visited_leaves']=[self.u(output+0x1c+n*4)
                                       for n in range(min(self.u(output+0x18),256))]
            self.return_watch.remove((ret,row,kind,values))

    def action(self, name, address, args=(), registers=()):
        row = super().action(name,address,args,registers)
        # The harness stops before executing its return sentinel. Nested
        # original returns are observed by the code hook; direct roots finish
        # here using exactly the same post-return register/output capture.
        self.complete_returns(STOP)
        return row

    def hook(self, g, a, size, user):
        self.complete_returns(a)
        sp = g.reg_read(UC_X86_REG_ESP)
        if a == 0x1e2020:
            self.collision_queries[-1]['lower_wrapper']=dict(
                output_pointer=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                flags=f'{g.reg_read(UC_X86_REG_ECX):08x}',
                collision_bsp=f'{g.reg_read(UC_X86_REG_EDX):08x}',
                breakable_count=self.u(sp+4,2),breakable_pointer=f'{self.u(sp+8):08x}',
                initial_best_fraction_bits=f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}')
        if a == 0x1e20c0 and self.u(sp)==0x1e20ad:
            descriptor=self.u(sp+4)
            output=self.u(descriptor+0x18)
            lower=dict(descriptor_hex=bytes(g.mem_read(descriptor,0x28)).hex(),
                root_node=f'{self.u(sp+8):08x}',range_bits=[f'{self.u(sp+12):08x}',f'{self.u(sp+16):08x}'],
                flags=f'{self.u(descriptor):08x}',collision_bsp=f'{self.u(descriptor+4):08x}',
                breakable_count=self.u(descriptor+8,2),breakable_pointer=f'{self.u(descriptor+0xc):08x}',
                origin=self.vec(self.u(descriptor+0x10)),delta=self.vec(self.u(descriptor+0x14)),
                initial_best_fraction_bits=f'{self.u(output):08x}',
                prior_leaf=f'{self.u(descriptor+0x1c):08x}',prior_class=self.u(descriptor+0x20,1),
                prior_plane=f'{self.u(descriptor+0x24):08x}')
            self.collision_queries[-1]['lower']=lower
            self.return_watch.append((self.u(sp),lower,'lower',(descriptor,output)))
        row, kind, values = None, '', ()
        if a == 0xbbf40:
            row = dict(registry_mask=f'{self.u(sp+4):08x}', type_mask=f'{self.u(sp+8):08x}',
                       location_hex=bytes(g.mem_read(self.u(sp+12),8)).hex(),
                       center=self.vec(self.u(sp+16)), radius_bits=f'{self.u(sp+20):08x}',
                       maximum=self.u(sp+28,2))
            kind, values = 'enumeration', (self.u(sp+24), self.u(sp+28,2))
        elif a == 0x14bd00:
            row = dict(start_cluster=self.u(sp+4,2), center=self.vec(self.u(sp+8)),
                       radius_bits=f'{self.u(sp+12):08x}', maximum=self.u(sp+16,2))
            kind, values = 'clusters', (self.u(sp+20),self.u(sp+16,2))
        elif a in (0xb9830, 0xb9900):
            cluster = self.u(sp+4,2)
            row = dict(cluster=cluster, registry='collidable' if a==0xb9830 else 'noncollidable',
                       stored_order=self.cluster_chain(cluster,a==0xb9830))
        elif a == 0xc57a0:
            row = dict(unit=f'{g.reg_read(UC_X86_REG_EDI):08x}',
                       target=f'{g.reg_read(UC_X86_REG_ESI):08x}',
                       collision_query_range=[len(self.collision_queries),None])
            kind = 'visibility'
        elif a == 0xe0c20:
            kind = 'capsule'
            values = (g.reg_read(UC_X86_REG_ECX),self.u(sp+4),self.u(sp+8))
            row = dict(unit=f'{g.reg_read(UC_X86_REG_EAX):08x}')
        elif a == 0xfbfa0:
            row = dict(target=f'{g.reg_read(UC_X86_REG_ECX):08x}')
            kind, values = 'point', (g.reg_read(UC_X86_REG_EAX),)
        elif a == 0xcec00:
            row = dict(unit=f'{self.u(sp+4):08x}', target=f'{self.u(sp+8):08x}')
            kind, values = 'admission', (self.u(sp+12),)
        if row is not None:
            row.update(phase=self.active, entry=f'{a:08x}')
            self.spatial_calls.append(row)
            if kind:
                self.return_watch.append((self.u(sp),row,kind,values))
        if a == 0x18fce0 and self.heap_backing_ready:
            address = self.spatial_heap_next
            self.spatial_heap_next += 0x2000
            assert self.spatial_heap_next <= 0x4020000
            self.w(self.u(sp+4),address)
            self.heap_requests.append(dict(kind='allocate',pointer_out=f'{self.u(sp+4):08x}',
                                           requested_bytes=self.u(sp+8),supplied_address=f'{address:08x}',
                                           supplied_capacity=0x2000))
            InventoryTrace.hook(self,g,a,size,user)
            return
        super().hook(g,a,size,user)

    def cluster_chain(self, cluster, collidable):
        heads = self.u(0x4e58b0 if collidable else 0x4e58a4)
        pool = self.u(0x4e58b4 if collidable else 0x4e58a8)
        link = self.u(heads+cluster*4)
        result = []
        while link != 0xffffffff:
            assert len(result) < 64
            p = self.u(pool+0x44)+(link&0xffff)*self.u(pool+0x24)
            result.append(dict(link=f'{link:08x}',object=f'{self.u(p+4):08x}',
                               next=f'{self.u(p+8):08x}',type=self.u(p+0xc,1),
                               center=self.vec(p+0x10),radius_bits=f'{self.u(p+0x1c):08x}',
                               raw_hex=bytes(self.g.mem_read(p,32)).hex()))
            link = self.u(p+8)
        return result

    def registry(self):
        root = self.u(0x4e7918)
        return [dict(cluster=n,collidable=self.cluster_chain(n,True),
                     noncollidable=self.cluster_chain(n,False))
                for n in range(self.u(root+0x9c))]

    def unit_pose(self, authored_index=0, position=None):
        p = self.starts_authored[authored_index]
        v = bytes(self.cache.data[p:p+12]) if position is None else struct.pack('<3f',*position)
        self.g.mem_write(UNIT+0x64,v)
        self.f(UNIT+0xa0,1)
        self.sh(UNIT+0x116,0x500)
        self.g.mem_write(SCRATCH+0x700,struct.pack('<3f',self.cache.f(p+12),0,0))
        self.action('root_basis_from_authored_start_facing',0x11f680,
                    registers=((UC_X86_REG_EAX,SCRATCH+0x700),(UC_X86_REG_EDX,UNIT+0x70),
                               (UC_X86_REG_ECX,UNIT+0x7c)))
        self.action('original_root_node_transform',0xbeb30,
                    (UNIT+0x64,0x3f800000,0,0,UNIT+0x500),
                    ((UC_X86_REG_EAX,UNIT+0x7c),(UC_X86_REG_ECX,UNIT+0x70),(UC_X86_REG_EBX,0)))
        self.action('original_unit_bounding_sphere',0xbede0,(UI,))
        self.action('original_current_bsp_point_location',0x11d4a0,
                    registers=((UC_X86_REG_EDX,UNIT+0x30),(UC_X86_REG_ESI,UNIT+0x28)))
        self.g.mem_write(HEADERS+14,b'\x40')
        return dict(authored_start=authored_index,file_offset=f'{p:08x}',
                    authored_record_hex=bytes(self.cache.data[p:p+0x34]).hex(),
                    facing_bits=f'{self.cache.u(p+12):08x}',
                    supplied_position_override=position,root=self.vec(UNIT+0x64),
                    center=self.vec(UNIT+0x30),radius_bits=f'{self.u(UNIT+0x3c):08x}',
                    supplied_crouch_bits=f'{self.u(UNIT+0x2c4):08x}',
                    location_hex=bytes(self.g.mem_read(UNIT+0x28,8)).hex(),
                    root_node_hex=bytes(self.g.mem_read(UNIT+0x500,52)).hex())

    def choose(self,name):
        self.action(name,0x1527c0,(0,BEST))
        return dict(priority=signed(self.u(BEST,2),16),modes_flags=self.u(BEST+2,1),
                    object=f'{self.u(BEST+4):08x}',bytes=bytes(self.g.mem_read(BEST,12)).hex())

    def enumerate(self,name,radius=10.0,registry_mask=0,type_mask=4,maximum=64):
        self.action(name,0xbbf40,(registry_mask,type_mask,UNIT+0x28,UNIT+0x30,
                                struct.unpack('<I',struct.pack('<f',radius))[0],QUERY_OUT,maximum))
        return next(row for row in reversed(self.spatial_calls) if row['entry']=='000bbf40')

    def visibility(self,datum,name):
        self.action(name,0xc57a0,registers=((UC_X86_REG_EDI,UI),(UC_X86_REG_ESI,datum)))
        return next(row for row in reversed(self.spatial_calls) if row['entry']=='000c57a0')

    def relocate(self,datum,source):
        p = self.u(HEADERS+(datum&0xffff)*12+8)
        q = self.u(HEADERS+(source&0xffff)*12+8)
        for off in (0x64,0x70,0x7c):
            self.g.mem_write(p+off,bytes(self.g.mem_read(q+off,12)))
        self.action('original_pose_bounds_and_registry_after_explicit_coincident_transform',0xbdf10,(datum,))

    def relocate_position(self,datum,position):
        p=self.u(HEADERS+(datum&0xffff)*12+8)
        self.g.mem_write(p+0x64,struct.pack('<3f',*position))
        self.action('original_pose_bounds_and_registry_after_explicit_root_position',0xbdf10,(datum,))


def cache_record(t,address,size):
    row=t.cache.block(t.cache.scenario+0x210,0x44)[0]
    off,length,base=(t.cache.u(row+n) for n in (0,4,8))
    file_offset=off+address-base if base<=address<base+length else address-t.cache.magic
    assert bytes(t.g.mem_read(address,size)) == bytes(t.cache.data[file_offset:file_offset+size])
    return dict(address=f'{address:08x}',file_offset=f'{file_offset:08x}',
                raw_hex=bytes(t.g.mem_read(address,size)).hex())


def listings(t,output):
    output.mkdir(parents=True,exist_ok=True)
    rows=[]
    for entry in ENTRIES:
        routine=_trace(t.code,entry,t.starts|set(ENTRIES))
        code=[t.code.at(a) for a in sorted(routine.code)]
        path=output/f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in code)+'\n')
        rows.append(dict(entry=f'{entry:08x}',listing=str(path.relative_to(ROOT)),
                         sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                         original_instruction_bytes_sha256=hashlib.sha256(b''.join(i.bytes for i in code)).hexdigest(),
                         instruction_count=len(code),executed_instruction_count=sum(i.address in t.executed for i in code),
                         direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}') for a,b,k in routine.sites if k=='call']))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=OUT/'report.json')
    args=parser.parse_args()
    t=SpatialTrace()
    allocated={str(row):t.object_state(datum) for row,datum in t.world_datums.items()}
    initial_registry=t.registry()
    authored=[]
    for n in range(len(t.starts_authored)):
        pose=t.unit_pose(n)
        chosen=t.choose(f'authored_start_{n}_ordinary_pickup_target')
        assert chosen['object']=='ffffffff'
        visible={str(row):t.visibility(datum,f'authored_start_{n}_terrain_visibility_row_{row}')
                 for row,datum in t.world_datums.items()}
        authored.append(dict(pose=pose,candidate=chosen,terrain_visibility=visible))
    near=[]
    for row,datum in t.world_datums.items():
        center=allocated[str(row)]['bound_center']['value']
        pose=t.unit_pose(0,[center[0]+0.05,center[1],center[2]])
        chosen=t.choose(f'explicit_near_row_{row}_ordinary_pickup_target')
        visible=t.visibility(datum,f'explicit_near_row_{row}_terrain_visibility')
        near.append(dict(row=row,pose=pose,candidate=chosen,terrain_visibility=visible))
    t.unit_pose(0)
    queries=[t.enumerate('broad_query_all_source_registered_weapons',100),
             t.enumerate('broad_query_maximum_one',100,maximum=1),
             t.enumerate('broad_query_collidable_only',100,registry_mask=1),
             t.enumerate('broad_query_noncollidable_only',100,registry_mask=2),
             t.enumerate('broad_query_excludes_weapon_type',100,type_mask=1)]
    assert len(queries[0]['returned'])==3 and len(set(queries[0]['returned']))==3
    assert queries[1]['returned']==queries[0]['returned'][:1]
    assert queries[4]['returned']==[]
    # This override isolates registration order and strict equal-distance ties.
    # It does not stand in for movement, gravity or settling.
    t.relocate(t.world_datums[1],t.world_datums[0])
    center=t.object_state(t.world_datums[0])['bound_center']['value']
    pose=t.unit_pose(0,[center[0]+.05,center[1],center[2]])
    order=[]
    for row in (1,0,1):
        t.action(f'original_unregister_row_{row}_before_reregistration',0xb96a0,(t.world_datums[row],))
        t.action(f'original_reregister_row_{row}_without_pose_change',0xb94f0,(t.world_datums[row],0))
        query=t.enumerate(f'coincident_query_after_row_{row}_registration',1)
        chosen=t.choose(f'coincident_target_after_row_{row}_registration')
        assert chosen['object']==f'{t.world_datums[row]:08x}'
        assert query['returned'][0]==chosen['object']
        order.append(dict(reregistered_row=row,query=query,candidate=chosen,registry=t.registry()))
    edge_queries=[t.enumerate('zero_radius_uses_only_source_current_cluster',0),
                  t.enumerate('zero_type_mask_accepts_all_registered_types',1,type_mask=0),
                  t.enumerate('zero_maximum_stops_before_first_output',1,maximum=0)]
    assert len(edge_queries[0]['returned'])==2 and edge_queries[2]['returned']==[]
    outside_pose=t.unit_pose(0,[100,100,100])
    assert t.u(UNIT+0x2c,2)==0xffff
    edge_queries.append(t.enumerate('original_outside_bsp_location_has_no_query_clusters',100))
    assert edge_queries[-1]['returned']==[]
    # Actual authored portal center is an explicit relocation input; original
    # membership allocation must create both adjacent-cluster records.
    bsp=t.u(0x4e7918)
    cluster9=t.u(bsp+0xa0)+9*0xb0
    portal_index=t.u(t.u(cluster9+0x90),2)
    portal=t.u(bsp+0x60)+portal_index*0x24
    portal_position=t.vec(portal+8)
    t.relocate_position(t.world_datums[0],portal_position['value'])
    portal_registry=t.registry()
    memberships=[r['cluster'] for r in portal_registry
                 if any(x['object']==f'{t.world_datums[0]:08x}' for x in r['collidable'])]
    assert len(memberships)>1
    t.unit_pose(0)
    deduplicated=t.enumerate('multi_cluster_original_registry_deduplicates_datum',100)
    assert len(deduplicated['returned'])==3 and len(set(deduplicated['returned']))==3
    assert t.u(SEED)==0x12345678
    assert not t.return_watch and not t.collision_returns
    used=sorted(set(e['service'] for e in t.events if e.get('service')))
    assert not set(used)&{'000bbf40','000c57a0','0016b240','001e20c0','00246a50'}
    collision=t.u(0x4e790c)
    bsp=t.u(0x4e7918)
    for query in t.collision_queries:
        lower=query['lower']
        assert lower['flags']==('00000029' if query['mask']=='14800001' else '00000009')
        assert lower['range_bits']==['00000000','3f800000'] and lower['root_node']=='00000000'
        assert lower['initial_best_fraction_bits']=='7f7fffff'
        assert (lower['prior_leaf'],lower['prior_class'],lower['prior_plane'])==('ffffffff',0,'ffffffff')
        assert lower['breakable_count']==256 and lower['breakable_pointer']=='04030001'
        if query['collision_type'] != 1:
            continue
        raw=bytes.fromhex(query['result_hex'])
        leaf,surface,plane=struct.unpack_from('<III',raw,0x4c)
        material=struct.unpack_from('<H',raw,0x5a)[0]
        query['terrain_cache_contact']=dict(leaf=leaf,surface=surface,plane=f'{plane:08x}',
            surface_record=cache_record(t,t.u(collision+0x2c)+surface*8,8),
            plane_record=cache_record(t,t.u(collision+0xc)+(plane&0x7fff)*16,16),
            collision_material=cache_record(t,t.u(bsp+0x10)+material*0x14,0x14))
        assert t.u(t.u(bsp+0x10)+material*0x14+8,2)==query['material']
    fields={}
    p=t.authored['spartan']['offset']
    for off in (4,8,0xc,0x10,0x264,0x268,0x26c,0x270):
        fields[f'{off:03x}']=dict(file_offset=f'{p+off:08x}',bits=f'{t.cache.u(p+off):08x}',value=t.cache.f(p+off))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    bsp_row=t.cache.block(t.cache.scenario+0x210,0x44)[0]
    bsp_chunk={name:f'{t.cache.u(bsp_row+off):08x}' for name,off in (('file_offset',0),('length',4),('base',8))}
    collision_blocks={}
    for name,off,stride in (('nodes3d',0,8),('planes',8,16),('leaves',0x10,4),
                            ('references2d',0x18,4),('nodes2d',0x20,16),('surfaces',0x28,8),
                            ('edges',0x30,12),('vertices',0x38,16)):
        collision_blocks[name]=dict(descriptor_offset=f'{off:02x}',count=t.u(collision+off),stride=stride,
                                   data=cache_record(t,t.u(collision+off+4),t.u(collision+off)*stride))
    report=dict(xbe_sha256=PIN,cache_sha256=CACHE_PIN,
        oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        source_tool_dependencies={name:hashlib.sha256((ROOT/'tools'/name).read_bytes()).hexdigest()
            for name in ('oracle_mp_inventory_world_1_1.py','oracle_mp_inventory_netgame_1_1.py',
                         'oracle_mp_inventory_1_1.py','oracle_smg_firing_1_1.py')},
        allocated_world_objects=allocated,initial_carried_weapon=f'{t.initial_weapon:08x}',
        initial_registry=initial_registry,authored_starts=authored,explicit_near_positions=near,
        bounded_registry_queries=queries,coincident_transform_pose=pose,registration_order_cases=order,
        edge_queries=edge_queries,outside_pose=outside_pose,
        multi_cluster_case=dict(portal_index=portal_index,portal_record=cache_record(t,portal,0x24),
            explicit_root_position=portal_position,memberships=memberships,
            registry=portal_registry,query=deduplicated),
        unit_metadata=dict(tag=t.authored['spartan'],fields=fields),
        terrain_metadata=dict(scenario_bsp_row_file_offset=f'{bsp_row:08x}',
            chunk=bsp_chunk,root=cache_record(t,bsp,0x18),
            collision_bsp=cache_record(t,collision,0x40),blocks=collision_blocks,
            collision_materials=cache_record(t,t.u(bsp+0x10),t.u(bsp+0xc)*0x14),
            supplied_intact_breakable_storage=dict(global_pointer='004f4850',base='04030000',
                bytes=0x4201,initializer_fragment=['0015b670','0015b680'],surrounding_full_reset_excluded=True)),
        original_constants={f'{a:08x}':dict(bits=f'{t.u(a):08x}',value=t.fv(a))
            for a in (0x4651a0,0x465044,0x4650d8,0x452000,0x446ee4,0x4653ac,0x46508c,
                      0x452044,0x4652ec,0x4652e8)},
        rows=t.inventory_rows,spatial_calls=t.spatial_calls,collision_queries=t.collision_queries,
        heap_requests=t.heap_requests,
        events=[{k:v for k,v in e.items() if k!='state'} for e in t.events],
        used_services=used,supplied_services={f'{a:08x}':dict(name=n,return_value=r,stack_cleanup=s)
            for a,(n,r,s) in t.services.items()},instructions=listings(t,args.output.parent/'instructions'),
        scope='Original default SMG loadout, actual authored BR/SMG allocation and cluster links, '
              'all 66 authored start root poses, full weapon query/admission and BSP terrain visibility. '
              'Direct visibility sweep is separate from pickup distance. Root pose and active clusters '
              'are supplied inputs; full animated unit, nonweapon registrants and dynamic world shapes '
              'are excluded. No heap allocator internals, complete physics/settling or live Xbox parity claim.')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(output=str(args.output),authored_starts=len(authored),
        visibility_calls=sum(r['entry']=='000c57a0' for r in t.spatial_calls),
        collision_queries=len(t.collision_queries),routine_rows=len(t.inventory_rows),
        near_candidates=[r['candidate']['object'] for r in near],used_services=used)))


if __name__=='__main__':
    main()
