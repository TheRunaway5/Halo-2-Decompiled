#!/usr/bin/env python3
"""Qualify native post-solver semantics from genuine physical TU1.5 returns.

The unchanged complete326200 executes actual326A40. Declared initialized solver
inputs select its zero-iteration schema tail; no solver outcome is supplied.
The native continuation starts from the actual service-return banks, locals,
result and FP. This component proof does not qualify a native core solver.
"""
import argparse
import copy
import hashlib
import json
import os
import struct
import subprocess
import sys
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_constraint_step_tu15 import (
    BASE, ISLAND, STEP, SOLVER, INPUT, WORKSPACE, ALLOCATOR,
    BODY_ARRAY, CONSTRAINT_ARRAY, BODY, MOTION, ARENA, EXTRA, STACK, SP, STOP,
    cases as prefix_cases,
)

BANKS = ((BASE, 0x10000), (0x41e000, 0x2000), (0x45f000, 0x1000),
         (0x48c000, 0x1000), (0x48d000, 0x1000), (0x491000, 0x1000))
ENTRIES = (0x326200, 0x326a40, 0x323560, 0x3248d0, 0x324800, 0x326630, 0x326620)
BACKEND = 'unchanged-tu15-full-constraint-call-genuine-solver-return-v1'


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def metadata(image):
    code = _Code(image, '.text')
    registers = {name: n for n, name in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
    sites, counts, listings = [], [], {}
    for entry in ENTRIES:
        body = complete_body(code, entry)
        counts.append(len(body.code))
        listing = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            listing.append(f'{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
            operands = []
            for op in instruction.operands:
                if op.type != X86_OP_MEM or instruction.mnemonic == 'lea':
                    continue
                access = CS_AC_WRITE if instruction.mnemonic in ('fst', 'fstp') else op.access
                assert access & (CS_AC_READ | CS_AC_WRITE), (hex(pc), 'Missing architectural access')
                base, index = instruction.reg_name(op.mem.base), instruction.reg_name(op.mem.index)
                assert not op.mem.segment
                operands.append(dict(base=registers[base] if base else -1,
                                     index=registers[index] if index else -1,
                                     scale=op.mem.scale, displacement=op.mem.disp & 0xffffffff,
                                     bytes=op.size, read=bool(access & CS_AC_READ),
                                     write=bool(access & CS_AC_WRITE)))
            sites.append(dict(instruction=pc, operands=operands))
        listings[hex(entry)] = hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts == [306, 2208, 25, 17, 3, 5, 4], counts
    assert image.read(0x328824, 17).hex() == '0001020304020205060404020402070707'
    return dict(sites=sites, body_sites=counts, listing_sha256=listings,
                regions=[dict(address=p, bytes=n) for p, n in BANKS])


def put(case, address, value, width=4):
    for bank in case['banks']:
        data = bytearray.fromhex(bank['before'])
        offset = address - bank['address']
        if 0 <= offset and offset + width <= len(data):
            data[offset:offset + width] = (value & ((1 << (width * 8)) - 1)).to_bytes(width, 'little')
            bank['before'] = data.hex()
            return
    raise AssertionError(('Unowned original input address', hex(address), width))


def cases(image):
    out = []
    originals = prefix_cases(image)
    for bodies in (0, 1, 3):
        for mode in ('fits', 'whole', 'jacobians', 'schemas'):
            for results in (0, 1, 5, 0xffffffff):
                selected = next(c for c in originals if c['name'] == f'fixed_{bodies}_{mode}_{results:x}')
                for profile in (False, True):
                    c = copy.deepcopy(selected)
                    c['name'] = selected['name'] + ('_profile' if profile else '_plain')
                    c['banks'].append(dict(address=0x48d000, before=bytes(0x1000).hex()))
                    c['banks'].sort(key=lambda bank: bank['address'])
                    c['stop_pc'] = STOP
                    put(c, 0x48d480, 1, 1)
                    put(c, SOLVER + 0x1c, 0 if profile else 0xffffffff)
                    put(c, SOLVER + 0x20, 0x7fc12345)
                    put(c, 0x48c974, BASE + 0xe000 if profile else 0)
                    put(c, 0x48c978, BASE + 0xe010 if profile else 0)
                    out.append(c)
    for mode in ('whole', 'jacobians', 'schemas'):
        c = copy.deepcopy(next(c for c in out if c['name'] == f'fixed_1_{mode}_0_profile'))
        c.update(name='actual_free_' + mode, stop_pc=0x2e1110)
        put(c, ALLOCATOR + 0x14, 0)
        out.append(c)
    c = copy.deepcopy(next(c for c in out if c['name'] == 'fixed_3_whole_0_profile'))
    c.update(name='actual_commit_after_fixed', stop_pc=0x3235f0)
    put(c, MOTION + 0x100, 0x41f1e0)
    out.append(c)
    c = copy.deepcopy(next(c for c in out if c['name'] == 'fixed_1_whole_0_profile'))
    c['name'] = 'profile_alias_allocator_cursor'
    put(c, 0x48c974, ALLOCATOR + 8)
    put(c, 0x48c978, ALLOCATOR + 0x18)
    out.append(c)
    c = copy.deepcopy(next(c for c in out if c['name'] == 'fixed_1_fits_0_profile'))
    c['name'] = 'profile_alias_island_count'
    put(c, 0x48c974, ISLAND + 0x18)
    put(c, 0x48c978, ISLAND + 0x28)
    out.append(c)
    return out


def observe(image, metadata_value, case, owned, observer):
    (owned / 'metadata.json').write_text(json.dumps(metadata_value, separators=(',', ':')))
    section = next(s for s in image.sections if s.name == '.text')
    text = owned / 'text.bin'
    if not text.exists():
        text.write_bytes(image.section_bytes(section))
    sections = [f'.original 0x{section.va:x} : {{ *(.original) }}']
    assembly = [f'.section .original,"ax"\n.incbin "{text}"']
    for n, bank in enumerate(case['banks']):
        path = owned / f'bank{n}.bin'
        path.write_bytes(bytes.fromhex(bank['before']))
        assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"')
        sections.append(f'.bank{n} 0x{bank["address"]:x} : {{ *(.bank{n}) }}')
    stack = bytearray(0x10000)
    struct.pack_into('<10I', stack, SP - STACK, STOP, *case['arguments'])
    (owned / 'stack.bin').write_bytes(stack)
    assembly.append(f'.section .stack,"aw"\n.incbin "{owned / "stack.bin"}"')
    sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
    cw, sw, mxcsr = case['before_fp']
    environment = bytearray(28)
    struct.pack_into('<H', environment, 0, cw)
    struct.pack_into('<H', environment, 4, sw)
    struct.pack_into('<H', environment, 8, 0xffff)
    (owned / 'environment.bin').write_bytes(environment)
    assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long 0x326200
sse_control: .long {mxcsr}
environment: .incbin "{owned / 'environment.bin'}"
''')
    sections.append('.bridge 0x70000000 : { *(.bridge) }')
    (owned / 'reference.s').write_text('\n'.join(assembly))
    (owned / 'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n' + '\n'.join(sections) + '\n}\n')
    subprocess.run(['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')], check=True, capture_output=True)
    subprocess.run(['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o', str(owned / 'reference')], check=True, capture_output=True)
    result = subprocess.run([str(observer), str(owned / 'reference'), str(owned / 'metadata.json'), str(owned / 'output.json'), hex(case['stop_pc'])], text=True, capture_output=True)
    assert result.returncode == 0, (case['name'], result.stdout, result.stderr)
    return json.loads((owned / 'output.json').read_text())


def physical(image, m, inputs, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir(exist_ok=True)
    observer = owned / 'observer'
    if not observer.exists():
        subprocess.run(['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_constraint_continuation_tu15.cpp'), '-o', str(observer)], check=True, capture_output=True)
    out = []
    for original in inputs:
        observed = observe(image, m, original, owned, observer)
        service = observed['service_return']
        assert service is not None, (original['name'], 'Original core did not return')
        assert service['instruction'] == 0x3264e7 and service['gp'][0] == 1
        assert service['argument_words'] == original['arguments']
        assert service['x87_tag'] == 0, (original['name'], 'Original core returned nonempty x87', service['x87_tag'])
        locals_ = service['root_local_words']
        c = dict(name=original['name'], entry=0x326200, arguments=service['argument_words'],
                 physical_verified=True, genuine_service_return=True,
                 original_root_banks=copy.deepcopy(original['banks']),
                 original_fp_before=original['before_fp'], before_fp=service['fp'],
                 before_x87_tag=service['x87_tag'], actual_solver_result=service['gp'][0],
                 continuation=dict(first_accumulator=locals_[15], body_end=locals_[13],
                                   allocated_block=locals_[5], arena_end=service['gp'][7]),
                 solver_return_local_words=locals_, solver_return_gp=service['gp'],
                 original_instructions=observed['original_instructions'],
                 original_prefix_instructions=service['original_instructions'],
                 reads=observed['reads'][service['read_index']:],
                 writes=observed['writes'][service['write_index']:],
                 after_fp=observed['after_fp'], stop_pc=original['stop_pc'], boundary=None)
        c['banks'] = [dict(address=b['address'], before=b['after'], after=e['after'])
                      for b, e in zip(service['banks'], observed['banks'])]
        assert all(b['address'] == e['address'] for b, e in zip(service['banks'], observed['banks']))
        if 'fault' in original:
            c['fault'] = original['fault']
        elif original['stop_pc'] == 0x2e1110:
            c['boundary'] = dict(kind='free', owner=0x2e1110, receiver=observed['boundary_gp'][1], arguments=observed['boundary_stack'][1:3])
        elif original['stop_pc'] == 0x3235f0:
            c['boundary'] = dict(kind='commit', owner=0x3235f0, receiver=observed['boundary_gp'][1], arguments=observed['boundary_stack'][1:4])
        else:
            assert original['stop_pc'] == STOP
        out.append(c)
    return out


def tail_cases(all_cases):
    out = []
    for seed_name in ('fixed_1_fits_0_profile', 'fixed_1_whole_0_profile'):
        seed = next(c for c in all_cases if c['name'] == seed_name)
        schema, slots = seed['solver_return_local_words'][12], seed['solver_return_local_words'][11]
        def fresh(name):
            return dict(name=seed_name + '_' + name, banks=copy.deepcopy(seed['original_root_banks']), arguments=seed['arguments'], before_fp=seed['original_fp_before'], stop_pc=STOP)
        c = fresh('actual_skip_schema')
        put(c, schema + 0x18, 14, 1)
        put(c, schema + 0x19, 2, 1)
        out.append(c)
        for word in (16777217, 0xffffffff, 0x80000000, 0x7fc12345):
            for payload in (0x3f000000, 0x7fc98765, 0x7f812345):
                c = fresh(f'actual_tail_{word:x}_{payload:x}')
                put(c, schema + 4, BASE + 0xc000)
                put(c, schema + 8, ISLAND + 0x18)
                put(c, schema + 0x14, 8)
                put(c, schema + 0x18, 2, 1)
                put(c, schema + 0x19, 2, 1)
                put(c, slots, word)
                put(c, BASE + 0xc00c, payload)
                put(c, SOLVER + 0x10, 0x3f800000)
                out.append(c)
    return out


def fault_cases(all_cases):
    out = []
    for seed_name in ('fixed_1_fits_0_profile', 'fixed_3_whole_0_profile', 'actual_free_schemas', 'profile_alias_allocator_cursor', 'profile_alias_island_count'):
        seed = next(c for c in all_cases if c['name'] == seed_name)
        seen, occurrences = set(), {}
        for read in seed['reads']:
            key = read['address'], read['bytes']
            occurrences[key] = occurrences.get(key, 0) + 1
            if read['instruction'] in seen:
                continue
            seen.add(read['instruction'])
            out.append(dict(name=seed_name + '_fault_' + hex(read['instruction']),
                            banks=copy.deepcopy(seed['original_root_banks']), arguments=seed['arguments'],
                            before_fp=seed['original_fp_before'], stop_pc=read['instruction'],
                            fault=dict(address=read['address'], bytes=read['bytes'], occurrence=occurrences[key])))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    m = metadata(image)
    all_cases = physical(image, m, cases(image), scratch)
    all_cases += physical(image, m, tail_cases(all_cases), scratch)
    faults = fault_cases(all_cases)
    all_cases += physical(image, m, faults, scratch)
    fixture_value = dict(image_sha256=NEW_PIN, required_backend=BACKEND,
                         root_sites=306, core_sites=2208, service_return_instruction=0x3264e7,
                         listing_sha256=m['listing_sha256'], cases=all_cases)
    fixture = scratch / 'continuations.json'
    fixture.write_text(json.dumps(fixture_value, separators=(',', ':')))
    project = scratch / 'project'
    project.mkdir()
    source = ROOT / 'src/physics/native_tu15_constraint_step.cpp'
    source_text = source.read_text()
    mutation = 'std::bit_cast<std::int32_t>(count)'
    assert source_text.count(mutation) == 1
    (project / 'mutated.cpp').write_text(source_text.replace(mutation, 'std::bit_cast<std::int32_t>(count+1u)'))
    (project / 'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],"--oracle-only",sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"ordered continuation store"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects changed profiling count:",r.stderr.strip())
''')
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(native_constraint_continuation_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name, src in (('native_constraint_continuation_tests', source), ('native_constraint_continuation_mutation', project / 'mutated.cpp')):
        cmake += f'add_executable({name} "{ROOT}/tests/native_tu15_constraint_continuation_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake += f'add_test(NAME native_constraint_continuation_original COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_constraint_continuation_tests> "{fixture}")\nadd_test(NAME native_constraint_continuation_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_constraint_continuation_mutation> "{fixture}")\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    for compiler in ('g++', 'clang++'):
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / compiler), '--target', 'native_constraint_continuation_tests', '--target', 'native_constraint_continuation_mutation', '--filter', '^native_constraint_continuation_(original|negative)$', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command, check=False)
        results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
    files = (source, ROOT / 'include/halo2/native_tu15_constraint_step.hpp', ROOT / 'tests/native_tu15_constraint_continuation_tests.cpp', ROOT / 'tools/observe_native_constraint_continuation_tu15.cpp', Path(__file__))
    report = dict(passed=all(r['exit_code'] == 0 for r in results), compiler_results=results,
                  original_cases=len(all_cases), full_original_completions=sum(c['stop_pc'] == STOP for c in all_cases),
                  genuine_dependency_entry_stops=sum(c['boundary'] is not None for c in all_cases),
                  read_fault_prefixes=len(faults), actual_tail_schema_cases=sum('actual_tail' in c['name'] for c in all_cases),
                  ordered_original_reads=sum(len(c['reads']) for c in all_cases), ordered_original_stores=sum(len(c['writes']) for c in all_cases),
                  physical_full_original_instructions=sum(c['original_instructions'] for c in all_cases),
                  physical_original_postcore_instructions=sum(c['original_instructions'] - c['original_prefix_instructions'] for c in all_cases),
                  actual_solver_results=sorted(set(c['actual_solver_result'] for c in all_cases)),
                  image_sha256=NEW_PIN, body_sites=m['body_sites'], listing_sha256=m['listing_sha256'],
                  required_backend=BACKEND, fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size,
                  source_hashes={str(p.relative_to(ROOT)): sha(p) for p in files}, owned_scratch=str(scratch),
                  negative_control='Private native mutation changes actual signed FILD profiling count by1; unchanged physical original must reject ordered store',
                  declared_original_solver_input='ready byte48D480=1; configuration iteration count0 or-1; actual unchanged326A40 executes empty/skip/one-row tail and produces real result+output+FP',
                  retained_large_artifacts=[], excluded=['native solver326A40 closure', 'positive iteration solver schema coverage', 'initializer failure/result0 original continuation', 'unmasked FP traps', 'motion commit kinds beyond fixed324800 execution', 'virtual free2E1110 execution', 'full live constraint/world composition'])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'original_cases', 'full_original_completions', 'read_fault_prefixes', 'ordered_original_stores')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
