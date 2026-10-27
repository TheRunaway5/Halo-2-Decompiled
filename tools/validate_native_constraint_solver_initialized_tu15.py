#!/usr/bin/env python3
"""Whole original326A40 versus native solver plus native initializer.

Every case begins with the real ready byte zero. Actual client/key data is
input, never a captured success. Date cases consume only the genuine original
time service return, backed by the declared native Linux FILETIME import.
Use run_test.py; raw private key banks and all build artifacts are disposable.
"""
import argparse, copy, hashlib, json, os, struct, subprocess, sys, time
from pathlib import Path
import validate_native_constraint_solver_tu15 as core
import validate_native_constraint_initializer_tu15 as initialization
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BACKEND = 'unchanged-tu15-whole-solver-native-initializer-composition-v1'
DATE_KEY = 0x1007000
EXTRA_BANKS = ((0x41b668, 4), (0x41f4f0, 0xc0), (0x450108, 4),
               (0x461900, 0x100), (0x47c1b0, 4), (DATE_KEY, 0x100))

def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()

def metadata(image, scratch):
    first = core.metadata(image)
    second = initialization.metadata(image, scratch)
    return dict(sites=first['sites'] + second['sites'],
                body_sites=first['body_sites'] + second['body_sites'],
                listing_sha256={**first['listing_sha256'], **second['listing_sha256']},
                regions=[dict(address=p, bytes=n) for p, n in sorted((*core.BANKS, *EXTRA_BANKS))],
                clock_source_sha256=second['clock_source_sha256'],
                clock_instruction_sha256=second['clock_instruction_sha256'], sdk_sha256=second['sdk_sha256'])

def cases(image):
    original = {c['name']: c for c in core.cases(image, list(range(2, 14)))}
    names = ['finite_0_0'] + [f'finite_{n}_2_0' for n in range(2, 14)]
    names += ['finite_3_3_3_2_0', 'finite_3_3_8_2_0', 'finite_13_12_2_2_0',
              'radial_header_fastpath_7', 'radial_header_fastpath_8',
              'pair3_isolated_control_1_-8', 'pair3_isolated_control_-8_1']
    names += [n for n in original if n.startswith('paired_nan_8_coupling_')][:4]
    out = []
    for name in names:
        case = copy.deepcopy(original[name])
        case['name'] = 'client_' + name
        for address, size in EXTRA_BANKS:
            raw = bytes(size) if address == DATE_KEY else image.read(address, size)
            case['banks'].append(dict(address=address, before=raw.hex()))
        case['banks'].sort(key=lambda b: b['address'])
        core.put(case, 0x48d480, 0, 1)
        case['discover_log_boundary'] = True
        out.append(case)
    out[0]['before_fp'] = [0x27f, 0x4500, 0x1f80]
    wrong = copy.deepcopy(out[0])
    wrong.update(name='actual_wrong_client_hash_logger', stop_pc=0x2e3850)
    core.put(wrong, 0x450108, struct.unpack('<I', image.read(0x450108, 4))[0] ^ 1)
    out.append(wrong)
    mask = struct.unpack('<I', image.read(0x41f4f0, 4))[0]
    today = int(time.time()) // 86400
    for name, source, platform, day in [('date_full_8', 'client_finite_8_2_0', b'PC', 3),
                                         ('date_full_7', 'client_finite_7_2_0', b'PC', 3),
                                         ('date_prefix_P', 'client_finite_12_2_0', b'P', 3),
                                         ('date_empty_platform', 'client_finite_13_2_0', b'', 3),
                                         ('date_actual_expired_logger', 'client_finite_2_2_0', b'PC', -3)]:
        case = copy.deepcopy(next(c for c in out if c['name'] == source))
        value = (((today + day) * 86400) & 0xffffffff) ^ mask
        key = b'composition-' + platform + b'-' + bytes(0x41 + ((value >> (n*4)) & 15) for n in range(8)) + b'\0'
        case.update(name=name, stop_pc=core.STOP)
        core.put(case, 0x450108, 0)
        core.put(case, 0x47c1b0, DATE_KEY)
        bank = next(b for b in case['banks'] if b['address'] == DATE_KEY)
        bank['before'] = (key + bytes(0x100-len(key))).hex()
        out.append(case)
    return out

def physical(image, meta, inputs, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir(exist_ok=True)
    observer = owned / 'observer'
    if not observer.exists():
        subprocess.run(['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_constraint_initializer_tu15.cpp'), '-o', str(observer)], check=True, capture_output=True)
    section = next(s for s in image.sections if s.name == '.text')
    (owned / 'text.bin').write_bytes(image.section_bytes(section))
    for number, case in enumerate(inputs):
        actual_metadata = dict(meta, stop_occurrence=1, stop_at_logger='fault' not in case)
        (owned / 'metadata.json').write_text(json.dumps(actual_metadata, separators=(',', ':')))
        assembly = [f'.section .original,"ax"\n.incbin "{owned / "text.bin"}"']
        segments = [('original', section.va, 5)]
        for n, bank in enumerate(case['banks']):
            path = owned / f'bank{n}.bin'
            path.write_bytes(bytes.fromhex(bank['before']))
            assembly.append(f'.section .bank{n},"aw"\n.incbin "{path}"')
            segments.append((f'bank{n}', bank['address'], 6))
        stack = bytearray(0x10000)
        struct.pack_into('<5I', stack, core.SP-core.STACK, core.STOP, *case['arguments'])
        (owned / 'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned / "stack.bin"}"')
        segments.append(('stack', core.STACK, 6))
        cw, sw, mxcsr = case['before_fp']
        environment = bytearray(28)
        struct.pack_into('<H', environment, 0, cw)
        struct.pack_into('<H', environment, 4, sw)
        struct.pack_into('<H', environment, 8, 0xffff)
        (owned / 'environment.bin').write_bytes(environment)
        assembly.append(f'''.code32
.section .entry,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
 mov ${core.SP},%esp
 int3
 jmp *root
.balign 4
root:.long 0x326a40
sse_control:.long {mxcsr}
environment:.incbin "{owned / 'environment.bin'}"
''')
        assembly.append(initialization.CLOCK_ASSEMBLY)
        segments += [('entry', 0x70000000, 5), ('native_clock', 0x80000080, 5)]
        (owned / 'reference.s').write_text('\n'.join(assembly))
        phdrs = '\n'.join(f'{name} PT_LOAD FLAGS({flags});' for name, _, flags in segments)
        sections = '\n'.join(f'.{name} 0x{address:x} : {{ *(.{name}) }} :{name}' for name, address, _ in segments)
        (owned / 'link.ld').write_text('ENTRY(_start)\nPHDRS {\n' + phdrs + '\n}\nSECTIONS {\n' + sections + '\n}\n')
        subprocess.run(['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')], check=True, capture_output=True)
        subprocess.run(['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o', str(owned / 'reference')], check=True, capture_output=True)
        result = subprocess.run([str(observer), str(owned / 'reference'), str(owned / 'metadata.json'), str(owned / 'observed.json'), hex(case['stop_pc'])], text=True, capture_output=True)
        assert result.returncode == 0, (case['name'], result.stdout, result.stderr)
        observed = json.loads((owned / 'observed.json').read_text())
        for bank, after in zip(case['banks'], observed.pop('banks')):
            assert bank['address'] == after['address']
            bank['after'] = after['after']
        entry, returned = observed.pop('time_entry'), observed.pop('time_return')
        case.update(time_called=entry is not None, actual_time=None, clock_dependency_reads=0)
        if returned is not None:
            filetime = int.from_bytes(bytes.fromhex(returned['filetime']), 'little')
            assert returned['seconds'] == ((filetime-116444736000000000)//10000000) & 0xffffffff
            case['actual_time'] = dict(genuine_original_return=True, seconds=returned['seconds'], fp=returned['fp'], filetime=returned['filetime'])
            assert entry['write_index'] == returned['write_index']
            first, last = entry['read_index'], returned['read_index']
            case['clock_dependency_reads'] = last-first
            observed['reads'] = observed['reads'][:first] + observed['reads'][last:]
        case.update(observed, physical_verified=True)
        if case['boundary_instruction'] == 0x2e3850:
            case['boundary'] = dict(kind='log', message=case['boundary_stack'][1])
        else:
            case['boundary'] = None
            assert case['boundary_instruction'] == case['stop_pc']
        if number and number % 20 == 0:
            print('Full original ready-zero solver cases', number, flush=True)
    return inputs

def faults(cases):
    output = []
    selected = {0x326a4c,0x3268d0,0x3268d9,0x326800,0x326812,0x3268e4,0x326a5d,
                0x326a72,0x326aa0,0x326aa3,0x326aa6,0x326ab7,0x3270a8,0x327104,
                0x327115,0x327636,0x327715,0x32772a,0x3277b3,0x3272ff,0x327432,
                0x327442,0x327477,0x32748a,0x327593,0x3281c9,0x3281f0,0x3286ed,
                0x328738,0x328776,0x326982,0x3269b7}
    seeds = ['client_finite_2_2_0', 'client_finite_7_2_0', 'client_finite_8_2_0', 'date_full_8']
    for name in seeds:
        seed = next(c for c in cases if c['name'] == name)
        seen, occurrences = set(), {}
        for read in seed['reads']:
            key = read['address'], read['bytes']
            occurrences[key] = occurrences.get(key, 0) + 1
            pc = read['instruction']
            if pc not in selected or pc in seen:
                continue
            seen.add(pc)
            case = copy.deepcopy(seed)
            case.update(name=f'{name}_composed_fault_{pc:x}', stop_pc=pc,
                        fault=dict(address=read['address'], bytes=read['bytes'], occurrence=occurrences[key]))
            output.append(case)
    return output

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    project = scratch / 'project'
    project.mkdir()
    paths = [ROOT / 'src/physics' / name for name in ('native_tu15_constraint_solver.cpp', 'native_tu15_constraint_solver_rows.cpp',
              'native_tu15_constraint_solver_internal.hpp', 'native_tu15_constraint_solver_initializer.cpp')]
    for path in paths:
        (project / path.name).write_bytes(path.read_bytes())
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    meta = metadata(image, scratch)
    complete = physical(image, meta, cases(image), scratch)
    fault_cases = faults(complete)
    all_cases = complete + physical(image, meta, fault_cases, scratch)
    fixture = scratch / 'initialized-solver.json'
    fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN, required_backend=BACKEND, body_sites=meta['body_sites'], cases=all_cases), separators=(',', ':')))
    init_source = (project / paths[3].name).read_text()
    assert init_source.count('0xc18f77a2u') == 1
    (project / 'mutated-initializer.cpp').write_text(init_source.replace('0xc18f77a2u', '0xc18f77a3u'))
    root_source = (project / paths[0].name).read_text()
    assert root_source.count('throw;}resume();}') == 1
    (project / 'mutated-fp.cpp').write_text(root_source.replace('throw;}resume();}', 'throw;}fp_.status=0;resume();}'))
    (project / 'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and sys.argv[3] in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Full original entry rejects composed native mutation:",r.stderr.strip())\n')
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(native_initialized_solver_validation LANGUAGES CXX)\ninclude(CTest)\n'
    for name, solver, initializer in [('original', paths[0].name, paths[3].name), ('hash', paths[0].name, 'mutated-initializer.cpp'), ('fp', 'mutated-fp.cpp', paths[3].name)]:
        target = 'native_initialized_solver_' + name
        cmake += f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_solver_initialized_tests.cpp" "{project}/{solver}" "{project}/{paths[1].name}" "{project}/{initializer}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
        expected = 'unexpected initialized solver logger boundary' if name == 'hash' else 'complete initialized solver FP differs'
        command = f'$<TARGET_FILE:{target}> "{fixture}"' if name == 'original' else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}" "{expected}"'
        cmake += f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    for compiler in ('g++', 'clang++'):
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / compiler)]
        for name in ('original', 'hash', 'fp'):
            command += ['--target', 'native_initialized_solver_' + name]
        command += ['--filter', '^native_initialized_solver_', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command)
        results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
    visited = {pc for case in all_cases for pc in case['executed_sites']}
    code = _Code(image, '.text')
    coverage = {}
    for owner in (0x326a40, 0x326640, 0x3268d0):
        body = set(complete_body(code, owner).code)
        coverage[hex(owner)] = dict(total=len(body), visited=len(visited & body), missing=sorted(body - visited))
    files = paths + [ROOT / 'include/halo2/native_tu15_constraint_solver.hpp', ROOT / 'include/halo2/native_tu15_constraint_solver_initializer.hpp',
             ROOT / 'tests/native_tu15_constraint_solver_initialized_tests.cpp', ROOT / 'tools/observe_native_constraint_initializer_tu15.cpp', Path(core.__file__), Path(initialization.__file__), Path(__file__)]
    hashes = {str(path.relative_to(ROOT)): sha(project / path.name if path in paths else path) for path in files}
    report = dict(passed=all(r['exit_code'] == 0 for r in results), required_backend=BACKEND, compiler_results=results,
                  original_cases=len(all_cases), complete_original_calls=sum(c['boundary_instruction'] == core.STOP for c in all_cases),
                  read_fault_prefixes=len(fault_cases), genuine_clock_returns=sum(c['actual_time'] is not None for c in all_cases),
                  genuine_logger_entry_stops=sum(c['boundary_instruction'] == 0x2e3850 for c in all_cases),
                  primary_domain=sorted({kind for case in all_cases for kind in case['primary_types']}),
                  ready_zero_on_every_original_entry=True, ready_or_solver_outcome_supplied=False,
                  ordered_reads=sum(len(c['reads']) for c in all_cases), ordered_stores=sum(len(c['writes']) for c in all_cases),
                  original_instructions=sum(c['original_instructions'] for c in all_cases), original_coverage=coverage,
                  image_sha256=NEW_PIN, listing_sha256=meta['listing_sha256'], clock_source_sha256=meta['clock_source_sha256'],
                  clock_instruction_sha256=meta['clock_instruction_sha256'], sdk_sha256=meta['sdk_sha256'], source_hashes=hashes,
                  current_sources_match_snapshot=all(sha(ROOT / p) == value for p, value in hashes.items()),
                  fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size, owned_scratch=str(scratch),
                  key_text_printed=False, retained_large_artifacts=[],
                  negative_controls=['Native initializer hash mixing seed changed', 'Native solver loses real FP status across actual native initializer callback'],
                  excluded=['original logger body execution', 'actual custom schema14 execution', 'negative/overflowed host clock outputs', 'whole326200/frame/live multiplayer composition'])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'original_cases', 'read_fault_prefixes', 'genuine_clock_returns')}))
    return 0 if report['passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
