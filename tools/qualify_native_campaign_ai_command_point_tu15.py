#!/usr/bin/env python3
"""Owned 1FC720 compiler, mutation and sanitizer checks with borrowed closure."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path('src/campaign/ai/native_campaign_ai_publication_tu15.cpp')
DRIVER = Path('tests/native_campaign_ai_publication_tu15_tests.cpp')
MUTANTS = (
    ('ground_navigation_gate', 'if(!r(row+0x229u,1) && r(entry+8u)==0xffffffffu)', 'if(false && !r(row+0x229u,1) && r(entry+8u)==0xffffffffu)'),
    ('same_reference', 'if(r(row+0x4c8u)==reference)', 'if(r(row+0x4c8u)!=reference)'),
    ('distance_restart', 'restart=comiss_above(fp.mxcsr,distance,r(0x46a560u));', 'restart=!comiss_above(fp.mxcsr,distance,r(0x46a560u));'),
    ('completion_gate', 'if(!r(row+0x40u,1) || r(row+0x506u,1))return 1;\n                u32 call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc7bau);',
     'if(!r(row+0x40u,1) || !r(row+0x506u,1))return 1;\n                u32 call_sp=sp;push(call_sp,0);push(call_sp,actor);push(call_sp,0x1fc7bau);'),
    ('flag_value', 'w(0x1fc818u,row+0x4aeu,flag,1);', 'w(0x1fc818u,row+0x4aeu,0,1);'),
    ('object_value', 'w(0x1fc84cu,row+0x4e4u,object,4);', 'w(0x1fc84cu,row+0x4e4u,0xffffffffu,4);'),
    ('flying_reset', 'if(flying)w(0x1fc862u,row+0x656u,0,2);', 'if(false && flying)w(0x1fc862u,row+0x656u,0,2);'),
    ('callee_pop', 'movement.command_point(sp,caller));cleanup=0x14u;', 'movement.command_point(sp,caller));cleanup=0x10u;'),
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', required=True, type=Path)
    parser.add_argument('--fixture', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--baseline-only', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    build = args.build.resolve()
    assert (build.parent / 'source').exists() and '.local/test-scratch/' in str(build), 'Only an owned build may supply dependencies'
    fixture = scratch / 'fixture.json'
    fixture.write_bytes(gzip.decompress(args.fixture.read_bytes()))
    link = shlex.split((build / 'CMakeFiles/native_campaign_ai_publication_tu15_tests.dir/link.txt').read_text())
    link = [x for x in link if not x.startswith('-Wl,--dependency-file=')]
    for n, value in enumerate(link):
        if not value.startswith('-') and not Path(value).is_absolute() and value.endswith(('.o', '.a')):
            link[n] = str(build / value)
    source_index = next(n for n, value in enumerate(link) if value.endswith('/' + str(SOURCE) + '.o'))
    driver_index = next(n for n, value in enumerate(link) if value.endswith('/' + str(DRIVER) + '.o'))
    output_index = link.index('-o') + 1
    original = (ROOT / SOURCE).read_text()
    report = dict(passed=False, results=[], commands=[], source_sha256={str(p): sha(ROOT / p) for p in
                  (SOURCE, DRIVER, Path(__file__).relative_to(ROOT))}, fixture_sha256=sha(args.fixture),
                  owned_scratch=str(scratch), borrowed_owned_build=str(build), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)

    def save():
        args.report.write_text(json.dumps(report, indent=2) + '\n')

    def run(command, expected=0, env=None):
        report['commands'].append(command)
        result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, env=env)
        if expected == 0 and result.returncode != 0:
            report['failure'] = dict(command=command, returncode=result.returncode,
                                     stdout=result.stdout[-8000:], stderr=result.stderr[-8000:])
            save()
            raise RuntimeError(result.stdout[-8000:] + result.stderr[-8000:])
        return result

    configurations = [('baseline', None, None)]
    if not args.baseline_only:
        configurations += list(MUTANTS) + [('asan_ubsan', None, None)]
    for name, old, new in configurations:
        changed = original
        if old is not None:
            assert original.count(old) == 1, (name, original.count(old))
            changed = original.replace(old, new)
        source = scratch / (name + '.cpp')
        obj = scratch / (name + '.o')
        driver = scratch / ('sanitized-driver.o' if name == 'asan_ubsan' else 'driver.o')
        probe = scratch / (name + '.probe')
        source.write_text(changed)
        sanitizer = name == 'asan_ubsan'
        options = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if sanitizer else []
        common = ['c++', '-std=c++20', '-O1' if sanitizer else '-O2', '-fno-fast-math', '-ffp-contract=off',
                  '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                  '-I' + str(ROOT / 'src/campaign/ai'), *options]
        run([*common, '-c', str(source), '-o', str(obj)])
        if not driver.exists():
            run([*common, '-c', str(ROOT / DRIVER), '-o', str(driver)])
        command = link.copy()
        command[source_index] = str(obj)
        command[driver_index] = str(driver)
        command[output_index] = str(probe)
        run(command + options)
        result = run([str(probe), str(fixture)], expected=1 if old else 0,
                     env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
        if old:
            assert result.returncode != 0, name + ' survived'
        report['results'].append(dict(name=name, passed=True, rejected=bool(old), returncode=result.returncode,
                                      stdout=result.stdout[-2000:], stderr=result.stderr[-2000:]))
        save()
        print(json.dumps(report['results'][-1]), flush=True)
    assert sha(ROOT / SOURCE) == report['source_sha256'][str(SOURCE)], 'Publication source changed during qualification'
    report['passed'] = True
    save()


if __name__ == '__main__':
    main()
