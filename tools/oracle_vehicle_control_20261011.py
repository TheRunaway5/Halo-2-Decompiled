#!/usr/bin/env python3
"""Execute both original vehicle rate routines without instruction patches.

Inputs are declared scalar controls, not captured driving scenarios. Both images
execute their real complete routines and the target routine's actual callee.
The fixture compares the output bank and all ordered writes to that bank; caller
stack, GP/SSE flags and x87-stack state are outside this semantic kernel contract.
"""
import argparse
import hashlib
import itertools
import json
import random
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE = 0x1000000
STACK = BASE + 0x8000
STOP = BASE + 0xf000


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def word(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def inputs():
    rng = random.Random(0x208d80)
    samples = []
    # Exact zero, turn-through-zero, equal limits, overshoot and clamp branches.
    for initial, direction, dt, target in itertools.product(
            (-10., -1., -0.25, -0., 0., 0.25, 1., 10.),
            (-1., -0., 0., 0.25, 1.), (0., 1. / 30., 0.25, 1.),
            (-1., 0., 1.)):
        samples.append(dict(value=word(initial), direction=word(direction), dt=word(dt),
                            target=word(target), rates=list(map(word, (4., 3., 2., 5.)))))
    # Exercise arbitrary signs, finite exponent ranges, subnormals and signed
    # zero without a mathematical normalizer replacing the original behavior.
    for _ in range(700):
        samples.append(dict(value=word(rng.uniform(-20, 20)),
            direction=word(rng.uniform(-2, 2)), dt=word(rng.uniform(-0.5, 1)),
            target=word(rng.uniform(-5, 5)),
            rates=[word(rng.uniform(-10, 10)) for _ in range(4)]))
    edges = (0, 0x80000000, 1, 0x80000001, 0x007fffff, 0x00800000,
             0x3f800000, 0xbf800000, 0x7f7fffff, 0xff7fffff,
             0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000)
    for field in ('value', 'direction', 'dt', 'target', 'rates'):
        for edge in edges:
            for sign in (-1, 1):
                row = dict(value=word(sign * 0.25), direction=word(sign), dt=word(1. / 30.),
                           target=0, rates=list(map(word, (4., 3., 2., 5.))))
                if field == 'rates':
                    for offset in range(4):
                        clone = {**row, 'rates': list(row['rates'])}
                        clone['rates'][offset] = edge
                        samples.append(clone)
                else:
                    row[field] = edge
                    samples.append(row)
    rows = []
    for index, row in enumerate(samples):
        bank = bytearray(rng.randbytes(64))
        struct.pack_into('<4I', bank, 32, *row.pop('rates'))
        offset = (16, 32, 36, 40, 44)[index % 5]
        struct.pack_into('<I', bank, offset, row.pop('value'))
        rows.append({**row, 'value_address': BASE + offset,
                     'rates_address': BASE + 32, 'initial_hex': bank.hex()})
    return rows


def execute(path, pin, mapping, samples):
    assert sha(path) == pin
    image = Xbe(path)
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    g.mem_map(BASE, 0x10000)
    known = {later: int(old, 16) for row in mapping['routines'] if row['native_implemented']
             for old, later in row['instruction_map'].items()}
    entries = {row['entry']: row['later_entry'] for row in mapping['routines']}
    state = {}

    def code(_g, address, _size, _user):
        if address not in known:
            raise AssertionError(('Unimplemented original dependency', hex(address)))
        state['instructions'].add(known[address])

    def store(_g, _access, address, size, value, _user):
        if BASE <= address and address + size <= BASE + 64:
            state['stores'].append(dict(instruction=known[g.reg_read(R.UC_X86_REG_EIP)],
                address=address, before=bytes(g.mem_read(address, size)).hex(),
                written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))

    g.hook_add(UC_HOOK_CODE, code)
    g.hook_add(UC_HOOK_MEM_WRITE, store)
    outputs = []
    coverage = set()
    for kernel, (precision, rounding), (index, row) in itertools.product(
            ('accelerate', 'approach'), itertools.product((0, 0x200, 0x300), range(4)), enumerate(samples)):
        fp = dict(x87_control_word=0x7f | precision | (rounding << 10),
                  mxcsr=0x1f80 | (rounding << 13))
        state.update(stores=[], instructions=set())
        g.mem_write(BASE, bytes.fromhex(row['initial_hex']))
        g.mem_write(STACK - 64, bytes(128))
        g.mem_write(STACK, struct.pack('<3I', STOP, row['direction'], row['dt']))
        for name in ('EAX', 'ECX', 'EDX', 'EBX', 'EBP', 'ESI', 'EDI'):
            g.reg_write(getattr(R, 'UC_X86_REG_' + name), 0)
        for n in range(8):
            g.reg_write(getattr(R, f'UC_X86_REG_XMM{n}'), 0)
        g.reg_write(R.UC_X86_REG_XMM3, row['target'])
        g.reg_write(R.UC_X86_REG_ECX, row['value_address'])
        g.reg_write(R.UC_X86_REG_EDX, row['rates_address'])
        g.reg_write(R.UC_X86_REG_ESP, STACK)
        g.reg_write(R.UC_X86_REG_EFLAGS, 2)
        g.reg_write(R.UC_X86_REG_FPCW, fp['x87_control_word'])
        g.reg_write(R.UC_X86_REG_MXCSR, fp['mxcsr'])
        g.reg_write(R.UC_X86_REG_FPSW, 0)
        g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        entry = entries[0x208d80 if kernel == 'accelerate' else 0x208e70]
        g.emu_start(entry, STOP, count=200)
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        assert g.reg_read(R.UC_X86_REG_ESP) == STACK + 12
        assert g.reg_read(R.UC_X86_REG_FPTAG) == 0xffff
        coverage.update(state['instructions'])
        outputs.append(dict(kernel=kernel, fp=fp, input_index=index,
            final_hex=bytes(g.mem_read(BASE, 64)).hex(), stores=state['stores'],
            result=(g.reg_read(R.UC_X86_REG_EAX) & 255) if kernel == 'approach' else None))
    return outputs, coverage


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--correspondence', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    paired = json.loads(args.correspondence.read_text())
    assert paired['tool_sha256'] == sha(ROOT / 'tools/research_vehicle_control_20261011.py')
    assert paired['native_entries'] == [0x208d80, 0x208e70]
    samples = inputs()
    old_mapping = json.loads(json.dumps(paired))
    for row in old_mapping['routines']:
        row['later_entry'] = row['entry']
        row['instruction_map'] = {p: int(p, 16) for p in row['instruction_map']}
    original, coverage = execute(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
                                  PIN, old_mapping, samples)
    later, later_coverage = execute(NEW_DEFAULT, NEW_PIN, paired, samples)
    assert original == later and coverage == later_coverage
    expected = {int(p, 16) for row in paired['routines'] if row['native_implemented']
                for p in row['instruction_map']}
    assert coverage == expected, ('Original branches unexercised', sorted(expected - coverage))
    args.output.mkdir(parents=True, exist_ok=False)
    fixture = dict(schema=1, inputs=samples, outputs=original)
    path = args.output / 'fixture.json'
    path.write_text(json.dumps(fixture, sort_keys=True, separators=(',', ':')) + '\n')
    report = dict(schema=1, tool_sha256=sha(__file__), correspondence_sha256=sha(args.correspondence),
        fixture_sha256=sha(path), original_sha256=PIN, later_original_sha256=NEW_PIN,
        cases_per_image=len(original), covered_native_instructions=len(coverage),
        ordered_stores_per_image=sum(len(row['stores']) for row in original),
        full_output_bytes_per_image=len(original) * 64, samples=len(samples),
        source_instruction_patches=[], native_expected_data_used=False,
        actual_vehicle_scenarios=False, boundary=__doc__)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
