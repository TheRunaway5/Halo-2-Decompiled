#!/usr/bin/env python3
"""Compare the selected 42CE0 camera-facing branch and its retained continuation."""

import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import re
import subprocess

import native_contrail_camera_facing_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'src/rendering/effects/source_contrail_geometry_tu15_generated.cpp'
ADAPTER = ROOT/'src/rendering/effects/native_tu15_contrail_camera_facing.hpp'
FIXTURE = ROOT/'tests/native_tu15_contrail_camera_facing_tests.cpp'
SWITCH = 'HALO2_REFERENCE_CONTRAIL_CAMERA_FACING'


def command(argv, scratch, env=None):
    result = subprocess.run(argv, cwd=scratch, env=env, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{argv[0]} returned {result.returncode}\n{result.stdout[-1200:]}{result.stderr[-4000:]}')
    return result.stdout


def qualify(args, scratch):
    canonical = SOURCE.read_text()
    prepared, _ = specialize(canonical)
    active = binding.INCLUDE in prepared
    if not active:
        prepared = binding.install(canonical, prepared)
    whole = binding.owner(prepared)[0]
    selected = binding.span(whole)
    retained = selected.replace(binding.HOOK, '', 1)
    for op in ('mul', 'add'):
        retained = retained.replace(f'halo2::native_tu15_leaves::ordered_{op}_ss(', f'_mm_{op}_ss(')
    if sha256(retained.encode()).hexdigest() != binding.PREPARED_SPAN or selected.count(binding.HOOK) != 1:
        raise ValueError('Injected camera-facing fragment differs from its production binding')
    spills = re.findall(r'(?m)^std::uint32_t native_single_spill\([^\n]+\n[\s\S]*?^\}\n', canonical)
    if len(spills) != 1:
        raise ValueError('The exact retained FSTP32 helper changed')
    (scratch/'contrail_camera_facing_cases.inc').write_text(spills[0] +
        'static void camera_reference(WorldState &s) {\n' + binding.ordered(retained) + 'L43542: return;\n}\n' +
        'static bool camera_binding(WorldState &s) {\n' +
        selected.replace(binding.HOOK, binding.HOOK.replace('goto L43542;', 'return true;'), 1) +
        'L43542: return false;\n}\n')

    flags = ['-std=c++20', '-O2', '-g0', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
             *(f'-I{path}' for path in (ROOT/'include', ROOT/'src', ROOT/'tests', scratch))]
    profiles = {}
    for label, cases in (('optimized', args.cases), ('sanitized', args.sanitized_cases)):
        if not cases:
            continue
        program = scratch/f'camera-{label}'
        extra = []
        if label == 'sanitized':
            extra = ['-O1', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
        command([args.compiler, *flags, *extra, FIXTURE, '-o', program], scratch)
        outcomes = {}
        for mode in ('native', 'reference'):
            env = {name: value for name, value in os.environ.items() if name != SWITCH}
            if mode == 'reference':
                env[SWITCH] = '1'
            outcomes[mode] = json.loads(command([program, str(cases), mode], scratch, env))
        profiles[label] = outcomes

    print(json.dumps({'passed': True, 'scratch': str(scratch), 'production_binding': active,
        'source_sha256': sha256(canonical.encode()).hexdigest(), 'bound_alias_sha256': sha256(prepared.encode()).hexdigest(),
        'prepared_owner_sha256': binding.PREPARED, 'prepared_span_sha256': binding.PREPARED_SPAN,
        'adapter_sha256': sha256(ADAPTER.read_bytes()).hexdigest(), 'profiles': profiles,
        'compiler': command([args.compiler, '--version'], scratch).splitlines()[0]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--cases', type=int, default=256)
    parser.add_argument('--sanitized-cases', type=int, default=64)
    args = parser.parse_args()
    owner = os.environ.get('HALO2_TEST_SCRATCH')
    if not owner or not Path(owner).is_dir():
        parser.error('Run through tools/run_test.py for owned scratch')
    qualify(args, Path(owner))


if __name__ == '__main__':
    main()
