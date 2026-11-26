#!/usr/bin/env python3
"""Prove the two 252480 velocity modes that dispatch the random vector child."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_velocity_direction_replacements as binding
import validate_native_particle_velocity_properties_tu15 as properties


ROOT = properties.ROOT
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_velocity_direction.hpp'
TEST = ROOT / 'tests/native_tu15_particle_velocity_direction_tests.cpp'
MACHINE = (
    (0x252628,0x252672,'5175bd9c99fb0c053387f7207ce6dfca7fed9af67febf2cb7d8a1ff015ca2968'),
    (0x252672,0x2526ee,'ac6d234d5c00403a4a39e2843da196f442846cfded674f0207d6f2213ea331c0'),
)
MUTANTS = (
    ('outward_axis','gp[edi] + velocity + 4u), x[0]','gp[edi] + velocity + 8u), x[0]'),
    ('inward_axis','x[1] = _mm_sub_ss(x[1], x[2]);','x[1] = _mm_sub_ss(x[1], x[3]);'),
    ('random_seed','gp[edx] = flags.plus(gp[edx], 4u, 4);\n        gp[eax] = gp[ebp];',
     'gp[edx] = flags.plus(gp[edx], 0u, 4);\n        gp[eax] = gp[ebp];'),
    ('vector_return','generate(0x25267fu, 0x252684u);','generate(0x25267fu, 0x252685u);'),
)


def prepare(xbe):
    properties.image(xbe)
    from xbe import Xbe

    reader = Xbe(xbe)
    if any(properties.sha(reader.read(a,b-a)) != pin for a,b,pin in MACHINE):
        raise ValueError('Particle velocity direction mode bytes drifted')
    for site in (0x25263e,0x25267f):
        call = reader.read(site,5)
        if call[0] != 0xe8 or site + 5 + int.from_bytes(call[1:],'little',signed=True) != 0x148de0:
            raise ValueError(f'Particle velocity direction real dispatch at {site:x} drifted')
    ordinary, accounted, common, _, site_groups, child = properties.prepare()
    canonical = properties.UNIT.read_text()
    owner = binding.properties.world.body(ordinary,binding.properties.OWNER)[0]
    if binding.install(canonical,ordinary) != (ordinary,[]):
        raise ValueError('Particle velocity direction binder is already installed once')
    refs = []
    for name, first, after, call_site, count in binding.MODES:
        a,b = binding.extract(owner,first,after)
        span = owner[a:b].removeprefix(binding.hook(name))
        replay = binding.reference(name,call_site,span)
        if replay not in ordinary or len(properties.literal_sites(replay)) != count:
            raise ValueError(f'Particle velocity direction {name} replay drifted')
        site_groups[name] = properties.literal_sites(replay)
        refs += [replay,binding.reference(name,call_site,span,replay=False)]
    site_groups['retained'] -= site_groups['outward'] | site_groups['inward']
    if len(set.union(*site_groups.values())) != sum(map(len,site_groups.values())) or len(site_groups['retained']) != 468:
        raise ValueError('Particle velocity direction should reach exactly 111 of 579 sites')
    _, manifest, _ = properties.instrument_translation(ordinary,canonical,properties.UNIT.relative_to(ROOT).as_posix(),ROOT)
    rows = {row['symbol']:row for row in manifest['functions']
            if row['symbol'] == binding.properties.OWNER or row.get('canonical_reference_owner') == binding.properties.OWNER}
    if any(rows[binding.alias(mode[0])]['literal_sites'] != mode[4] for mode in binding.MODES):
        raise ValueError('Particle velocity direction accounting sites drifted')
    fixture = common + 'namespace source {\nusing children::dispatch;\n' + '\n'.join(refs) + '}\n'
    return ordinary,accounted,fixture,rows,site_groups,child


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary,accounted,fixture,rows,sites,child = prepare(args.xbe)
    command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
               f'-I{ROOT / "src"}',f'-I{ROOT / "include"}',f'-I{scratch}']
    for name, source in (('ordinary',ordinary),('accounted',accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        result = subprocess.run([*command,'-fsyntax-only',str(cpp)], cwd=scratch, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    (scratch/'particle_velocity_direction_reference.inc').write_text(fixture)
    comparisons = {}
    for name, count, flags in (('release',900,[]),('sanitized',300,[
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])):
        binary = scratch/name
        subprocess.run([*command,*flags,str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        comparisons[name] = {}
        for mode in properties.MODES:
            result = properties.run(binary,count,mode)
            if result.returncode:
                raise RuntimeError(f'{name} {mode}:\n{result.stderr}')
            row = comparisons[name][mode] = json.loads(result.stdout)
            if mode == 'verify':
                hit = re.search(r'Particle velocity directions verify: (\d+) outward, (\d+) inward',result.stderr)
                if not hit or not all(int(word) == count//2 for word in hit.groups()):
                    raise ValueError(f'Particle velocity directions missed a normal mode replay: {result.stderr}')
                row['verified'] = {name:int(word) for name,word in zip(('outward','inward'),hit.groups())}
    rejected = []
    header = HEADER.read_text()
    for name,before,after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle velocity direction mutant {name} needs one source anchor')
        directory = scratch/name
        target = directory / HEADER.relative_to(ROOT/'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before,after,1))
        binary = directory/'probe'
        subprocess.run([command[0],f'-I{directory}',*command[1:],str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        result = properties.run(binary,64,'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle velocity direction accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,owned_scratch=str(scratch),retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)):properties.sha(path.read_bytes()) for path in
                       (properties.UNIT,HEADER,TEST,Path(binding.__file__),Path(__file__))},
        original_pins=MACHINE,manifest=rows,instruction_sites={name:sorted(pcs) for name,pcs in sites.items()},
        actual_curve_child_sha=child,syntax=dict(ordinary=properties.sha(ordinary),accounted=properties.sha(accounted)),
        comparisons=comparisons,rejected_semantics=rejected),indent=2)+'\n')
    print('TU1.5 native velocity direction modes matched both original child routes and memory schedules.')


if __name__ == '__main__':
    main()
