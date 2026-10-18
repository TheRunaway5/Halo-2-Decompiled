#!/usr/bin/env python3
"""Probe the closed original listener selection graph against its native port.

Use actual captured entry RAM/CPU. Explicit byte and register fixtures exercise
local-slot selection, distance, range and cached/uncached pair-byte paths. A
declared pointer-rebinding control is separate from actual captured input.
These controls are not captured gameplay or supplied outcomes.
Run through tools/run_test.py; all native/RAM products remain in owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import copy
from functools import lru_cache
from capstone import x86_const as X
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE, UC_PROT_ALL
from unicorn import x86_const as R
from original_xbox_pose_case import BASE, ROOT, cpu, reconstructed_replay_ram, sha


@lru_cache(maxsize=None)
def instruction_flags(pc):
    from oracle_smg_firing_1_1 import Xbe, _Code
    from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
    if not hasattr(instruction_flags, 'code'):
        assert sha(NEW_DEFAULT) == NEW_PIN
        instruction_flags.code = _Code(Xbe(NEW_DEFAULT), '.text')
    return instruction_flags.code.at(pc).eflags


def defined_flag_mask(trace):
    bits = {'CF': 1, 'PF': 4, 'AF': 16, 'ZF': 64, 'SF': 128, 'DF': 1024, 'OF': 2048}
    defined = {name: True for name in bits}
    for pc in trace:
        flags = instruction_flags(pc)
        for name in bits:
            if flags & getattr(X, 'X86_EFLAGS_UNDEFINED_' + name, 0):
                defined[name] = False
            elif any(flags & getattr(X, 'X86_EFLAGS_' + action + '_' + name, 0) for action in ('MODIFY', 'RESET', 'SET')):
                defined[name] = True
    return sum(bit for name, bit in bits.items() if defined[name])


def make_driver(scratch):
    return ROOT / 'tests/source_sound_listener_selection_tu15_tests.cpp'


def listener_cases(memory, initial):
    word = lambda at: struct.unpack('<I', memory.read(at, 4))[0]
    sp = initial['gp'][4]
    request = word(sp + 4)
    sound = word(0x4f2c50)
    local = word(0x4f54f0)
    bsp = word(0x4ecc18)
    assert word(bsp + 0x9c) == 16 and word(bsp + 0x224) == 1 and word(bsp + 0xe8) == 0
    cached = word(bsp + 0x228)
    table = word(cached + 0x28)
    view = sound + 0x88
    reference_view = memory.read(view, 72)
    position = struct.unpack('<3f', reference_view[0x30:0x3c])
    source_handle = memory.read(local + 0xc, 4)
    source_guid = word(view)
    flags = struct.unpack('<H', memory.read(request, 2))[0]
    high_type = memory.read(request + 3, 1)[0] & 0xf0
    cases = []

    def add(name, patches=(), entry=0x12a810, gp=None, expected_fault=False):
        state = copy.deepcopy(initial)
        if gp:
            for index, value in gp.items():
                state['gp'][index] = value & 0xffffffff
        cases.append(dict(name=name, entry=entry, cpu=state, expected_fault=expected_fault,
            declared_input_bytes=[dict(address=at, hex=raw.hex()) for at, raw in patches]))

    def occupied(slot):
        return [(local + 0xc + i*4, source_handle if i == slot else bytes.fromhex('ffffffff')) for i in range(4)]

    def spatial(active=(0,), at=None, mode=0, extra_flags=1, limit=45., height=2.):
        changes = [(view + i*72, reference_view) for i in range(4)]
        changes += [(view + i*72 + 6, bytes([int(i in active)])) for i in range(4)]
        changes += [(request, struct.pack('<H', flags | extra_flags)),
            (request + 2, bytes([mode])), (request + 3, bytes([high_type | 1])),
            (request + 0xc, struct.pack('<3f', *(position if at is None else at))),
            (request + 0x30, struct.pack('<f', height)), (sp + 8, struct.pack('<f', limit))]
        return changes

    add('original captured entry')
    for slot in (-1, 0, 1, 2, 3):
        add(f'nonpositional first occupied slot {slot}', occupied(slot))
    for slot in range(4):
        add(f'positional single active slot {slot}', spatial((slot,)))
    add('positional four equal distances', spatial((0, 1, 2, 3)))
    offsets = [(view + i*72 + 0x30, struct.pack('<f', position[0] + i*4)) for i in range(4)]
    add('positional closest fourth slot', spatial((0, 1, 2, 3), (position[0]+12, position[1], position[2])) + offsets)
    add('positional no active views', spatial(()))
    add('positional no active views and bit8 fallback', spatial((), extra_flags=0x101))
    add('positional no active views and bit11 fallback', spatial((), extra_flags=0x801))
    add('positional range rejection', spatial(at=(position[0]+128, position[1], position[2]), limit=2.))
    for z in (-3., -1., 0., 1., 3.):
        add(f'cylindrical height input {z}', spatial(at=(position[0]+1, position[1]+2, position[2]+z), mode=1))
    for kind, vector in ((0, (0.,0.,0.)), (2, (3.,4.,0.)), (2, (1.,1.,1.)), (15, (3.,4.,0.))):
        add(f'distance leaf type {kind}, vector {vector}',
            [(request + 3, bytes([high_type | kind])), (request + 0xc, struct.pack('<3f', *vector))],
            entry=0x12a930, gp={1: request, 2: 0})
    for a, b in ((0,0), (15,15), (0,1), (1,0), (0,15), (15,0)):
        add(f'cached pair lookup {a},{b}', entry=0x253070, gp={0:a, 1:b, 2:bsp})
    for value in (0, 1, 127, 254, 255):
        add(f'cached pair byte {value}', [(table, bytes([value]))], entry=0x253070, gp={0:0, 1:1, 2:bsp})
    # The original uncached pointer is absent. A separate declared control
    # binds it to the already owned original pair-byte array; this is not an
    # observed uncached gameplay bank or a production resource binding.
    uncached = [(bsp + 0x224, bytes(4)), (bsp + 0xe8, struct.pack('<I', table))]
    for value in (0, 127, 128, 255):
        add(f'declared uncached pointer rebind, byte {value}', uncached + [(table, bytes([value]))],
            entry=0x253070, gp={0:0, 1:1, 2:bsp})
    for a, b in ((0,0), (0,1), (1,0)):
        add(f'uncached leaf register inputs {a},{b}', uncached, entry=0x2531e0, gp={0:a, 1:b, 7:bsp})
    add('original absent uncached pointer faults', [(bsp + 0x224, bytes(4))],
        entry=0x253070, gp={0:0, 1:1, 2:bsp}, expected_fault=True)
    gate = [(request, struct.pack('<H', flags & ~1)), (request + 3, bytes([high_type | 1])),
        (request + 0x28, struct.pack('<I', source_guid ^ 1)), (request + 0x2c, struct.pack('<h', 0)),
        (view + 4, struct.pack('<h', 1)), (sp + 4, struct.pack('<f', 1.))]
    add('gate no listener slot', gate, entry=0x2287b0, gp={1:0xffffffff, 2:request})
    add('gate request bit0 bypass', gate + [(request, struct.pack('<H', flags | 1))], entry=0x2287b0, gp={1:0,2:request})
    add('gate nonpositional type bypass', gate + [(request+3, bytes([high_type|2]))], entry=0x2287b0, gp={1:0,2:request})
    add('gate absent request cluster', gate + [(request+0x2c, bytes.fromhex('ffff'))], entry=0x2287b0, gp={1:0,2:request})
    add('gate absent listener cluster', gate + [(view+4, bytes.fromhex('ffff'))], entry=0x2287b0, gp={1:0,2:request})
    add('gate same owner', gate + [(request+0x28, struct.pack('<I', source_guid))], entry=0x2287b0, gp={1:0,2:request})
    add('gate same cluster', gate + [(view+4, bytes(2))], entry=0x2287b0, gp={1:0,2:request})
    for value in (0, 255):
        add(f'gate actual pair path, byte {value}', gate + [(table, bytes([value]))], entry=0x2287b0, gp={1:0,2:request})
    occlusion = spatial(at=(position[0]+2,position[1],position[2]), extra_flags=0x100, limit=.5)
    occlusion += [(request+0x28, struct.pack('<I', source_guid^1)), (request+0x2c, bytes(2)),
                  (view+4, struct.pack('<h', 1)), (table, bytes([255]))]
    add('root rejected gate and occupied fallback', occlusion + occupied(0))
    add('root rejected gate and no occupied fallback', occlusion + occupied(-1))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--generated-source', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute() if args.output else scratch / 'listener-result'
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    case = BASE / 'smg-original-fire-v1'
    prefix = BASE / 'smg-original-listener-entry-v1'
    if not (prefix / 'report.json').exists():
        return 77
    prefix_report = json.loads((prefix / 'report.json').read_text())
    assert not prefix_report['passed'] and prefix_report['declared_prefix_stop_reached'] and prefix_report['declared_prefix_stop'] == 0x12a810
    generated = args.generated_source.absolute() if args.generated_source else scratch / 'listener.cpp'
    regenerated = scratch / 'regenerated-listener.cpp'
    generator_command = [str(Path(os.sys.executable).absolute()), str(ROOT / 'tools/generate_sound_listener_selection_tu15.py'),
        '--output', str(regenerated), '--manifest', str(scratch / 'generation.json')]
    generation = subprocess.run(generator_command, capture_output=True, text=True)
    (output / 'generation.log').write_text((generation.stdout + generation.stderr)[-65536:])
    assert generation.returncode == 0, generation.stderr[-3000:]
    if args.generated_source:
        assert generated.read_bytes() == regenerated.read_bytes(), 'Production source differs from authenticated generation'
    else:
        shutil.copyfile(regenerated, generated)
    for name in ('generation.json', 'listener-bodies.json', 'listener-callers.json'):
        shutil.copyfile(scratch / name, output / name)
    initial = cpu(case, 'before')
    initial.update(prefix_report['actual_CPU'])
    registers = [getattr(R, 'UC_X86_REG_' + name) for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
    fixtures = [dict(name='original captured entry', declared_input_bytes=[])]
    originals = []
    covered = set()
    with reconstructed_replay_ram(case, prefix) as (bank, memory, replay):
        entry_bytes = bank[:]
        fixtures = listener_cases(memory, initial)
        for fixture_input in fixtures:
            bank[:] = entry_bytes
            for patch in fixture_input['declared_input_bytes']:
                raw = bytes.fromhex(patch['hex'])
                assert (patch['address'] & 4095) + len(raw) <= 4096
                at = memory.translate(patch['address'])[0]
                bank[at:at+len(raw)] = raw
            machine = Uc(UC_ARCH_X86, UC_MODE_32)
            mapped = set()

            def map_page(address):
                page = address & ~4095
                if page not in mapped:
                    raw = memory.read(page, 4096)
                    machine.mem_map(page, 4096, UC_PROT_ALL)
                    machine.mem_write(page, raw)
                    mapped.add(page)

            def invalid(_machine, access, address, size, value, user):
                map_page(address)
                return True

            trace = []
            writes = []

            def write_observation(uc, access, address, size, value, user):
                writes.append(dict(instruction=uc.reg_read(R.UC_X86_REG_EIP), address=address,
                    before=bytes(uc.mem_read(address, size)).hex(), written=value.to_bytes(size, 'little').hex()))

            machine.hook_add(UC_HOOK_MEM_INVALID, invalid)
            machine.hook_add(UC_HOOK_CODE, lambda uc, address, size, user: trace.append(address))
            machine.hook_add(UC_HOOK_MEM_WRITE, write_observation)
            abi = fixture_input['cpu']
            for register, value in zip(registers, abi['gp']):
                machine.reg_write(register, value)
            for index, words in enumerate(abi['xmm']):
                machine.reg_write(getattr(R, f'UC_X86_REG_XMM{index}'), int.from_bytes(struct.pack('<4I', *words), 'little'))
            machine.reg_write(R.UC_X86_REG_EFLAGS, abi['eflags'])
            top = (abi['x87_status'] >> 11) & 7
            for index, raw in enumerate(abi['x87_register_bytes']):
                machine.reg_write(R.UC_X86_REG_FP0 + ((index + top) & 7), struct.unpack('<QH', bytes.fromhex(raw)))
            for name, key in (('FPCW','x87_control'), ('FPSW','x87_status'), ('FPTAG','x87_tag'), ('MXCSR','mxcsr')):
                machine.reg_write(getattr(R, 'UC_X86_REG_' + name), abi[key])
            map_page(fixture_input['entry'])
            map_page(initial['gp'][4])
            map_page(initial['gp'][4] - 16)
            end = struct.unpack('<I', memory.read(initial['gp'][4], 4))[0]
            assert end == 0x129861
            error = None
            try:
                machine.emu_start(fixture_input['entry'], end, count=1000)
            except Exception as failure:
                error = str(failure)
            returned = machine.reg_read(R.UC_X86_REG_EIP) == end and error is None
            assert returned or (fixture_input.get('expected_fault') and trace[-1] == 0x253213), (fixture_input['name'], trace[-1], error)
            covered.update(trace)
            mask = defined_flag_mask(trace)
            originals.append(dict(name=fixture_input['name'], returned=returned, error=error,
                gp=[machine.reg_read(reg) for reg in registers], trace=trace, writes=writes,
                eflags=machine.reg_read(R.UC_X86_REG_EFLAGS), defined_flags_mask=mask,
                x87_status=machine.reg_read(R.UC_X86_REG_FPSW), x87_control=machine.reg_read(R.UC_X86_REG_FPCW),
                x87_tag=machine.reg_read(R.UC_X86_REG_FPTAG), mxcsr=machine.reg_read(R.UC_X86_REG_MXCSR),
                mapped_pages=sorted(mapped),
                xmm=[list(struct.unpack('<4I', machine.reg_read(getattr(R, f'UC_X86_REG_XMM{index}')).to_bytes(16, 'little'))) for index in range(8)]))
        (scratch / 'entry-ram.bin').write_bytes(entry_bytes)
    (scratch / 'fixtures.json').write_text(json.dumps(dict(cpu=initial, fixtures=fixtures)) + '\n')
    shutil.copyfile(scratch / 'fixtures.json', output / 'declared-inputs.json')
    source = make_driver(scratch)
    header = ROOT / 'include/halo2/source_sound_listener_selection_tu15.hpp'
    pins = {str(path): sha(path) for path in (source, generated, header)}
    binary = args.binary.absolute() if args.binary else scratch / 'listener'
    command = None
    if not args.binary:
        command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
            '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(source), str(generated), '-Wl,--gc-sections', '-o', str(binary)]
        compiled = subprocess.run(command, capture_output=True, text=True)
        (output / 'compile.log').write_text((compiled.stdout + compiled.stderr)[-65536:])
        assert compiled.returncode == 0, compiled.stderr[-3000:]
    run_command = [str(binary), str(scratch / 'fixtures.json'), str(scratch / 'entry-ram.bin'), str(scratch / 'native.json')]
    run = subprocess.run(run_command, capture_output=True, text=True, timeout=60)
    (output / 'run.log').write_text((run.stdout + run.stderr)[-65536:])
    assert run.returncode == 0, run.stderr[-3000:]
    actual = json.loads((scratch / 'native.json').read_text())
    assert len(actual) == len(originals)
    authority = BASE / 'first-person-original-call-v1-native-evidence-v1.json'
    evidence = json.loads(authority.read_text())
    assert evidence['passed'] and evidence['raw_x87_control']['xor'] == 0x40
    comparisons = []
    for native, original in zip(actual, originals):
        mask = original['defined_flags_mask']
        equal = (native['returned'] == original['returned'] and native['gp'] == original['gp'] and
            native['xmm'] == original['xmm'] and native['trace'] == original['trace'] and native['writes'] == original['writes'] and
            (native['eflags'] & mask) == (original['eflags'] & mask) and
            native['x87_status'] == original['x87_status'] and native['x87_depth'] == 0 and original['x87_tag'] == 65535 and
            ((native['x87_control'] ^ original['x87_control']) & ~0x40) == 0 and native['mxcsr'] == original['mxcsr'])
        comparisons.append(dict(name=original['name'], equal=equal, native=native, original=original,
            raw_original_x87_control=initial['x87_control'], raw_control_xor=native['x87_control'] ^ initial['x87_control']))
    for path, pin in pins.items():
        assert sha(path) == pin
    coverage = len(covered) == 312
    assert sum(not row['returned'] for row in originals) == 1
    result = dict(passed=all(row['equal'] for row in comparisons) and coverage, fixtures=len(fixtures),
        original_unmodified_entry_cases=1, declared_byte_and_ABI_fixture_cases=len(fixtures)-1,
        unique_original_instructions_executed=len(covered), complete_original_bodies=5,
        all_312_original_instructions_covered=coverage,
        successful_original_cases=sum(row['returned'] for row in originals), real_missing_input_fault_cases=1,
        original_full_call_independently_verified=False, production_integration=False, full_weapon_parity=False,
        raw_reserved_control_bit_difference_preserved=True, measured_control_authority_sha256=sha(authority),
        comparisons=comparisons, source_files_sha256=pins,
        original_prefix_sha256=sha(prefix / 'report.json'), original_call_sha256=sha(case / 'call.json'),
        native_generated_source_sha256=sha(generated), native_binary_sha256=sha(binary),
        compile_command=command, generator_command=generator_command, run_command=run_command,
        runner_scratch=str(scratch), large_artifacts_retained=[])
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    for path in (Path(__file__), source, generated, header, ROOT / 'tools/generate_sound_listener_selection_tu15.py'):
        shutil.copyfile(path, output / path.name)
    print(json.dumps({key: result[key] for key in ('passed', 'fixtures', 'unique_original_instructions_executed', 'runner_scratch')}), flush=True)
    assert result['passed']
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
