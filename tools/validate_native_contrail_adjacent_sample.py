#!/usr/bin/env python3
"""Qualify the exact adjacent 42CE0 sample over ordered provider accesses."""

import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import subprocess

import native_contrail_adjacent_sample_replacements as binding
import specialize_native_operands as loader


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'src/rendering/effects/source_contrail_geometry_tu15_generated.cpp'
ADAPTER = ROOT/'src/rendering/effects/native_tu15_contrail_adjacent_sample.hpp'
FIXTURE = ROOT/'tests/native_tu15_contrail_adjacent_sample_tests.cpp'
SWITCH = 'HALO2_REFERENCE_CONTRAIL_ADJACENT_SAMPLE'


def run(argv, scratch, env=None):
    result = subprocess.run(argv, cwd=scratch, env=env, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} returned {result.returncode}\n{result.stdout[-1200:]}{result.stderr[-4000:]}')
    return result.stdout


def specialize(source, allow_staging):
    prepared, _ = loader.specialize(source)
    if binding.INCLUDE in prepared or not allow_staging:
        return prepared
    path = Path(loader.__file__)
    code = path.read_text()
    boundary = '        output=install_camera_facing(text,output)\n'
    if code.count(boundary) != 1:
        raise ValueError('Expected one active camera-facing binder before adjacent sampling')
    staged = code.replace(boundary, boundary +
        '        from native_contrail_adjacent_sample_replacements import install as install_adjacent\n'
        '        output=install_adjacent(text,output)\n', 1)
    scope = {'__file__': str(path), '__name__': 'staged_specializer'}
    exec(compile(staged, str(path), 'exec'), scope)
    return scope['specialize'](source)[0]


def qualify(args, scratch):
    source = SOURCE.read_text()
    active = binding.INCLUDE in loader.specialize(source)[0]
    prepared = specialize(source, args.staged_binding)
    selected = binding.span(binding.owner(prepared)[0])
    retained = selected.replace(binding.HOOK, '', 1)
    for op, helper in (('mul', 'mul'), ('add', 'sum'), ('sub', 'difference')):
        retained = retained.replace(f'halo2::native_tu15_leaves::ordered_{helper}_ss(', f'_mm_{op}_ss(')
    if sha256(retained.encode()).hexdigest() != binding.PREPARED_SPAN or selected.count(binding.HOOK) != 1:
        raise ValueError('Exact production adjacent sampling binding missing')
    (scratch/'contrail_adjacent_sample_cases.inc').write_text(
        'static void adjacent_reference(WorldState &s) {\n' + binding.ordered(retained) + 'L42fc4: return;\n}\n' +
        'static bool adjacent_binding(WorldState &s) {\n' +
        selected.replace(binding.HOOK, binding.HOOK.replace('goto L42fc4;', 'return true;'), 1) +
        'L42fc4: return false;\n}\n')

    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT/'include', ROOT/'src', ROOT/'tests', scratch))]
    profiles = {}
    for label, cases in (('optimized', args.cases), ('sanitized', args.sanitized_cases)):
        if not cases:
            continue
        program = scratch/f'adjacent-{label}'
        extra = [] if label == 'optimized' else [
            '-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        run([args.compiler, *flags, *extra, FIXTURE, '-o', program], scratch)
        profiles[label] = {}
        for mode in ('native', 'reference'):
            env = {key: value for key, value in os.environ.items() if key != SWITCH}
            if mode == 'reference':
                env[SWITCH] = '1'
            profiles[label][mode] = json.loads(run([program, str(cases), mode], scratch, env))

    print(json.dumps({'passed': True, 'scratch': str(scratch), 'production_binding': active,
        'prepared_span_sha256': binding.PREPARED_SPAN, 'bound_alias_sha256': sha256(prepared.encode()).hexdigest(),
        'adapter_sha256': sha256(ADAPTER.read_bytes()).hexdigest(), 'profiles': profiles,
        'compiler': run([args.compiler, '--version'], scratch).splitlines()[0]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--cases', type=int, default=256)
    parser.add_argument('--sanitized-cases', type=int, default=64)
    parser.add_argument('--staged-binding', action='store_true')
    args = parser.parse_args()
    owner = os.environ.get('HALO2_TEST_SCRATCH')
    if not owner or not Path(owner).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch')
    qualify(args, Path(owner))


if __name__ == '__main__':
    main()
