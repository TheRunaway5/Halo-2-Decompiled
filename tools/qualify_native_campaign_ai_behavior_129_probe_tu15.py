#!/usr/bin/env python3
"""Owned behavior129 probe compiler, mutation and sanitizer checks with borrowed native closure."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path('src/campaign/ai/native_campaign_ai_behavior_129_probe_tu15.cpp')
DRIVER = Path('tests/native_campaign_ai_behavior_129_probe_tu15_tests.cpp')
MUTANTS = (
    ('confidence','std::uint8_t(r(row+0x27u,1)))<2','std::uint8_t(r(row+0x27u,1)))<3'),
    ('nearest_comparison','compare(best_distance,r(row+0x28u),true)','compare(r(row+0x28u),best_distance,true)'),
    ('nearest_position','perception_position_tu15(memory,record)+4u','perception_position_tu15(memory,record)+8u'),
    ('normalization_fallback','if(fallback)point(direction','if(!fallback)point(direction'),
    ('fan_diagonal','directions[1]={add(sny,sx)','directions[1]={sub(sny,sx)'),
    ('fan_cross_axis','six_x=sub(tx,sny)','six_x=add(tx,sny)'),
    ('request_argument','w(0,sp-4u,request)','w(0,sp-4u,actor)'),
    ('no_target_return','abi.gp[0]=0xffffff00u','abi.gp[0]=0xffffff01u'),
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
    target='native_campaign_ai_behavior_129_probe_tu15_tests'
    target_driver=DRIVER
    if not (build / ('CMakeFiles/'+target+'.dir/link.txt')).exists():
        target='native_campaign_hs_command_builtins_tu15_tests'
        target_driver=Path('tests/native_campaign_hs_command_builtins_tu15_tests.cpp')
    link = shlex.split((build / ('CMakeFiles/'+target+'.dir/link.txt')).read_text())
    link = [x for x in link if not x.startswith('-Wl,--dependency-file=')]
    for n, value in enumerate(link):
        if not value.startswith('-') and not Path(value).is_absolute() and value.endswith(('.o', '.a')):
            link[n] = str(build / value)
    source_index = next((n for n, value in enumerate(link) if value.endswith('/' + str(SOURCE) + '.o')),None)
    driver_index = next(n for n, value in enumerate(link) if value.endswith('/' + str(target_driver) + '.o'))
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
        command[driver_index] = str(driver)
        command[output_index] = str(probe)
        if source_index is None:
            # The runtime archive owns this production source. An explicit
            # replacement before the archive resolves its symbols, leaving
            # that archive member unextracted; every other child is borrowed.
            command.insert(1,str(obj))
        else:
            command[source_index] = str(obj)
        run(command + options)
        result = run([str(probe), str(fixture)], expected=1 if old else 0,
                     env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0:halt_on_error=1', 'UBSAN_OPTIONS': 'halt_on_error=1'})
        if old:
            assert result.returncode != 0, name + ' survived'
        report['results'].append(dict(name=name, passed=True, rejected=bool(old), returncode=result.returncode,
                                      stdout=result.stdout[-2000:], stderr=result.stderr[-2000:]))
        save()
        print(json.dumps(report['results'][-1]), flush=True)
        for generated in (probe, obj, source):
            generated.unlink()
    assert sha(ROOT / SOURCE) == report['source_sha256'][str(SOURCE)], 'HS builtin source changed during qualification'
    report['passed'] = True
    save()


if __name__ == '__main__':
    main()
