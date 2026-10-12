#!/usr/bin/env python3
"""Freeze and test the native owner against actual original Xbox sound calls."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

from oracle_mp_title_update_display import ROOT, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fixture-directory', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    fixture = args.fixture_directory.resolve()
    provenance = json.loads((fixture / 'provenance.json').read_text())
    assert sha(fixture / 'fixtures.tsv') == provenance['fixture_sha256']
    roots = ['src/sound_admission.cpp', 'src/sound_driver_parameters.cpp', 'tests/live_sound_admission_tests.cpp']
    files, pending = set(roots), list(roots)
    while pending:
        for name in re.findall(r'#include\s+"(halo2/[^\"]+)"', (ROOT / pending.pop()).read_text()):
            path = 'include/' + name
            if path not in files:
                files.add(path)
                pending.append(path)
    for name in sorted(files):
        target = output / 'source' / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / name, target)
    shutil.copytree(fixture, output / 'fixture')
    commands = []
    env = dict(os.environ, TMPDIR=str(output))
    def run(argv, label, expected=0, environment=None):
        result = subprocess.run(argv, cwd=output, env=environment or env, capture_output=True, text=True, timeout=60)
        for suffix, value in [('stdout', result.stdout), ('stderr', result.stderr)]:
            (output / f'{label}.{suffix}').write_text(value)
        commands.append(dict(argv=argv, label=label, exit_code=result.returncode,
                             stdout_sha256=sha(output / f'{label}.stdout'), stderr_sha256=sha(output / f'{label}.stderr')))
        assert result.returncode == expected, (label, result.returncode, result.stdout, result.stderr)
        return result
    run(['g++', '--version'], 'compiler-version')
    common = ['g++', '-std=c++20', '-pthread', '-Wall', '-Wextra', '-Wpedantic',
              '-ffp-contract=off', '-fno-fast-math', '-frounding-math', '-I'+str(output / 'source/include')]
    inputs = [str(output / 'source' / name) for name in roots]
    normal, sanitized = output / 'native', output / 'native-asan'
    run([*common, '-O2', *inputs, '-o', str(normal)], 'normal-build')
    run([str(normal), str(output / 'fixture/fixtures.tsv')], 'normal')
    run([*common, '-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', *inputs,
         '-o', str(sanitized)], 'sanitizer-build')
    run([str(sanitized), str(output / 'fixture/fixtures.tsv')], 'sanitizer', environment=dict(
        env, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
    # Reconstruct only the previously corrected distance operand error in an
    # isolated diagnostic source. The production source is never reverted.
    driver = output / 'source/src/sound_driver_parameters.cpp'
    before = driver.read_text()
    old = before.replace('const auto denominator=minimum>distance?minimum:distance;',
                         'const auto denominator=maximum>minimum?maximum:minimum;')
    old = old.replace('_mm_div_ss(_mm_set_ss(minimum),_mm_set_ss(denominator))',
                      '_mm_div_ss(_mm_set_ss(maximum),_mm_set_ss(denominator))')
    assert old != before and old.count('const auto denominator=maximum>minimum?maximum:minimum;') == 1
    control = output / 'pre-correction-distance.cpp'
    control.write_text(old)
    red = output / 'pre-correction-native'
    red_inputs = [str(control) if name == 'src/sound_driver_parameters.cpp' else str(output / 'source' / name) for name in roots]
    run([*common, '-O2', *red_inputs, '-o', str(red)], 'pre-correction-build')
    result = run([str(red), str(output / 'fixture/fixtures.tsv')], 'pre-correction', expected=1)
    assert 'numeric entry 12c370' in result.stderr and 'Actual game numeric admission return differs' in result.stderr
    assert all(sha(ROOT / name) == sha(output / 'source' / name) for name in files)
    report = dict(boundary=__doc__, validator_sha256=sha(__file__), commands=commands,
                  source_sha256={name: sha(output / 'source' / name) for name in sorted(files)},
                  fixture_sha256=sha(output / 'fixture/fixtures.tsv'),
                  provenance_sha256=sha(output / 'fixture/provenance.json'),
                  binary_sha256={p.name: sha(p) for p in [normal, sanitized, red]},
                  regression_control='Previously corrected distance numerator/denominator only; isolated source copy.',
                  limits=['CPU admission and original internal numeric returns, not live native audio routing or playback.'])
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print((output / 'normal.stdout').read_text().strip())
    print('ASan/UBSan pass; prior distance operand error fails on an actual original game request.')


if __name__ == '__main__':
    main()
