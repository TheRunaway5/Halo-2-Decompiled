#!/usr/bin/env python3
"""Compare 1778D0 velocity basis and shared final lane with Effect Frame."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import sys

import native_particle_system_velocity_basis_replacements as binder
import specialize_native_operands as specialization
from generated_routine_ownership import function_bodies


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_system_velocity_basis_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_system_velocity_basis.hpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE_PINS = {
    'velocity': (0x177b7c, 0x177ccf, '92f475d4e56ce3595b83e1e1a478000e82446144c21bd617d22acd50c871cbe9'),
    'final_lane': (0x177ccf, 0x177cd9, '28433f7ed5a8e7f499aeacc1eeb048b6365be86c7b934bef22526ed0c787661f'),
}
MODES = ('', 'census', 'verify', 'collector')
MUTANTS = (
    ('x87-stack-pop', 'spill_extended(store, 0x40u + 4u * axis);\n        stack.pop();',
      'spill_extended(store, 0x40u + 4u * axis);'),
    ('x87-c1', 'spill.rounded_up ? 0x200u : 0u', 'spill.rounded_up ? 0u : 0x200u'),
    ('epsilon-test', 'return flags.cf || flags.zf;', 'return !flags.cf && !flags.zf;'),
    ('normalize-y', 'x[2] = ordered_mul_ss(x[2], x[0]);', 'x[2] = ordered_mul_ss(x[2], x[4]);'),
    ('perpendicular-x', 'x[1] = _mm_sub_ss(x[1], x[2]);', 'x[1] = _mm_sub_ss(x[2], x[1]);'),
    ('shared-final-lane', 'port.gp[esp] + 0x54u', 'port.gp[esp] + 0x50u'),
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
    for address, word in ((0x46a0dc,0xbf800000), (0x46a0d0,0x3f800000), (0x46a0ec,0x38d1b717)):
        if image.read(address, 4) != word.to_bytes(4, 'little'):
            raise ValueError('Particle system velocity basis coefficient drifted')
    canonical = UNIT.read_text()
    setup = binder.setup_helper()
    world = setup.world_helper()
    if not setup.admitted(canonical, world):
        raise ValueError('Particle system update source owner drifted')
    source, receipt = specialization.specialize(canonical)
    if (receipt.get('native_spatial_owners', []).count('1778d0') != 1 or
            '1778d0' in receipt.get('native_partial_frame_owners', {}) or
            binder.install(canonical, source) != (source, [])):
        raise ValueError('Particle system update must have one complete production binding')
    owner = world.body(source, binder.ENTRY)[0]
    callbacks = re.compile(r'\bs\.instruction_literal\(0x([0-9a-f]+)u\)')
    references = [body for match, body in function_bodies(source)
                  if re.fullmatch(r'native_particle_system_\w+_reference_1778d0', match[1])]
    original_sites = set(callbacks.findall(owner))
    reference_sites = {pc for body in references for pc in callbacks.findall(body)}
    if len(references) != 9 or len(original_sites) != 372 or reference_sites != original_sites:
        raise ValueError('Particle system replay sites must cover the complete original owner')
    _, _, _, _, velocity, lane = binder.extract(owner)
    if any('selected(' in part for part in (velocity,lane)) or tuple(
            part.count('s.instruction_literal(') for part in (velocity,lane)) != (87,2):
        raise ValueError('Particle velocity basis site count drifted')
    if any(owner.count(hook) != 1 for hook in (binder.HOOK,binder.LANE_HOOK)):
        raise ValueError('Particle velocity basis hooks drifted')
    spill = re.search(r'^std::uint32_t native_single_spill\(.*?^}', canonical, re.S | re.M)[0]
    (scratch / 'system_velocity_basis.inc').write_text(spill + '\n' +
        binder.reference(velocity) + binder.reference(lane, lane=True) +
        binder.reference(velocity, live=True) + binder.reference(lane, lane=True, live=True))
    return {'velocity':sha(velocity.encode()), 'final_lane':sha(lane.encode())}


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
             Path(specialization.__file__).resolve(), ROOT / 'tests/native_tu15_particle_system_geometry_fixture.hpp'}

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
    print(f'PASS 1778D0 velocity basis and shared lane: {sum(row["compared"] for row in results.values())} translated comparisons, '
          f'{sum(row["faults"] for row in results.values())} faults, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
