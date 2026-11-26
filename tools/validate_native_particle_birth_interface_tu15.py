#!/usr/bin/env python3
"""Compare the complete native 250C60 birth closure with specialized Death Cycle."""

import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess
import sys

import native_particle_birth_interface_replacements as binder
import native_particle_birth_placement_replacements as placement
import specialize_native_operands as specialization


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
TEST = ROOT / 'tests/native_tu15_particle_birth_interface_tests.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_birth_interface.hpp'
IMAGE_PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE_PIN = 'd60e24fc0d0e9ab9d34cf45cc9421e918f52dad56681fe12d21404c0f4399567'
MODES = ('', 'census', 'verify', 'collector')
MUTANTS = (
    ('attachment-group', 'standard_group = 0x5052544du', 'standard_group = 0x5062544du'),
    ('loop-condition', 'if (flags.sf == flags.of) return;', 'if (flags.cf) return;'),
    ('tag-method-slot', 'call_method(gp[edx] + 0x50u', 'call_method(gp[edx] + 0x18u'),
    ('child-fault-state', 'gp = port.s.gp;', '(void)gp;'),
    ('notification-argument', 'port.push(0x2512b5u, gp[edi]);', 'port.push(0x2512b5u, gp[eax]);'),
    ('return-stack', 'caller_args = 24u', 'caller_args = 20u'),
)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def prepare(scratch, xbe):
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    image = Xbe(xbe)
    if sha(image.data) != IMAGE_PIN or sha(image.read(0x25113a, 0x2512d4 - 0x25113a)) != MACHINE_PIN:
        raise ValueError('TU1.5 particle birth interface machine bytes drifted')
    for address, word in ((0x46a0d0, 0x3f800000), (0x48603c, 0x45f330), (0x486048, 0x45f2d8)):
        if image.read(address, 4) != word.to_bytes(4, 'little'):
            raise ValueError('Particle birth interface constants drifted')
    canonical = UNIT.read_text()
    prefix = binder.prefix_helper()
    world = prefix.world_helper()
    if not prefix.admitted(canonical, world):
        raise ValueError('Particle birth source owner drifted')
    source, metadata = specialization.specialize(canonical)
    if binder.ENTRY not in metadata.get('native_spatial_owners', []):
        raise ValueError('Complete particle birth binding is missing')
    owner = world.body(source, binder.ENTRY)[0]
    hooks = (prefix.PREFIX, placement.HOOK, binder.HOOK, binder.EARLY_HOOK)
    if any(owner.count(hook) != 1 for hook in hooks):
        raise ValueError('A combined birth entry or its return hook did not install')
    _, _, head = prefix.extract(owner)
    _, _, properties = placement.extract(owner)
    _, _, interface, epilogue = binder.extract(owner)
    parts = (head, properties, interface, epilogue)
    if any('selected(' in part for part in parts) or tuple(
            part.count('s.instruction_literal(') for part in parts) != (132, 174, 118, 6):
        raise ValueError('Combined particle birth site set drifted')
    spill = re.search(r'^std::uint32_t native_single_spill\(.*?^}', canonical, re.S | re.M)[0]
    translated = ('static void translated_250c60(WorldState &s) {' + head + prefix.ANCHOR
                  + placement.ordered(properties) + interface + epilogue)
    (scratch / 'birth_combined.inc').write_text(
        spill + '\n' + prefix.reference(head) + placement.reference(properties)
        + binder.references(interface, epilogue) + owner + '\n' + translated + '\n')
    return tuple(sha(part.encode()) for part in parts)


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
             Path(specialization.__file__).resolve(), Path(placement.__file__).resolve(),
             ROOT / 'tools/native_particle_birth_prefix_replacements.py',
             ROOT / 'src/rendering/effects/native_tu15_particle_birth_placement.hpp',
             ROOT / 'src/rendering/effects/native_tu15_particle_birth_prefix.hpp',
             ROOT / 'src/rendering/effects/native_tu15_particle_birth_observer.hpp'}

    def source_hashes():
        return {str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in sorted(files)}

    hashes = source_hashes()
    pin = prepare(scratch, args.xbe.resolve())
    binary = compile_test(args.compiler, scratch, 'release')
    results = {mode or 'literal':run(binary, args.count, mode) for mode in MODES}
    expected = (1 << results['literal']['branches']) - 1
    for mode, row in results.items():
        if row['coverage'] != expected:
            raise ValueError(f'Particle birth interface decisions missing in {mode}: {row["coverage"]:x}/{expected:x}')
    mutants, sanitized = [], {}
    if not args.smoke:
        san = compile_test(args.compiler, scratch, 'sanitized', sanitize=True)
        sanitized = {mode or 'literal':run(san, max(500, args.count//3), mode) for mode in ('', 'verify', 'collector')}
        mutants = run_mutants(args.compiler, scratch)
    if hashes != source_hashes():
        raise ValueError('Particle birth proof sources changed during validation')
    receipt = {'passed':True, 'image_sha256':IMAGE_PIN, 'machine_sha256':MACHINE_PIN, 'specialized_span_sha256':dict(zip(('allocation', 'placement', 'interface', 'epilogue'), pin)),
               'source_sha256':hashes, 'results':results, 'sanitized':sanitized, 'mutants':mutants, 'scratch':str(scratch)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(receipt, indent=2) + '\n')
    print(f'PASS 250C60 birth full closure: {sum(row["compared"] for row in results.values())} translated comparisons, '
          f'{sum(row["faults"] for row in results.values())} faults, {len(mutants)} mutants rejected')


if __name__ == '__main__':
    main()
