#!/usr/bin/env python3
"""Execute untouched vehicle/object updates on explicit original-layout inputs.

Vehicle definition bytes come from the owned Coagulation, Headlong and Zanzibar
maps. Controlled definition overlays, object hierarchies, component and motion
records, animation states, velocities, controls and FP state are explicit test
inputs, not original construction or live driving. No callee output, contact,
successful owner result or patched instruction is supplied. Paths stop at the
actual next unported owner.
"""
import argparse
import itertools
import json
import math
import random
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from research_vehicle_control_20261011 import sha
from inspect_multiplayer import Cache
from oracle_vehicle_runtime_20261011 import controls as parent_controls
from export_mp_physics_contact_lifecycle import segments

BASE = 0x1000000
POOL, HEADERS, BODY, TAGS, DEFINITION, CLOCK = (BASE + n for n in (0, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000))
STACK = BASE + 0xd000
OUTPUT = BASE + 0x6000
STOP = BASE + 0xf000
UNIT = 0xe6f80000
PARENT_BODY = BODY + 0x800
COMPONENT_POOL, COMPONENT, BODY_ARRAY, HAVOK_BODY = (BASE + n for n in (0x7000, 0x8000, 0x8300, 0x8400))
MOTION = BASE + 0x8500
STATE_AREA = BASE + 0x10000
ACTUATOR, ACTUATOR_DATA = BASE + 0x6100, BASE + 0x6200


def capture(g):
    result = dict(gp=[g.reg_read(getattr(R, 'UC_X86_REG_' + name))
        for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')],
        xmm=[f'{g.reg_read(getattr(R, f"UC_X86_REG_XMM{n}")):032x}' for n in range(8)],
        eflags=g.reg_read(R.UC_X86_REG_EFLAGS), fs_base=g.reg_read(R.UC_X86_REG_FS_BASE),
        x87_control_word=g.reg_read(R.UC_X86_REG_FPCW), mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
        x87_status=g.reg_read(R.UC_X86_REG_FPSW), x87_tag=g.reg_read(R.UC_X86_REG_FPTAG))
    top = (result['x87_status'] >> 11) & 7
    result['x87_values'] = []
    empty = False
    for n in range(8):
        index = (top + n) & 7
        if (result['x87_tag'] >> (index * 2)) & 3 == 3:
            empty = True
            continue
        assert not empty, 'Sparse x87 stack requires a separate owner'
        mantissa, exponent = g.reg_read(getattr(R, f'UC_X86_REG_FP{index}'))
        result['x87_values'].append(mantissa.to_bytes(8, 'little').hex() + exponent.to_bytes(2, 'little').hex())
    return result


def definitions():
    records = []
    for name in ('coagulation', 'headlong', 'zanzibar'):
        path = ROOT / 'assets/disc/maps' / (name + '.map')
        pin = sha(path)
        c = Cache(path)
        for datum, tag in sorted(c.tags.items()):
            if tag['group'] != 'vehi':
                continue
            assert tag['offset'] is not None and tag['size'] >= 0x300
            p = tag['offset']
            raw = bytes(c.data[p:p + 0x300])
            records.append(dict(map=str(path.relative_to(ROOT)), map_sha256=pin,
                datum=datum, name=tag['name'], prefix_hex=raw.hex(),
                file_offset=p, prefix_sha256=__import__('hashlib').sha256(raw).hexdigest()))
        c.data.close()
        c.stream.close()
    return records



def controls(records):
    rows = parent_controls(records)
    baseline = dict(rows[0])

    def add(entry, **values):
        row = dict(baseline)
        row.update(entry=entry, **values)
        rows.append(row)

    # Both actual authored bytes and explicit field overlays are retained in
    # the declared inputs. The overlays test the recovered ABI, not tuning.
    for current, previous, rates in itertools.product(
            (-2., -0., 0., 0.25, 1., 2., float('inf'), float('nan')),
            (-1., 0., 0.5, 1., float('nan')), (None, (2., 4.))):
        add(0xd26c0, unit_current=current, unit_previous=previous, unit_rates=rates)
    for definition, flag in itertools.product(range(len(records)), (0, 4)):
        add(0xf2960, definition=definition, flags10a=flag)
    for flags, value, gear in itertools.product((0, 1, 2, 4, 8, 32, 47),
            (0., 1., -1., float('nan')), (-1, 0)):
        add(0xf11a0, definition_flags_1ec=flags, query_values=(value,) * 5, gear=gear)
    for value, dt in itertools.product((0., 1.e-8, -1.e-8, 1., -1., 1.e8, -1.e8,
            float('inf'), -float('inf'), float('nan')), (0., 1. / 30., 0.1, 1., -0.1)):
        add(0xf1270, decay_values=(value,) * 6, dt=dt)
    add(0xf1d00, registers={'EAX': STATE_AREA})
    for count, flag, alias in itertools.product((0, 1), (0, 1), (False, True)):
        add(0x208ed0, actuator_count=count, actuator_flag=flag,
            registers={'EDX': ACTUATOR, 'ECX': ACTUATOR_DATA + 0x48 if alias else OUTPUT})
    for parent, component, active, linear, angular in itertools.product(
            (False, True), (-1, 0), (False, True), (False, True), (False, True)):
        add(0xbb0c0, parent=parent, component=component, flags_c0=0x100 if active else 0,
            registers={'ECX': OUTPUT if linear else 0}, stack_values={4: OUTPUT + 0x10 if angular else 0})
    animation_states = [dict(animation_overrides={}),
        *[dict(animation_overrides={p: value}) for p, value in
          ((0x68, -1), (0, -1), (6, -1), (0x6c, 1), (0x40, -1), (0x46, -1))],
        *[dict(animation_overrides={0x52: primary, 0x51: secondary, 0x12: primary, 0x11: secondary})
          for primary, secondary in itertools.product((0, 1), (0, 1, 8))]]
    for entry, state in itertools.product((0x115600, 0x1105e0, 0xe7e30), animation_states):
        registers = {'ECX': BODY + 0x480} if entry == 0x1105e0 else {'EAX': BODY + 0x480} if entry == 0xe7e30 else {}
        add(entry, animation_valid=True, registers=registers, **state)
    for definition, flags, component in itertools.product(range(len(records)),
            (0, 0x40, 0x100, 0x1040, 0x1000), (-1, 0)):
        add(0xf1420, definition=definition, flags_c0=flags, component=component, flags348=0)
    for state, elapsed, up in itertools.product(range(5), (0, 255), (1., -1.)):
        add(0xf0eb0, flags348=0x40, self_right_state=state, self_right_elapsed=elapsed, up_z=up)
    for entry, component, linear, angular in itertools.product((0xb8630, 0xb86c0, 0x1c8430),
            (-1, 0), (False, True), (False, True)):
        registers = {}
        stack_values = {}
        if entry == 0xb8630:
            registers.update(ECX=OUTPUT if linear else 0, EDI=OUTPUT + 0x10 if angular else 0)
            stack_values[4] = 0
        elif entry == 0xb86c0:
            registers.update(EAX=OUTPUT + 0x10 if angular else 0, ESI=UNIT)
            stack_values[4] = OUTPUT if linear else 0
        else:
            registers['EBX'] = OUTPUT if linear else 0
            stack_values.update({4: OUTPUT + 0x10 if angular else 0, 8: 1})
        add(entry, component=component, registers=registers, stack_values=stack_values)
    for entry, flags in itertools.product((0xb8250, 0xbc910, 0xbaa80), (0, 1, 2, 4, 128, 255)):
        add(entry, object_header_flags=flags, registers={'EDX': 1})
    for entry, motion, alias in itertools.product((0x1d43c0, 0x1d44c0), (0, 1), (False, True)):
        add(entry, motion_flag=motion, registers={'EAX': 0, 'EDX': COMPONENT,
            'ECX': MOTION + (0x40 if entry == 0x1d43c0 else 0x50) if alias else OUTPUT})
    for entry, parent in itertools.product((0xbbe70, 0xbd6a0), (False, True)):
        add(entry, parent=parent if entry == 0xbbe70 else False,
            child=parent if entry == 0xbd6a0 else False, registers={'ECX': UNIT})
    add(0xbbe70, registers={'ECX': 0xffffffff})
    add(0xb8630, object_field_d4=UNIT + 1, registers={'ECX': OUTPUT, 'EDI': OUTPUT + 0x10}, stack_values={4: 0})
    for mask in (0, 1, 2, 0x80000000, 0xffffffff):
        add(0xe7e60, update_mask=mask)
    return rows


def declared_json(value):
    if isinstance(value, float) and not math.isfinite(value):
        return dict(float32_hex=struct.pack('<f', value).hex())
    if isinstance(value, dict):
        return {key: declared_json(item) for key, item in value.items()}
    if isinstance(value, (tuple, list)):
        return [declared_json(item) for item in value]
    return value


def execute(path, paired, latest, records, rows):
    assert sha(path) == (NEW_PIN if latest else PIN)
    image = Xbe(path)
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    g.mem_map(BASE, 0x20000)
    selected = {int(p, 16): q for name in ('address_map', 'instruction_map', 'entries')
                for p, q in paired[name].items()}
    sites = ({q: int(p, 16) for p, q in paired['instruction_map'].items()} if latest
             else {int(p, 16): int(p, 16) for p in paired['instruction_map']})

    def w(p, value):
        g.mem_write(p, struct.pack('<I', value & 0xffffffff))

    def address(p):
        return selected[p] if latest else p

    for p, value in ((0x4e78d0, POOL), (0x4eb114, TAGS), (0x518444, CLOCK), (0x526248, COMPONENT_POOL)):
        w(address(p), value)
    w(POOL + 0x44, HEADERS)
    w(HEADERS + 8, BODY)
    w(TAGS + 8, DEFINITION)
    state = {}

    def code(_g, p, _n, _u):
        if p not in sites:
            state['exit_entry'] = p
            g.emu_stop()
        else:
            state['instructions'].append(sites[p])

    def read(_g, _a, p, n, _x, _u):
        state['events'].append(dict(kind='R', instruction=sites[g.reg_read(R.UC_X86_REG_EIP)],
                                    address=p, raw=bytes(g.mem_read(p, n)).hex()))

    def write(_g, _a, p, n, value, _u):
        state['events'].append(dict(kind='W', instruction=sites[g.reg_read(R.UC_X86_REG_EIP)],
            address=p, before=bytes(g.mem_read(p, n)).hex(),
            written=(value & ((1 << (n * 8)) - 1)).to_bytes(n, 'little').hex()))

    g.hook_add(UC_HOOK_CODE, code)
    g.hook_add(UC_HOOK_MEM_READ, read)
    g.hook_add(UC_HOOK_MEM_WRITE, write)
    rng = random.Random(0xf06b0)
    cases = []
    for index, row in enumerate(rows):
        assert not (row.get('parent') and row.get('child')), 'Declared object hierarchy must be acyclic'
        # Real field layouts with explicit, independently seeded input state.
        # No constructor, body creation, solver or activation result is supplied.
        g.mem_write(HEADERS, bytes(0x40))
        w(HEADERS + 8, BODY)
        w(HEADERS + 20, PARENT_BODY)
        g.mem_write(HEADERS + 2, bytes((row.get('object_header_flags', 0), 1)))
        w(COMPONENT_POOL + 0x44, COMPONENT)
        component_bytes = bytearray(0xa0)
        component_bytes[0x18] = 0
        struct.pack_into('<I', component_bytes, 0x70, BODY_ARRAY)
        struct.pack_into('<I', component_bytes, 0x74, 1)
        g.mem_write(COMPONENT, bytes(component_bytes))
        w(BODY_ARRAY + 0x40, HAVOK_BODY)
        g.mem_write(HAVOK_BODY, bytes(0x80))
        g.mem_write(HAVOK_BODY + 0x40, bytes((row.get('motion_flag', 1),)))
        w(HAVOK_BODY + 0x3c, MOTION)
        g.mem_write(MOTION, rng.randbytes(0x100))
        body = bytearray(rng.randbytes(0x600))
        struct.pack_into('<I', body, 0, 0xab120000)
        struct.pack_into('<2f', body, 0x150, *row['velocity'])
        struct.pack_into('<3f', body, 0x70, *row['forward'], 0.)
        struct.pack_into('<3f', body, 0x7c, 0., 0., 1.)
        struct.pack_into('<h', body, 0x116, row['node_position_offset'])
        struct.pack_into('<3f', body, 0x1b0, *row['controls'])
        struct.pack_into('<f', body, 0x358, row['turn'])
        body[0x38b] = row['gear'] & 255
        body[0x349] = row['flags349']
        body[0x350] = row.get('transient_state', 0)
        body[0x351] = row.get('transient_ticks', 0)
        struct.pack_into('<I', body, 0x14, UNIT + 1 if row.get('parent') else 0xffffffff)
        struct.pack_into('<I', body, 0xb4, row.get('component', -1) & 0xffffffff)
        struct.pack_into('<I', body, 0xd4, row.get('object_field_d4', 0xffffffff))
        struct.pack_into('<I', body, 0x10, UNIT + 1 if row.get('child') else 0xffffffff)
        struct.pack_into('<I', body, 0xc, 0xffffffff)
        struct.pack_into('<h', body, 0x346, 0x580)
        body[0x580:0x590] = bytes(0x10)
        struct.pack_into('<I', body, 0x584, row.get('update_mask', 0))
        struct.pack_into('<H', body, 0xc0, row.get('flags_c0', 0))
        struct.pack_into('<h', body, 0x12a, 0x480)
        body[0x10a] = row.get('flags10a', 0)
        body[0x348] = row.get('flags348', body[0x348])
        body[0x34d] = row.get('self_right_state', 0)
        body[0x34e] = row.get('self_right_elapsed', 0)
        struct.pack_into('<f', body, 0x84, row.get('up_z', 1.))
        for offset, key in ((0x334, 'unit_current'), (0x338, 'unit_previous')):
            if key in row:
                struct.pack_into('<f', body, offset, row[key])
        if 'query_values' in row:
            for offset, value in zip((0x354, 0x35c, 0x25c, 0x260, 0x358), row['query_values']):
                struct.pack_into('<f', body, offset, value)
        if 'decay_values' in row:
            for offset, value in zip((0x378, 0x354, 0x35c, 0x358, 0x370, 0x374), row['decay_values']):
                struct.pack_into('<f', body, offset, value)
        body[0x480:0x580] = bytes(0x100)
        if not row.get('animation_valid'):
            struct.pack_into('<I', body, 0x480 + 0x68, 0xffffffff)
        for offset, value in row.get('animation_overrides', {}).items():
            width = 2 if offset in (6, 0x46) else 4 if offset in (0, 0x40, 0x68) else 1
            body[0x480 + offset:0x480 + offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
        g.mem_write(BODY, bytes(body))
        parent_body = bytearray(body)
        struct.pack_into('<I', parent_body, 0x14, UNIT if row.get('child') else 0xffffffff)
        struct.pack_into('<I', parent_body, 0x10, 0xffffffff)
        g.mem_write(PARENT_BODY, bytes(parent_body))
        g.mem_write(DEFINITION, bytes.fromhex(records[row['definition']]['prefix_hex']))
        if 'scalar_rates' in row:
            g.mem_write(DEFINITION + 0x22c, struct.pack('<4f', *row['scalar_rates']))
        if row.get('unit_rates'):
            for offset, value in zip((0x1d4, 0x1dc), row['unit_rates']):
                g.mem_write(DEFINITION + offset, struct.pack('<f', value))
        if 'definition_flags_1ec' in row:
            w(DEFINITION + 0x1ec, row['definition_flags_1ec'])
        g.mem_write(STACK - 0x4000, rng.randbytes(0x5000))
        g.mem_write(OUTPUT, rng.randbytes(0x100))
        g.mem_write(STATE_AREA, rng.randbytes(0x2000))
        g.mem_write(ACTUATOR, rng.randbytes(0x100))
        g.mem_write(ACTUATOR_DATA, rng.randbytes(0x100))
        w(ACTUATOR + 0x4c, row.get('actuator_count', 0))
        w(ACTUATOR + 0x50, ACTUATOR_DATA)
        g.mem_write(ACTUATOR_DATA + 0x3c, bytes((row.get('actuator_flag', 0),)))
        w(STACK, STOP)
        w(STACK + 4, UNIT)
        w(STACK + 8, OUTPUT)
        if row['entry'] == 0xf02d0:
            w(STACK + 4, OUTPUT)
        if row.get('scalar_control'):
            w(STACK + 4, struct.unpack('<I', struct.pack('<f', row['direction']))[0])
            w(STACK + 8, struct.unpack('<I', struct.pack('<f', row['dt']))[0])
        for offset, value in row.get('stack_values', {}).items():
            w(STACK + int(offset), value)
        g.mem_write(CLOCK, struct.pack('<I', 30 << 16))
        w(CLOCK + 4, struct.unpack('<I', struct.pack('<f', row['dt']))[0])
        w(CLOCK + 8, 123)
        for n, name in enumerate(('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')):
            value = rng.getrandbits(32)
            if name == 'EAX':
                value = row.get('allocation_bytes', UNIT)
            elif name == 'ESP':
                value = STACK
            elif row.get('scalar_control') and name == 'ECX':
                value = BODY + 0x358
            elif row.get('scalar_control') and name == 'EDX':
                value = DEFINITION + 0x22c
            value = row.get('registers', {}).get(name, value)
            g.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
        for n in range(8):
            g.reg_write(getattr(R, f'UC_X86_REG_XMM{n}'), rng.getrandbits(128))
        if row.get('scalar_control'):
            g.reg_write(R.UC_X86_REG_XMM3, struct.unpack('<I', struct.pack('<f', row['target']))[0])
        g.reg_write(R.UC_X86_REG_EFLAGS, 2)
        g.reg_write(R.UC_X86_REG_FPCW, row['fp_control'])
        g.reg_write(R.UC_X86_REG_MXCSR, row['mxcsr'])
        g.reg_write(R.UC_X86_REG_FPSW, row['top'] << 11)
        tag = 0xffff
        values = ((0x8000000000000000, 0x3fff), (0x8000000000000000, 0xbffe),
                  (0x8000000000000000, 0x4000))
        for n in range(row['depth']):
            physical = (row['top'] + n) & 7
            tag &= ~(3 << (physical * 2))
            g.reg_write(getattr(R, f'UC_X86_REG_FP{physical}'), values[n])
        g.reg_write(R.UC_X86_REG_FPTAG, tag)
        state.update(events=[], instructions=[], exit_entry=None)
        initial_abi = capture(g)
        try:
            g.emu_start(address(row['entry']), 0, count=10000)
        except Exception as error:
            raise RuntimeError(dict(case=index, input=row, eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
        assert state['exit_entry'] is not None, dict(case=index, input=row,
            eip=hex(g.reg_read(R.UC_X86_REG_EIP)), last_sites=state['instructions'][-16:])
        complete = state['exit_entry'] == STOP
        if row['entry'] in (0xf06b0, 0xf5930, 0x208d80, 0x208e70, 0x326af0, 0xd26c0, 0xf2960, 0xf1270, 0xf11a0, 0xf1d00, 0x208ed0):
            assert complete
        first = {}
        current = {}
        for event in state['events']:
            p = event['address']
            raw = bytes.fromhex(event['raw'] if event['kind'] == 'R' else event['before'])
            for n, value in enumerate(raw):
                if p + n not in current:
                    first[p + n] = value
                    current[p + n] = value
                assert current[p + n] == value
            if event['kind'] == 'W':
                for n, value in enumerate(bytes.fromhex(event['written'])):
                    current[p + n] = value
        cases.append(dict(id=index, entry=row['entry'], entry_abi=initial_abi, exit=capture(g),
            exit_entry=state['exit_entry'], return_address=STOP, complete=complete,
            initial_banks=[dict(address=p, hex=b.hex()) for p, b in segments(first)],
            final_banks=[dict(address=p, hex=b.hex()) for p, b in segments(current)],
            instructions=state['instructions'], writes=[e for e in state['events'] if e['kind'] == 'W'],
            estimates=[], timestamps=[], packets=[], declared_input=declared_json(row)))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--correspondence', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    paired = json.loads(args.correspondence.read_text())
    assert sha(ROOT / 'tools/oracle_vehicle_runtime_20261011.py') == 'ac008f33905cff72941f4b789ea173f18e166c07bffa3be8b00430307a40d412'
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_vehicle_update_20261011.py')
    records = definitions()
    rows = controls(records)
    args.output.mkdir(parents=True, exist_ok=False)
    stats = {}
    for name, latest, path in (('disc', False, ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
                               ('u5', True, NEW_DEFAULT)):
        cases = execute(path, paired, latest, records, rows)
        fixture = dict(schema=1, image='1.5' if latest else '1.1', cases=cases,
            tool_sha256=sha(__file__), correspondence_sha256=sha(args.correspondence),
            definitions=records, instruction_patches=[], native_expected_data_used=False,
            live_vehicle_driving_proof=False)
        fixture_path = args.output / f'{name}.json'
        fixture_path.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':'), allow_nan=False) + '\n')
        seen = set(p for case in cases for p in case['instructions'])
        stats[name] = dict(fixture_sha256=sha(fixture_path), cases=len(cases),
            complete=sum(case['complete'] for case in cases), instructions=sum(len(case['instructions']) for case in cases),
            ordered_writes=sum(len(case['writes']) for case in cases), covered_instructions=len(seen),
            full_state_bytes=sum(len(bytes.fromhex(bank['hex'])) for case in cases for bank in case['final_banks']),
            stop_entries=sorted({case['exit_entry'] for case in cases if not case['complete']}))
    report = dict(schema=1, tool_sha256=sha(__file__), correspondence_sha256=sha(args.correspondence),
        parent_controls_tool_sha256=sha(ROOT / 'tools/oracle_vehicle_runtime_20261011.py'),
        reader_sha256=sha(ROOT / 'tools/inspect_multiplayer.py'),
        segments_helper_sha256=sha(ROOT / 'tools/export_mp_physics_contact_lifecycle.py'),
        fixtures=stats, declared_inputs=len(rows), authored_definitions=len(records),
        instruction_patches=[], native_expected_data_used=False, live_vehicle_driving_proof=False,
        boundary=__doc__)
    (args.output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
