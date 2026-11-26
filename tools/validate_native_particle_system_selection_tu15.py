#!/usr/bin/env python3
"""Compare 1778D0 attachment selection with the specialized particle frame."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import sys

import native_particle_system_selection_replacements as binder
import native_particle_system_update_replacements as system_update
import native_particle_system_attachment_replacements as attachment
import specialize_native_operands as specialization


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_system_selection_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_system_selection.hpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE_PINS = {
    'selection': (0x177985, 0x177a2f, 'ee7a7612b6db6c3b5adcce3344a08bd0893bb3dd64133eb01b17a4f2be69e0d1'),
    'fixed_b': (0x177ad8, 0x177b2a, 'beb1f29302ebee69a947e50571dbf3b35a3315dd125af793d59d8e08b8ec1a6d'),
    'fixed_a': (0x177b2a, 0x177b7c, 'a38ab07d5f83cf0d97af236e63b51a9f66e9a15482e50eaf713ac3fdcafa58e4'),
}
MODES = ('', 'census', 'verify', 'collector')
MUTANTS = (
    ('particle-capacity', 'flags.minus(gp[eax], gp[ebx], 4);\n        return flags.sf != flags.of;',
      'flags.minus(gp[eax], gp[ebx], 4);\n        return flags.cf;'),
    ('salt-polarity', 'return flags.zf;\n    }\n\n    bool valid_contact', 'return !flags.zf;\n    }\n\n    bool valid_contact'),
    ('child-fault-state', 'gp = port.s.gp;', '(void)gp;'),
    ('preserve-next', 'port.store(0x1779a8u, gp[esp] + next_slot, 4, gp[ecx]);',
      'port.store(0x1779a8u, gp[esp] + next_slot, 4, gp[edi]);'),
    ('fixed-basis-source', 'gp[ecx] = port.read(0x177b00u + delta, 0x474d0cu, 4);',
      'gp[ecx] = port.read(0x177b00u + delta, 0x474d08u, 4);'),
    ('fixed-basis-sites', 'basis_shift = second_basis - first_basis;', 'basis_shift = second_basis - first_basis + 1u;'),
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
        raise ValueError('Particle system selection production binding must install exactly once')
    body = world.body(source, binder.ENTRY)[0]
    _, _, _, reload, common, bases = binder.extract(body)
    parts = (reload, common, bases)
    hooks = (binder.FIRST_HOOK, binder.REPEAT_HOOK, attachment.PREPARE_HOOK, attachment.RECURSE_HOOK,
             system_update.ENTRY_HOOK, system_update.DRAIN_HOOK)
    if any('selected(' in part for part in parts) or tuple(
            part.count('s.instruction_literal(') for part in parts) != (2, 42, 44) or any(
                body.count(hook) != 1 for hook in hooks):
        raise ValueError('Particle selection or prior stage site count drifted')
    sites = [int(match, 16) for match in re.findall(r's.instruction_literal\(0x([0-9a-f]+)u\)', bases)]
    if any(right - left != 0x52 for left, right in zip(sites[:22], sites[22:])):
        raise ValueError('Fixed particle contact bases no longer share the exact site offset')
    (scratch / 'system_selection.inc').write_text(''.join(
        binder.reference(reload, common, bases, repeat, replay)
        for replay in (True, False) for repeat in (False, True)))
    return {label:sha(part.encode()) for label, part in zip(('loop_reload', 'selection', 'fixed_bases'), parts)}


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
             Path(attachment.__file__).resolve(), ROOT / 'src/rendering/effects/native_tu15_particle_system_update.hpp'}

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
    print(f'PASS 1778D0 attachment selection and fixed bases: {sum(row["compared"] for row in results.values())} translated comparisons, '
          f'{sum(row["faults"] for row in results.values())} faults, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
