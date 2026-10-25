#!/usr/bin/env python3
"""Qualify native damage difficulty selection against whole unchanged TU1.5 owners.

Original execution is strictly an offline oracle. The native test executes C++
semantics and separately checks the retained literal source compositions.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess
import sys

from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ_UNMAPPED
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]
GP = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)
ENTRIES = (0x1f1950, 0x1f19b0)


def body(text, entry):
    matches = list(re.finditer(r'static void run_' + format(entry, 'x') + r'\((\w+) &s\) \{\n', text))
    if len(matches) != 1:
        raise ValueError('Literal difficulty owner changed')
    start = matches[0].start(); end = text.index('\n}\n\n', start) + 2
    return text[start:end], matches[0][1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--negative-control', action='store_true')
    parser.add_argument('--installed-routes', action='store_true',
        help='Execute the installed owner bodies, requiring their native activity counters')
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output = args.output.resolve(); args.output.mkdir(parents=True, exist_ok=False)
    if sha(NEW_DEFAULT) != NEW_PIN:
        raise ValueError('Original TU1.5 changed')
    original = Xbe(NEW_DEFAULT); code = _Code(original, '.text')
    bodies = {entry: _trace(code, entry, set(ENTRIES)) for entry in ENTRIES}
    if [len(bodies[entry].code) for entry in ENTRIES] != [26, 7]:
        raise ValueError('Complete original difficulty CFG changed')
    old_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    if sha(old_path) != PIN:
        raise ValueError('Canonical source observation reference changed')
    old = Xbe(old_path); old_code = _Code(old, '.text'); correspondence = {}
    for entry, canonical in zip(ENTRIES, (0x1ed030, 0x1ed090)):
        prior = _trace(old_code, canonical, {0x1ed030, 0x1ed090})
        left = [old_code.at(pc) for pc in sorted(prior.code)]
        right = [code.at(pc) for pc in sorted(bodies[entry].code)]
        if full_tokens(left, old) != full_tokens(right, original):
            raise ValueError('Original/canonical complete owner correspondence changed')
        correspondence.update({str(a): b for a, b in zip(sorted(prior.code), sorted(bodies[entry].code))})
    sites = set().union(*(set(item.code) for item in bodies.values()))
    kernel = 'src/gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp'
    observed = {int(pc, 16) for pc in re.findall(r'site\(0x[0-9a-f]+u,0x([0-9a-f]+)u\)', (ROOT / kernel).read_text())}
    if observed != sites:
        raise ValueError('Native observation sites differ from the complete original owners')
    source = owner / 'source'; source.mkdir(); pins = {}
    for path in [*ROOT.joinpath('include').rglob('*.hpp'), *ROOT.joinpath('src').rglob('*.hpp'),
                 ROOT / 'tests/source_difficulty_parameter_native_tu15_tests.cpp']:
        name = path.relative_to(ROOT); target = source / name
        target.parent.mkdir(parents=True, exist_ok=True)
        before = sha(path); shutil.copyfile(path, target)
        if sha(path) != before or sha(target) != before:
            raise ValueError('Native qualification source changed while freezing')
        pins[str(name)] = before
    literal = ['#pragma once\nnamespace difficulty_literal_routes {\nusing namespace halo2::physics_world_runtime_detail;\nusing halo2::SourcePhysicsContactImage;\n']
    helper_source = (ROOT / 'src/gameplay/units/source_direct_damage_runtime_generated.cpp').read_text()
    helper_start = helper_source.index('std::uint32_t sign_extend(')
    helper_end = helper_source.index('\n}', helper_start) + 2
    literal.append(helper_source[helper_start:helper_end] + '\n')
    # The delivery owner supports canonical or actual observation sites. This
    # isolated literal comparison declares canonical sites and maps them back
    # using the independently authenticated complete original correspondence.
    literal.append('inline std::uint32_t damage_observation_pc(std::uint32_t canonical,std::uint32_t,WorldState&){return canonical;}\n')
    literal_pins = {}; routes = []
    names = ('campaign/ai/source_ai_frame_tu15_generated.cpp', 'gameplay/units/source_direct_damage_runtime_generated.cpp',
             'gameplay/units/source_vitality_runtime_generated.cpp', 'gameplay/units/source_damage_delivery_runtime_generated.cpp')
    for index, name in enumerate(names, 1):
        path = ROOT / 'src' / name; text = path.read_text(); canonical = index != 1
        if not args.installed_routes:
            from native_difficulty_parameter_kernel_tu15 import strip_native_difficulty_routes
            text = strip_native_difficulty_routes(text)
        entries = (0x1ed030,) if index == 3 else (0x1ed030, 0x1ed090) if canonical else ENTRIES
        literal.append('namespace route' + str(index) + ' {\n')
        # These are address relocations, authenticated by complete owner tokens
        # above. They provide no computed result or dependency return.
        literal.append('static std::uint32_t selected(std::uint32_t at,halo2::SourcePhysicsContactImage) {\n')
        literal.append('switch(at) {case 0x4e791c:return 0x4ecc1c;case 0x465030:return 0x46a0d0;case 0x4edf18:return 0x4f3218;default:return at;}\n}\n' if canonical else 'return at;\n}\n')
        records = []
        for entry in entries:
            raw, state_type = body(text, entry)
            records.append(dict(entry=entry, sha256=hashlib.sha256(raw.encode()).hexdigest()))
            literal.append(raw.replace('(' + state_type + ' &s)', '(WorldState &s)', 1) + '\n')
        literal.append('}\n'); literal_pins[str(path.relative_to(ROOT))] = dict(sha256=sha(path), bodies=records)
        choices = 'if(entry==0x1f1950u)route' + str(index) + '::run_' + format(entries[0], 'x') + '(s);'
        if len(entries) == 2: choices += 'else route' + str(index) + '::run_' + format(entries[1], 'x') + '(s);'
        else: choices += 'else throw std::runtime_error("Absent literal wrapper");'
        routes.append('case ' + str(index) + ':' + choices + 'break;')
    literal += ['inline void run(unsigned route,std::uint32_t entry,WorldState& s){switch(route){', *routes, 'default:throw std::runtime_error("Unknown literal route");}}\n}\n']
    (source / 'tests/difficulty_literal_routes.hpp').write_text(''.join(literal))
    mutation = None
    if args.negative_control:
        target = source / kernel; text = target.read_text(); needle = 'signed_difficulty<0?0:signed_difficulty>3?3:signed_difficulty'
        if text.count(needle) != 1: raise ValueError('Difficulty mutation seam changed')
        before = sha(target); target.write_text(text.replace(needle, 'signed_difficulty<0?1:signed_difficulty>3?3:signed_difficulty'))
        pins[kernel] = sha(target); mutation = dict(path=kernel, before=before, after=pins[kernel])
    cpu = Uc(UC_ARCH_X86, UC_MODE_32); cpu.mem_map(0x10000, 0x800000)
    for section in original.sections: cpu.mem_write(section.va, original.section_bytes(section))
    cpu.mem_map(0x10000000, 0x200000); stop = 0x70000000; cpu.mem_map(stop, 4096)
    stack, globals_at, options, table = 0x10000800, 0x10002000, 0x10003000, 0x10100000
    rng = random.Random(0x1f1950)
    specs = [dict(entry=entry, present=present, difficulty=difficulty, parameter=parameter, count=count, mode=mode)
        for entry, present, difficulty, parameter, count, mode in itertools.product(ENTRIES, range(4),
        (-32768, -1, 0, 1, 2, 3, 4, 32767), (-32768, -1, 0, 34, 32767), (1,), (1,))]
    specs += [dict(entry=entry, present=3, difficulty=difficulty, parameter=2, count=count, mode=mode)
        for entry, difficulty, count, mode in itertools.product(ENTRIES, (-1, 0, 3, 4),
        (0, 2, 0x7fffffff, 0x80000000, 0xffffffff), (0, 1, 2, 0xffffffff))]
    specs += [dict(entry=entry, present=present, difficulty=difficulty, parameter=2, count=1, mode=1,
                   missing=missing)
        for entry, present, difficulty, missing in itertools.product(ENTRIES, (0, 3), (-1, 4),
            ('globals_pointer', 'default_word', 'globals', 'table', 'stack', 'options_pointer', 'options'))
        if entry == 0x1f19b0 or not missing.startswith('options')]
    cases = []; covered = set(); current = {}; reads = []; trace = []
    def instruction(_cpu, pc, size, _):
        if pc not in sites or bytes(cpu.mem_read(pc, size)) != original.read(pc, size):
            raise ValueError('Offline original difficulty execution escaped its authenticated owners')
        current['pc'] = pc; trace.append(pc); covered.add(pc)
    def read(_cpu, _kind, address, size, _value, _):
        try: data = bytes(cpu.mem_read(address, size))
        except UcError: return
        reads.append(dict(instruction=current['pc'], address=address, bytes=data.hex()))
    def unmapped(_cpu, _kind, address, size, _value, _):
        current['fault'] = dict(instruction=current['pc'], address=address, size=size)
        return False
    def write(*_): raise ValueError('Original read-only difficulty owner emitted a store')
    cpu.hook_add(UC_HOOK_CODE, instruction); cpu.hook_add(UC_HOOK_MEM_READ, read); cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    cpu.hook_add(UC_HOOK_MEM_READ_UNMAPPED, unmapped)
    for case_id, spec in enumerate(specs):
        trace.clear(); reads.clear(); current['fault'] = None; pieces = []
        def raw(at, data): cpu.mem_write(at, data); pieces.append([at, data.hex()])
        def word(at, value, size=4): raw(at, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))
        word(stack, stop); word(stack + 4, spec['parameter'] | (rng.getrandbits(16) << 16))
        word(0x4ecc1c, globals_at if spec['present'] else 0)
        word(globals_at + 0xf8, spec['count'] if spec['present'] >= 2 else 0)
        word(globals_at + 0xfc, table if spec['present'] == 3 else 0)
        raw(0x46a0d0, original.read(0x46a0d0, 4)); word(0x4f3218, options)
        word(options + 8, spec['mode']); word(options + 0x132, spec['difficulty'], 2)
        difficulty = spec['difficulty'] if spec['entry'] == 0x1f1950 or spec['mode'] == 1 else 1
        selected = max(0, min(3, difficulty)); address = (table + (spec['parameter'] * 4 + selected) * 4) & 0xffffffff
        # Bit payloads include NaNs, signed zero, infinity and denormals. The
        # original MOVSS, rather than host floating-point arithmetic, selects it.
        payloads = (0, 0x80000000, 0x7fc12345, 0x7f800001, 0x7f800000, 1, 0xffffffff)
        payload = payloads[case_id % len(payloads)] if case_id < 100 else rng.getrandbits(32)
        for column in range(4):
            word(table + spec['parameter'] * 16 + column * 4,
                 payload if column == selected else payload ^ (0x1020304 * (column + 1)))
        gp = [rng.getrandbits(32) for _ in GP]; gp[4] = stack
        gp[1] = (rng.getrandbits(16) << 16) | (spec['difficulty'] & 0xffff)
        xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
        flags = 0x202 | (rng.getrandbits(12) & 0x8d5)
        for reg, value in zip(GP, gp): cpu.reg_write(reg, value)
        for n, lanes in enumerate(xmm): cpu.reg_write(R.UC_X86_REG_XMM0 + n, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        cpu.reg_write(R.UC_X86_REG_EFLAGS, flags); cpu.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        cpu.reg_write(R.UC_X86_REG_FPSW, 0); cpu.reg_write(R.UC_X86_REG_FPTAG, 0xffff); cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        missing_addresses = dict(globals_pointer=0x4ecc1c, default_word=0x46a0d0, globals=globals_at,
            table=address, stack=stack, options_pointer=0x4f3218, options=options)
        fault_page = missing_addresses[spec['missing']] & ~4095 if 'missing' in spec else None
        saved_page = bytes(cpu.mem_read(fault_page, 4096)) if fault_page is not None else None
        if fault_page is not None: cpu.mem_unmap(fault_page, 4096)
        try:
            try: cpu.emu_start(spec['entry'], stop, count=100)
            except UcError:
                if current['fault'] is None: raise
            if current['fault'] is None and cpu.reg_read(R.UC_X86_REG_EIP) != stop:
                raise ValueError('Original difficulty owner did not return')
        finally:
            if fault_page is not None:
                cpu.mem_map(fault_page, 4096); cpu.mem_write(fault_page, saved_page)
        cases.append(dict(id=case_id, entry=spec['entry'], inputs=spec, banks=pieces, fault_page=fault_page, fault=current['fault'],
            before=dict(gp=gp, xmm=xmm, eflags=flags), instructions=list(trace), reads=list(reads),
            after=dict(gp=[cpu.reg_read(reg) for reg in GP], eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
                xmm=[list(struct.unpack('<4I', cpu.reg_read(R.UC_X86_REG_XMM0+n).to_bytes(16, 'little'))) for n in range(8)])))
    fixture = owner / 'original-difficulty-cases.json'
    fixture.write_text(json.dumps(dict(original_xbe_sha256=NEW_PIN, canonical_observation_map=correspondence, cases=cases), separators=(',', ':')) + '\n')
    (source / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(Halo2DifficultyNative LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(source_difficulty_parameter_native_tu15_tests tests/source_difficulty_parameter_native_tu15_tests.cpp)
target_include_directories(source_difficulty_parameter_native_tu15_tests PRIVATE include src tests)
target_compile_features(source_difficulty_parameter_native_tu15_tests PRIVATE cxx_std_20)
target_compile_options(source_difficulty_parameter_native_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(source_difficulty_parameter_native_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
''' + f'add_test(NAME native_difficulty_parameter_original COMMAND "${{Python3_EXECUTABLE}}" "{ROOT / "tools/run_test.py"}" -- $<TARGET_FILE:source_difficulty_parameter_native_tu15_tests> "{fixture}")\n')
    if args.installed_routes:
        with (source / 'CMakeLists.txt').open('a') as stream:
            stream.write('target_compile_definitions(source_difficulty_parameter_native_tu15_tests PRIVATE HALO2_DIFFICULTY_INSTALLED_ROUTES=1)\n')
    command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--build-worker', '--source', str(source),
        '--target', 'source_difficulty_parameter_native_tu15_tests', '--filter', '^native_difficulty_parameter_original$',
        '--jobs', '2', '--test-jobs', '1', '--output', str(args.output / 'validation')]
    result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    validation = json.loads((args.output / 'validation/report.json').read_text())
    log = (args.output / 'validation/tests.log').read_text() if (args.output / 'validation/tests.log').exists() else ''
    rejected = bool(result and all(step['exit_code'] == 0 for step in validation['steps'] if step['name'] != 'tests')
        and 'Difficulty ordered reads differ' in log)
    unchanged = all(sha(source / name) == pin for name, pin in pins.items())
    passed = (rejected if args.negative_control else result == 0) and unchanged and covered == sites
    report = dict(passed=passed, original_sha256=NEW_PIN, cases=len(cases), complete_original_instructions=len(sites),
        fault_cases=sum(row['fault'] is not None for row in cases),
        original_route_comparisons=sum(5 if row['entry'] == 0x1f1950 else 4 for row in cases),
        instruction_coverage_complete=covered == sites, source_unchanged=unchanged, source_hashes=pins,
        literal_source_bodies=literal_pins, original_fixture_sha256=sha(fixture), original_fixture_bytes=fixture.stat().st_size,
        native_kernel_sha256=sha(source / kernel), source_instruction_patches=[], supplied_dependency_results=[],
        negative_control=mutation, comparison_rejected_mutation=rejected if args.negative_control else None,
        validation_returncode=result, owned_scratch=str(owner), retained_large_artifacts=[],
        production_route_installed=args.installed_routes, full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=passed, cases=len(cases), report=str(args.output / 'report.json'))), flush=True)
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
