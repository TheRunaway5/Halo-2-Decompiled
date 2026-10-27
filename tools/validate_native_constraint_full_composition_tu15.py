#!/usr/bin/env python3
"""Compare complete original326200 with typed setup, solver and motion commits.

The unchanged original executes from the actual root arguments and banks.
Native execution begins at the same root, not at an observed core return.
These declared zero-builder inputs qualify the full owned call composition;
they do not qualify real constraint builders or positive-iteration schemas.
Run through run_test.py. All raw banks and build products are disposable.
"""
import argparse, copy, hashlib, json, os, subprocess, sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import validate_native_constraint_commit_continuation_tu15 as previous

BACKEND = 'unchanged-tu15-full-constraint-native-setup-solver-commit-composition-v1'

def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()

def physical(image, metadata, inputs, scratch):
    owned = scratch / 'physical-original'
    owned.mkdir(exist_ok=True)
    observer = owned / 'observer'
    if not observer.exists():
        subprocess.run(['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_constraint_continuation_tu15.cpp'), '-o', str(observer)], check=True, capture_output=True)
    output = []
    for number, source in enumerate(inputs):
        case = copy.deepcopy(source)
        observed = previous.continuation.observe(image, metadata, case, owned, observer)
        after_banks = {bank['address']: bank['after'] for bank in observed.pop('banks')}
        assert set(after_banks) == {bank['address'] for bank in case['banks']}
        for bank in case['banks']:
            bank['after'] = after_banks[bank['address']]
        service = observed.pop('service_return')
        if 'fault' not in case:
            assert service is not None and service['gp'][0] == 1 and service['x87_tag'] == 0
        case.update(observed, physical_verified=True)
        output.append(case)
        if number and number % 30 == 0:
            print('Full original root/native owned composition cases', number, flush=True)
    return output

def faults(complete):
    output = []
    for owner in (0x3235f0, 0x3249e0, 0x3255d0):
        seed = next(c for c in complete if c['name'] == f'actual_commit_{owner:x}_3_whole_3f000000')
        seen, occurrences = set(), {}
        for read in seed['reads']:
            key = read['address'], read['bytes']
            occurrences[key] = occurrences.get(key, 0) + 1
            if read['instruction'] in seen:
                continue
            seen.add(read['instruction'])
            case = {k: copy.deepcopy(seed[k]) for k in ('name', 'arguments', 'before_fp', 'banks')}
            for bank in case['banks']:
                bank.pop('after')
            case.update(name=seed['name'] + '_whole_fault_' + hex(read['instruction']), stop_pc=read['instruction'],
                        fault=dict(address=read['address'], bytes=read['bytes'], occurrence=occurrences[key]))
            output.append(case)
    return output

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    meta = previous.metadata(image)
    complete = physical(image, meta, previous.cases(image), scratch)
    fault_cases = faults(complete)
    all_cases = complete + physical(image, meta, fault_cases, scratch)
    fixture = scratch / 'full-native-constraint-composition.json'
    fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN, required_backend=BACKEND, cases=all_cases), separators=(',', ':')))
    project = scratch / 'project'
    project.mkdir()
    names = ('native_tu15_constraint_step.cpp', 'native_tu15_constraint_solver.cpp', 'native_tu15_constraint_solver_rows.cpp',
             'native_tu15_constraint_solver_internal.hpp', 'native_tu15_dynamic_accumulator.cpp',
             'native_tu15_motion_forces.cpp', 'native_tu15_motion_commit.cpp')
    sources = [ROOT / 'src/physics' / name for name in names]
    for path in sources:
        (project / path.name).write_bytes(path.read_bytes())
    text = (project / names[0]).read_text()
    assert text.count('if(result==1u)') == 1
    (project / 'mutated.cpp').write_text(text.replace('if(result==1u)', 'if(result==2u)'))
    (project / 'negative.py').write_text('import subprocess,sys\nr=subprocess.run([sys.argv[1],sys.argv[2]],text=True,capture_output=True)\nassert r.returncode!=0 and "ordered full constraint store count differs" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Full unchanged original root rejects missing native commit traversal:",r.stderr.strip())\n')
    cmake = 'cmake_minimum_required(VERSION 3.20)\nproject(native_constraint_full_composition LANGUAGES CXX)\ninclude(CTest)\n'
    for kind in ('original', 'negative'):
        target = 'native_constraint_full_' + kind
        actual = ['mutated.cpp' if kind == 'negative' and p.name == names[0] else p.name for p in sources if p.suffix == '.cpp']
        cpp = ' '.join('"' + str(project / p) + '"' for p in actual)
        cmake += f'add_executable({target} "{ROOT}/tests/native_tu15_constraint_full_composition_tests.cpp" {cpp})\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
        command = f'$<TARGET_FILE:{target}> "{fixture}"' if kind == 'original' else f'"{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{target}> "{fixture}"'
        cmake += f'add_test(NAME {target} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
    (project / 'CMakeLists.txt').write_text(cmake)
    results = []
    for compiler in ('g++', 'clang++'):
        command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--source', str(project), '--scratch-root', str(scratch / 'builds'), '--output', str(args.output / compiler), '--target', 'native_constraint_full_original', '--target', 'native_constraint_full_negative', '--filter', '^native_constraint_full_', '--cmake-arg=-DCMAKE_CXX_COMPILER=' + compiler, '--jobs', '2', '--test-jobs', '1']
        result = subprocess.run(command)
        results.append(dict(compiler=compiler, exit_code=result.returncode, command=command))
    files = sources + [ROOT / 'tests/native_tu15_constraint_full_composition_tests.cpp', Path(previous.__file__),
                       Path(previous.continuation.__file__), ROOT / 'tools/observe_native_constraint_continuation_tu15.cpp', Path(__file__)]
    hashes = {str(p.relative_to(ROOT)): sha(project / p.name if p in sources else p) for p in files}
    report = dict(passed=all(r['exit_code'] == 0 for r in results), compiler_results=results, required_backend=BACKEND,
                  original_cases=len(all_cases), complete_original_calls=len(complete), read_fault_prefixes=len(fault_cases),
                  ordered_original_reads=sum(len(c['reads']) for c in all_cases), ordered_original_stores=sum(len(c['writes']) for c in all_cases),
                  original_instructions=sum(c['original_instructions'] for c in all_cases),
                  image_sha256=NEW_PIN, listing_sha256=meta['listing_sha256'], fixture_sha256=sha(fixture), fixture_bytes=fixture.stat().st_size,
                  source_hashes=hashes, current_sources_match_snapshot=all(sha(ROOT / p) == value for p, value in hashes.items()),
                  owned_scratch=str(scratch), retained_large_artifacts=[],
                  native_owned_call_closure=['326200', '323560/3248D0', '325410/2E4820/3250A0', '326A40/326620/326630', '3235F0/3249E0/3255D0'],
                  actual_core_or_commit_outcome_supplied=False, negative_control='Actual status1 native commit condition changed to2',
                  excluded=['nonzero constraint builders', 'positive-iteration schema composition', 'ready-zero initializer at full326200 entry', 'allocation/free virtual-owner bodies', 'full live physics world'])
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'original_cases', 'complete_original_calls', 'read_fault_prefixes')}))
    return 0 if report['passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
