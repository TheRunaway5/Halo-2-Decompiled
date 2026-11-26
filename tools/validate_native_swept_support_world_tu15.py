#!/usr/bin/env python3
"""Compare the native World sweep support route and pinned TU1.5 body."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

import native_swept_support_world_replacements as binding
import native_world_leaves_remap_sites as remaps


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/physics/runtime/physics_world_runtime_generated.cpp'
HEADER = ROOT / 'src/physics/runtime/native_tu15_swept_support_world.hpp'
TEST = ROOT / 'tests/native_tu15_swept_support_world_tests.cpp'
MODES = ('', 'census', 'verify', *remaps.MODES)
MUTANTS = (
    ('child-site', 'target, 0x2e22c9u, dispatch', 'target, 0x2e22c8u, dispatch'),
    ('child-return', 'port.push(0x2e5dc9u, 0x2e5dccu);', 'port.push(0x2e5dc9u, 0x2e5dcdu);'),
    ('sweep-direction', '0x2e5dd0u, gp[ebx]', '0x2e5dd0u, gp[esi]'),
    ('sum-order', '(_mm_shuffle_ps(x[0], x[0], 0x55), x[0])', '(x[0], _mm_shuffle_ps(x[0], x[0], 0x55))'),
    ('comparison-mask', '(gp[eax] >> 8u) & 0x41u', '(gp[eax] >> 8u) & 0x40u'),
    ('replace-tag', 'source::u(source::spill(state.fp.st(0)))', '0u'),
    ('third-vector-lane', 'port.store_vector(0x2e5e11u, gp[edi], x[0]);',
     'port.store(0x2e5e11u, gp[edi], 4, source::u(_mm_cvtss_f32(x[0])));'),
    ('ret-frame', 'port.ret(0x2e5e1du, 8u);', 'port.ret(0x2e5e1du, 4u);'),
)


def hashes():
    files = (UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__), *remaps.SOURCES)
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in files}


def prepare(scratch):
    world = binding.world_helper()
    text = UNIT.read_text()
    if not binding.admitted(text, world):
        raise ValueError('World sweep support canonical body or operand drift')
    output, installed = binding.install(text, text)
    if installed != [binding.ENTRY] or binding.install(text, output) != (output, []):
        raise ValueError('World sweep support binding must install exactly once')
    body = world.body(text, binding.NAME)[0]
    changed = text.replace(body, body.replace('{\n', '{\n// changed source\n', 1), 1)
    if binding.install(changed, changed) != (changed, []):
        raise ValueError('World sweep support admitted a changed body')
    (scratch / 'body.inc').write_text(binding.ordered(body, world) + '\n')
    (scratch / 'reference.inc').write_text(binding.reference(body, world) + '\n')
    remaps.write(scratch)


def compile_harness(scratch, compiler, name, overlay=None, sanitize=False):
    binary = scratch / name
    command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math']
    if sanitize:
        command.extend(('-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'))
    if overlay:
        command.append(f'-I{overlay}')
    command.extend((f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}', str(TEST), '-o', str(binary)))
    subprocess.run(command, cwd=scratch, check=True)
    return binary


def run(binary, cases, mode):
    return subprocess.run([str(binary), str(cases), mode], capture_output=True, text=True)


def cohort(binary, cases, modes):
    results = {}
    for mode in modes:
        result = run(binary, cases, mode)
        if result.returncode:
            raise RuntimeError(f'{binary.name} {mode}: {result.stderr.strip()}')
        results[mode or 'literal'] = json.loads(result.stdout)
    return results


def verify_mutants(scratch, compiler):
    rejected = []
    source = HEADER.read_text()
    for name, old, new in MUTANTS:
        if source.count(old) != 1:
            raise ValueError(f'Mutation anchor drift: {name}')
        overlay = scratch / 'mutants' / name
        file = overlay / HEADER.relative_to(ROOT / 'src')
        file.parent.mkdir(parents=True)
        file.write_text(source.replace(old, new, 1))
        binary = compile_harness(scratch, compiler, name, overlay=overlay)
        result = run(binary, 1000, '')
        if result.returncode != 1 or 'case ' not in result.stderr:
            raise ValueError(f'World sweep support did not reject {name}: {result.stderr.strip()}')
        rejected.append({'name': name, 'diagnostic': result.stderr.strip()[-600:]})
        binary.unlink()
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--cases', type=int, default=4000)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    source_hashes = hashes()
    prepare(scratch)
    results = cohort(compile_harness(scratch, args.compiler, 'release'), args.cases, MODES)
    sanitized = cohort(compile_harness(scratch, args.compiler, 'sanitized', sanitize=True), 1000,
                       ('', 'verify', 'remap-verify'))
    mutants = verify_mutants(scratch, args.compiler)
    if hashes() != source_hashes:
        raise ValueError('World sweep support proof inputs changed during the run')
    report = dict(passed=True, pin=binding.PIN, source_sha256=source_hashes, results=results,
                  sanitized=sanitized, rejected_mutants=mutants, owned_scratch=str(scratch))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS 2E5DB0: {sum(row["compared"] for row in results.values())} comparisons, {len(mutants)} rejected mutants')


if __name__ == '__main__':
    main()
