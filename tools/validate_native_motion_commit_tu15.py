#!/usr/bin/env python3
"""Unchanged physical TU1.5 motion commit/read/store/RSQRT qualification.

Actual3235F0/3249E0/3255D0 execute all their closed original math on this
physical host. No emulator or result provider supplies estimates or outputs.
All generated original banks, ELF files and compiler products are disposable
run_test.py scratch. Xbox/PentiumIII estimate bits remain a separate gate.
"""
import argparse
import copy
import hashlib
import json
import os
import random
import struct
import subprocess
import sys
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE, MOTION, STEP, ACCUMULATOR = 0x1000000, 0x1000000, 0x1000300, 0x1000400
STACK, SP, STOP = 0x2000000, 0x200f000, 0x3000000
BANKS = ((BASE, 0x800), (0x41f000, 0x1000), (0x46a000, 0x1000))
OWNERS = (0x3235f0, 0x3249e0, 0x3255d0, 0x323430, 0x1e1010, 0x37c5d0, 0x2e5810)
BACKEND = 'unchanged-tu15-motion-commits-physical-operands-rsqrt-full-fp-v1'


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def metadata(image):
    code = _Code(image, '.text')
    registers = {n: i for i, n in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
    sites, counts, listings = [], [], {}
    for entry in OWNERS:
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
                                     bytes=op.size, read=bool(access & CS_AC_READ), write=bool(access & CS_AC_WRITE)))
            sites.append(dict(instruction=pc, operands=operands))
        listings[hex(entry)] = hashlib.sha256(''.join(listing).encode()).hexdigest()
    assert counts == [88, 129, 156, 58, 31, 69, 62], counts
    for table, owner in ((0x41f1e0, 0x3235f0), (0x41f2e0, 0x3249e0), (0x41f360, 0x3255d0)):
        assert struct.unpack('<I', image.read(table + 0x10, 4))[0] == owner
    return dict(sites=sites, body_sites=counts, listing_sha256=listings,
                regions=[dict(address=p, bytes=n) for p, n in BANKS])


def cases(image):
    constants = [dict(address=p, before=image.read(p, n).hex()) for p, n in BANKS[1:]]
    out = []
    def raw(seed, mode=1):
        rng = random.Random(seed)
        data = bytearray(0x800)
        def vector(offset, scale):
            struct.pack_into('<4f', data, offset, *(rng.uniform(-scale, scale) for _ in range(4)))
        for offset, scale in ((0x20, 1), (0x30, 1), (0x40, 4), (0x50, 2), (0x70, 10),
                              (0x80, 1), (0x90, 1), (0xa0, 1),
                              (0x410, 4), (0x420, 2), (0x440, 4), (0x450, 2)):
            vector(offset, scale)
        struct.pack_into('<f', data, 0xc, .25)
        struct.pack_into('<f', data, STEP - BASE, (0, 1/30, 1/60, .5, -.125, 1/120)[mode])
        return data
    def put(data, offset, value):
        struct.pack_into('<I', data, offset, value)
    def add(name, entry, data, parameter=0x3f000000, step=STEP, accumulator=ACCUMULATOR, **kwargs):
        n = len(out)
        out.append(dict(name=f'{entry:x}_{name}', entry=entry, motion=MOTION, step=step,
                        parameter_bits=parameter, accumulator=accumulator,
                        banks=[dict(address=BASE, before=data.hex()), *constants],
                        before_fp=[((0x27f, 0x37f, 0x7f)[n % 3] & ~0xc00) | ((n % 4) << 10),
                                   ((n % 8) << 11) | ((n % 4) << 8),
                                   (0x1f80, 0x3f80, 0x5f80, 0x7f80, 0x9fc0, 0xdfc0)[n % 6]],
                        **kwargs))
    for entry in OWNERS[:3]:
        for seed in range(8):
            for mode in range(6):
                add(f'finite_{seed}_{mode}', entry, raw(seed, mode), parameter=(0, 0x3f000000, 0x3f800000, 0xc0000000)[seed % 4])
        offsets = (STEP - BASE, 0x30, 0x3c, 0x40, 0x50, 0x70, 0xc, 0x20)
        if entry != 0x3235f0:
            offsets += (0x440, 0x450, 0x80, 0x9c)
        for offset in offsets:
            for value in (0x80000000, 1, 0x00800000, 0x7f800000, 0xff800000, 0x7fc12345, 0x7f812345):
                data = raw(11)
                put(data, offset, value)
                add(f'edge_{offset:x}_{value:x}', entry, data)
        for n, (first, second) in enumerate(((0x7fc12345, 0xffc54321), (0x7f812345, 0xffc54321),
                                            (0x7fc12345, 0xff854321), (0x7f812345, 0xff854321))):
            pairs = ((0x30, 0x34), (0x50, 0x30), (STEP - BASE, 0x40), (0x38, 0x3c))
            if entry != 0x3235f0:
                pairs += ((0x450, 0x80), (0x450, 0x90), (0x450, 0x450 + 4), (0x420, 0xa0))
            for left, right in pairs:
                data = raw(n)
                put(data, left, first)
                put(data, right, second)
                add(f'paired_nan_{n}_{left:x}_{right:x}', entry, data)
            data = raw(n)
            put(data, 0x440, second)
            add(f'paired_nan_parameter_{n}', entry, data, parameter=first)
        for offset in (0x40, 0x6c, 0x70, 0x30, 0x50, 0x7c):
            add(f'alias_step_{offset:x}', entry, raw(offset), step=MOTION + offset)
        for offset in (0, 0x10, 0x20, 0x30, 0x40, 0x70, 0x80):
            add(f'alias_accumulator_{offset:x}', entry, raw(offset), accumulator=MOTION + offset)
        for n, quaternion in enumerate(((0, 0, 0, 0), (0, 0, 0, 1), (1, 0, 0, 0),
                                         (0, 1, 0, 0), (0, 0, 1, 0), (1e-30, 1e-30, 1e-30, 1e-30))):
            data = raw(4)
            struct.pack_into('<4f', data, 0x30, *quaternion)
            add(f'quaternion_{n}', entry, data)
        add('read_fault_seed', entry, raw(3))
    for address in (0xf0000000, 0xfffffff0):
        add(f'unread_accumulator_{address:x}', 0x3235f0, raw(7), parameter=0x7f812345, accumulator=address)
    return out


def physical(image, m, inputs, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir(exist_ok=True)
    observer = owned / 'observer'
    if not observer.exists():
        subprocess.run(['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_constraint_step_tu15.cpp'), '-o', str(observer)], check=True, capture_output=True)
    section = next(s for s in image.sections if s.name == '.text')
    text = owned / 'text.bin'
    text.write_bytes(image.section_bytes(section))
    (owned / 'metadata.json').write_text(json.dumps(m, separators=(',', ':')))
    for case in inputs:
        sections = [f'.original 0x{section.va:x} : {{ *(.original) }}']
        assembly = [f'.section .original,"ax"\n.incbin "{text}"']
        for n, bank in enumerate(case['banks']):
            path = owned / f'bank{n}.bin'
            path.write_bytes(bytes.fromhex(bank['before']))
            assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"')
            sections.append(f'.bank{n} 0x{bank["address"]:x} : {{ *(.bank{n}) }}')
        stack = bytearray(0x10000)
        struct.pack_into('<4I', stack, SP - STACK, STOP, case['step'], case['parameter_bits'], case['accumulator'])
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
 mov ${MOTION},%ecx
 mov ${SP},%esp
 int3
 jmp *entry
.balign 4
entry: .long {case['entry']}
sse_control: .long {mxcsr}
environment: .incbin "{owned / 'environment.bin'}"
''')
        sections.append('.bridge 0x70000000 : { *(.bridge) }')
        (owned / 'reference.s').write_text('\n'.join(assembly))
        (owned / 'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n' + '\n'.join(sections) + '\n}\n')
        subprocess.run(['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')], check=True, capture_output=True)
        subprocess.run(['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o', str(owned / 'reference')], check=True, capture_output=True)
        result = subprocess.run([str(observer), str(owned / 'reference'), str(owned / 'metadata.json'), str(owned / 'output.json'), hex(case.get('stop_pc', STOP))], text=True, capture_output=True)
        assert result.returncode == 0, (case['name'], result.stdout, result.stderr)
        observed = json.loads((owned / 'output.json').read_text())
        for bank, after in zip(case['banks'], observed.pop('banks')):
            assert bank['address'] == after['address']
            bank['after'] = after['after']
        case.update(observed, physical_verified=True)
        if 'fault' not in case:
            assert case['boundary_gp'][4] == SP + 16, (case['name'], 'Actual RET12 boundary changed')
    return inputs


def fault_cases(inputs):
    out = []
    for seed in (c for c in inputs if c['name'].endswith('read_fault_seed')):
        seen, occurrences = set(), {}
        for read in seed['reads']:
            key = read['address'], read['bytes']
            occurrences[key] = occurrences.get(key, 0) + 1
            if read['instruction'] in seen:
                continue
            seen.add(read['instruction'])
            case = copy.deepcopy(seed)
            case.update(name=seed['name'] + '_fault_' + hex(read['instruction']), stop_pc=read['instruction'],
                        fault=dict(address=read['address'], bytes=read['bytes'], occurrence=occurrences[key]))
            out.append(case)
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
    faults = fault_cases(all_cases)
    all_cases += physical(image, m, faults, scratch)
    fixture_value = dict(image_sha256=NEW_PIN, required_backend=BACKEND, body_sites=m['body_sites'], listing_sha256=m['listing_sha256'], cases=all_cases)
    fixture = scratch / 'commits.json'
    fixture.write_text(json.dumps(fixture_value, separators=(',', ':')))
    project = scratch / 'project'
    project.mkdir()
    source = ROOT / 'src/physics/native_tu15_motion_commit.cpp'
    text = source.read_text()
    mutation = 'const auto estimate=_mm_rsqrt_ss(sum);'
    assert text.count(mutation) == 1
    (project / 'mutated.cpp').write_text(text.replace(mutation, 'const auto estimate=_mm_div_ss(broadcast(0x3f800000u),_mm_sqrt_ss(sum));'))
    restore_mutation = 's.restored_linear_velocity(s.accumulator_saved_linear());'
    assert text.count(restore_mutation) == 1
    (project / 'restore-mutated.cpp').write_text(text.replace(restore_mutation, 's.restored_linear_velocity(words(mul_ps(broadcast(parameter_bits),simd(s.accumulator_saved_linear()))));'))
    cursor_mutation = 's.accumulator().identity+0x80u'
    assert text.count(cursor_mutation) == 1
    (project / 'cursor-mutated.cpp').write_text(text.replace(cursor_mutation, 's.accumulator().identity+0x70u'))
    (project / 'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"native RSQRTSS estimate differs"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects precise reciprocal-root substitution:",r.stderr.strip())
''')
    (project / 'restore-negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"ordered commit store differs"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects scaling the unscaled saved velocity restoration:",r.stderr.strip())
''')
    (project / 'cursor-negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"actual source commit cursor differs"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Physical original rejects changed accumulator cursor stride:",r.stderr.strip())
''')
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(native_motion_commit_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name, src in (('native_motion_commit_tests', source), ('native_motion_commit_mutation', project / 'mutated.cpp'), ('native_motion_commit_restore_mutation', project / 'restore-mutated.cpp'), ('native_motion_commit_cursor_mutation', project / 'cursor-mutated.cpp')):
        cmake += f'add_executable({name} "{ROOT}/tests/native_tu15_motion_commit_tests.cpp" "{src}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake += f'add_test(NAME native_motion_commit_original COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_motion_commit_tests> "{fixture}")\nadd_test(NAME native_motion_commit_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_motion_commit_mutation> "{fixture}")\n'
    cmake += f'add_test(NAME native_motion_commit_restore_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/restore-negative.py" $<TARGET_FILE:native_motion_commit_restore_mutation> "{fixture}")\nadd_test(NAME native_motion_commit_cursor_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/cursor-negative.py" $<TARGET_FILE:native_motion_commit_cursor_mutation> "{fixture}")\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    for compiler in ('g++', 'clang++'):
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / compiler), '--target', 'native_motion_commit_tests', '--target', 'native_motion_commit_mutation', '--target', 'native_motion_commit_restore_mutation', '--target', 'native_motion_commit_cursor_mutation', '--filter', '^native_motion_commit_(original|negative|restore_negative|cursor_negative)$', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command, check=False)
        results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
    files = (source, ROOT / 'include/halo2/native_tu15_motion_commit.hpp', ROOT / 'tests/native_tu15_motion_commit_tests.cpp', ROOT / 'tools/observe_native_constraint_step_tu15.cpp', Path(__file__))
    report = dict(passed=all(r['exit_code'] == 0 for r in results), compiler_results=results,
                  original_cases=len(all_cases), complete_original_calls=len(all_cases) - len(faults),
                  cases_by_owner={hex(entry): sum(c['entry'] == entry for c in all_cases) for entry in OWNERS[:3]},
                  read_fault_prefixes=len(faults), paired_nan_cases=sum('paired_nan' in c['name'] for c in all_cases),
                  ordered_original_reads=sum(len(c['reads']) for c in all_cases), ordered_original_stores=sum(len(c['writes']) for c in all_cases),
                  physical_original_instructions=sum(c['original_instructions'] for c in all_cases),
                  native_rsqrt_observations=sum(len(c['rsqrt_estimates']) for c in all_cases),
                  image_sha256=NEW_PIN, body_sites=m['body_sites'], listing_sha256=m['listing_sha256'],
                  required_backend=BACKEND, fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size,
                  source_hashes={str(p.relative_to(ROOT)): sha(p) for p in files}, owned_scratch=str(scratch),
                  negative_control='Private native mutation substitutes precise reciprocal square root for actual native RSQRTSS; unchanged original must reject observed estimate',
                  additional_negative_controls=['Private native mutation scales the source unscaled saved linear velocity restoration; unchanged original must reject ordered store', 'Private native mutation changes actual accumulator cursor stride80 to70; unchanged original must reject cursor'],
                  retained_large_artifacts=[], excluded=['Xbox/PentiumIII-specific RSQRT estimate bits', 'unmasked platform traps', 'full live constrained solver/world composition'])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'original_cases', 'read_fault_prefixes', 'paired_nan_cases', 'ordered_original_stores')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
