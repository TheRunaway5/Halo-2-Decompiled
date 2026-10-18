#!/usr/bin/env python3
"""Probe the closed original sound range-selection graph before integration.

Use actual captured entry RAM/CPU. Declared record and pointer controls exercise
extra-info selection, context/scenario encodings and property flags. The controls
reuse captured stack pages and are never production banks or claimed gameplay.
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
    original=ROOT/'tests/source_sound_listener_selection_tu15_tests.cpp'
    text=original.read_text().replace('#include "halo2/source_sound_listener_selection_tu15.hpp"',
        '#include "halo2/physics_world_runtime.hpp"\nnamespace halo2 {void source_run_sound_range_selection_tu15(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver & = {});}')
    text=text.replace('source_run_sound_listener_selection_tu15(', 'source_run_sound_range_selection_tu15(')
    p=scratch/'probe.cpp';p.write_text(text);return p


def range_cases(memory,initial):
    word=lambda at:struct.unpack('<I',memory.read(at,4))[0]
    definition=initial['gp'][1]
    gestalt=word(0x52d664)
    temporary=initial['gp'][4]-0x800
    memory.read(temporary,0x800)
    parameters=temporary
    ranges=temporary+0x100
    cases=[]
    w=lambda n:struct.pack('<I',n&0xffffffff)
    f=lambda v:struct.pack('<f',v)
    h=lambda n:struct.pack('<h',n)
    def add(name,patches=(),preferred=None,value=None,mxcsr=None):
        abi=copy.deepcopy(initial)
        if preferred is not None:abi['gp'][0]=(abi['gp'][0]&0xffff0000)|(preferred&65535)
        if value is not None:abi['xmm'][1][0]=value
        if mxcsr is not None:abi['mxcsr']=mxcsr
        cases.append(dict(name=name,entry=0x21f960,cpu=abi,declared_input_bytes=[dict(address=at,hex=raw.hex())for at,raw in patches]))
    def controls(count=3,enabled=(1,1,1),windows=((-20,-10,-15,-12),(-5,5,-2,2),(10,20,12,15))):
        rows=[]
        for i in range(max(0,count)):
            rows.append(struct.pack('<6h',0,i%3,0,0,0,enabled[i%3]))
        return [(gestalt+0x1c,w(parameters)),(gestalt+0x24,w(ranges)),
            (definition+8,h(0)),(definition+10,struct.pack('<b',count)),
            (parameters,b''.join(struct.pack('<5h',0,*values)for values in windows)),
            (ranges,b''.join(rows))]
    add('original unmodified prefix input')
    for preferred in [-1,0,1,2,3]:
        for value in [-40.,-15.,-12.,-7.,0.,7.,12.,15.,40.]:
            add(f'declared preferred {preferred}, value {value}',controls(),preferred=preferred,value=struct.unpack('<I',f(value))[0])
    for count in [-128,-1,0,1,127]:
        add(f'declared signed range count {count}',controls(count),preferred=-1,value=0)
    for enabled in [(0,0,0),(1,0,0),(0,1,0),(0,0,1),(-1,1,1)]:
        add(f'declared occupied range controls {enabled}',controls(enabled=enabled),preferred=0,value=0)
    for bits in [0x80000000,0x7fc12345,0x7f812345,0x7f800000,0xff800000,1]:
        add(f'declared raw value {bits:08x}',controls(),preferred=0,value=bits)
    for value in [-7.5,7.5]:
        add(f'declared equal nearest distance {value}',controls(),preferred=-1,value=struct.unpack('<I',f(value))[0])
    for rounding in [0,0x2000,0x4000,0x6000]:
        add(f'declared SSE rounding {rounding:04x}',controls(),preferred=-1,value=0x3dcccccd,
            mxcsr=(initial['mxcsr']&~0x603f)|rounding)
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--generated-source', type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute() if args.output else scratch / 'gain-result'
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    case = BASE / 'smg-original-fire-v1'
    prefix = BASE / 'smg-original-sound-21f960-entry-v1'
    if not (prefix / 'report.json').exists():
        return 77
    prefix_report = json.loads((prefix / 'report.json').read_text())
    assert not prefix_report['passed'] and prefix_report['declared_prefix_stop_reached'] and prefix_report['declared_prefix_stop'] == 0x21f960
    generated = args.generated_source.absolute() if args.generated_source else scratch / 'gain.cpp'
    regenerated = scratch / 'regenerated-gain.cpp'
    generator_command = [str(Path(os.sys.executable).absolute()), str(ROOT / 'tools/generate_sound_range_selection_tu15.py'),
        '--output', str(regenerated), '--manifest', str(scratch / 'generation.json')]
    generation = subprocess.run(generator_command, capture_output=True, text=True)
    (output / 'generation.log').write_text((generation.stdout + generation.stderr)[-65536:])
    assert generation.returncode == 0, generation.stderr[-3000:]
    if args.generated_source:
        assert generated.read_bytes() == regenerated.read_bytes(), 'Production source differs from authenticated generation'
    else:
        shutil.copyfile(regenerated, generated)
    for name in ('generation.json', 'sound-range-selection-bodies.json', 'sound-range-selection-callers.json'):
        shutil.copyfile(scratch / name, output / name)
    initial = cpu(case, 'before')
    initial.update(prefix_report['actual_CPU'])
    registers = [getattr(R, 'UC_X86_REG_' + name) for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
    fixtures = [dict(name='original captured entry', declared_input_bytes=[])]
    originals = []
    covered = set()
    with reconstructed_replay_ram(case, prefix) as (bank, memory, replay):
        entry_bytes = bank[:]
        fixtures = range_cases(memory, initial)
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
            assert end == 0x128dd4
            error = None
            try:
                machine.emu_start(fixture_input['entry'], end, count=1000)
            except Exception as failure:
                error = str(failure)
            returned = machine.reg_read(R.UC_X86_REG_EIP) == end and error is None
            assert returned or (fixture_input.get('expected_fault') and trace[-1] == fixture_input['expected_fault']), (fixture_input['name'], trace[-1], error)
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
    header = ROOT / 'include/halo2/physics_world_runtime.hpp'
    pins = {str(path): sha(path) for path in (source, generated, header)}
    binary = args.binary.absolute() if args.binary else scratch / 'gain'
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
    coverage = len(covered) == 106
    body_manifest = json.loads((output / 'sound-range-selection-bodies.json').read_text())
    all_sites = {int(pc, 16) for pc in body_manifest['instruction_map']}
    assert sum(not row['returned'] for row in originals) == 0
    result = dict(passed=all(row['equal'] for row in comparisons) and coverage, fixtures=len(fixtures),
        original_unmodified_entry_cases=1, declared_byte_and_ABI_fixture_cases=len(fixtures)-1,
        unique_original_instructions_executed=len(covered), complete_original_bodies=1,
        all_original_instructions_covered=coverage, missing_original_instructions=sorted(all_sites-covered),
        complete_original_cfg_instructions=106,
        declared_record_and_pointer_controls_use_existing_captured_stack_pages=True,
        controlled_record_bytes_are_not_captured_gameplay_or_production_banks=True,
        successful_original_cases=sum(row['returned'] for row in originals), real_missing_input_fault_cases=0,
        original_full_call_independently_verified=False, production_integration=False, full_weapon_parity=False,
        raw_reserved_control_bit_difference_preserved=True, measured_control_authority_sha256=sha(authority),
        comparisons=comparisons, source_files_sha256=pins,
        original_prefix_sha256=sha(prefix / 'report.json'), original_call_sha256=sha(case / 'call.json'),
        native_generated_source_sha256=sha(generated), native_binary_sha256=sha(binary),
        compile_command=command, generator_command=generator_command, run_command=run_command,
        runner_scratch=str(scratch), large_artifacts_retained=[])
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    for path in (Path(__file__), source, generated, header, ROOT / 'tools/generate_sound_range_selection_tu15.py'):
        shutil.copyfile(path, output / path.name)
    print(json.dumps({key: result[key] for key in ('passed', 'fixtures', 'unique_original_instructions_executed', 'runner_scratch')}), flush=True)
    assert result['passed']
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
