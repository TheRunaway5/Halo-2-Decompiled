#!/usr/bin/env python3
"""Bounded mutation/sanitizer controls borrowing an active owned CMake build.

Invoke through tools/run_test.py. The borrowed build is read-only and must stay
stable while this command runs; all replacement objects and binaries are scratch.
"""
import argparse
import hashlib
import gzip
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MOVEMENT = 'src/campaign/ai/native_campaign_ai_command_movement_tu15.cpp'
VECTOR = 'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp'
DRIVER = 'tests/native_campaign_ai_command_movement_tu15_tests.cpp'
TARGET = 'native_campaign_ai_command_movement_tu15_tests'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--borrowed-build', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    build = args.borrowed_build.resolve()
    if not build.is_relative_to(ROOT / '.local/test-scratch') or build == scratch:
        raise ValueError('Expected a distinct active owned CMake build')
    source = {path: (ROOT / path).read_text() for path in (MOVEMENT, VECTOR, DRIVER)}
    fixture = scratch / 'fixture.json'
    fixture.write_bytes(gzip.decompress(args.fixture.read_bytes()))
    digest = lambda value: hashlib.sha256(value).hexdigest()
    report = dict(scratch=str(scratch), borrowed_build=str(build),
                  fixture_sha256=digest(args.fixture.read_bytes()),
                  source_sha256={p: digest(s.encode()) for p, s in source.items()},
                  mutations=[], passed=False, retained_binaries=False)
    link = shlex.split((build / f'CMakeFiles/{TARGET}.dir/link.txt').read_text())
    link = [x for x in link if not x.startswith('-Wl,--dependency-file=')]
    for i, arg in enumerate(link):
        if not arg.startswith('-') and arg.endswith(('.o', '.a')):
            link[i] = str((build / arg).resolve())
    flags = ['-std=c++20', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src')]

    def run(command, env=None):
        done = subprocess.run(command, cwd=scratch, capture_output=True, text=True,
                              timeout=240, env=env)
        return dict(returncode=done.returncode, stdout=done.stdout[-6000:], stderr=done.stderr[-6000:])

    def compile_and_run(name, replacements, sanitizer=False):
        folder = scratch / name
        folder.mkdir()
        command = list(link)
        extra = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'] if sanitizer else ['-O2']
        for path, text in replacements.items():
            cpp = folder / Path(path).name
            obj = cpp.with_suffix('.o')
            cpp.write_text(text)
            compiled = run(['/usr/bin/c++', *flags, *extra, '-c', str(cpp), '-o', str(obj)])
            if compiled['returncode']:
                raise RuntimeError(f'{name} compile failed: {compiled}')
            original = str(build / f'CMakeFiles/{TARGET}.dir/{path}.o')
            command[command.index(original)] = str(obj)
        binary = folder / 'replay'
        command[command.index('-o') + 1] = str(binary)
        if sanitizer:
            command[1:1] = ['-fsanitize=address,undefined']
        compiled = run(command)
        if compiled['returncode']:
            raise RuntimeError(f'{name} link failed: {compiled}')
        env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        return run([str(binary), str(fixture)], env)

    mutants = [
        ('second-point-validation', MOVEMENT, 'if(!valid(face))return 2;', 'if(false)return 2;'),
        ('state-counter-reset', MOVEMENT, 'w(0x262aebu,state,0,2)', 'w(0x262aebu,state,1,2)'),
        ('reversed-facing-vector', MOVEMENT, '{ar+0x4d8u,point+0x20u,ar+0x4d8u,target,f,point,ar,points}', '{ar+0x4d8u,target,ar+0x4d8u,point+0x20u,f,point,ar,points}'),
        ('facing-enabled-field', MOVEMENT, 'w(0x263e09u,ar+0x4d5u,1,1)', 'w(0x263e09u,ar+0x4d4u,1,1)'),
        ('facing-dot-threshold', MOVEMENT, "if(above(r(0x46aaccu),scalar(ydot,xdot,'+')))", "if(above(scalar(ydot,xdot,'+'),r(0x46aaccu)))"),
        ('premature-completion', MOVEMENT, 'done=false;', 'done=true;'),
        ('facing-request-mode', MOVEMENT, 'w(0x2633f9u,ar+0x420u,4,2)', 'w(0x2633f9u,ar+0x420u,3,2)'),
        ('planar-y-normalization', VECTOR, 'y_result=bits(multiply(inverse,scalar(read(memory,vector+4u))))', 'y_result=bits(multiply(inverse,scalar(read(memory,vector))))'),
    ]
    try:
        for name, path, old, new in mutants:
            if source[path].count(old) != 1:
                raise ValueError(f'Mutation witness drift: {name}')
            altered = source[path].replace(old, new)
            result = compile_and_run(name, {path: altered})
            rejected = result['returncode'] == 1 and any(s in result['stderr'] for s in ('differs', 'mismatch', 'store', 'Unbacked'))
            report['mutations'].append(dict(name=name, source_sha256=digest(altered.encode()), rejected=rejected, **result))
            args.report.write_text(json.dumps(report, indent=2) + '\n')
            if not rejected:
                raise RuntimeError(f'Mutant survived or failed unexpectedly: {name}: {result}')
        report['sanitizer'] = compile_and_run('sanitizer', source, True)
        report['sanitizer']['scope'] = 'movement owner, vector math owner, and driver instrumented; reused children and archives are not reinstrumented'
        report['passed'] = report['sanitizer']['returncode'] == 0 and not report['sanitizer']['stderr']
        if not report['passed']:
            raise RuntimeError(f'Sanitizer failed: {report["sanitizer"]}')
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
