#!/usr/bin/env python3
"""Verify the final particle contact callbacks and complete 2C2D90 coverage."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_contact_actions_replacements as binding
import validate_native_particle_contact_walk_tu15 as walk


ROOT = walk.ROOT
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_contact_actions.hpp'
TEST = ROOT / 'tests/native_tu15_particle_contact_actions_tests.cpp'
MACHINE = '5c6bfb769e3763ab28dfb3a509ee0b68c560f59e3d28f67a2ba49aae100f49ac'
MUTANTS = (
    ('notification_preselection', 'gp[eax] + 4u);\n        flags.minus', 'gp[eax]);\n        flags.minus'),
    ('notification_group', '0x736e6421u', '0x736e6422u'),
    ('landing_flag', 'flags.logic(state | 0x10u, 1)', 'flags.logic(state | 0x20u, 1)'),
    ('method_result_width', 'flags.logic(gp[eax] & 0xffu, 1);\n        return flags.zf',
     'flags.logic(gp[eax], 4);\n        return flags.zf'),
)


def prepare(xbe):
    walk.image(xbe)
    from xbe import Xbe

    image = Xbe(xbe)
    if walk.sha(image.read(0x2c3248, 0x102)) != MACHINE:
        raise ValueError('Particle contact action machine bytes drifted')
    for site, target in ((0x2c3257,0x178070), (0x2c3275,0x178070), (0x2c32a9,0x250310),
                         (0x2c32bf,0x250380), (0x2c32d4,0x2502b0)):
        code = image.read(site, 5)
        if code[0] != 0xe8 or site + 5 + int.from_bytes(code[1:], 'little', signed=True) != target:
            raise ValueError(f'Particle contact action target at {site:x} drifted')

    canonical = walk.UNIT.read_text()
    _, _, children, _, _, child_pins = walk.prepare()
    ordinary, metadata = walk.specialize(canonical)
    name = binding.walk.OWNER
    if (metadata.get('native_spatial_owners', []).count(name) != 1 or
            name in metadata.get('native_partial_frame_owners', {})):
        raise ValueError('Expected complete native 2C2D90 binding')
    owner = binding.walk.world.body(ordinary, name)[0]
    if owner.count(binding.HOOK) != 1 or binding.install(canonical, ordinary) != (ordinary, []):
        raise ValueError('Particle contact actions expects its production binding once')
    first, end = binding.extract(owner)
    span = owner[first:end].removeprefix(binding.HOOK)
    replay = binding.reference(span)
    if replay not in ordinary or len(walk.literal_sites(replay)) != 80:
        raise ValueError('Particle contact action replay sites drifted')
    live = binding.reference(span, replay=False).replace('dispatch(s,target,', 'methods::dispatch(s,target,')
    for target in binding.CHILDREN:
        live = live.replace(f'run_{target}(s);', f'children::run_{target}(s);')
    fixture = (children + 'namespace children {\n'
               'inline constexpr auto action_local = [](WorldState &value, auto child) {\n'
               + binding.DIRECT + '};\n}\n'
               'namespace prepared {\n' + replay + '}\n'
               'namespace original {\n' + live + '}\n')

    accounted, manifest, _ = walk.instrument_translation(ordinary, canonical, walk.UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
            if row['symbol'] == name or row.get('canonical_reference_owner') == name}
    aliases = {symbol: walk.literal_sites(walk.function(ordinary, symbol)) for symbol in rows if symbol != name}
    total = set.union(*aliases.values())
    original = walk.literal_sites(owner)
    if (len(aliases[binding.REFERENCE]) != 80 or len(aliases) != 6 or
            sum(map(len, aliases.values())) != len(total) or len(original) != 401 or total != original):
        raise ValueError('Particle contact actions must complete the exact, disjoint 401-site native partition')
    return ordinary, accounted, fixture, rows, aliases, child_pins['178070']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    os.environ.pop('HALO2_REFERENCE_PARTICLE_CONTACT_ACTIONS', None)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary, accounted, fixture, rows, sites, resolver_pin = prepare(args.xbe)
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}']
    for name, source in (('ordinary', ordinary), ('accounted', accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    (scratch / 'particle_contact_actions_reference.inc').write_text(fixture)
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
                hit = re.search(r'Particle contact actions verify: (\d+) calls', result.stderr)
                if not hit or int(hit[1]) != count:
                    raise ValueError('Particle contact actions missed a normal verification replay')
                report['verified'] = int(hit[1])

    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle contact action mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before, after, 1))
        binary = directory / 'probe'
        subprocess.run([command[0], f'-I{directory}', *command[1:], str(TEST), '-o', str(binary)], cwd=scratch, check=True)
        result = walk.run(binary, 96, 'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle contact action accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): walk.sha(path.read_bytes()) for path in
                       (walk.UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))},
        machine_pin=MACHINE, manifest=rows, instruction_sites={name: sorted(group) for name,group in sites.items()},
        resolver_source_sha=resolver_pin, production_syntax=dict(ordinary=walk.sha(ordinary), accounted=walk.sha(accounted)),
        callback_fixture='Actual resolver, deterministic notification and indirect-method boundary controls.',
        comparisons=comparisons, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle contact actions completed the pinned 401-site original owner.')


if __name__ == '__main__':
    main()
