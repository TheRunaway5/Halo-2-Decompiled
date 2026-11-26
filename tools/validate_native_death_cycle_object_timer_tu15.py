#!/usr/bin/env python3
"""Compare the bound death-cycle object timer stage with its TU1.5 rollback."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_death_cycle_object_timer_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/gameplay/units/native_tu15_death_cycle_object_timer.hpp'
FIXTURE = ROOT / 'tests/native_tu15_death_cycle_object_timer_tests.cpp'


def export_reference(body):
    if body.count(binding.HOOK) != 1:
        raise ValueError('Expected the qualified object timer entry hook')
    phase = body.split(binding.END, 1)[0].split(binding.HOOK, 1)[1]
    sites = {int(pc, 16) for pc in re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)}
    if len(sites) != 93:
        raise ValueError('Expected all 93 original object timer sites')
    reference = ('void reference_object_timer(WorldState &s) {\n' + phase +
                 '\nL103436: s.instruction_literal(0x103436u);\n}\n')
    return reference, sites


def qualify(compiler, scratch, report):
    canonical = UNIT.read_text()
    prepared, _ = specialize(canonical)
    staged = binding.HOOK not in prepared
    if staged:
        prepared, owners = binding.install(canonical, prepared)
        if owners != [binding.OWNER]:
            raise ValueError('Staged death-cycle object timer owner did not bind')
    body, = binding.BODY.findall(prepared)
    report['staged_only'] = staged
    reference, sites = export_reference(body)
    (scratch / 'death_object_timer_reference.inc').write_text(reference)
    report.update(
        canonical_body_sha256=binding.CANONICAL,
        prepared_phase_sha256=binding.PHASE,
        bound_body_sha256=binding.digest(body),
        kernel_sha256=binding.digest(HEADER.read_text()),
        fixture_sha256=binding.digest(FIXTURE.read_text()),
        profiles={},
    )
    flags = ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
             '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
    for name, extra in (('release', []), ('asan_ubsan', flags)):
        executable = scratch / name
        command = [
            compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
            *extra, f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', f'-I{scratch}', str(FIXTURE),
            '-o', str(executable),
        ]
        subprocess.run(command, cwd=scratch, check=True)
        result = subprocess.run([str(executable)], cwd=scratch, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(f'{name}\n{result.stdout[-600:]}\n{result.stderr[-6000:]}')
        reached = {int(pc, 16) for pc in result.stderr.split()}
        report['profiles'][name] = {
            **json.loads(result.stdout),
            'unreached_sites': list(map(hex, sorted(sites - reached))),
        }
        if sites != reached:
            raise ValueError(f'{name} did not reach every original death-cycle object timer site')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed': False, 'scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        qualify(args.compiler, scratch, report)
        report['passed'] = True
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
