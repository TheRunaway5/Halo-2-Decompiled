#!/usr/bin/env python3
"""Verify the bound particle contact flag continuation and its actual child."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_contact_flags_replacements as binding
import validate_native_particle_contact_walk_tu15 as walk


ROOT = walk.ROOT
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_contact_flags.hpp'
TEST = ROOT / 'tests/native_tu15_particle_contact_flags_tests.cpp'
MACHINE_PIN = '72ec68aa6c97a2c0b69600e6cb4169e0dd559a69eaf261d6862b65ad496ada1e'
MUTANTS = (
    ('cached_particle', 'gp[eax] = gp[ebp];', 'gp[eax] = gp[ebp] + 4u;'),
    ('enumeration_mask', 'gp[esi] & 0xf80fu', 'gp[esi] & 0xf80eu'),
    ('collision_flag', 'copy_collision_bit(0x400u);', 'copy_collision_bit(0x200u);'),
    ('cleared_particle_flag', 'state & 0xfdu', 'state & 0xfbu'),
)


def execute(binary, count, mode):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_CONTACT_WALK', 'HALO2_REFERENCE_PARTICLE_CONTACT_FLAGS',
                 'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES', 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES',
                 'HALO2_CENSUS_NATIVE_BRIDGES'):
        env.pop(flag, None)
    return subprocess.run([str(binary), str(count), mode], cwd=binary.parent, env=env,
                          capture_output=True, text=True, timeout=90)


def prepare(xbe):
    walk.image(xbe)
    from xbe import Xbe

    image = Xbe(xbe)
    code = image.read(0x2c2f3a, 5)
    if (walk.sha(image.read(0x2c2ee0, 0xf4)) != MACHINE_PIN or
            code[0] != 0xe8 or 0x2c2f3f + int.from_bytes(code[1:], 'little', signed=True) != 0x176a40):
        raise ValueError('Particle contact flags machine or child target drifted')

    ordinary, accounted, children, _, sites, child_hashes = walk.prepare()
    owner = binding.walk.world.body(ordinary, binding.walk.OWNER)[0]
    retained = binding.span(owner)
    replay = binding.reference(retained)
    _, manifest, _ = walk.instrument_translation(ordinary, walk.UNIT.read_text(),
                                                  walk.UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
              if row['symbol'] in (binding.walk.OWNER, binding.walk.SETUP, binding.walk.WALK, binding.REFERENCE)}
    covered = walk.literal_sites(retained)
    if (owner.count(binding.HOOK) != 1 or replay not in ordinary or len(covered) != 72 or
            walk.literal_sites(replay) != covered or not covered < sites['retained'] or
            rows[binding.REFERENCE]['literal_sites'] != 72):
        raise ValueError('Particle contact flags did not authenticate the 72 new, disjoint sites')
    if sum(row['literal_sites'] for name,row in rows.items() if name != binding.walk.OWNER) != 176:
        raise ValueError('Particle contact flags should bring two-stage coverage to 176 sites')
    live = binding.reference(retained, replay=False).replace(binding.CHILD, 'children::run_176a40(s);', 1)
    fixture = (children + 'namespace prepared {\n' + replay + '}\n'
               'namespace original {\n' + live + '}\n')
    sites['retained'] -= covered
    sites['flags'] = covered
    return ordinary, accounted, fixture, rows, sites, child_hashes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary, accounted, fixture, rows, sites, child_hashes = prepare(args.xbe)
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}']
    for name, source in (('ordinary', ordinary), ('accounted', accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    (scratch / 'particle_contact_flags_reference.inc').write_text(fixture)
    comparisons = {}
    for variant, count, flags in (('release', 1200, []), ('sanitized', 300, [
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'])):
        binary = scratch / variant
        subprocess.run([*command, *flags, str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        comparisons[variant] = {}
        for mode in walk.MODES:
            result = execute(binary, count, mode)
            if result.returncode:
                raise RuntimeError(f'{variant} {mode}:\n{result.stderr}')
            report = comparisons[variant][mode] = json.loads(result.stdout)
            if mode == 'verify':
                hit = re.search(r'Particle contact flags verify: (\d+) calls', result.stderr)
                if not hit or int(hit[1]) != count:
                    raise ValueError('Particle contact flags missed a normal verification replay')
                report['verified'] = int(hit[1])

    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle contact flags mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before,after,1))
        binary = directory / 'probe'
        subprocess.run([command[0], f'-I{directory}', *command[1:], str(TEST), '-o', str(binary)], check=True, cwd=scratch)
        result = execute(binary, 64, 'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle contact flags accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): walk.sha(path.read_bytes()) for path in
                       (walk.UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))},
        machine_pin=MACHINE_PIN, manifest=rows, instruction_sites={name: sorted(group) for name,group in sites.items()},
        child_source_pins=child_hashes, production_syntax=dict(ordinary=walk.sha(ordinary), accounted=walk.sha(accounted)),
        comparisons=comparisons, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle contact flags passed their pinned source and genuine enumeration boundary.')


if __name__ == '__main__':
    main()
