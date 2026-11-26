#!/usr/bin/env python3
"""Prove the native contact response loop against its original instruction spans."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_contact_actions_replacements as actions
import native_particle_contact_response_replacements as binding
import validate_native_particle_contact_walk_tu15 as walk


ROOT = walk.ROOT
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_contact_response.hpp'
TEST = ROOT / 'tests/native_tu15_particle_contact_response_tests.cpp'
MACHINE = '6b228012ab1876a33c852d84604090a61960abac5e3c4904f844dc2d388bd50a'
MUTANTS = (
    ('projection', 'ordered_mul_ss(x[5], load(0x2c3147u, gp[esi] + 4u))',
     'ordered_mul_ss(x[5], load(0x2c3147u, gp[esi] + 0u))'),
    ('nan_destination', 'ordered_mul_ss(x[0], x[1]);\n        x[4] = x[2];',
     'ordered_mul_ss(x[1], x[0]);\n        x[4] = x[2];'),
    ('surface_child_return', '0x2c31e8u, 0x2c31edu', '0x2c31e8u, 0x2c31eau'),
    ('particle_phase', 'store(last, gp[ebp] + offsetof(walk::ParticleDatum, rotation_rate), x[3]);',
     'store(last, gp[ebp] + offsetof(walk::ParticleDatum, rotation_rate), x[0]);'),
)


def prepare(xbe):
    walk.image(xbe)
    from xbe import Xbe

    image = Xbe(xbe)
    code = image.read(0x2c31e8, 5)
    if (walk.sha(image.read(0x2c30b8, 0x190)) != MACHINE or
            code[0] != 0xe8 or 0x2c31ed + int.from_bytes(code[1:], 'little', signed=True) != 0x2c4110):
        raise ValueError('Particle contact response machine pin or transform target drifted')
    ordinary, accounted, children, _, _, _ = walk.prepare()
    owner = binding.walk.world.body(ordinary, binding.walk.OWNER)[0]
    if owner.count(binding.HOOK) != 1 or binding.install(walk.UNIT.read_text(), ordinary) != (ordinary, []):
        raise ValueError('Expected production contact response binding with exact local routes')
    first, end = binding.extract(owner)
    span = owner[first:end].removeprefix(binding.HOOK).removesuffix(actions.HOOK).removesuffix(binding.LABEL)
    replay = binding.reference(span)
    if replay not in ordinary or len(walk.literal_sites(replay)) != 85:
        raise ValueError('Particle contact response replay sites drifted')
    live = binding.reference(span, replay=False)
    for target in binding.CHILDREN:
        live = live.replace(f'run_{target}(s);', f'children::run_{target}(s);', 1)

    local_transform = binding.walk.world.body(ordinary, '2c4110')[0]
    region = '    if (target != 0x13de70u || site != 0x250004u || tail)\n'
    assert children.count(region) == 1
    children = children.replace(region, '''    if (target == 0x122b60u && site == 0x2c4134u && !tail) {
        log_call(entry_hash(s,target));
        s.pop();
        return;
    }
''' + region, 1)
    fixture = (children + 'namespace children {\n' + local_transform + '\n'
               'inline constexpr auto response_local = [](WorldState &value, auto child) {\n'
               + binding.DIRECT + '};\n}\n'
               'namespace prepared {\n' + replay + '}\n'
               'namespace original {\n' + live + '}\n')
    _, manifest, _ = walk.instrument_translation(ordinary, walk.UNIT.read_text(), walk.UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
            if row['symbol'] == binding.walk.OWNER or row.get('canonical_reference_owner') == binding.walk.OWNER}
    aliases = {name: walk.literal_sites(walk.function(ordinary, name)) for name in rows
               if name not in (binding.walk.OWNER, actions.REFERENCE)}
    total = set.union(*aliases.values())
    original = walk.literal_sites(owner)
    if (len(aliases[binding.REFERENCE]) != 85 or len(total) != 321 or
            sum(map(len, aliases.values())) != len(total) or len(original) != 401 or not total < original):
        raise ValueError('Particle contact response must extend disjoint static coverage to 321 instructions')
    aliases['retained'] = original - total
    return ordinary, accounted, fixture, rows, aliases, walk.sha(local_transform)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    os.environ.pop('HALO2_REFERENCE_PARTICLE_CONTACT_RESPONSE', None)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary, accounted, fixture, rows, sites, transform = prepare(args.xbe)
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}']
    for name, source in (('ordinary', ordinary), ('accounted', accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    (scratch / 'particle_contact_response_reference.inc').write_text(fixture)
    comparisons = {}
    for variant, count, flags in (('release', 1200, []), ('sanitized', 300, [
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer'])):
        binary = scratch / variant
        subprocess.run([*command, *flags, str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        comparisons[variant] = {}
        for mode in walk.MODES:
            result = walk.run(binary, count, mode)
            if result.returncode:
                raise RuntimeError(f'{variant} {mode}:\n{result.stderr}')
            report = comparisons[variant][mode] = json.loads(result.stdout)
            if mode == 'verify':
                hit = re.search(r'Particle contact response verify: (\d+) calls', result.stderr)
                if not hit or int(hit[1]) != count:
                    raise ValueError('Particle contact response missed a normal verification replay')
                report['verified'] = int(hit[1])

    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle contact response mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before, after, 1))
        binary = directory / 'probe'
        subprocess.run([command[0], f'-I{directory}', *command[1:], str(TEST), '-o', str(binary)], check=True, cwd=scratch)
        result = walk.run(binary, 64, 'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle contact response accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): walk.sha(path.read_bytes()) for path in
                       (walk.UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))},
        machine_pin=MACHINE, manifest=rows, instruction_sites={name: sorted(group) for name,group in sites.items()},
        transform_child_source_sha=transform, production_syntax=dict(ordinary=walk.sha(ordinary), accounted=walk.sha(accounted)),
        comparisons=comparisons, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle contact response passed the pinned projection, placement, and velocity comparisons.')


if __name__ == '__main__':
    main()
