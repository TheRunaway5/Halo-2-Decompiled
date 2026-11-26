#!/usr/bin/env python3
"""Verify both native random-angle modes and their live x87 boundaries."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_velocity_angles_replacements as binding
import native_particle_velocity_direction_replacements as direction
import validate_native_particle_velocity_properties_tu15 as properties


ROOT = properties.ROOT
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_velocity_angles.hpp'
TEST = ROOT / 'tests/native_tu15_particle_velocity_angles_tests.cpp'
MACHINE = (
    (0x2524d4,0x252597,'8234a4a4804b81b32ec6e7b92bfc34a7a57b0b4f47f4671d72682497057f8925'),
    (0x252597,0x252628,'ba7b74f64be10c16b5c31735850ff778241cdb1dd8d4bb18df534871d97459ef'),
)
MUTANTS = (
    ('rng_bearing','gp[eax] = port.read(site + 6u, gp[ecx] + 4u, 4);\n        gp[eax] = flags.product(gp[eax], 0x19660du);',
     'gp[eax] = port.read(site + 6u, gp[ecx] + 4u, 4);\n        gp[eax] = flags.product(gp[eax], 0x19661du);'),
    ('planar_sine','trig_top(0x2525dfu, false);','trig_top(0x2525dfu, true);'),
    ('x87_pop','(void)fp.st(0);\n        fp.pop();','(void)fp.st(0);'),
    ('planar_return','site == spatial ? 0x38u : 0x35u','site == spatial ? 0x38u : 0x38u'),
)


def prepare(xbe):
    properties.image(xbe)
    from xbe import Xbe

    reader = Xbe(xbe)
    if any(properties.sha(reader.read(a,b-a)) != pin for a,b,pin in MACHINE):
        raise ValueError('Particle velocity angle mode bytes drifted')
    for site in (0x252516,0x2525d6):
        call = reader.read(site,5)
        if call[0] != 0xe8 or site + 5 + int.from_bytes(call[1:],'little',signed=True) != 0x24ffe0:
            raise ValueError(f'Particle velocity angle real curve call at {site:x} drifted')

    ordinary,accounted,common,_,_,curve_hash = properties.prepare()
    canonical = properties.UNIT.read_text()
    owner = binding.properties.world.body(ordinary,binding.properties.OWNER)[0]
    if binding.install(canonical,ordinary) != (ordinary,[]):
        raise ValueError('Particle velocity angle binder is already installed once')
    refs = []
    for name, first, after, next_label, count, multiplies in binding.MODES:
        a,b = binding.extract(owner,first,after)
        span = owner[a:b].removeprefix(binding.hook(name,next_label))
        replay = binding.reference(name,next_label,multiplies,span)
        if replay not in ordinary or len(properties.literal_sites(replay)) != count:
            raise ValueError(f'Particle velocity angle {name} replay drifted')
        refs += [replay,binding.reference(name,next_label,multiplies,span,replay=False)]
    _, manifest, _ = properties.instrument_translation(ordinary,canonical,properties.UNIT.relative_to(ROOT).as_posix(),ROOT)
    rows = {row['symbol']:row for row in manifest['functions']
            if row['symbol'] == binding.properties.OWNER or row.get('canonical_reference_owner') == binding.properties.OWNER}
    symbols = [properties.binding.PREFIX,properties.binding.SUFFIX]
    symbols += [direction.alias(mode[0]) for mode in direction.MODES]
    symbols += [binding.alias(mode[0]) for mode in binding.MODES]
    sites = {symbol: properties.literal_sites(re.search(
        rf'static void {symbol}\(WorldState &s[^\n]*\) \{{.*?\n\}}',ordinary,re.S)[0]) for symbol in symbols}
    covered = set.union(*sites.values())
    all_sites = properties.literal_sites(owner)
    if (len(covered) != 196 or sum(map(len,sites.values())) != 196 or len(all_sites) != 579 or
            not covered < all_sites or any(rows[key]['literal_sites'] != len(value) for key,value in sites.items())):
        raise ValueError('Particle velocity angle must extend the partition to 196 exact sites')
    sites['retained'] = all_sites - covered
    trig = '\n'.join(re.search(rf'^long double native_f{fn}\(.*?^}}',canonical,re.S|re.M)[0] for fn in ('sin','cos'))
    fixture = (common + 'namespace source {\nusing children::native_single_spill;\n' +
               trig + '\n' + '\n'.join(refs) + '}\n')
    return ordinary,accounted,fixture,rows,sites,curve_hash,properties.sha(trig)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary,accounted,fixture,rows,sites,curve,arithmetic = prepare(args.xbe)
    command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
               f'-I{ROOT / "src"}',f'-I{ROOT / "include"}',f'-I{scratch}']
    for name, source in (('ordinary',ordinary),('accounted',accounted)):
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        result = subprocess.run([*command,'-fsyntax-only',str(cpp)],cwd=scratch,text=True,capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    (scratch/'particle_velocity_angles_reference.inc').write_text(fixture)
    comparisons = {}
    for name,count,flags in (('release',900,[]),('sanitized',300,[
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
                hit = re.search(r'Particle velocity angles verify: (\d+) spatial, (\d+) planar',result.stderr)
                overflow = len(range(0,count,38))
                if not hit or tuple(map(int,hit.groups())) != (count//2-overflow,count//2):
                    raise ValueError(f'Particle velocity angles missed a normal mode replay: {result.stderr}')
                row['verified'] = {name:int(word) for name,word in zip(('spatial','planar'),hit.groups())}
    rejected = []
    header = HEADER.read_text()
    for name,before,after in MUTANTS:
        if header.count(before) != 1:
            raise ValueError(f'Particle velocity angle mutant {name} needs one source anchor')
        directory = scratch/name
        target = directory / HEADER.relative_to(ROOT/'src')
        target.parent.mkdir(parents=True)
        target.write_text(header.replace(before,after,1))
        binary = directory/'probe'
        subprocess.run([command[0],f'-I{directory}',*command[1:],str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        result = properties.run(binary,64,'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle velocity angle accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,owned_scratch=str(scratch),retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)):properties.sha(path.read_bytes()) for path in
                       (properties.UNIT,HEADER,TEST,Path(binding.__file__),Path(__file__))},
        original_pins=MACHINE,manifest=rows,instruction_sites={name:sorted(pcs) for name,pcs in sites.items()},
        actual_curve_child_sha=curve,actual_trig_helper_sha=arithmetic,
        syntax=dict(ordinary=properties.sha(ordinary),accounted=properties.sha(accounted)),comparisons=comparisons,
        rejected_semantics=rejected),indent=2)+'\n')
    print('TU1.5 native velocity angle modes passed the pinned x87 and random-seed comparisons.')


if __name__ == '__main__':
    main()
