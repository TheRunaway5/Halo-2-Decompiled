#!/usr/bin/env python3
"""Compare the original127B20/228220 scalar query with its compiled native port.

Use actual captured entry RAM/CPU. Explicit byte fixtures select the three
original scalar paths, signed indices, raw float encodings and the real absent
context fault. These controls are not captured gameplay or supplied outcomes.
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
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE, UC_PROT_ALL
from unicorn import x86_const as R
from original_xbox_pose_case import BASE, ROOT, cpu, reconstructed_replay_ram, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--generated-source', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute() if args.output else scratch / 'scalar-result'
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    case = BASE / 'smg-original-fire-v1'
    prefix = BASE / 'smg-original-sound-scalar-entry-v1'
    if not (prefix / 'report.json').exists():
        return 77
    prefix_report = json.loads((prefix / 'report.json').read_text())
    assert not prefix_report['passed'] and prefix_report['declared_prefix_stop_reached'] and prefix_report['declared_prefix_stop'] == 0x127b20
    generated = args.generated_source.absolute() if args.generated_source else scratch / 'scalar.cpp'
    generator_command = None
    if not args.generated_source:
        generator_command = [str(Path(os.sys.executable).absolute()), str(ROOT / 'tools/generate_sound_scalar_tu15.py'),
            '--output', str(generated), '--manifest', str(scratch / 'generation.json')]
        generation = subprocess.run(generator_command, capture_output=True, text=True)
        (output / 'generation.log').write_text((generation.stdout + generation.stderr)[-65536:])
        assert generation.returncode == 0, generation.stderr[-3000:]
        for name in ('generation.json', 'sound-scalar-bodies.json', 'sound-scalar-callers.json'):
            shutil.copyfile(scratch / name, output / name)
    initial = cpu(case, 'before')
    initial.update(prefix_report['actual_CPU'])
    registers = [getattr(R, 'UC_X86_REG_' + name) for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
    fixtures = [dict(name='original captured entry', declared_input_bytes=[])]
    originals = []
    covered = set()
    with reconstructed_replay_ram(case, prefix) as (bank, memory, replay):
        entry_bytes = bank[:]
        word = lambda at: struct.unpack('<I', memory.read(at, 4))[0]
        tags = word(0x4f0414)
        definition = word(tags + (initial['gp'][0] & 65535) * 16 + 8)
        request = initial['gp'][1]
        gestalt = word(0x52d664)
        parameters = word(gestalt + 4)
        parameter_index = struct.unpack('<h', memory.read(definition + 6, 2))[0]
        context = word(0x4ecc1c)
        context_owner = word(context + 0xc4)
        class_definition = word(tags + (word(context_owner + 4) & 65535) * 16 + 8)
        classes = word(class_definition + 4)
        class_index = struct.unpack('<b', memory.read(definition + 2, 1))[0]
        assert word(context + 0xc0) and parameter_index == 17 and class_index == 4
        request_flags = memory.read(request, 1)[0]
        sound_flags = memory.read(definition + 1, 1)[0]

        def fixture(name, changes):
            fixtures.append(dict(name=name, declared_input_bytes=[dict(address=at, hex=raw.hex()) for at, raw in changes]))

        def select(path):
            return [(request, bytes([(request_flags | 8) if path == 'request' else (request_flags & ~8)])),
                    (definition + 1, bytes([(sound_flags | 8) if path == 'class' else (sound_flags & ~8)]))]

        bits = [0, 0x80000000, 0x3f800000, 0x7fc12345, 0x7f812345, 0x7f800000]
        for path, address in [('request', request + 0x38), ('class', classes + class_index * 92 + 0x1c),
                              ('parameter', parameters + parameter_index * 56 + 4)]:
            for value in bits:
                fixture(f'{path} scalar bits {value:08x}', select(path) + [(address, struct.pack('<I', value))])
        for index in (-128, -1, 0, 4, 53, 127):
            fixture(f'declared signed class index {index}', select('class') + [(definition + 2, struct.pack('<b', index))])
        for index in (-1, 0, 17, 111):
            fixture(f'declared signed parameter index {index}', select('parameter') + [(definition + 6, struct.pack('<h', index))])
        fixture('missing class context preserves source fault', select('class') + [(context + 0xc0, bytes(4))])
        fixture('request selection does not read missing class context', select('request') + [(context + 0xc0, bytes(4))])
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
            for register, value in zip(registers, initial['gp']):
                machine.reg_write(register, value)
            for index, words in enumerate(initial['xmm']):
                machine.reg_write(getattr(R, f'UC_X86_REG_XMM{index}'), int.from_bytes(struct.pack('<4I', *words), 'little'))
            machine.reg_write(R.UC_X86_REG_EFLAGS, initial['eflags'])
            map_page(0x127b20)
            map_page(initial['gp'][4])
            map_page(initial['gp'][4] - 16)
            end = struct.unpack('<I', memory.read(initial['gp'][4], 4))[0]
            assert end == 0x12984b
            error = None
            try:
                machine.emu_start(0x127b20, end, count=80)
            except Exception as failure:
                error = str(failure)
            returned = machine.reg_read(R.UC_X86_REG_EIP) == end and error is None
            assert returned or trace[-1] == 0x228239
            covered.update(trace)
            # AF is undefined after TEST/XOR. IMUL defines only CF/OF among
            # the arithmetic flags; preserve both raw flag words in evidence.
            mask = 0xcc5 if trace[-1] in (0x127b2f, 0x228239) else 0xc01 if trace[-1] == 0x127b6f else 0xcd5
            originals.append(dict(name=fixture_input['name'], returned=returned, error=error,
                gp=[machine.reg_read(reg) for reg in registers], trace=trace, writes=writes,
                eflags=machine.reg_read(R.UC_X86_REG_EFLAGS), defined_flags_mask=mask,
                xmm=[list(struct.unpack('<4I', machine.reg_read(getattr(R, f'UC_X86_REG_XMM{index}')).to_bytes(16, 'little'))) for index in range(8)]))
        (scratch / 'entry-ram.bin').write_bytes(entry_bytes)
    (scratch / 'fixtures.json').write_text(json.dumps(dict(cpu=initial, fixtures=fixtures)) + '\n')
    source = ROOT / 'tests/source_sound_scalar_tu15_tests.cpp'
    header = ROOT / 'include/halo2/source_sound_scalar_tu15.hpp'
    pins = {str(path): sha(path) for path in (source, header, generated)}
    binary = args.binary.absolute() if args.binary else scratch / 'scalar'
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
            native['x87_status'] == initial['x87_status'] and ((native['x87_control'] ^ initial['x87_control']) & ~0x40) == 0 and
            native['mxcsr'] == initial['mxcsr'])
        comparisons.append(dict(name=original['name'], equal=equal, native=native, original=original,
            raw_original_x87_control=initial['x87_control'], raw_control_xor=native['x87_control'] ^ initial['x87_control']))
    for path, pin in pins.items():
        assert sha(path) == pin
    assert len(covered) == 40
    assert sum(not row['returned'] for row in originals) == 1
    result = dict(passed=all(row['equal'] for row in comparisons), fixtures=len(fixtures),
        original_unmodified_entry_cases=1, explicitly_declared_byte_fixture_cases=len(fixtures)-1,
        unique_original_instructions_executed=len(covered), complete_original_bodies=2,
        successful_original_cases=sum(row['returned'] for row in originals), real_missing_context_fault_cases=1,
        raw_float_patterns=[f'{value:08x}' for value in bits], defined_flags_only_compared=True,
        original_full_call_independently_verified=False, full_weapon_parity=False,
        raw_reserved_control_bit_difference_preserved=True, measured_control_authority_sha256=sha(authority),
        comparisons=comparisons, source_files_sha256=pins,
        original_prefix_sha256=sha(prefix / 'report.json'), original_call_sha256=sha(case / 'call.json'),
        native_generated_source_sha256=sha(generated), native_binary_sha256=sha(binary),
        compile_command=command, generator_command=generator_command, run_command=run_command,
        runner_scratch=str(scratch), large_artifacts_retained=[])
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    for path in (Path(__file__), source, header, generated, ROOT / 'tools/generate_sound_scalar_tu15.py'):
        shutil.copyfile(path, output / path.name)
    print(json.dumps({key: result[key] for key in ('passed', 'fixtures', 'unique_original_instructions_executed', 'runner_scratch')}), flush=True)
    assert result['passed']
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
