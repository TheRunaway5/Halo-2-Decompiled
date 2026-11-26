#!/usr/bin/env python3
"""Prove the complete Death Cycle 252480 native owner and its original dispatch."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_particle_velocity_remaining_replacements as binding
import validate_native_particle_velocity_properties_tu15 as properties


ROOT = properties.ROOT
TEST = ROOT / 'tests/native_tu15_particle_velocity_remaining_tests.cpp'
EFFECTS = ROOT / 'src/rendering/effects'
HEADERS = {name: EFFECTS / f'native_tu15_particle_velocity_{name}.hpp'
           for name in ('cone','bounds','special')}
MUTANTS = (
    ('cone','pitch','angle.trig_top(0x252783u, false);','angle.trig_top(0x252783u, true);'),
    ('cone','direction_return','0x252879u, 0x25287eu, child','0x252879u, 0x25287fu, child'),
    ('bounds','face_selection','if (choose_face()) first_axis();','if (!choose_face()) first_axis();'),
    ('bounds','range_return','range(0x252ae6u, 0x252aebu);','range(0x252ae6u, 0x252aacu);'),
    ('special','planar_velocity','gp[edi] + angles::velocity + 8u, x[0]','gp[edi] + angles::velocity + 4u, x[0]'),
    ('special','alternate_store','angle.store_extended(alternate_exit, gp[edi] + angles::velocity + 8u);',
     'angle.store_extended(alternate_exit, gp[edi] + angles::velocity + 4u);'),
    ('special','table_index','table + port.gp[eax] * 4u','table + port.gp[eax] * 8u'),
)


def prepare(xbe):
    properties.image(xbe)
    from xbe import Xbe

    reader = Xbe(xbe)
    child_calls = tuple(call for mode in binding.MODES for call in (
        tuple((at,0x24ffe0) for at in mode.curves) + mode.required))
    for at,target in child_calls:
        call = reader.read(at,5)
        if call[0] != 0xe8 or at + 5 + int.from_bytes(call[1:],'little',signed=True) != target:
            raise ValueError(f'Particle velocity remaining child {at:x} no longer calls {target:x}')
    ordinary,accounted,common,_,_,curve = properties.prepare()
    canonical = properties.UNIT.read_text()
    if binding.install(canonical,ordinary) != (ordinary,[]):
        raise ValueError('Complete particle velocity binder is already installed once')
    _,receipt = properties.specialize(canonical)
    if (receipt.get('native_spatial_owners',[]).count(binding.OWNER) != 1 or
            binding.OWNER in receipt.get('native_partial_frame_owners', {})):
        raise ValueError('Expected a complete production particle velocity owner')
    original = binding.properties.world.body(ordinary,binding.OWNER)[0]
    if binding.selection_hook() not in original:
        raise ValueError('Native particle velocity selector missing from actual production specialization')
    replay = binding.selection_reference()
    if replay not in ordinary:
        raise ValueError('Native particle velocity selector reference drifted')
    refs = (replay,)
    for mode in binding.MODES:
        first,last = binding.angles.extract(original,mode.start,mode.end)
        span = original[first:last]
        if not span.startswith(binding.hook(mode)):
            raise ValueError(f'Native particle velocity {mode.name} not at actual production route')
        span = span.removeprefix(binding.hook(mode))
        replay = binding.reference(mode,span)
        if replay not in ordinary or len(properties.literal_sites(replay)) != mode.sites:
            raise ValueError(f'Native particle velocity {mode.name} reference drifted')
        refs += (replay,binding.reference(mode,span,replay=False))
    trig = '\n'.join(re.search(rf'^long double native_f{fn}\(.*?^}}',canonical,re.S|re.M)[0] for fn in ('sin','cos'))
    lookup = re.search(r'^std::uint32_t canonical_branch\([^\n]+',canonical,re.M)[0]
    fixture = (common + 'namespace source {\nusing children::dispatch;\nusing children::native_single_spill;\n' +
               trig + '\n' + lookup + '\n' + '\n'.join(refs) + '}\n')
    _,manifest,_ = properties.instrument_translation(ordinary,canonical,properties.UNIT.relative_to(ROOT).as_posix(),ROOT)
    rows = {row['symbol']: row for row in manifest['functions']
            if row['symbol'] == binding.OWNER or row.get('canonical_reference_owner') == binding.OWNER}
    numbers = {binding.properties.PREFIX:20,binding.properties.SUFFIX:45,binding.alias('selection'):1}
    numbers.update({binding.direction.alias(mode[0]):mode[4] for mode in binding.direction.MODES})
    numbers.update({binding.angles.alias(mode[0]):mode[4] for mode in binding.angles.MODES})
    numbers.update({binding.alias(mode.name):mode.sites for mode in binding.MODES})
    sites = {}
    for name,count in numbers.items():
        match = re.search(rf'^static (?:void|std::uint32_t) {name}\(WorldState &s[^\n]*\) \{{.*?^}}',
                          ordinary,re.S | re.M)
        sites[name] = properties.literal_sites(match[0])
        if len(sites[name]) != count:
            raise ValueError(f'Particle velocity owner {name} owns the wrong number of literal sites')
    if ({name:row['literal_sites'] for name,row in rows.items()} != {binding.OWNER:579, **numbers} or
            sum(map(len,sites.values())) != 579 or set.union(*sites.values()) != properties.literal_sites(original)):
        raise ValueError('Particle velocity reference aliases do not partition all 579 literal sites exactly')
    return ordinary,accounted,fixture,rows,sites,curve,properties.sha(trig),properties.sha(lookup),child_calls


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary,accounted,fixture,rows,sites,curve,trig,lookup,calls = prepare(args.xbe)
    command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
               f'-I{ROOT / "src"}',f'-I{ROOT / "include"}',f'-I{scratch}']
    for name,source in (('ordinary',ordinary),('accounted',accounted)):
        cpp = scratch/f'{name}.cpp'
        cpp.write_text(source)
        result = subprocess.run([*command,'-fsyntax-only',str(cpp)],cwd=scratch,text=True,capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    (scratch/'particle_velocity_remaining_reference.inc').write_text(fixture)
    comparisons = {}
    summaries = (
        ('Particle velocity cone verify: (\\d+) mode 4, (\\d+) mode 5', ('mode4','mode5')),
        ('Particle velocity bounds verify: (\\d+) mode 6, (\\d+) mode 7', ('mode6','mode7')),
        ('Particle velocity special verify: (\\d+) mode 8, (\\d+) alternate, (\\d+) selections',
         ('mode8','alternate','selection')),
    )
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
                verified = row['verified'] = {}
                for pattern,names in summaries:
                    found = re.search(pattern,result.stderr)
                    if not found:
                        raise ValueError(f'Particle velocity mode missed reference verification: {result.stderr}')
                    verified.update(zip(names,map(int,found.groups())))
                expected = {mode.name:count // 6 for mode in binding.MODES}
                # Canonical resolution itself throws for one hostile table value per twelve.
                expected['selection'] = count - count // 12
                if verified != expected:
                    raise ValueError(f'Particle velocity replay counts {verified} differ from expected {expected}')
    rejected = []
    for name,mutant,before,after in MUTANTS:
        header = HEADERS[name]
        content = header.read_text()
        if content.count(before) != 1:
            raise ValueError(f'Particle velocity mutant {mutant} needs one source anchor')
        directory = scratch/mutant
        target = directory/header.relative_to(ROOT/'src')
        target.parent.mkdir(parents=True)
        target.write_text(content.replace(before,after,1))
        binary = directory/'probe'
        subprocess.run([command[0],f'-I{directory}',*command[1:],str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        result = properties.run(binary,72,'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle velocity accepted mutant {mutant}: {result.stderr}')
        rejected.append(mutant)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,owned_scratch=str(scratch),retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)):properties.sha(path.read_bytes()) for path in
                       (properties.UNIT,*HEADERS.values(),TEST,Path(binding.__file__),Path(__file__))},
        original_pins=dict(owner=properties.MACHINE, selector_table=properties.TABLE, child_calls=calls),
        manifest=rows,instruction_sites={name:sorted(pcs) for name,pcs in sites.items()},
        actual_curve_child_sha=curve,actual_trig_helper_sha=trig,actual_table_resolver_sha=lookup,
        syntax=dict(ordinary=properties.sha(ordinary),accounted=properties.sha(accounted)),comparisons=comparisons,
        rejected_semantics=rejected),indent=2)+'\n')
    print('TU1.5 particle velocity owner passed all native model and dispatch comparisons.')


if __name__ == '__main__':
    main()
