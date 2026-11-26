#!/usr/bin/env python3
"""Compare the 104371 native record blend with its pinned rollback."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_death_cycle_record_blend_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/gameplay/units/native_tu15_death_cycle_record_blend.hpp'
FIXTURE = ROOT / 'tests/native_tu15_death_cycle_record_blend_tests.cpp'


def qualify(compiler, scratch, report):
    canonical = UNIT.read_text()
    prepared, _ = specialize(canonical)
    if binding.HOOK not in prepared:
        prepared, owners = binding.install(canonical, prepared)
        if owners != [binding.OWNER]:
            raise ValueError('Record blend did not bind')
    body, = binding.BODY.findall(prepared)
    if body.count(binding.HOOK) != 1:
        raise ValueError('Expected a unique record blend hook')
    phase = body.split(binding.HOOK, 1)[1].split(binding.END, 1)[0]
    expected = {int(pc, 16) for pc in re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', phase)}
    if len(expected) != 104:
        raise ValueError('Expected all 104 original record blend sites')
    (scratch / 'death_record_blend_reference.inc').write_text(
        'void reference_record_blend(WorldState &s) {\n' + phase +
        '\nL1044f4: s.instruction_literal(0x1044f4u);\n}\n')
    report.update(canonical_body_sha256=binding.CANONICAL, prepared_phase_sha256=binding.PHASE,
                  bound_body_sha256=binding.digest(body), kernel_sha256=binding.digest(HEADER.read_text()),
                  fixture_sha256=binding.digest(FIXTURE.read_text()), profiles={})
    sanitizers = ['-O1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                  '-fno-sanitize-recover=all', '-fno-pie', '-no-pie']
    for name, flags in (('release', []), ('asan_ubsan', sanitizers)):
        binary = scratch / name
        command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                   *flags, f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', f'-I{scratch}', str(FIXTURE),
                   '-o', str(binary)]
        subprocess.run(command, cwd=scratch, check=True)
        result = subprocess.run([str(binary)], cwd=scratch, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(f'{name}\n{result.stdout[-600:]}\n{result.stderr[-6000:]}')
        reached = {int(pc, 16) for pc in result.stderr.split()}
        report['profiles'][name] = {**json.loads(result.stdout), 'unreached_sites': list(map(hex, expected - reached))}
        if reached != expected:
            raise ValueError(f'{name} record blend did not exercise every original site')


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
