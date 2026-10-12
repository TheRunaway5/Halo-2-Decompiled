#!/usr/bin/env python3
"""Bounded unchanged TU1.1 item allocation and world-update research.

Actual Lockout tag data is read directly. Heap storage and lower world/physics
services are supplied explicitly and do not constitute physical settling.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_mp_inventory_1_1 import (
    ROOT, PIN, CACHE_PIN, InventoryTrace, _trace, POOL, HEADERS, BITMAP,
    TIME, GAME, SEED, SECOND, THIRD, SCRATCH, UNIT, UI,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
)
from oracle_smg_firing_1_1 import TAGS
from oracle_mp_inventory_netgame_1_1 import NetgameTrace

OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/world'
ENTRIES = (0xb8a30, 0xbd170, 0xbd270, 0x16d2c0, 0x18fce0, 0x18fd70,
           0x10cbd0, 0x10cc20, 0x10cd10, 0x10cdf0, 0xf9900, 0x101bb0,
           0xfc1e0, 0xb8570, 0x1ce8c0, 0x1ceac0, 0x1cea90,
           0xbdf10, 0xbdf80, 0xbede0, 0x16f3c0, 0xc0490,
           0xbb430, 0xbb480, 0xbf130, 0xd7160, 0xbf7d0, 0xbf0c0,
           0xb8250, 0xb9780, 0xbf580, 0xbf650, 0xbfb90, 0x10d520,
           0xf9b10, 0xf9ea0, 0xfb620, 0xfbac0, 0xbd360, 0xb7e00,
           0xb94f0, 0xb97d0, 0xbfe20, 0xb81f0, 0xb8180, 0xc0400,
           0x15c110, 0x15c4b0, 0x15c820, 0xfb120, 0xfb3f0, 0xcfd80,
           0xb7f50, 0xb8040, 0xbd710, 0x1019f0, 0xc0270, 0x1ce650,
           0x1ce7e0, 0x1ce800, 0x16d4c0, 0xb96a0, 0xb9a60,
           0x11d4a0, 0x11d460, 0x14b9d0, 0x14bd00, 0x1eefe0,
           0x16b240, 0x1e2020, 0x1e20c0, 0x1e23f0, 0x246a50,
           0x10db90, 0xb9830, 0xbc910, 0xb86c0, 0xfaf20,
           0xfbbe0, 0xfaef0, 0xb9dd0, 0xba980, 0xba7d0, 0xb8320,
           0xcfed0, 0xcf8c0, 0xcfb00, 0xce510, 0xea440,
           0xfc1e0, 0xb8570, 0x11f680, 0xbeb30, 0x1057f0, 0x1050b0)


class WorldTrace(NetgameTrace):
    def __init__(self, row=12, active_world=False):
        self.collision_returns = []
        super().__init__(row)
        self.services.pop(0xb8a30)
        self.services.pop(0xbdf10)
        # Undo the inherited cloned SMG model: all definition/model payloads
        # must use their real cache addresses in this experiment.
        for datum, tag in self.cache.tags.items():
            if tag['offset'] is not None:
                self.w(TAGS + (datum & 0xffff)*16,
                       int.from_bytes(tag['group'].encode('ascii'), 'big'))
                self.w(TAGS + (datum & 0xffff)*16 + 8,
                       tag['offset'] + self.cache.magic)
        # The generic original datum allocator owns index, salt and bitmap.
        # Only backing storage is supplied to its heap leaf.
        self.w(POOL + 0x20, 64)
        self.w(POOL + 0x34, 2)
        self.w(POOL + 0x38, 2)
        self.w(POOL + 0x3c, 2)
        self.sh(POOL + 0x40, 1)
        self.heap_next = SECOND
        self.heap_requests = []
        self.original_calls = []
        self.collision_queries = []
        self.collision_returns = []
        self.services.update({
            0x18fce0: ('supplied object heap storage allocation', 1, 12),
            0x18fd70: ('supplied object heap storage growth without relocation', 1, 12),
            0xc10e0: ('supplied looping sound source allocation unavailable', 0xffffffff, 20),
        })
        self.w(0x526254, SCRATCH+0x600)
        self.action('initialize_source_physics_constants', 0x1eefe0)
        self.active_world = active_world
        if active_world:
            # Scenario's BSP allocation provides its own address mapping.
            c = self.cache
            bsp_row = c.block(c.scenario+0x210,0x44)[0]
            off, size, base = (c.u(bsp_row+n) for n in (0,4,8))
            self.g.mem_write(base, bytes(c.data[off:off+size]))
            root = c.u(off+4)
            self.w(0x4e7918,root)
            self.w(0x4e7910,self.u(root+0x18))
            self.w(0x4e790c,self.u(root+0x18))
            self.sh(0x46fb64,0)
            self.g.mem_write(self.u(0x4e58c4),b'\x01')
            # Explicit active-cluster mask input; no cluster scheduling claim.
            clusters = self.u(root+0x9c)
            self.g.mem_write(GAME+0x11b8,b'\xff'*((clusters+31)//32*4))
            for leaf in (0xb9a60,0xb81f0,0xb8180,0x1e20c0):
                self.services.pop(leaf)
            # Backing arrays for the original cluster registries. Original
            # 16D2C0/16D4C0, 14BD00 and 1CE650 own all entries and links.
            for i, globalp in enumerate((0x4e58b4,0x4e58b8,0x4e58a8,0x4e58ac)):
                p = 0x10b0000+i*0x3000
                stride = 32 if i % 2 == 0 else 12
                self.w(globalp,p)
                self.w(p+0x20,64)
                self.w(p+0x24,stride)
                self.w(p+0x44,p+0x1000)
                self.w(p+0x48,p+0x2000)
                self.sh(p+0x40,1)
            for i, globalp in enumerate((0x4e58b0,0x4e58a4)):
                p = 0x10c8000+i*0x1000
                self.w(globalp,p)
                self.g.mem_write(p,b'\xff'*1024)
            self.w(0x517fe0,0x10cc000)
            self.w(0x4e58bc,0x10ce000)
            self.w(0x10ce000+0x2c,0x20000)
            # Unrelated inherited unit/SMG object buffers are explicitly
            # outside this isolated world-object update pass.
            self.g.mem_write(HEADERS+2,b'\x00')
            self.g.mem_write(HEADERS+12+2,b'\x00')
            self.services[0x246a50] = ('supplied BSP Havok shape ray test unavailable; BSP plane/surface path runs unchanged',0,12)

    def supplied_unit_pose(self):
        """Explicit unit root-node fixture; the full unit graph is excluded."""
        p = self.cache.block(self.cache.scenario+0x100,0x34)[0]
        self.g.mem_write(UNIT+0x64,bytes(self.cache.data[p:p+12]))
        self.f(UNIT+0xa0,1)
        self.g.mem_write(SCRATCH+0x700,struct.pack('<3f',self.cache.f(p+12),0,0))
        self.action('supplied_unit_root_basis_from_authored_spawn',0x11f680,
                    registers=((UC_X86_REG_EAX,SCRATCH+0x700),
                               (UC_X86_REG_EDX,UNIT+0x70),(UC_X86_REG_ECX,UNIT+0x7c)))
        self.action('supplied_unit_root_node_matrix',0xbeb30,
                    (UNIT+0x64,0x3f800000,0,0,UNIT+0x500),
                    ((UC_X86_REG_EAX,UNIT+0x7c),(UC_X86_REG_ECX,UNIT+0x70),(UC_X86_REG_EBX,0)))
        self.g.mem_write(HEADERS+14,b'\x40')
        return dict(authored_spawn_index=0, file_offset=f'{p:08x}',
                    root_position=self.vec(UNIT+0x64), aim=self.vec(UNIT+0x168),
                    source_generated_root_node_hex=bytes(self.g.mem_read(UNIT+0x500,52)).hex(),
                    supplied_header_flags=0x40,
                    scope='root node and registered-parent input only; unit allocation, full animation and marker pose excluded')

    def hook(self, g, a, size, user):
        for ret, output, record in self.collision_returns[:]:
            if a == ret:
                record.update(return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                              result_hex=bytes(g.mem_read(output,0x5c)).hex(),
                              collision_type=self.u(output), fraction_bits=f'{self.u(output+4):08x}',
                              point=self.vec(output+8), normal=self.vec(output+0x28),
                              material=self.u(output+0x24,2))
                self.collision_returns.remove((ret,output,record))
        if a == 0x16b240:
            sp = g.reg_read(UC_X86_REG_ESP)
            output = self.u(sp+24)
            record = dict(phase=self.active, tick=self.tick, mask=f'{self.u(sp+4):08x}',
                          start=self.vec(self.u(sp+8)), delta=self.vec(self.u(sp+12)),
                          ignored=[f'{self.u(sp+16):08x}',f'{self.u(sp+20):08x}'])
            self.collision_queries.append(record)
            self.collision_returns.append((self.u(sp),output,record))
        if a == 0x18fce0:
            sp = g.reg_read(UC_X86_REG_ESP)
            pointer_out, length = self.u(sp+4), self.u(sp+8)
            address = self.heap_next
            self.heap_next += 0x2000
            assert self.heap_next <= THIRD + 0x2000
            self.g.mem_write(address, bytes(0x2000))
            self.w(pointer_out, address)
            self.heap_requests.append(dict(kind='allocate', pointer_out=f'{pointer_out:08x}',
                                           requested_bytes=length, supplied_address=f'{address:08x}',
                                           supplied_capacity=0x2000))
        elif a == 0x18fd70:
            self.heap_requests.append(dict(kind='grow', object_pointer=f'{self.u(g.reg_read(UC_X86_REG_EBX)):08x}',
                                           requested_total_bytes=g.reg_read(UC_X86_REG_EAX)))
        if a in ENTRIES:
            self.original_calls.append(dict(phase=self.active, entry=f'{a:08x}',
                                           registers=self.registers()))
        super().hook(g, a, size, user)

    def object_state(self, datum):
        h = HEADERS + (datum & 0xffff)*12
        p = self.u(h+8)
        return dict(datum=f'{datum:08x}', address=f'{p:08x}', header_hex=bytes(self.g.mem_read(h,12)).hex(),
                    definition=f'{self.u(p):08x}', flags=f'{self.u(p+4):08x}',
                    position=self.vec(p+0x64), forward=self.vec(p+0x70), up=self.vec(p+0x7c),
                    linear=self.vec(p+0x88), angular=self.vec(p+0x94), scale_bits=f'{self.u(p+0xa0):08x}',
                    bound_center=self.vec(p+0x30), bound_radius_bits=f'{self.u(p+0x3c):08x}',
                    row_index=self.u(p+0xaf,1), physical_component=f'{self.u(p+0xb4):08x}',
                    runtime_flags_c0=f'{self.u(p+0xc0,2):04x}', wake_ticks=self.u(p+0xb3,1),
                    parent=f'{self.u(p+0x14):08x}', location_hex=bytes(self.g.mem_read(p+0x28,8)).hex(),
                    contact_hex=bytes(self.g.mem_read(p+0x130,12)).hex(),
                    entity=f'{self.u(p+0xd4):08x}', item_flags=f'{self.u(p+0x12c,2):04x}',
                    ignored=f'{self.u(p+0x14c):08x}', created_tick=self.u(p+0x150), owner=f'{self.u(p+0x154):08x}',
                    spin_axis=self.vec(p+0x158), spin_sine_bits=f'{self.u(p+0x164):08x}', spin_cosine_bits=f'{self.u(p+0x168):08x}',
                    heat_bits=f'{self.u(p+0x184):08x}', reserve=self.u(p+0x22a,2), loaded=self.u(p+0x22c,2),
                    blocks={f'{n:03x}':dict(size=self.u(p+n,2), offset=self.u(p+n+2,2))
                            for n in range(0x10c,0x12c,4)}, raw_hex=bytes(self.g.mem_read(p,self.u(h+6,2))).hex())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT / 'report.json')
    args = parser.parse_args()
    traces, cases = [], []
    for active in (False,True):
      for row in (0,1,12):
        t = WorldTrace(row,active)
        t.tick = row
        t.w(TIME+8, row)
        t.action('allocate_authored_netgame_weapon', 0x15c110, (t.entries,row))
        datum = t.u(t.entries+row*8)
        initial = t.object_state(datum)
        assert initial['position']['bits'] == t.vec(t.record+0x40)['bits']
        assert initial['scale_bits'] == ('3fc00000' if row == 12 else '3fd9999a')
        assert (initial['loaded'],initial['reserve']) == ((60,120) if row==12 else (36,72))
        assert initial['heat_bits']=='00000000' and initial['owner']=='ffffffff'
        snapshots = []
        if active:
            t.action('ordinary_pass_skips_new_object',0xb7e00)
            assert t.object_state(datum) == initial
            t.action('dirty_postpass_skips_new_object',0xb7f50)
            assert t.object_state(datum) == initial
            t.action('new_object_postpass_first_tick',0xb8040)
            snapshots.append(dict(tick=row, stage='new_object_postpass', state=t.object_state(datum)))
            assert t.u(HEADERS+(datum&0xffff)*12+2,1)&8==0
            # Stop BR row1 before its later impact sound/effect allocation;
            # this slice claims the first normal tick, not complete settling.
            for tick in range(row+1,row+4):
                t.tick=tick
                t.w(TIME+8,tick)
                t.action('ordinary_item_update_pass',0xb7e00)
                t.action('dirty_object_postpass',0xb7f50)
                t.action('new_object_and_pending_cleanup_postpass',0xb8040)
                snapshots.append(dict(tick=tick,stage='ordinary_passes',state=t.object_state(datum)))
        assert t.u(SEED)==0x12345678
        cases.append(dict(name=f'row_{row}_world_{active}',row=row, active_world=active,
                          initial=initial, snapshots=snapshots))
        traces.append(t)
    for row in (0,1,12):
        t=WorldTrace(row,True)
        unit_fixture=t.supplied_unit_pose()
        t.tick=row
        t.w(TIME+8,row)
        t.action('allocate_authored_netgame_weapon',0x15c110,(t.entries,row))
        datum=t.u(t.entries+row*8)
        t.action('first_item_new_object_postpass',0xb8040)
        # Spent ammo and heat are explicit runtime inputs, without claiming a
        # native animation/firing-action translation.
        p=t.u(HEADERS+(datum&0xffff)*12+8)
        t.sh(p+0x22c,7)
        t.sh(p+0x22a,11)
        t.f(p+0x184,.5)
        before=t.object_state(datum)
        t.action('pickup_original_identity',0xce510,(UI,datum,1))
        carried=t.object_state(datum)
        assert carried['scale_bits']=='3f800000' and carried['owner']==f'{UI:08x}'
        assert carried['datum']==before['datum']
        t.tick=row+10
        t.w(TIME+8,t.tick)
        t.action('drop_original_carried_slot',0xcfd80,(0,0,1),((UC_X86_REG_EBX,UI),))
        dropped=t.object_state(datum)
        assert dropped['datum']==before['datum'] and dropped['row_index']==row
        assert dropped['scale_bits']==before['scale_bits'] and dropped['owner']=='ffffffff'
        assert (dropped['loaded'],dropped['reserve'],dropped['heat_bits'])==(7,11,'3f000000')
        assert t.u(SEED)==0x4b5c952c
        t.action('same_tick_postpass_clears_updated_flag',0xb8040)
        assert t.object_state(datum)['position']==dropped['position']
        t.tick+=1
        t.w(TIME+8,t.tick)
        # Inventory transitions can wake the supplied parent. Its full unit
        # update is excluded from this bounded item-world experiment.
        t.g.mem_write(HEADERS+14,bytes([t.u(HEADERS+14,1)&~3]))
        t.action('dropped_item_next_tick_update',0xb7e00)
        t.action('dropped_item_next_tick_dirty_postpass',0xb7f50)
        t.action('dropped_item_next_tick_cleanup',0xb8040)
        cases.append(dict(name=f'row_{row}_pickup_drop',row=row,active_world=True, unit_fixture=unit_fixture,
                          before=before,carried=carried,dropped=dropped,next_tick=t.object_state(datum)))
        traces.append(t)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    metadata=[]
    cache=traces[0].cache
    for name in ('objects\\weapons\\rifle\\battle_rifle\\battle_rifle',
                 'objects\\weapons\\rifle\\smg\\smg'):
        tag=next(t for t in cache.tags.values() if t['group']=='weap' and t['name']==name)
        p=tag['offset']
        model=cache.tags[cache.u(p+0x38)]
        render=cache.tags[cache.u(model['offset']+4)]
        nodes=cache.block(render['offset']+0x48,0x60)
        metadata.append(dict(definition=tag,model=model,render_model=render,
                             fields={f'{off:03x}':dict(file_offset=f'{p+off:08x}',bits=f'{cache.u(p+off):08x}',value=cache.f(p+off))
                                     for off in (4,8,0xc,0x10,0xc4,0xc8)},
                             node_count=len(nodes),root_node_file_offset=f'{nodes[0]:08x}',
                             root_node_translation=list(struct.unpack_from('<3f',cache.data,nodes[0]+0xc)),
                             root_node_quaternion=list(struct.unpack_from('<4f',cache.data,nodes[0]+0x18)),
                             root_node_raw_hex=bytes(cache.data[nodes[0]:nodes[0]+0x60]).hex()))
    instruction_dir=args.output.parent/'instructions'
    instruction_dir.mkdir(parents=True,exist_ok=True)
    instruction_rows=[]
    executed=set().union(*(t.executed for t in traces))
    for entry in dict.fromkeys(ENTRIES):
        routine=_trace(traces[0].code,entry,traces[0].starts|set(ENTRIES))
        code=[traces[0].code.at(a) for a in sorted(routine.code)]
        path=instruction_dir/f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in code)+'\n')
        instruction_rows.append(dict(entry=f'{entry:08x}',listing=str(path.relative_to(ROOT)),
                                     sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                     original_instruction_bytes_sha256=hashlib.sha256(b''.join(i.bytes for i in code)).hexdigest(),
                                     instruction_count=len(code),executed_instruction_count=sum(i.address in executed for i in code),
                                     direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}') for a,b,k in routine.sites if k=='call']))
    for case,t in zip(cases,traces):
        case.update(rows=t.inventory_rows,events=t.events,original_calls=t.original_calls,
                    heap_requests=t.heap_requests,collision_queries=t.collision_queries,
                    supplied_services={f'{a:08x}':dict(name=n,return_value=r,stack_cleanup=p)
                                       for a,(n,r,p) in t.services.items()},
                    used_services=sorted(set(e['service'] for e in t.events if e.get('service'))))
    report = dict(xbe_sha256=PIN, cache_sha256=CACHE_PIN,
                  oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  cases=cases,instructions=instruction_rows,
                  authored_weapon_metadata=metadata,
                  gravity_initializer=dict(original_entry='001eefe0',constant_address='0044c7a8',
                                           bits=f'{traces[0].u(0x44c7a8):08x}',value=traces[0].fv(0x44c7a8)),
                  scope='Unchanged original object allocation, initialization, registration, type dispatch and bounded item updates. Heap storage, cluster activation mask, animation/marker/audio leaves and Havok shape ray unavailable remain explicit fixture boundaries. C7660 general physics stage is not executed. No complete live settling parity claim.')
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(output=str(args.output), cases=len(cases))))


if __name__ == '__main__':
    main()
