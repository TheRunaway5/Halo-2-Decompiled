#!/usr/bin/env python3
"""Compare the production death-cycle opening with its pinned TU1.5 reference."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_death_cycle_opening_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/gameplay/units/native_tu15_death_cycle_opening.hpp'
FIXTURE = ROOT / 'tests/native_tu15_death_cycle_opening_tests.cpp'


def qualify(compiler, scratch, report):
    canonical = UNIT.read_text()
    prepared, metadata = specialize(canonical)
    body, = binding.BODY.findall(prepared)
    if binding.OWNER not in metadata.get('native_partial_frame_owners', {}) or body.count(binding.HOOK) != 1:
        raise ValueError('Current 1030F0 owner did not bind once')
    prefix = body.split(binding.CONTINUATION, 1)[0].replace(binding.HOOK, '', 1)
    reference = prefix + '\nL1031cb: s.instruction_literal(0x1031cbu);\n}\n'
    sites = {int(pc, 16) for pc in re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', prefix)}
    if len(sites) != 60:
        raise ValueError('Expected 60 original death-cycle opening sites')
    (scratch / 'death_cycle_opening_reference.inc').write_text(reference)
    report.update(
        canonical_sha256=binding.CANONICAL,
        prepared_sha256=binding.PREPARED,
        bound_body_sha256=binding.digest(body),
        kernel_sha256=binding.digest(HEADER.read_text()),
        fixture_sha256=binding.digest(FIXTURE.read_text()),
        profiles={},
    )

    profiles = (
        ('release', []),
        ('asan_ubsan', ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                        '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']),
    )
    for name, flags in profiles:
        executable = scratch / name
        command = [
            compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
            *flags, f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', f'-I{scratch}', str(FIXTURE),
            '-o', str(executable),
        ]
        subprocess.run(command, cwd=scratch, check=True)
        result = subprocess.run([str(executable)], cwd=scratch, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(f'{name}\n{result.stderr[-6000:]}')
        reached = {int(pc, 16) for pc in result.stderr.split()}
        report['profiles'][name] = {
            **json.loads(result.stdout),
            'unreached_sites': list(map(hex, sorted(sites - reached))),
        }
        if reached != sites:
            raise ValueError(f'{name} did not reach every original 1030F0 opening site')


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
