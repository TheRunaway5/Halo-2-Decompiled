#!/usr/bin/env python3
"""Differential check of unchanged1296E0 and its directly compiled native port.

The original entry is independently reconstructed. Additional branch fixtures
alter only explicitly listed input bytes in that bank; they are not original
gameplay observations and supply no expected return to either execution.
"""
import argparse
import hashlib
import itertools
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
    output = args.output.absolute() if args.output else scratch / 'query-result'
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    case = BASE / 'smg-original-fire-v1'
    prefix = BASE / 'smg-original-sound-query-entry-v1'
    if not (prefix / 'report.json').exists():
        return 77
    report = json.loads((prefix / 'report.json').read_text())
    assert not report['passed'] and report['declared_prefix_stop_reached'] and report['declared_prefix_stop'] == 0x1296e0
    generated = args.generated_source.absolute() if args.generated_source else scratch / 'query.cpp'
    generator_command = None
    if not args.generated_source:
        generator_command = [str(Path(os.sys.executable).absolute()), str(ROOT / 'tools/generate_sound_cache_query_tu15.py'),
            '--output', str(generated), '--manifest', str(scratch / 'generation.json')]
        generation = subprocess.run(generator_command, capture_output=True, text=True)
        (output / 'generation.log').write_text((generation.stdout + generation.stderr)[-65536:])
        assert generation.returncode == 0, generation.stderr[-3000:]
        for name in ('generation.json', 'sound-query-body.json', 'sound-query-caller.json'):
            shutil.copyfile(scratch / name, output / name)
    initial = cpu(case, 'before')
    initial.update(report['actual_CPU'])
    gp_registers = [getattr(R, 'UC_X86_REG_' + name) for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
    fixtures = [dict(name='original captured entry', declared_input_bytes=[])]
    expected = []
    covered = set()
    with reconstructed_replay_ram(case, prefix) as (bank, memory, replay):
        entry_bytes = bank[:]
        tags = struct.unpack('<I', memory.read(0x4f0414, 4))[0]
        definition = struct.unpack('<I', memory.read(tags + (initial['gp'][0] & 65535) * 16 + 8, 4))[0]
        pitch = struct.unpack('<h', memory.read(definition + 8, 2))[0]
        gestalt = struct.unpack('<I', memory.read(0x52d664, 4))[0]
        pitch_rows = struct.unpack('<I', memory.read(gestalt + 0x24, 4))[0]
        pitch_flag = pitch_rows + pitch * 12 + 10
        for field5, field4, mode, field10, word in itertools.product((0, 1), (0, 1, 2), (0, 1), (0, 1), (0, 1)):
            patches = [(definition + 5, bytes([field5])), (definition + 4, bytes([field4])),
                       (0x48b8fc, struct.pack('<I', mode)), (definition + 10, bytes([field10])),
                       (pitch_flag, struct.pack('<H', word))]
            fixtures.append(dict(name=f'fields5={field5},4={field4},mode={mode},10={field10},pitchword={word}',
                declared_input_bytes=[dict(address=at, hex=data.hex()) for at, data in patches]))
        for fixture in fixtures:
            bank[:] = entry_bytes
            for patch in fixture['declared_input_bytes']:
                at = memory.translate(patch['address'])[0]
                raw = bytes.fromhex(patch['hex'])
                assert (patch['address'] & 4095) + len(raw) <= 4096
                bank[at:at+len(raw)] = raw
            machine = Uc(UC_ARCH_X86, UC_MODE_32)
            mapped = set()

            def map_page(address):
                page = address & ~4095
                if page not in mapped:
                    machine.mem_map(page, 4096, UC_PROT_ALL)
                    machine.mem_write(page, memory.read(page, 4096))
                    mapped.add(page)

            def invalid(_machine, access, address, size, value, user):
                map_page(address)
                return True

            trace = []
            writes = []
            machine.hook_add(UC_HOOK_MEM_INVALID, invalid)
            machine.hook_add(UC_HOOK_CODE, lambda uc, address, size, user: trace.append(address))
            machine.hook_add(UC_HOOK_MEM_WRITE, lambda uc, access, address, size, value, user: writes.append((address, size)))
            for register, value in zip(gp_registers, initial['gp']):
                machine.reg_write(register, value)
            for index, words in enumerate(initial['xmm']):
                machine.reg_write(getattr(R, f'UC_X86_REG_XMM{index}'), int.from_bytes(struct.pack('<4I', *words), 'little'))
            machine.reg_write(R.UC_X86_REG_EFLAGS, initial['eflags'])
            map_page(0x1296e0)
            end = struct.unpack('<I', memory.read(initial['gp'][4], 4))[0]
            assert end == 0x129838
            machine.emu_start(0x1296e0, end, count=64)
            assert machine.reg_read(R.UC_X86_REG_EIP) == end and not writes
            covered.update(trace)
            expected.append(dict(name=fixture['name'], gp=[machine.reg_read(reg) for reg in gp_registers],
                eflags=machine.reg_read(R.UC_X86_REG_EFLAGS), trace=trace))
        (scratch / 'entry-ram.bin').write_bytes(entry_bytes)
    (scratch / 'fixtures.json').write_text(json.dumps(dict(cpu=initial, fixtures=fixtures)) + '\n')
    source = ROOT / 'tests/source_sound_cache_query_tu15_tests.cpp'
    binary = args.binary.absolute() if args.binary else scratch / 'query'
    compile_command = None
    if not args.binary:
        compile_command = ['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
            '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(source), str(generated), '-Wl,--gc-sections', '-o', str(binary)]
        compiled = subprocess.run(compile_command, capture_output=True, text=True)
        (output / 'compile.log').write_text((compiled.stdout + compiled.stderr)[-65536:])
        assert compiled.returncode == 0, compiled.stderr[-3000:]
    run_command = [str(binary), str(scratch / 'fixtures.json'), str(scratch / 'entry-ram.bin'), str(scratch / 'native.json')]
    run = subprocess.run(run_command, capture_output=True, text=True, timeout=60)
    (output / 'run.log').write_text((run.stdout + run.stderr)[-65536:])
    assert run.returncode == 0, run.stderr[-3000:]
    actual = json.loads((scratch / 'native.json').read_text())
    assert len(actual) == len(expected)
    comparisons = []
    control_authority = BASE / 'first-person-original-call-v1-native-evidence-v1.json'
    control_evidence = json.loads(control_authority.read_text())
    assert control_evidence['passed'] and control_evidence['raw_x87_control']['xor'] == 0x40
    assert control_evidence['host_control_probe'][:2] == [[575, 639], [639, 639]]
    for native, original in zip(actual, expected):
        # XOR leaves AF undefined; the successful path's final CMP defines it.
        mask = 0xcd5 if original['gp'][0] else 0xcc5
        equal = (native['gp'] == original['gp'] and native['trace'] == original['trace'] and
                 (native['eflags'] & mask) == (original['eflags'] & mask) and native['xmm'] == initial['xmm'] and
                 native['x87_status'] == initial['x87_status'] and
                 ((native['x87_control'] ^ initial['x87_control']) & ~0x40) == 0 and
                 native['mxcsr'] == initial['mxcsr'])
        comparisons.append(dict(name=original['name'], equal=equal, native=native, original=original, defined_flags_mask=mask,
            raw_original_x87_control=initial['x87_control'], raw_control_xor=native['x87_control'] ^ initial['x87_control']))
    assert len(covered) == 27
    result = dict(passed=all(row['equal'] for row in comparisons), fixtures=len(fixtures),
        original_unmodified_entry_cases=1, declared_branch_input_cases=len(fixtures)-1,
        unique_original_instructions_executed=len(covered), all_27_original_instructions_covered=True,
        raw_x87_reserved_bit_6_difference_preserved=True,
        existing_measured_host_FLDCW_authority_sha256=sha(control_authority),
        every_other_control_bit_compared=True,
        source_writes_or_external_owners=0, comparisons=comparisons,
        original_prefix_sha256=sha(prefix / 'report.json'), original_call_sha256=sha(case / 'call.json'),
        native_generated_source_sha256=sha(generated), native_binary_sha256=sha(binary),
        source_files_sha256={str(path): sha(path) for path in (source, ROOT / 'include/halo2/source_sound_cache_query_tu15.hpp')},
        compile_command=compile_command, generator_command=generator_command, run_command=run_command,
        runner_scratch=str(scratch), large_artifacts_retained=[], full_weapon_parity=False)
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    for path in (Path(__file__), source, generated, ROOT / 'tools/generate_sound_cache_query_tu15.py'):
        shutil.copyfile(path, output / path.name)
    print(json.dumps({key: result[key] for key in ('passed', 'fixtures', 'unique_original_instructions_executed', 'runner_scratch')}), flush=True)
    assert result['passed']
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
