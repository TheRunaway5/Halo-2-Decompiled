#!/usr/bin/env python3
"""Bounded original player placement and real Lockout BSP object construction.

Earlier spawn reports are frozen. This experiment copies each actual authored
start into a one-row scenario view, executes original normal spawn setup with
named creation/loadout leaves, then executes the actual TU1.1 generic biped
constructor with real tags and BSP. Complete creation uses explicitly suspended
physics. Live physics runs only to the serialized shape's virtual dispatch.
No executable instruction is patched and no live Havok admission is claimed.
"""
import argparse
import gc
import hashlib
import json
from pathlib import Path
import struct

import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

from oracle_mp_spawn import (ROOT, VERSIONS, CACHE_PIN, Cache, STARTS, SCENARIO,
    PLAYERS, SEED, MULG, OBJECTS)
from oracle_mp_spawn_lifecycle import TABLE, DEFINITION, CAPSULES, POINT
from oracle_mp_spawn_ownership import OwnershipOracle, NONE
from oracle_mp_inventory_world_1_1 import WorldTrace, SCRATCH, HEADERS
from oracle_smg_firing_1_1 import STACK, STOP
from functions import _trace

OUT = ROOT / 'analysis/multiplayer/integration/spawn-source'
POOL_PHYSICS = 0x10d0000
PLACEMENT = SCRATCH + 0x1000
ENTRIES = (0x1509c0, 0x150640, 0x11dda0, 0xb8820, 0xb8a30,
    0xc52c0, 0xddb40, 0xdfaf0, 0xe6f60, 0xbdf10, 0xbdf80, 0xbede0,
    0xc0400, 0x11d4a0, 0x11d460, 0x14b9d0, 0x1e1800, 0x1e1890,
    0xb94f0, 0xb97d0, 0x1ce650, 0x14bd00, 0xb8180, 0x1c7050,
    0x1c6f50, 0x1c72e0, 0x1d2aa0, 0x1d3340, 0x1d2b10, 0x1da570,
    0x1da6f0, 0xbacc0, 0xe23a0, 0x1daac0, 0x1c8b40, 0x311c10,
    0x31c160, 0x3120b0, 0x314a00, 0x314b30, 0xdac40, 0xcb410,
    0xcbdc0, 0x14e240)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def vector(raw):
    return dict(bits=[f'{n:08x}' for n in struct.unpack('<3I', raw)],
                value=list(struct.unpack('<3f', raw)))


def spawn_fixture(o, c):
    o.fixture(c)
    tag = next(t for t in c.tags.values()
               if t['group'] == 'bipd' and t['name'].endswith('masterchief_mp'))
    o.w(o.a['tags'], TABLE)
    o.w(TABLE + (int(tag['datum'], 16) & 0xffff)*16 + 8, DEFINITION)
    o.g.mem_write(DEFINITION, bytes(c.data[tag['offset']:tag['offset']+0x340]))
    o.w(TABLE+8, MULG)
    rows = c.block(tag['offset']+0x28c, 0x50)
    o.g.mem_write(CAPSULES, b''.join(bytes(c.data[q:q+0x50]) for q in rows))
    o.w(DEFINITION+0x290, CAPSULES)
    o.spawn_tag = int(tag['datum'], 16)
    for i in range(3):
        o.w(OBJECTS+i*0x800, o.spawn_tag)
    o.shape_hit = False
    o.services = {
        o.owner['representation']: ('representation', 1, None),
        o.owner['create']: ('create', 1, 0x10001),
        o.owner['custom']: ('customization', 0, 0),
        o.owner['loadout']: ('loadout', 0, None),
    }
    return tag


def setup_cases(c):
    starts = c.block(c.scenario+0x100, 0x34)
    rows = []
    for version in VERSIONS:
        o = OwnershipOracle(version)
        for index, p in enumerate(starts):
            tag = spawn_fixture(o, c)
            raw = bytes(c.data[p:p+0x34])
            # Actual authored bytes; restrict this case to one row so the
            # original selector itself supplies the selected index.
            o.g.mem_write(STARTS, raw)
            o.w(SCENARIO+0x100, 1)
            before = o.state()
            result = o.call(o.owner['spawn'], (0x10000,)) & 255
            creation = next(e for e in o.events if e.get('name') == 'create')
            placement = bytes.fromhex(creation['placement_bytes'])
            assert result == 1 and placement[0x1c:0x28] == raw[:12]
            assert placement[0x34:0x40] == struct.pack('<3f', 0, 0, 1)
            assert placement[0x40:0x58] == bytes(24)
            assert o.state()['spawn_count'] == 1
            rows.append(dict(version=version, authored_index=index,
                file_offset=f'{p:08x}', raw_start_hex=raw.hex(),
                authored_bsp=struct.unpack_from('<h', raw, 0x12)[0],
                definition=tag['datum'], result=result, before=before,
                after=o.state(), placement_hex=placement.hex(),
                position=vector(placement[0x1c:0x28]),
                forward=vector(placement[0x28:0x34]),
                up=vector(placement[0x34:0x40]),
                shape_requests=o.shape_requests, events=o.events,
                global_rng_after=[f'{o.u(SEED+n):08x}' for n in (0, 4)]))
        del o
        gc.collect()
    # Paired complete source failure paths; failures supply neither poses nor
    # a successful created world object.
    failures = []
    for version in VERSIONS:
        o = OwnershipOracle(version)
        for name in ('no_starts', 'shape_contact', 'create_failure', 'no_definition'):
            spawn_fixture(o, c)
            if name == 'no_starts':
                o.w(SCENARIO+0x100, 0)
            elif name == 'shape_contact':
                o.shape_hit = True
            elif name == 'create_failure':
                o.services[o.owner['create']] = ('create', 1, NONE)
            else:
                o.spawn_tag = NONE
            before = o.state()
            result = o.call(o.owner['spawn'], (0x10000,)) & 255
            after = o.state()
            assert result == 0 and after['current'] == before['current'] == 'ffffffff'
            assert after['previous'] == before['previous']
            assert int(after['player_flags'], 16) & 8
            assert after['live_countdown'] == before['live_countdown']
            assert after['history'] == before['history']
            assert after['spawn_count'] == int(name in ('create_failure', 'no_definition'))
            failures.append(dict(version=version, case=name, result=result,
                before=before, after=after, events=o.events,
                shape_requests=o.shape_requests,
                global_rng_after=[f'{o.u(SEED+n):08x}' for n in (0, 4)]))
        # Explicit metadata-only fixture tests which field owns placement.
        spawn_fixture(o, c)
        p = starts[0]
        raw = bytearray(c.data[p:p+0x34])
        struct.pack_into('<h', raw, 0x12, 7)
        o.g.mem_write(STARTS, bytes(raw)); o.w(SCENARIO+0x100, 1)
        result = o.call(o.owner['spawn'], (0x10000,)) & 255
        creation = next(e for e in o.events if e.get('name') == 'create')
        placement = bytes.fromhex(creation['placement_bytes'])
        assert result == 1 and placement[0x1c:0x28] == raw[:12]
        failures.append(dict(version=version, case='explicit_start_bsp_short_7',
            metadata_override='Only start +12 is changed from the authored row.',
            result=result, raw_start_hex=bytes(raw).hex(),
            placement_hex=placement.hex(), after=o.state(), events=o.events))
        del o
        gc.collect()
    return rows, failures


class PlacementTrace(WorldTrace):
    def __init__(self, live=False):
        self.capture = []
        self.position_writes = []
        self.object_pointer = None
        self.stop_at = 0x1da93e if live else None
        self.boundary = None
        super().__init__(0, True)
        self.services[0x178120] = ('supplied optional attachment effect allocation unavailable', NONE, 16)
        self.w(0x526230, SCRATCH+0x900); self.w(SCRATCH+0x900, 0)
        self.g.mem_write(0x486560, bytes([int(live)]))
        if live:
            self.w(0x526248, POOL_PHYSICS)
            self.w(POOL_PHYSICS+0x20, 64); self.w(POOL_PHYSICS+0x24, 0xa0)
            self.sh(POOL_PHYSICS+0x40, 1)
            self.w(POOL_PHYSICS+0x44, POOL_PHYSICS+0x1000)
            self.w(POOL_PHYSICS+0x48, POOL_PHYSICS+0x5000)
        self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE, self.position_hook)
        self.capture = []
        self.events = []
        self.heap_requests = []

    def position_hook(self, g, access, address, size, value, user):
        if self.object_pointer is not None and (
                address < self.object_pointer+0x70 and
                address+size > self.object_pointer+0x64):
            self.position_writes.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                offset=f'{address-self.object_pointer:03x}', size=size,
                value=f'{value & ((1 << (size*8))-1):0{size*2}x}'))

    def hook(self, g, a, size, user):
        if a == 0x18fce0:
            self.object_pointer = self.heap_next
        if a in ENTRIES:
            row = dict(entry=f'{a:08x}', registers=self.registers())
            if self.object_pointer is not None:
                row['root'] = self.vec(self.object_pointer+0x64)
                row['location_hex'] = bytes(g.mem_read(self.object_pointer+0x28, 8)).hex()
            self.capture.append(row)
        if a == self.stop_at:
            sp = g.reg_read(UC_X86_REG_ESP)
            shape = g.reg_read(UC_X86_REG_EDI)
            self.boundary = dict(stop_before=f'{a:08x}',
                reason='Real serialized shape vtable dispatch requires original runtime fixup and a Havok world.',
                shape=f'{shape:08x}', serialized_vtable_word=f'{self.u(shape):08x}',
                source_body_position=self.vec(sp+0xa0),
                source_body_transform_hex=bytes(g.mem_read(sp+0x70, 48)).hex(),
                component_hex=bytes(g.mem_read(POOL_PHYSICS+0x1000, 0xa0)).hex(),
                biped_state=self.u(self.object_pointer+0x3dc, 1),
                biped_adjustment_count=self.u(self.object_pointer+0x3aa, 1))
            g.emu_stop()
            return
        super().hook(g, a, size, user)

    def create(self, placement):
        self.g.mem_write(PLACEMENT, placement)
        self.active = 'actual_player_biped_creation'
        self.count = 0; self.w(STACK, STOP); self.w(STACK+4, PLACEMENT)
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        for r, v in ((UC_X86_REG_FPCW, 0x27f), (UC_X86_REG_MXCSR, 0x1f80),
                     (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):
            self.g.reg_write(r, v)
        self.g.emu_start(0xb8a30, STOP, count=1000000)
        expected = self.stop_at or STOP
        assert self.g.reg_read(UC_X86_REG_EIP) == expected
        datum = 0x10002 if self.stop_at else self.g.reg_read(UC_X86_REG_EAX)
        assert datum == 0x10002
        state = self.object_state(datum)
        assert state['position'] == vector(placement[0x1c:0x28])
        assert self.u(self.object_pointer+0x14) == NONE
        if self.stop_at:
            assert state['physical_component'] == '00010000'
            assert self.boundary['source_body_position'] == state['position']
        else:
            assert state['physical_component'] == 'ffffffff'
        root_writes = self.position_writes
        # Original allocator clears storage before creation copies the root.
        # Source writes each authored root float only once after that; no subsequent
        # ray-ground placement or root-position adjustment exists in this slice.
        assert [(r['offset'], r['size']) for r in root_writes] == [('064', 4), ('068', 4), ('06c', 4)]*2
        assert [r['value'] for r in root_writes[:3]] == ['00000000']*3
        assert [r['value'] for r in root_writes[3:]] == state['position']['bits']
        return dict(physics_flag_486560=int(bool(self.stop_at)),
            completion='bounded_live_physics_prefix' if self.stop_at else 'complete_suspended_physics_constructor',
            result_eax=None if self.stop_at else f'{datum:08x}', object=state,
            root_position_writes=root_writes, ordered_source_entries=self.capture,
            used_services=[dict(entry=e['service'], description=e['description'],
                result=e['return_value'], registers=e['registers'],
                stack_arguments=e['stack_arguments']) for e in self.events if 'service' in e],
            heap_requests=self.heap_requests, collision_queries=self.collision_queries,
            boundary=self.boundary)


class AdmissionPrefix(OwnershipOracle):
    def __init__(self):
        super().__init__('tu11')
        self.query_prefix = None

    def hook(self, g, address, size, user):
        if address == 0x1c8c34:
            sp = g.reg_read(UC_X86_REG_ESP)
            # At this point one 2C argument has been pushed. Original callback
            # is at sp+14, query descriptor at sp+34, transform at sp+54.
            self.query_prefix = dict(stop_before='001c8c34',
                reason='Original query worker allocator, live Havok world and runtime-fixed shape are unprovided.',
                callback_hex=bytes(g.mem_read(sp+0x14, 0x20)).hex(),
                query_hex=bytes(g.mem_read(sp+0x34, 0x60)).hex(),
                shape=f'{self.u(sp+0x38):08x}',
                world_transform_hex=bytes(g.mem_read(sp+0x54, 0x40)).hex())
            g.emu_stop(); return
        super().hook(g, address, size, user)


def admission_prefix(c):
    o = AdmissionPrefix(); tag = spawn_fixture(o, c)
    raw = bytes(c.data[c.block(c.scenario+0x100, 0x34)[0]:][:0x34])
    o.g.mem_write(STARTS, raw); o.w(SCENARIO+0x100, 1)
    o.shape_hit = None
    # call() normally returns to STOP; here stop is explicit source boundary.
    o.call(o.owner['spawn'], (0x10000,), end=0x1c8c34)
    # emu_start's end is exclusive, so snapshot this reached boundary after
    # execution stops and before the unresolved allocator pointer is read.
    o.hook(o.g, 0x1c8c34, 0, None)
    assert o.query_prefix is not None
    o.query_prefix.update(authored_index=0, raw_start_hex=raw.hex(),
        biped_definition=tag['datum'], eligibility_result=None,
        scope='Original chooser, shape helper and Havok query descriptor prefix only; no contact result supplied.')
    return o.query_prefix


def listings(t):
    directory = OUT / 'placement-instructions'
    directory.mkdir(parents=True, exist_ok=True)
    rows = []
    for entry in ENTRIES:
        routine = _trace(t.code, entry, t.starts | set(ENTRIES))
        instructions = [t.code.at(a) for a in sorted(routine.code)]
        path = directory / f'{entry:08x}.asm'
        path.write_text('\n'.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
                                  for i in instructions)+'\n')
        rows.append(dict(entry=f'{entry:08x}', count=len(instructions),
            bytes_sha256=hashlib.sha256(b''.join(i.bytes for i in instructions)).hexdigest(),
            listing=str(path.relative_to(ROOT)), listing_sha256=digest(path),
            direct_calls=[dict(site=f'{s:08x}', target=f'{d:08x}')
                          for s, d, k in routine.sites if k == 'call']))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=OUT/'placement-report.json')
    args = parser.parse_args()
    c = Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(c.data).hexdigest() == CACHE_PIN
    setup, failures = setup_cases(c)
    paired = []
    for index in range(66):
        a, b = setup[index], setup[66+index]
        assert a['position'] == b['position'] and a['forward'] == b['forward'] and a['up'] == b['up']
        paired.append(dict(authored_index=index, base_tu_setup_vectors_equal=True))
    creations = []
    for row in setup[66:]:
        t = PlacementTrace()
        result = t.create(bytes.fromhex(row['placement_hex']))
        assert not result['collision_queries']
        result.update(authored_index=row['authored_index'])
        creations.append(result)
        del t; gc.collect()
    live = []
    for index in (0, 32, 65):
        t = PlacementTrace(True)
        result = t.create(bytes.fromhex(setup[66+index]['placement_hex']))
        result.update(authored_index=index)
        live.append(result)
        if index == 65:
            instruction_rows = listings(t)
        del t; gc.collect()
    report = dict(executable_pins={v: data['pin'] for v, data in VERSIONS.items()},
        cache_sha256=CACHE_PIN, tool_sha256=digest(__file__),
        dependencies={str(p.relative_to(ROOT)): digest(p) for p in (
            ROOT/'tools/oracle_mp_spawn.py', ROOT/'tools/oracle_mp_spawn_lifecycle.py',
            ROOT/'tools/oracle_mp_spawn_ownership.py', ROOT/'tools/oracle_smg_firing_1_1.py',
            ROOT/'tools/oracle_mp_inventory_1_1.py', ROOT/'tools/oracle_mp_inventory_netgame_1_1.py',
            ROOT/'tools/oracle_mp_inventory_world_1_1.py')},
        normal_setup=setup, failures=failures, comparisons=paired,
        real_bsp_constructors=creations, live_body_prefixes=live,
        admission_query_prefix=admission_prefix(c), instructions=instruction_rows,
        summary=dict(authored_starts=66, complete_paired_normal_setups=len(setup),
            complete_suspended_physics_constructors=len(creations),
            live_body_prefixes=len(live), failure_or_metadata_cases=len(failures),
            root_initialization_writes_per_constructor=3,
            authored_position_writes_per_constructor=3, constructor_ground_ray_calls=0),
        supplied_boundaries=[
            'Each normal setup uses one actual authored row and original chooser; all other players lack current units.',
            'Normal setup supplies real representation datum/variant NONE, clear shape result, successful object datum, customization false and loadout no-op.',
            'Constructor uses all actual cache tags/model nodes/BSP, empty real source pool backing arrays, heap backing storage and all clusters active.',
            'Complete constructor explicitly sets physics flag 486560=0 (suspended/deferred); animation runtime node unavailable and optional sound/effect unavailable.',
            'Live flag=1 constructor uses original physics component allocator and stops before embedded serialized shape virtual dispatch 1DA93E.',
            'Havok body/world initialization, shape runtime fixup, chooser live broadphase/narrowphase and subsequent game ticks are unproved.',
            'No unit head-marker transform is supplied. No complete original death, live spawn or collision parity claim.'])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report['summary']))


if __name__ == '__main__':
    main()
