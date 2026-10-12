#!/usr/bin/env python3
"""Execute untouched original vehicle controllers on declared raw body inputs.

Vehicle definition bytes come from the owned Coagulation, Headlong and Zanzibar
maps. Bodies, velocities, controls and FP perturbations are explicit test inputs,
not a claim of original vehicle construction or live driving. No callee output,
contact, successful owner result or patched instruction is supplied. The larger
update/controller paths stop at the actual next unported owner.
"""
import argparse
import itertools
import json
import random
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from research_vehicle_control_20261011 import sha
from inspect_multiplayer import Cache
from export_mp_physics_contact_lifecycle import segments

BASE = 0x1000000
POOL, HEADERS, BODY, TAGS, DEFINITION, CLOCK = (BASE + n for n in (0, 0x1000, 0x2000, 0x3000, 0x4000, 0x5000))
STACK = BASE + 0xd000
OUTPUT = BASE + 0x6000
STOP = BASE + 0xf000
UNIT = 0xe6f80000


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
    rows = []
    # Different orientations, movement/control directions, zero-speed defaults,
    # turn clamps, held gear, reverse admission and the source long-step flag.
    states = (
        ((0., 0.), (1., 0.), (0., 0., 0.), 0., 1. / 30.),
        ((1., 0.), (1., 0.), (1., 0., 0.), -10., 1. / 30.),
        ((-1., 0.), (1., 0.), (-1., 0., 0.), 10., 0.125),
        ((0., 1.), (1., 0.), (0., 1., 0.), 0., 0.125),
        ((0., -1.), (0., 1.), (0., -1., 0.), -1., 0.09),
        ((1., 1.), (1., 1.), (0.5, -0.5, 0.), 1., 0.),
        ((0.00001, 0.), (0., 0.), (1., 1., 1.), 0., 1.),
        ((0.0001, 0.), (-1., 0.), (0., 0., 0.), 10., 1. / 60.),
        ((1., -2.), (-2., -1.), (-0.75, 0.25, 0.), -10., 0.1),
        ((2., 1.), (1., -2.), (0.25, -0.75, 0.), 10., -0.125),
        ((1., 0.), (1., 0.), (0., 1., 0.), 0., 0.125),
        ((3.e38, 3.e38), (3.e38, 3.e38), (1., 0., 0.), 0., 0.125),
        ((1., 0.), (1., 0.), (-0.2, 0.98, 0.), 0., 0.125),
        ((1., 0.), (1., 0.), (-0.2, -0.98, 0.), 0., 0.125),
    )
    for index, record in enumerate(records):
        for ordinal, (velocity, forward, direction, turn, dt) in enumerate(states):
            rows.append(dict(entry=0xf06b0, definition=index, velocity=velocity,
                forward=forward, controls=direction, turn=turn, dt=dt, gear=ordinal % 3 - 1,
                flags349=(ordinal * 29) & 255, fp_control=0x27f, mxcsr=0x1f80, top=0, depth=0,
                node_position_offset=0x400, declared_control=True))
    for entry, initial, direction, target in itertools.product(
            (0x208d80, 0x208e70), (-10., -1., -0.25, -0., 0., 0.25, 1., 10.),
            (-1., 0., 1.), (-1., 0., 1.)):
        row = dict(rows[0])
        row.update(entry=entry, turn=initial, dt=0.25, direction=direction, target=target,
                   scalar_rates=(4., 3., 2., 5.), scalar_control=True)
        rows.append(row)
    for precision, rounding in itertools.product((0, 0x200, 0x300), range(4)):
        row = dict(rows[8])
        row.update(fp_control=0x7f | precision | (rounding << 10), mxcsr=0x1f80 | (rounding << 13),
                   fp_perturbation=True)
        rows.append(row)
    for top, depth in itertools.product(range(8), (0, 1, 3)):
        row = dict(rows[8])
        row.update(top=top, depth=depth, fp_perturbation=True)
        rows.append(row)
    for entry in (0xf02d0, 0xf0b30, 0xf1420):
        row = dict(rows[0])
        row.update(entry=entry, required_owner_prefix=True)
        rows.append(row)
    for state, ticks, dt in itertools.product(range(5), (0, 1, 255), (0., 1. / 30., 1.)):
        row = dict(rows[0])
        row.update(entry=0xf5930, transient_state=state, transient_ticks=ticks, dt=dt)
        rows.append(row)
    # The vehicle dispatcher selects the original subtype from the authored
    # definition. These are controlled dispatch inputs, not constructed bodies.
    for initial in rows[:len(records) * len(states)]:
        row = dict(initial)
        row.update(entry=0xf0b30, required_owner_prefix=True)
        rows.append(row)
    for amount in (0, 1, 4, 16, 0xfff, 0x1000, 0x1001, 0x1984, 0x2000, 0x3001):
        row = dict(rows[0])
        row.update(entry=0x326af0, allocation_bytes=amount)
        rows.append(row)
    return rows


def execute(path, paired, latest, records, rows):
    assert sha(path) == (NEW_PIN if latest else PIN)
    image = Xbe(path)
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    g.mem_map(BASE, 0x10000)
    selected = {int(p, 16): q for name in ('address_map', 'instruction_map', 'entries')
                for p, q in paired[name].items()}
    sites = ({q: int(p, 16) for p, q in paired['instruction_map'].items()} if latest
             else {int(p, 16): int(p, 16) for p in paired['instruction_map']})

    def w(p, value):
        g.mem_write(p, struct.pack('<I', value & 0xffffffff))

    def address(p):
        return selected[p] if latest else p

    for p, value in ((0x4e78d0, POOL), (0x4eb114, TAGS), (0x518444, CLOCK)):
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
        g.mem_write(BODY, bytes(body))
        g.mem_write(DEFINITION, bytes.fromhex(records[row['definition']]['prefix_hex']))
        if 'scalar_rates' in row:
            g.mem_write(DEFINITION + 0x22c, struct.pack('<4f', *row['scalar_rates']))
        g.mem_write(STACK - 0x4000, rng.randbytes(0x5000))
        g.mem_write(OUTPUT, rng.randbytes(0x100))
        w(STACK, STOP)
        w(STACK + 4, UNIT)
        w(STACK + 8, OUTPUT)
        if row['entry'] == 0xf02d0:
            w(STACK + 4, OUTPUT)
        if row.get('scalar_control'):
            w(STACK + 4, struct.unpack('<I', struct.pack('<f', row['direction']))[0])
            w(STACK + 8, struct.unpack('<I', struct.pack('<f', row['dt']))[0])
        w(CLOCK + 4, struct.unpack('<I', struct.pack('<f', row['dt']))[0])
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
        assert state['exit_entry'] is not None
        complete = state['exit_entry'] == STOP
        if row['entry'] in (0xf06b0, 0xf5930, 0x208d80, 0x208e70, 0x326af0):
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
            estimates=[], timestamps=[], packets=[], declared_input=row))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--correspondence', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    paired = json.loads(args.correspondence.read_text())
    assert paired['extension_tool_sha256'] == sha(ROOT / 'tools/research_vehicle_runtime_20261011.py')
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
        fixture_path.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
        seen = set(p for case in cases for p in case['instructions'])
        stats[name] = dict(fixture_sha256=sha(fixture_path), cases=len(cases),
            complete=sum(case['complete'] for case in cases), instructions=sum(len(case['instructions']) for case in cases),
            ordered_writes=sum(len(case['writes']) for case in cases), covered_instructions=len(seen),
            full_state_bytes=sum(len(bytes.fromhex(bank['hex'])) for case in cases for bank in case['final_banks']),
            stop_entries=sorted({case['exit_entry'] for case in cases if not case['complete']}))
    report = dict(schema=1, tool_sha256=sha(__file__), correspondence_sha256=sha(args.correspondence),
        reader_sha256=sha(ROOT / 'tools/inspect_multiplayer.py'),
        segments_helper_sha256=sha(ROOT / 'tools/export_mp_physics_contact_lifecycle.py'),
        fixtures=stats, declared_inputs=len(rows), authored_definitions=len(records),
        instruction_patches=[], native_expected_data_used=False, live_vehicle_driving_proof=False,
        boundary=__doc__)
    (args.output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
