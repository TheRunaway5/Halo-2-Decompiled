#!/usr/bin/env python3
"""Original 1.1 SMG impact oracle with explicit collision and runtime boundaries.

Runs the real projectile construction, flight callback, impact callback, event
creation and damage routines against unchanged authored Lockout metadata.
Collision results and target runtime state are supplied, never claimed to be a
recovered world-intersection or complete original simulation.
"""
import argparse
import csv
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import (
    ROOT, PIN, CACHE_PIN, POOL, HEADERS, BITMAP, TAGS, PROJECTILE, UNIT,
    SEED, TIME, Cache, Xbe, _Code, _trace, UC_X86_REG_EAX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_XMM0,
)
from oracle_smg_projectile_1_1 import AllocationTrace, PI

TARGET = 0x10D8000
TI = 0x10003
COLLISION = 0x10E8000
LOWER_PLANE = 0x10E9000
QUERY_ORIGIN = 0x10E9100
QUERY_DELTA = 0x10E9200


class ImpactTrace(AllocationTrace):
    def __init__(self, mode=1, shield=1., body=1., material_entry=1,
                 collision_material=56, intercept_damage=False, clear=False,
                 fraction=.5):
        super().__init__(mode, (1., 0., 0.), (0., 0., 0.))
        self.impact_events = []
        self.impact_returns = []
        self.material_entry = material_entry
        self.collision_material = collision_material
        self.intercept_damage = intercept_damage
        self.clear = clear
        self.fraction = fraction
        self.collision_count = 0
        c = Cache(ROOT / 'assets/disc/maps/lockout.map')
        assert hashlib.sha256(c.data).hexdigest() == CACHE_PIN
        globals_tag = next(t for t in c.tags.values() if t['group'] == 'matg')
        self.w(0x4E791C, globals_tag['offset'] + c.magic)
        spartan = c.tags[0xE9F2086D]
        self.target_definition = int(spartan['datum'], 16)
        self.authored_impact = {'globals': globals_tag, 'spartan': spartan}
        model = c.tags[c.u(spartan['offset'] + 0x38)]
        info = c.block(model['offset'] + 0x60, 0xF8)[0]
        self.authored_impact['model'] = model
        self.render_region_count = c.u(model['offset']+0x70)
        self.node_count = c.u(model['offset']+0x78)
        self.maximum_body = c.f(info+0x28)
        self.maximum_shield = c.f(info+0x8C)
        self.authored_impact['damage_info'] = {
            'file_offset': info, 'original_va': f'{info+c.magic:08x}',
            'raw_hex': bytes(c.data[info:info+0xF8]).hex()}
        self.authored_impact['model_materials'] = [
            {'index': i, 'file_offset': at, 'original_va': f'{at+c.magic:08x}',
             'raw_hex': bytes(c.data[at:at+0x14]).hex(),
             'name': c.text(c.u(368)+c.u(c.u(364)+(c.u(at)&0xFFFF)*4)),
             'damage_section': c.h(at+6), 'global_material': c.h(at+0x10)}
            for i, at in enumerate(c.block(model['offset']+0x58, 0x14))]
        proj = c.tags[0xE59B0426]
        jpt = c.tags[c.u(proj['offset']+0x14C)]
        self.authored_impact['projectile'] = proj
        self.authored_impact['impact_damage'] = jpt
        self.authored_impact['damage_raw_hex'] = bytes(
            c.data[jpt['offset']:jpt['offset']+jpt['size']]).hex()
        self.authored_impact['material_responses'] = [
            {'index': i, 'file_offset': at, 'original_va': f'{at+c.magic:08x}',
             'raw_hex': bytes(c.data[at:at+0x58]).hex(),
             'material': c.h(at+0x10), 'response': c.h(at+2)}
            for i, at in enumerate(c.block(proj['offset']+0x19C, 0x58))]
        self.collision_node, self.collision_region = -1, -1
        coll = c.tags[c.u(model['offset']+0xC)]
        self.authored_impact['collision_model'] = coll
        if material_entry in (0, 1):
            region_index = 2 if material_entry == 0 else 1
            region = c.block(coll['offset']+0x1C, 12)[region_index]
            permutation = c.block(region+4, 20)[0]
            bsp = c.block(permutation+4, 0x44)[0]
            surface = c.block(bsp+0x2C, 8)[0]
            assert c.h(surface+6) == material_entry
            self.collision_region = region_index
            self.collision_node = c.h(bsp)
            self.authored_impact['supplied_selected_surface'] = {
                'region_index': region_index, 'permutation_index': 0,
                'bsp_index': 0, 'bsp_file_offset': bsp, 'surface_index': 0,
                'surface_file_offset': surface, 'node_index': c.h(bsp),
                'surface_hex': bytes(c.data[surface:surface+8]).hex()}
        c.data.close(); c.stream.close()
        self.execute(1, 0)
        self.impact_events.clear()
        self.active = 'supplied_target_state'
        self.w(POOL+0x38, 4); self.w(BITMAP, 15)
        self.g.mem_write(HEADERS+36, struct.pack('<HBBHHI', 1, 3, 0, 0, 0x1000, TARGET))
        # Target fields are explicitly supplied. The original unit constructor,
        # player spawn, animation pose and world-collision services are outside
        # this oracle; authored definitions remain unchanged.
        self.g.mem_write(TARGET, bytes(self.g.mem_read(UNIT, 0x1000)))
        self.w(TARGET, self.target_definition)
        for offset in (0xC, 0x10, 0x14, 0xB4, 0xC4, 0xC8, 0xD4, 0x12C,
                       0x13C, 0x194, 0x218, 0x21C, 0x220, 0x224, 0x24C):
            self.w(TARGET+offset, -1)
        self.sh(TARGET+0xC2, -1)
        self.g.mem_write(TARGET+0xAA, b'\x00')
        self.g.mem_write(TARGET+0xAF, b'\xff')
        self.f(TARGET+0xE4, self.maximum_body)
        self.f(TARGET+0xE8, self.maximum_shield)
        self.f(TARGET+0xEC, body); self.f(TARGET+0xF0, shield)
        self.f(TARGET+0xF4, 0.)
        self.sh(TARGET+0x104, 0); self.sh(TARGET+0x106, 0)
        self.sh(TARGET+0x10A, 0)
        self.sh(TARGET+0x120, 16); self.sh(TARGET+0x122, 0xA00)
        self.g.mem_write(TARGET+0x100, bytes(4))
        # Original node state layout: two damage sections supplied unbroken.
        self.g.mem_write(TARGET+0xA00, bytes(0x100))
        self.sh(TARGET+0x118, self.render_region_count*10)
        self.sh(TARGET+0x11A, 0xB00)
        self.g.mem_write(TARGET+0xB00, bytes(0x100))
        self.sh(TARGET+0x116, 0x1000)
        for node in range(self.node_count):
            self.g.mem_write(TARGET+0x1000+node*0x34,
                            struct.pack('<13f', 1., 1., 0., 0., 0., 1., 0.,
                                        0., 0., 1., 0., 0., 0.))
        self.w(PROJECTILE+0x140, -1)
        self.services.update({
            0x1CE3E0: ('supplied collision-contact notification sink', 0, 8),
            0x1CE080: ('supplied collision audiovisual notification sink', 0, 20),
            0x1781F0: ('supplied effect notification sink', 0, 16),
            0xB6760: ('supplied network dirty-state notification sink', 0, 8),
            0xB8250: ('supplied object hit notification sink', 0, 0),
            0xCB1D0: ('supplied unit damage notification sink', 0, 16),
            0xCB410: ('supplied unit damage response notification sink', 0, 8),
            0xA8ED0: ('supplied damage replication notification sink', 0, 4),
            0x177A10: ('supplied impact effect notification sink', 0, 4),
            0x189E10: ('supplied material effect lookup: unavailable runtime effects', 0, 48),
            0x1F3900: ('supplied posed collision BSP intersection result', 1, 20),
        })
        if intercept_damage:
            self.services[0xD9180] = ('supplied damage sink for event isolation', 0, 24)
        self.collision_producer = None
        if material_entry in (0, 1):
            self.active = 'collision_producer'
            self.g.mem_write(COLLISION, bytes(0x5C))
            self.f(COLLISION+4, 1.)
            self.g.mem_write(LOWER_PLANE, struct.pack('<4f', -1., 0., 0., 0.))
            self.g.mem_write(QUERY_ORIGIN, struct.pack('<3f', 0., 0., 0.))
            self.g.mem_write(QUERY_DELTA, struct.pack('<3f', 10., 0., 0.))
            event_start = len(self.events)
            self.run(0x16AF90, (TI, 1, 0x2480000F, 0x100, QUERY_ORIGIN,
                               QUERY_DELTA, QUERY_ORIGIN, QUERY_DELTA, COLLISION))
            assert self.u(COLLISION) == 4
            assert self.u(COLLISION+0x24, 2) == 56
            assert self.u(COLLISION+0x5A, 2) == material_entry
            self.collision_producer = {
                'original_result_hex': bytes(self.g.mem_read(COLLISION, 0x5C)).hex(),
                'events': self.events[event_start:],
                'scope': 'Original16AF90,1F3500,DA0E0,16A5C0 with actual coll/hlmt; supplied1F3900 intersection of selected authored surface and identity node poses. Fraction/point are explicit motion-query fixtures; full segment traversal is not executed.'}

    def impact_snapshot(self):
        return {'projectile_hex': bytes(self.g.mem_read(PROJECTILE, 428)).hex(),
                'projectile_header_hex': bytes(self.g.mem_read(HEADERS+24, 12)).hex(),
                'target_hex': bytes(self.g.mem_read(TARGET, 0xC00)).hex(),
                'body_bits': f'{self.u(TARGET+0xEC):08x}',
                'shield_bits': f'{self.u(TARGET+0xF0):08x}',
                'damage_flags': f'{self.u(TARGET+0x10A,2):04x}',
                'seed': f'{self.u(SEED):08x}'}

    def hook(self, guest, address, size, user):
        sp = guest.reg_read(UC_X86_REG_ESP)
        if hasattr(self, 'impact_events') and self.active == 'collision_producer' and address == 0x1F3900:
            out = self.u(sp+20)
            raw = bytearray(0x24)
            packed = (self.collision_region << 24) | self.collision_node
            struct.pack_into('<IHHfIiiiBBh', raw, 0, packed,
                             self.collision_node, self.collision_region, .5,
                             LOWER_PLANE, 0, 0, 0, 0, 0, self.material_entry)
            guest.mem_write(out, bytes(raw))
        if hasattr(self, 'impact_events') and self.active.startswith(('impact', 'expiry')):
            if address == 0x189E10:
                for at in range(28, 49, 4):
                    pointer = self.u(sp+at)
                    if pointer:
                        self.w(pointer, -1)
            for ret, row in self.impact_returns[:]:
                if address == ret:
                    row['returned_eax'] = f'{guest.reg_read(UC_X86_REG_EAX):08x}'
                    row['returned_xmm0_low_bits'] = f'{guest.reg_read(UC_X86_REG_XMM0)&0xFFFFFFFF:08x}'
                    row['after'] = self.impact_snapshot()
                    if 'event_pointer' in row:
                        row['event_after_hex'] = bytes(guest.mem_read(row['event_pointer'], 0x88)).hex()
                    if address == ret and row.get('entry') == '001015a0':
                        pointer = guest.reg_read(UC_X86_REG_EAX)
                        row['response_hex'] = bytes(guest.mem_read(pointer, 0x58)).hex()
                    self.impact_returns.remove((ret, row))
            if address == 0x16B240:
                self.collision_count += 1
                output = self.u(sp+24)
                origin = self.u(sp+8); delta = self.u(sp+12)
                hit = not self.clear and self.collision_count == 1
                raw = bytearray(0x5C)
                struct.pack_into('<I', raw, 0, 4 if hit else 0)
                struct.pack_into('<f', raw, 4, self.fraction if hit else 1.)
                fraction = self.fraction if hit else 1.
                point = [self.fv(origin+i*4)+fraction*self.fv(delta+i*4) for i in range(3)]
                struct.pack_into('<3f', raw, 8, *point)
                for at in (0x14, 0x1C, 0x38, 0x3C, 0x40, 0x48, 0x4C, 0x50):
                    struct.pack_into('<I', raw, at, 0xFFFFFFFF)
                for at in (0x18, 0x20, 0x44, 0x46, 0x5A):
                    struct.pack_into('<H', raw, at, 0xFFFF)
                struct.pack_into('<I', raw, 0x24, self.collision_material if hit else 0xFFFFFFFF)
                if hit:
                    if self.collision_producer:
                        produced = bytes.fromhex(self.collision_producer['original_result_hex'])
                        raw[0x24:0x38] = produced[0x24:0x38]
                        raw[0x40:] = produced[0x40:]
                    else:
                        struct.pack_into('<3f', raw, 0x28, -1., 0., 0.)
                        struct.pack_into('<I', raw, 0x40, TI)
                        struct.pack_into('<H', raw, 0x5A, self.material_entry & 0xFFFF)
                guest.mem_write(output, bytes(raw))
                self.impact_events.append({'collision_service': '0016b240',
                    'supplied': True, 'mask': f'{self.u(sp+4):08x}',
                    'origin': self.vec(origin), 'delta': self.vec(delta),
                    'ignored_object': f'{self.u(sp+16):08x}',
                    'ignored_object_2': f'{self.u(sp+20):08x}',
                    'result_hex': raw.hex(), 'result_hit': hit})
                guest.reg_write(UC_X86_REG_EAX, int(hit))
                guest.reg_write(UC_X86_REG_ESP, sp+28)
                guest.reg_write(UC_X86_REG_EIP, self.u(sp)); return
            if address in (0xFEB40, 0xD7C60, 0xBD080, 0xD9180, 0xDB160,
                           0xDA710, 0xDA2B0, 0xDA620, 0xD71C0, 0xD8070,
                           0xDC690, 0xDAA90, 0xDAC40, 0x1015A0, 0xFEB10,
                           0x100410, 0xB9430, 0xD7E00, 0xDC0A0, 0xDC460,
                           0xDB710, 0xBB6E0, 0xBB5E0, 0xB92A0, 0xB9350,
                           0xBD1F0, 0x16D4C0, 0xD7320, 0xD7380):
                row = {'entry': f'{address:08x}', 'registers': self.registers(),
                       'stack': [f'{self.u(sp+4+i*4):08x}' for i in range(6)],
                       'before': self.impact_snapshot()}
                if address == 0xD9180:
                    row['event_pointer'] = self.u(sp+4)
                    row['event_before_hex'] = bytes(guest.mem_read(self.u(sp+4), 0x88)).hex()
                if address == 0xDAA90:
                    row['accumulator_hex'] = bytes(guest.mem_read(guest.reg_read(UC_X86_REG_EDI), 0x34)).hex()
                if address == 0xDAC40:
                    row['response_record_hex'] = bytes(guest.mem_read(guest.reg_read(UC_X86_REG_EDI), 0x54)).hex()
                if address == 0x1015A0:
                    row['selected_global_material'] = self.u(sp+8, 2)
                if address == 0xD71C0:
                    row['armor_keys'] = [f'{guest.reg_read(reg):08x}' for reg in
                                         (UC_X86_REG_ECX, UC_X86_REG_EDX)] + [
                                             f'{self.u(sp+at):08x}' for at in (4, 8)]
                self.impact_events.append(row)
                self.impact_returns.append((self.u(sp), row))
        super().hook(guest, address, size, user)


def execute_case(mode, shield, body, entry, intercept=False, clear=False,
                 fraction=.5, initial_distance=None, motion_ticks=1):
    trace = ImpactTrace(mode, shield, body, entry, intercept_damage=intercept,
                        clear=clear, fraction=fraction)
    trace.active = 'expiry_motion' if clear else 'impact_motion'
    trace.tick = 1; trace.w(TIME+8, 1)
    if initial_distance is not None:
        trace.f(PROJECTILE+0x168, initial_distance)
    before = trace.impact_snapshot()
    event_start = len(trace.events)
    motion_rows = []
    for tick in range(1, motion_ticks+1):
        trace.tick = tick; trace.w(TIME+8, tick)
        motion_before = trace.impact_snapshot()
        trace.run(0xFCEC0, (PI,))
        motion_rows.append({'tick': tick, 'before': motion_before,
                            'after': trace.impact_snapshot()})
        if trace.u(HEADERS+24+2, 1)&0x10:
            break
    after = trace.impact_snapshot()
    postpass = None
    if trace.u(HEADERS+24+2, 1)&0x10:
        # Explicit absent AI/sound/effect state and abstract lower heap context.
        # B92A0's eight cleanup callbacks and BD1F0/16D4C0 execute unchanged.
        contexts = [(0x4E58BC, 0x10EA000), (0x4FCDA8, 0x10EB000),
                    (0x4F4854, 0x10EC000), (0x4F1F0C, 0x10ED000)]
        for address, pointer in contexts:
            trace.w(address, pointer)
        trace.active = 'impact_postpass'
        trace.run(0xB8040)
        assert trace.u(HEADERS+24+8) == 0
        assert trace.u(HEADERS+24+2, 1) == 0
        assert trace.u(BITMAP)&4 == 0
        postpass = {'after': trace.impact_snapshot(),
                    'bitmap_bits': f'{trace.u(BITMAP):08x}',
                    'supplied_empty_contexts': [{'global': f'{a:08x}',
                                                'pointer': f'{p:08x}'} for a,p in contexts],
                    'supplied_heap_link_header_hex': bytes(trace.g.mem_read(PROJECTILE-16, 16)).hex()}
    return {'mode': mode, 'supplied_initial_shield': shield, 'supplied_initial_body': body,
            'supplied_model_material_entry': entry, 'intercept_damage': intercept,
            'supplied_collision_fraction': fraction,
            'supplied_initial_distance': initial_distance,
            'clear_expiry_case': clear, 'authored': trace.authored_impact,
            'collision_producer': trace.collision_producer,
            'before': before, 'after': after, 'impact_events': trace.impact_events,
            'motion_rows': motion_rows, 'original_removal_postpass': postpass,
            'runtime_events': trace.events[event_start:],
            'supplied_services': {f'{a:08x}': {'name': n, 'result': r, 'cleanup': p}
                                  for a, (n, r, p) in trace.services.items()}}


def generate_suite(directory):
    directory = directory.resolve(); directory.mkdir(parents=True, exist_ok=True)
    cases = []
    reports = {}
    configs = []
    for entry in (-1, 0, 1):
        for shield in (1., .1, .01, 0.):
            for body in (1., .01):
                configs.append((f'entry{entry}-shield{shield:g}-body{body:g}',
                                dict(shield=shield, body=body, entry=entry)))
    configs += [('fraction_zero', dict(shield=1., body=1., entry=1, fraction=0.)),
                ('fraction_one', dict(shield=1., body=1., entry=1, fraction=1.)),
                ('clear_range_sequence', dict(shield=1., body=1., entry=1,
                                             clear=True, motion_ticks=6)),
                ('clear_range39', dict(shield=1., body=1., entry=1,
                                      clear=True, initial_distance=39.)),
                ('clear_range40', dict(shield=1., body=1., entry=1,
                                      clear=True, initial_distance=40.))]
    for mode in (1, 5):
        for name, config in configs:
            result = execute_case(mode=mode, **config)
            result.update({'xbe_sha256': PIN, 'cache_sha256': CACHE_PIN,
                           'oracle_tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()})
            path = directory/f'{name}-mode{mode}.json'
            path.write_text(json.dumps(result, indent=2)+'\n')
            reports[mode, name] = result
            cases.append({'name': name, 'mode': mode,
                          'report': str(path.relative_to(ROOT)),
                          'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                          'motion_ticks': len(result['motion_rows']),
                          'damage_events': sum(e.get('entry') == '000d9180' for e in result['impact_events']),
                          'impact_rng_stores': sum('rng_write_instruction' in e for e in result['runtime_events']),
                          'final_body_bits': result['after']['body_bits'],
                          'final_shield_bits': result['after']['shield_bits'],
                          'final_damage_flags': result['after']['damage_flags'],
                          'original_pool_removed': result['original_removal_postpass'] is not None})
    comparisons = []
    for name, _ in configs:
        a,b = reports[1,name], reports[5,name]
        assert a['after'] == b['after']
        assert a['motion_rows'] == b['motion_rows']
        assert a['original_removal_postpass'] == b['original_removal_postpass']
        for address, field in (('000d9180','event_after_hex'),
                               ('000daa90','accumulator_hex'),
                               ('000dac40','response_record_hex')):
            assert [e.get(field) for e in a['impact_events'] if e.get('entry') == address] == [
                e.get(field) for e in b['impact_events'] if e.get('entry') == address]
        comparisons.append({'name': name, 'mode1_mode5_result_equal': True})
    assert all(row['impact_rng_stores'] == 0 for row in cases)
    output = {'xbe_sha256': PIN, 'cache_sha256': CACHE_PIN,
              'oracle_tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'cases': cases, 'comparisons': comparisons,
              'case_count': len(cases), 'motion_ticks': sum(c['motion_ticks'] for c in cases),
              'scope': 'Unchanged original impact/event/damage/material/death-state/deletion and pool-removal instructions; actual authored metadata; explicit selected collision surface, lower query and target state; recorded notification sinks and empty cleanup contexts. No native-world intersection or complete multiplayer proof.'}
    (directory/'suite.json').write_text(json.dumps(output, indent=2)+'\n')
    print(json.dumps({'case_count': len(cases), 'motion_ticks': output['motion_ticks']}))


def dump_instructions(directory):
    directory = directory.resolve(); directory.mkdir(parents=True, exist_ok=True)
    image = Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(image.data).hexdigest() == PIN
    code = _Code(image, '.text')
    starts = {int(r['address'],16) for r in csv.DictReader(
        (ROOT/'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')}
    entries = (0xFCEC0, 0xFCF90, 0x100110, 0xFEB40, 0xFEB10, 0x100410,
               0xD7C60, 0xBD080, 0xD9180, 0xDB160, 0xD7160, 0xDA710,
               0xDA2B0, 0xDA620, 0xD71C0, 0xD7320, 0xD7380, 0xD8070,
               0xDC690, 0xDC0A0, 0xDC460, 0xDB710, 0xD7E00,
               0xDAA90, 0xDAC40, 0x1015A0, 0x18A0C0,
               0x16B240, 0x16AF90, 0x1F3500, 0x1F3660, 0x1F3900,
               0xDA0E0, 0x16A5C0, 0xBB6E0, 0xB9430, 0xB8040,
               0xB92A0, 0xB9350, 0xBD1F0, 0x16D4C0)
    rows = []
    for address in entries:
        routine = _trace(code, address, starts|set(entries))
        instructions = [code.at(at) for at in sorted(routine.code)]
        path = directory/f'{address:08x}.asm'
        path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'
                                for i in instructions))
        rows.append({'start': f'{address:08x}', 'end_exclusive': f'{routine.end:08x}',
                     'instruction_count': len(instructions),
                     'reachable_instruction_bytes_sha256': hashlib.sha256(
                         b''.join(i.bytes for i in instructions)).hexdigest(),
                     'listing': str(path.relative_to(ROOT)),
                     'listing_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                     'direct_calls': [{'site': f'{a:08x}', 'target': f'{b:08x}'}
                                      for a,b,kind in routine.sites if kind == 'call']})
    report = {'xbe_sha256': PIN, 'oracle_tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'routines': rows, 'scope': 'Reachable original static instructions; listings alone do not claim every branch executed.'}
    (directory/'inventory.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'routines': len(rows)}))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/map-pack/smg-impact-1.1-oracle.json')
    p.add_argument('--intercept-damage', action='store_true')
    p.add_argument('--clear-expiry', action='store_true')
    p.add_argument('--shield', type=float, default=1.)
    p.add_argument('--body', type=float, default=1.)
    p.add_argument('--material-entry', type=int, choices=(-1, 0, 1), default=1)
    p.add_argument('--mode', type=int, choices=(1, 5), default=1)
    p.add_argument('--fraction', type=float, default=.5)
    p.add_argument('--initial-distance', type=float)
    p.add_argument('--motion-ticks', type=int, default=1)
    p.add_argument('--suite', type=Path)
    p.add_argument('--instruction-inventory', type=Path)
    args = p.parse_args()
    if args.suite is not None:
        generate_suite(args.suite); return
    if args.instruction_inventory is not None:
        dump_instructions(args.instruction_inventory); return
    result = execute_case(args.mode, args.shield, args.body, args.material_entry,
                          args.intercept_damage, args.clear_expiry, args.fraction,
                          args.initial_distance, args.motion_ticks)
    result.update({'xbe_sha256': PIN, 'cache_sha256': CACHE_PIN,
                   'oracle_tool_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                   'scope': 'Untouched original instructions and authored metadata, explicit target state and collision output. Supplied runtime notifications are recorded; world intersection, unit spawn and complete original frame are not executed.'})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({'case': str(args.output), 'body': result['after']['body_bits'],
                      'shield': result['after']['shield_bits'], 'events': len(result['impact_events'])}))


if __name__ == '__main__':
    main()
