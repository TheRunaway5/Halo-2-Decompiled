#!/usr/bin/env python3
"""Compare 1778D0 attachment preparation and recursive update with the specialized particle frame."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

import native_particle_system_attachment_replacements as binder
import native_particle_system_update_replacements as system_update
import specialize_native_operands as specialization


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_system_attachment_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_system_attachment.hpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE_PINS = {
    'prepare': (0x177cd9, 0x177da2, '3c7d833916c7b434d1b42d85349ac6b7287b1e438d00964933434e0669db5dfe'),
    'recurse': (0x177da2, 0x177dd7, '46abaf6f772e0c301e203135a685d430ea721c8cd4af6f8cebf3366d6186a76d'),
}
MODES = ('', 'census', 'verify', 'collector')
MUTANTS = (
    ('prepare-position', 'port.read(0x177cf4u, gp[edx] + 8u, 4)', 'port.read(0x177cf4u, gp[edx] + 4u, 4)'),
    ('interface-width', 'port.store(0x177d73u, gp[esp] + 0x13u, 1', 'port.store(0x177d73u, gp[esp] + 0x13u, 2'),
    ('transform-address', 'gp[eax] = gp[esp] + basis_slot;', 'gp[eax] = gp[esp] + transform_slot;'),
    ('child-fault-state', 'gp = port.s.gp;', '(void)gp;'),
    ('nested-return', 'flags.logic(gp[eax] & 0xffu, 1)', 'flags.logic(gp[eax] & 2u, 1)'),
    ('next-link', 'port.store(0x177dcdu, gp[esp] + attachment_slot, 4, gp[eax]);',
      'port.store(0x177dcdu, gp[esp] + next_slot, 4, gp[eax]);'),
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
    world = system_update.world_helper()
    if not system_update.admitted(canonical, world):
        raise ValueError('Particle system update source owner drifted')
    source, receipt = specialization.specialize(canonical)
    owners = set(receipt.get('native_spatial_owners', ())) | receipt.get('native_partial_frame_owners', {}).keys()
    if '1778d0' not in owners or binder.install(canonical, source) != (source, []):
        raise ValueError('Particle system attachment production binding must install exactly once')
    body = world.body(source, binder.ENTRY)[0]
    _, _, prepare, recurse = binder.extract(body)
    _, _, _, entry, drain = system_update.extract(body)
    spans = (prepare, recurse, entry, drain)
    hooks = (binder.PREPARE_HOOK, binder.RECURSE_HOOK, system_update.ENTRY_HOOK, system_update.DRAIN_HOOK)
    if any('selected(' in span for span in spans) or tuple(span.count('s.instruction_literal(') for span in spans) != (
            58, 17, 59, 26) or any(body.count(hook) != 1 for hook in hooks):
        raise ValueError('Particle attachment or nested system site set drifted')
    (scratch / 'system_attachment.inc').write_text(
        binder.reference_prepare(prepare) + binder.reference_recurse(recurse) +
        binder.reference_prepare(prepare, for_replay=False) + binder.reference_recurse(recurse, for_replay=False) +
        system_update.reference_entry(entry) + system_update.reference_drain(drain) +
        system_update.reference_entry(entry, replay=False) + system_update.reference_drain(drain, replay=False))
    return {label:sha(span.encode()) for label, span in zip(('prepare','recurse','nested_entry','nested_drain'),spans)}


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
             Path(specialization.__file__).resolve(), Path(system_update.__file__).resolve(),
             ROOT / 'src/rendering/effects/native_tu15_particle_system_update.hpp'}

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
    print(f'PASS 1778D0 attachment preparation and recursion: {sum(row["compared"] for row in results.values())} translated comparisons, '
          f'{sum(row["faults"] for row in results.values())} faults, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
