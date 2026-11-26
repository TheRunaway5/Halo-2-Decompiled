#!/usr/bin/env python3
"""Compare 1778D0 setup and drain with the specialized particle frame."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import native_particle_system_update_replacements as binder
import specialize_native_operands as specialization


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_system_update_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_system_update.hpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE_PINS = {
    'setup': (0x1778d0, 0x177985, '75b3f27aecdedcfb0cd3c11175ee3b655cdafc229be1e227e85f4dacd1e5029e'),
    'drain': (0x177dd7, 0x177e1b, 'c944434d35016164f6ce98a60d5fb4cc7406dd5af251d1d31af4407e1cafb7a8'),
}
MODES = ('', 'census', 'verify', 'collector')
MUTANTS = (
    ('activity-mask', 'flags.logic(gp[eax] & 0xfdffu, 4)', 'flags.logic(gp[eax] & 0xffffu, 4)'),
    ('active-bitset-word', 'active_effects + gp[eax] * 4u', 'active_effects + gp[eax] * 2u'),
    ('location-argument', 'port.push(0x17794cu, gp[ebp]);', 'port.push(0x17794cu, gp[esi]);'),
    ('child-fault-state', 'gp = port.s.gp;', '(void)gp;'),
    ('drain-survival-flag', 'flags.logic(gp[ecx] & 1u, 1)', 'flags.logic(gp[ecx] & 2u, 1)'),
    ('drain-ret-width', 'port.ret(last, 4u);', 'port.ret(last, 0u);'),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def prepare(scratch, xbe):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    image = Xbe(xbe)
    if sha(image.data) != IMAGE_PIN:
        raise ValueError('TU1.5 particle frame image drifted')
    for label, (first, end, pin) in MACHINE_PINS.items():
        if sha(image.read(first, end - first)) != pin:
            raise ValueError(f'TU1.5 particle system update {label} bytes drifted')
    canonical = UNIT.read_text()
    world = binder.world_helper()
    if not binder.admitted(canonical, world):
        raise ValueError('Particle system update source owner drifted')
    source, receipt = specialization.specialize(canonical)
    owners = set(receipt.get('native_spatial_owners', ())) | receipt.get('native_partial_frame_owners', {}).keys()
    if '1778d0' not in owners or binder.install(canonical, source) != (source, []):
        raise ValueError('Particle system update production binding must install exactly once')
    body = world.body(source, binder.ENTRY)[0]
    _, _, _, entry, tail = binder.extract(body)
    if ('selected(' in entry + tail or entry.count('s.instruction_literal(') != 59 or
            tail.count('s.instruction_literal(') != 26 or
            any(body.count(hook) != 1 for hook in (binder.ENTRY_HOOK, binder.DRAIN_HOOK))):
        raise ValueError('Particle system update site set drifted')
    (scratch / 'system_update.inc').write_text(binder.reference_entry(entry) +
        binder.reference_drain(tail) + binder.reference_entry(entry, replay=False) +
        binder.reference_drain(tail, replay=False))
    return {'setup':sha(entry.encode()), 'drain':sha(tail.encode())}


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
    result = subprocess.run([str(binary), str(count), mode], text=True, capture_output=True, timeout=60)
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
        result = subprocess.run([str(binary), '640', 'verify'], text=True, capture_output=True, timeout=60)
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
    for mode, row in results.items():
        if row['coverage'] != expected:
            raise ValueError(f'Particle system update decisions missing in {mode}: {row["coverage"]:x}/{expected:x}')
    mutants, sanitized = [], {}
    if not args.smoke:
        san = compile_test(args.compiler, scratch, 'sanitized', sanitize=True)
        sanitized = {mode or 'literal':run(san, max(500, args.count//3), mode) for mode in ('', 'verify', 'collector')}
        mutants = run_mutants(args.compiler, scratch)
    if hashes != source_hashes():
        raise ValueError('Particle birth proof sources changed during validation')
    receipt = {'passed':True, 'image_sha256':IMAGE_PIN, 'machine_sha256':{label:region[2] for label, region in MACHINE_PINS.items()}, 'specialized_span_sha256':pin,
               'source_sha256':hashes, 'results':results, 'sanitized':sanitized, 'mutants':mutants, 'scratch':str(scratch)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'PASS 1778D0 setup and drain: {sum(row["compared"] for row in results.values())} translated comparisons, '
          f'{sum(row["faults"] for row in results.values())} faults, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
