#!/usr/bin/env python3
"""Compare the native 251E30 emitter continuation against specialized source."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import native_particle_location_emitters_replacements as binder
import specialize_native_operands as specialization


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_location_emitters_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_location_emitters.hpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE_PIN = '55ef2c9f0c0ac4cca8cca83ef63f59cc7ad30a4e0adbfb0e81fc2618e1764d04'
MODES = ('', 'census', 'verify')
MUTANTS = (
    ('allocator-return', '0x251e78u, allocate_emitter', '0x251e79u, allocate_emitter'),
    ('emitter-argument', 'port.push(0x251f24u, gp[edx]);', 'port.push(0x251f24u, gp[ecx]);'),
    ('def-stride', 'flags.plus(gp[ebx], sizeof(EmitterDefinition), 4)',
     'flags.plus(gp[ebx], sizeof(EmitterDatum), 4)'),
    ('forget-partial-child', 'gp = port.s.gp;', '(void)gp;'),
    ('bad-head-sentinel', 'gp[ebp] = flags.logic(0xffffffffu, 4);', 'gp[ebp] = flags.logic(0u, 4);'),
    ('own-return-cleanup', 'port.ret(last, 20u);', 'port.ret(last, 24u);'),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def prepare(scratch, xbe):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    image = Xbe(xbe)
    if sha(image.data) != IMAGE_PIN or sha(image.read(0x251e30, 0x137)) != MACHINE_PIN:
        raise ValueError('TU1.5 particle emitter tail machine bytes drifted')
    canonical = UNIT.read_text()
    observer = binder.observer_helper()
    world = observer.world_helper()
    if not observer.admitted(canonical, world):
        raise ValueError('Particle emitter tail source owner drifted')
    source, _ = specialization.specialize(canonical)
    body = world.body(source, binder.ENTRY)[0]
    _, tail = binder.extract(body)
    if 'selected(' in tail or tail.count('s.instruction_literal(') != 100:
        raise ValueError('Particle emitter tail is no longer a literal owner')
    (scratch / 'tail.inc').write_text(binder.reference(tail) + binder.reference(tail, replay=False))
    return sha(tail.encode())


def compile_test(compiler, scratch, name, override=None, sanitize=False):
    target = scratch / name
    command = [compiler, '-std=c++20', '-O3', '-fno-fast-math', '-ffp-contract=off', '-frounding-math']
    if sanitize:
        command += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    if override:
        command += [f'-I{override}']
    command += [f'-I{ROOT / "include"}', f'-I{ROOT / "src"}', f'-I{scratch}', str(TEST), '-o', str(target)]
    subprocess.run(command, check=True, cwd=scratch)
    return target


def run(binary, count, mode):
    result = subprocess.run([str(binary), str(count), mode], text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(f'{binary.name} ({mode or "literal"}) failed:\n{result.stderr.strip()}')
    return json.loads(result.stdout)


def run_mutants(compiler, scratch):
    rejected = []
    source = HEADER.read_text()
    for name, before, after in MUTANTS:
        override = scratch / name
        header = override / HEADER.relative_to(ROOT / 'src')
        header.parent.mkdir(parents=True)
        if source.count(before) != 1:
            raise ValueError(f'Mutation anchor drift: {name}')
        header.write_text(source.replace(before, after, 1))
        binary = compile_test(compiler, scratch, f'mutant-{name}', override)
        result = subprocess.run([str(binary), '1024', 'verify'], text=True, capture_output=True)
        if not result.returncode:
            raise ValueError(f'Mutant survived: {name}')
        rejected.append({'name': name, 'diagnostic': result.stderr.strip()[-400:]})
        binary.unlink()
    return rejected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--count', default=1600, type=int)
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    files = {UNIT, TEST, HEADER, Path(__file__).resolve(), Path(binder.__file__).resolve(),
             Path(specialization.__file__).resolve()}

    def source_hashes():
        return {str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in sorted(files)}

    hashes = source_hashes()
    pin = prepare(scratch, args.xbe.resolve())
    binary = compile_test(args.compiler, scratch, 'release')
    results = {mode or 'literal':run(binary, args.count, mode) for mode in MODES}
    expected = (1 << results['literal']['branches']) - 1
    if results['literal']['coverage'] != expected:
        raise ValueError(f'Particle emitter tail decisions missing: {results["literal"]["coverage"]:x}/{expected:x}')
    mutants, sanitized = [], {}
    if not args.smoke:
        san = compile_test(args.compiler, scratch, 'sanitized', sanitize=True)
        sanitized = {mode or 'literal':run(san, max(500, args.count//3), mode) for mode in ('', 'verify')}
        mutants = run_mutants(args.compiler, scratch)
    if hashes != source_hashes():
        raise ValueError('Particle emitter tail proof sources changed during validation')
    receipt = {'passed':True, 'image_sha256':IMAGE_PIN, 'machine_sha256':MACHINE_PIN, 'specialized_tail_sha256':pin,
               'source_sha256':hashes, 'results':results, 'sanitized':sanitized, 'mutants':mutants, 'scratch':str(scratch)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'PASS 251E30: {sum(row["compared"] for row in results.values())} translated comparisons, '
          f'{sum(row["faults"] for row in results.values())} faults, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
