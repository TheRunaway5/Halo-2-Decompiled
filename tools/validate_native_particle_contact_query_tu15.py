#!/usr/bin/env python3
"""Verify the bound particle sweep inputs and the live query ABI."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_contact_query_replacements as binding
import validate_native_particle_contact_walk_tu15 as walk


ROOT = walk.ROOT
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_contact_query.hpp'
TEST = ROOT / 'tests/native_tu15_particle_contact_query_tests.cpp'
MACHINE = (
    (0x2c2fd4, 0x2c30b8, '4653630c441c37ab9d0edf7bb11dcac35ee0f01148033e93e6d3fc8ff0b4e3d3'),
    (0x2c334a, 0x2c3366, '9fc7de56eadebad4ccd290a106135d6ab037a60d91f44e6532fbd2b7ffd70355'),
)
MUTANTS = (
    ('nan_destination', 'ordered_mul_ss(x[0], x[3])', 'ordered_mul_ss(x[3], x[0])'),
    ('sweep_axis', 'x[0] = _mm_sub_ss(x[0], x[2]);', 'x[0] = _mm_sub_ss(x[0], x[1]);'),
    ('query_arguments', 'gp[edx] = gp[esp] + 0x30u;', 'gp[edx] = gp[esp] + 0x34u;'),
    ('query_rejection', 'flags.cf || flags.zf', 'flags.cf && flags.zf'),
    ('query_fault_prefix', '            gp = port.s.gp;\n            flags = port.s.flags;\n', ''),
)


def prepare(xbe):
    walk.image(xbe)
    from xbe import Xbe

    image = Xbe(xbe)
    code = image.read(0x2c308b, 5)
    if (any(walk.sha(image.read(start, end-start)) != pin for start,end,pin in MACHINE) or
            code[0] != 0xe8 or 0x2c3090 + int.from_bytes(code[1:], 'little', signed=True) != 0x16c620):
        raise ValueError('Particle contact sweep machine ranges or child target drifted')

    canonical = walk.UNIT.read_text()
    ordinary, receipt = walk.specialize(canonical)
    owner = binding.walk.world.body(ordinary, binding.walk.OWNER)[0]
    admitted = (binding.walk.OWNER in receipt.get('native_partial_frame_owners', {}) or
                receipt.get('native_spatial_owners', []).count(binding.walk.OWNER) == 1)
    if (owner.count(binding.HOOK) != 1 or not admitted or
            binding.install(canonical, ordinary) != (ordinary, [])):
        raise ValueError('Particle contact sweep expects its production continuation after prepared flags')
    begin, end, reject, next_span = binding.extract(owner)
    source = owner[begin:end].removeprefix(binding.HOOK)
    rejects = owner[reject:next_span]
    replay = binding.reference(source, rejects)
    if replay not in ordinary or len(walk.literal_sites(replay)) != 60:
        raise ValueError('Particle contact sweep replay differs from its exact prepared spans')
    live = binding.reference(source, rejects, replay=False).replace(binding.CALL, 'child::' + binding.CALL)

    accounted, manifest, _ = walk.instrument_translation(ordinary, canonical, walk.UNIT.relative_to(ROOT).as_posix(), ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
            if row['symbol'] == binding.walk.OWNER or row.get('canonical_reference_owner') == binding.walk.OWNER}
    aliases = {label: name for label, name in (
        ('setup', binding.walk.SETUP), ('walk', binding.walk.WALK),
        ('flags', binding.flags.REFERENCE), ('query', binding.REFERENCE))}
    sites = {label: walk.literal_sites('static ' + walk.function(ordinary, name)) for label,name in aliases.items()}
    owned = set.union(*sites.values())
    original = walk.literal_sites(owner)
    if (list(map(len, sites.values())) != [71,33,72,60] or len(owned) != 236 or len(original) != 401 or
            not owned < original or {label: rows[name]['literal_sites'] for label,name in aliases.items()} !=
            {label: len(pcs) for label,pcs in sites.items()}):
        raise ValueError('Particle contact sweep did not extend exact disjoint coverage to 236 sites')
    fixture = 'namespace prepared {\n' + replay + '}\nnamespace original {\n' + live + '}\n'
    sites['retained'] = original - owned
    return ordinary, accounted, fixture, rows, sites


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    os.environ.pop('HALO2_REFERENCE_PARTICLE_CONTACT_QUERY', None)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary, accounted, fixture, rows, sites = prepare(args.xbe)
    command = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
               f'-I{ROOT / "src"}', f'-I{ROOT / "include"}', f'-I{scratch}']
    for name, source in (('ordinary', ordinary), ('accounted', accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        subprocess.run([*command, '-fsyntax-only', str(cpp)], cwd=scratch, check=True)
    (scratch / 'particle_contact_query_reference.inc').write_text(fixture)
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
                hit = re.search(r'Particle contact query verify: (\d+) calls', result.stderr)
                if not hit or int(hit[1]) != count:
                    raise ValueError('Particle contact sweep missed a normal verification replay')
                report['verified'] = int(hit[1])

    rejected = []
    header = HEADER.read_text()
    for name, before, after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle contact sweep mutant {name} needs one source anchor')
        directory = scratch / name
        target = directory / HEADER.relative_to(ROOT / 'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before, after, 1))
        binary = directory / 'probe'
        subprocess.run([command[0], f'-I{directory}', *command[1:], str(TEST), '-o', str(binary)], check=True, cwd=scratch)
        result = walk.run(binary, 64, 'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle contact sweep accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True, owned_scratch=str(scratch), retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)): walk.sha(path.read_bytes()) for path in
                       (walk.UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))},
        machine_pins=MACHINE, manifest=rows, instruction_sites={name: sorted(group) for name,group in sites.items()},
        production_syntax=dict(ordinary=walk.sha(ordinary), accounted=walk.sha(accounted)),
        query_fixture='Deterministic external child with six-argument stack and returned register checks.',
        comparisons=comparisons, rejected_semantics=rejected), indent=2) + '\n')
    print('TU1.5 particle contact sweep passed exact source, mask, vector, and query ABI checks.')


if __name__ == '__main__':
    main()
