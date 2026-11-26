#!/usr/bin/env python3
"""Compare the bound death-cycle expiration stage with its TU1.5 rollback."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_death_cycle_expiration_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
HEADER = ROOT / 'src/gameplay/units/native_tu15_death_cycle_expiration.hpp'
FIXTURE = ROOT / 'tests/native_tu15_death_cycle_expiration_tests.cpp'


def export_reference(body):
    hooks = [binding.hook(*entry) for entry in binding.ENTRIES]
    if any(body.count(hook) != 1 for hook in hooks):
        raise ValueError('Expected all three expiration entry hooks')
    prefix = body.split(binding.END, 1)[0].split(hooks[0], 1)[1]
    for (pc, _), hook in zip(binding.ENTRIES[1:], hooks[1:], strict=True):
        prefix = prefix.replace(hook, binding.label(pc), 1)
    sites = {int(pc, 16) for pc in re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)', prefix)}
    if len(sites) != 69:
        raise ValueError('Expected all 69 original record expiration sites')
    reference = (
        'void reference_expiration(WorldState &s) {\n'
        '    if (policy == 1u) goto L10412e;\n'
        '    if (policy == 2u) goto L10414b;\n' + prefix + '\n' +
        ''.join(f'L{target}: s.instruction_literal(0x{target}u); return;\n' for target in binding.DESTINATIONS) +
        '}\n'
    )
    return reference, sites


def qualify(compiler, scratch, report):
    canonical = UNIT.read_text()
    prepared, metadata = specialize(canonical)
    body, = binding.BODY.findall(prepared)
    if 'per-record timer expiration' not in metadata['native_partial_frame_owners'][binding.OWNER]:
        raise ValueError('Current death-cycle expiration stage did not bind')
    reference, sites = export_reference(body)
    (scratch / 'death_expiration_reference.inc').write_text(reference)
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
            raise ValueError(f'{name} did not reach every original death-cycle expiration site')


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
