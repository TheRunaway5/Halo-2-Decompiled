#!/usr/bin/env python3
"""Compose native constrained motion commits after genuine original core returns.

Complete original326200 executes actual kinematic/dynamic accumulator setup,
actual326A40 and actual motion commits. Native postcore starts only from its
observed true service-return state and executes the real typed commit kernels.
No core/commit outcome is manufactured. Positive solver iterations stay open.
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
from oracle_smg_firing_1_1 import ROOT, Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import validate_native_constraint_continuation_tu15 as continuation
import validate_native_motion_commit_tu15 as commits
import validate_native_dynamic_accumulator_tu15 as dynamic

BACKEND = 'unchanged-tu15-full-constraint-genuine-return-native-motion-commits-v1'


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def metadata(image):
    m = continuation.metadata(image)
    constants = dict(address=0x46a000, bytes=0x1000)
    m['regions'].append(constants)
    seen = {site['instruction'] for site in m['sites']}
    for child in (commits.metadata(image), dynamic.metadata(image)):
        for site in child['sites']:
            if site['instruction'] not in seen:
                m['sites'].append(site)
                seen.add(site['instruction'])
        m['listing_sha256'].update(child['listing_sha256'])
    return m


def cases(image):
    out = []
    originals = continuation.cases(image)
    for table, owner in ((0x41f1e0, 0x3235f0), (0x41f2e0, 0x3249e0), (0x41f360, 0x3255d0)):
        for bodies in (1, 3):
            for mode in ('fits', 'whole', 'jacobians', 'schemas'):
                for parameter in (0, 0x3f000000, 0x3f800000):
                    selected = next(c for c in originals if c['name'] == f'fixed_{bodies}_{mode}_0_profile')
                    c = copy.deepcopy(selected)
                    c['name'] = f'actual_commit_{owner:x}_{bodies}_{mode}_{parameter:x}'
                    c['banks'].append(dict(address=0x46a000, before=image.read(0x46a000, 0x1000).hex()))
                    c['banks'].sort(key=lambda bank: bank['address'])
                    def put(p, value, n=4):
                        continuation.put(c, p, value, n)
                    def number(p, value):
                        put(p, struct.unpack('<I', struct.pack('<f', value))[0])
                    def vector(p, values):
                        for lane, value in enumerate(values):
                            number(p + lane*4, value)
                    number(continuation.STEP, .125)
                    put(continuation.SOLVER + 0x20, parameter)
                    vector(continuation.BASE + 0x900, (0, 0, -9.8, 0))
                    for n in range(bodies):
                        motion = continuation.MOTION + n*0x300
                        put(continuation.BODY + n*0x100 + 0x3c, motion)
                        put(motion, table)
                        number(motion + 0xc, .25)
                        number(motion + 0x14, .125)
                        number(motion + 0x18, .25)
                        number(motion + 0x2c, .75)
                        vector(motion + 0x20, (.25, -.5, .125, .75))
                        vector(motion + 0x30, (0, 0, 0, 1))
                        vector(motion + 0x40, (.25 + n, -1, 2, 0))
                        vector(motion + 0x50, (.5, 1 + n, -.75, 0))
                        vector(motion + 0x70, (3 + n, 4, 5, 1))
                        vector(motion + 0xe0, (.5, .25, .125, 2))
                        vector(motion + 0xf0, (1, -2, .5, 0))
                        vector(motion + 0x100, (-.25, .125, .5, n % 2))
                        rows = ((0, 1, 0, 0), (-1, 0, 0, 0), (0, 0, 1, 0)) if n % 2 else ((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0))
                        for axis, row in enumerate(rows):
                            vector(motion + 0x80 + axis*16, row)
                    out.append(c)
        seed = next(c for c in out if c['name'] == f'actual_commit_{owner:x}_1_fits_3f000000')
        for index, (first, second) in enumerate(((0x7fc12345, 0xffc54321), (0x7f812345, 0xff854321))):
            c = copy.deepcopy(seed)
            c['name'] = f'paired_nan_commit_{owner:x}_{index}'
            continuation.put(c, continuation.SOLVER + 0x20, first)
            continuation.put(c, continuation.MOTION + 0x70, second)
            out.append(c)
        for offset in (0x40, 0x6c, 0x70):
            c = copy.deepcopy(seed)
            c['name'] = f'alias_step_commit_{owner:x}_{offset:x}'
            c['arguments'][0] = continuation.MOTION + offset
            continuation.put(c, continuation.MOTION + offset, 0x3e000000)
            out.append(c)
    return out


def actual_row_cases(observed):
    out = []
    for owner in (0x3235f0, 0x3249e0, 0x3255d0):
        seed = next(c for c in observed if c['name'] == f'actual_commit_{owner:x}_1_fits_3f000000')
        schema, slots = seed['solver_return_local_words'][12], seed['solver_return_local_words'][11]
        for payload in (0x3f400000, 0x7fc12345, 0x7f812345):
            c = dict(name=f'actual_core_row_commit_{owner:x}_{payload:x}',
                     banks=copy.deepcopy(seed['original_root_banks']), arguments=seed['arguments'],
                     before_fp=seed['original_fp_before'], stop_pc=continuation.STOP)
            continuation.put(c, schema + 4, continuation.BASE + 0xc000)
            continuation.put(c, schema + 8, seed['continuation']['first_accumulator'] + 0x40)
            continuation.put(c, schema + 0x14, 8)
            continuation.put(c, schema + 0x18, 2, 1)
            continuation.put(c, schema + 0x19, 2, 1)
            continuation.put(c, slots, payload)
            continuation.put(c, continuation.BASE + 0xc00c, 0x3f000000)
            continuation.put(c, continuation.SOLVER + 0x10, 0x3f800000)
            out.append(c)
    return out


def fault_cases(observed):
    out = []
    for owner in (0x3235f0, 0x3249e0, 0x3255d0):
        seed = next(c for c in observed if c['name'] == f'actual_commit_{owner:x}_3_whole_3f000000')
        seen, occurrences = set(), {}
        for read in seed['reads']:
            key = read['address'], read['bytes']
            occurrences[key] = occurrences.get(key, 0) + 1
            if read['instruction'] in seen:
                continue
            seen.add(read['instruction'])
            out.append(dict(name=seed['name'] + '_fault_' + hex(read['instruction']),
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
    all_cases = continuation.physical(image, m, cases(image), scratch)
    all_cases += continuation.physical(image, m, actual_row_cases(all_cases), scratch)
    faults = fault_cases(all_cases)
    all_cases += continuation.physical(image, m, faults, scratch)
    fixture = scratch / 'constraint-commit-continuations.json'
    fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN, required_backend=BACKEND,
                                     root_sites=306, core_sites=2208, commit_sites=[88,129,156],
                                     listing_sha256=m['listing_sha256'], cases=all_cases), separators=(',', ':')))
    project = scratch / 'project'
    project.mkdir()
    root_source = ROOT / 'src/physics/native_tu15_constraint_step.cpp'
    commit_source = ROOT / 'src/physics/native_tu15_motion_commit.cpp'
    mutation = 'if(result==1u)'
    source_text = root_source.read_text()
    assert source_text.count(mutation) == 1
    (project / 'mutated.cpp').write_text(source_text.replace(mutation, 'if(result==2u)'))
    (project / 'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)
assert r.returncode!=0 and"ordered composed constraint store count differs"in r.stderr,(r.returncode,r.stdout,r.stderr)
print("Genuine original core status rejects omitted native commit traversal:",r.stderr.strip())
''')
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(native_constraint_commit_composition LANGUAGES CXX)\ninclude(CTest)\n'
    for name, source in (('native_constraint_commit_composition_tests', root_source), ('native_constraint_commit_composition_mutation', project / 'mutated.cpp')):
        cmake += f'add_executable({name} "{ROOT}/tests/native_tu15_constraint_commit_continuation_tests.cpp" "{source}" "{commit_source}")\ntarget_include_directories({name} PRIVATE "{ROOT}/include")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
    cmake += f'add_test(NAME native_constraint_commit_composition_original COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:native_constraint_commit_composition_tests> "{fixture}")\nadd_test(NAME native_constraint_commit_composition_negative COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:native_constraint_commit_composition_mutation> "{fixture}")\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    for compiler in ('g++', 'clang++'):
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / compiler), '--target', 'native_constraint_commit_composition_tests', '--target', 'native_constraint_commit_composition_mutation', '--filter', '^native_constraint_commit_composition_(original|negative)$', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command, check=False)
        results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
    files = (root_source, ROOT/'include/halo2/native_tu15_constraint_step.hpp', commit_source,
             ROOT/'include/halo2/native_tu15_motion_commit.hpp', ROOT/'tests/native_tu15_constraint_commit_continuation_tests.cpp',
             ROOT/'tools/observe_native_constraint_continuation_tu15.cpp', ROOT/'tools/validate_native_constraint_continuation_tu15.py',
             ROOT/'tools/validate_native_motion_commit_tu15.py', ROOT/'tools/validate_native_dynamic_accumulator_tu15.py', Path(__file__))
    report = dict(passed=all(r['exit_code'] == 0 for r in results), compiler_results=results,
                  original_cases=len(all_cases), complete_original_calls=len(all_cases)-len(faults), read_fault_prefixes=len(faults),
                  actual_one_row_core_output_cases=sum(c['name'].startswith('actual_core_row') for c in all_cases),
                  ordered_original_stores=sum(len(c['writes']) for c in all_cases), ordered_original_reads=sum(len(c['reads']) for c in all_cases),
                  full_original_instructions=sum(c['original_instructions'] for c in all_cases),
                  postcore_original_instructions=sum(c['original_instructions']-c['original_prefix_instructions'] for c in all_cases),
                  image_sha256=NEW_PIN, required_backend=BACKEND, listing_sha256=m['listing_sha256'],
                  fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size,
                  source_hashes={str(p.relative_to(ROOT)):sha(p) for p in files}, owned_scratch=str(scratch), retained_large_artifacts=[],
                  negative_control='Private native root mutation changes genuine status1 commit condition to2; unchanged original rejects omitted commit stores',
                  excluded=['native core326A40 closure', 'positive-iteration solver schemas', 'original initializer status0', 'unmasked platform traps', 'Xbox/PentiumIII-specific RSQRT estimate bits', 'full live world composition'])
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','original_cases','read_fault_prefixes','ordered_original_stores')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
