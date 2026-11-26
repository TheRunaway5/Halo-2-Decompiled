#!/usr/bin/env python3
"""Prove the complete Effect Particle Frame 2C40D0 shape iterator."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_particle_physics_shapes_replacements as binding
from generated_routine_ownership import qualified_dispatch_entries
from instrument_native_execution import instrument_translation
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT/'src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'
HEADER = ROOT/'src/rendering/effects/native_tu15_particle_physics_shapes.hpp'
TEST = ROOT/'tests/native_tu15_particle_physics_shapes_tests.cpp'
IMAGE = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE = 'b7e9213827413931b68874d7b0f81b7e7bf858e92e7d1249fe92f461ff969128'
MACHINE_CALLS = ((0x250757,0x2c40d0), (0x2c40f7,0x2c2ad0))
MUTANTS = (
    ('live_count', 'gp[eax] = port.read(0x2c40fcu, gp[edi] + shape_count, 4);',
     'port.read(0x2c40fcu, gp[edi] + shape_count, 4);\n            gp[eax] = 1u;'),
    ('stride', 'shape_stride = 0x14u;', 'shape_stride = 0x18u;'),
    ('signed_count', 'if (!flags.zf && flags.sf == flags.of)', 'if (!flags.zf && !flags.cf)'),
    ('motion_return', 'call_local<motion>(0x2c40f7u, 0x2c40fcu);',
     'call_local<motion>(0x2c40f7u, 0x2c40fdu);'),
    ('callee_stack_cleanup', 'port.ret(last, 12u);','port.ret(last, 0u);'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value,str) else value).hexdigest()


def prepare(xbe):
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    reader = Xbe(xbe)
    if sha(reader.data) != IMAGE or sha(reader.read(0x2c40d0,0x3d)) != MACHINE:
        raise ValueError('Particle physics shapes TU1.5 executable or owner drifted')
    for at,target in MACHINE_CALLS:
        call = reader.read(at,5)
        if call[0] != 0xe8 or at + 5 + int.from_bytes(call[1:],'little',signed=True) != target:
            raise ValueError(f'Particle physics shape iteration real call at {at:x} drifted')
    canonical = UNIT.read_text()
    entry = qualified_dispatch_entries(canonical,'tu15').get(binding.OWNER)
    if not entry or entry['entry'] != 0x2c40d0:
        raise ValueError('No original TU1.5 particle physics shape iterator entry')
    ordinary,receipt = specialize(canonical)
    if (not binding.admitted(canonical) or receipt.get('native_spatial_owners',[]).count(binding.OWNER) != 1 or
            binding.install(canonical,ordinary) != (ordinary,[])):
        raise ValueError('Expected one complete production particle physics shape iterator')
    original = binding.world.body(ordinary,binding.OWNER)[0]
    replay = binding.reference(original.replace(binding.HOOK,'',1))
    motion_body = binding.world.body(ordinary,binding.MOTION)[0]
    location_loop = binding.world.body(ordinary,'251a60')[0]
    if (original.count(binding.HOOK) != 1 or ordinary.count(replay) != 1 or
            '::bridge_2c2ad0(' not in motion_body or '[](auto &v){run_2c40d0(v);}' not in location_loop):
        raise ValueError('Particle physics shape iteration lost its live local motion or emitter route')
    sites = lambda body: re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)',body)
    ordered_sites = sites(replay)
    if (ordered_sites != sites(original) or len(ordered_sites) != 28 or len(set(ordered_sites)) != 28):
        raise ValueError('Particle physics shape iteration lost an original literal owner')
    fixture = ('namespace shape_source {\nusing namespace frame_misc_harness;\nusing shape_child::run_2c2ad0;\n' +
               replay + '\n' + binding.reference(original.replace(binding.HOOK,'',1),replay=False) + '\n}\n')
    accounted,manifest,_ = instrument_translation(ordinary,canonical,UNIT.relative_to(ROOT).as_posix(),ROOT)
    rows = {row['symbol']:row for row in manifest['functions']
            if row['symbol'] == binding.OWNER or row.get('canonical_reference_owner') == binding.OWNER}
    if {key:row['literal_sites'] for key,row in rows.items()} != {binding.OWNER:28,binding.REFERENCE:28}:
        raise ValueError('Particle physics shape iterator execution accounting changed')
    return ordinary,accounted,fixture,ordered_sites,rows,entry


def run(binary,count,mode):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_PHYSICS_SHAPES','HALO2_REFERENCE_PARTICLE_SYSTEM_UPDATE',
                 'HALO2_REFERENCE_PHYSICS_WORLD_LEAVES','HALO2_VERIFY_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES'):
        env.pop(flag,None)
    return subprocess.run([str(binary),str(count),mode],cwd=binary.parent,env=env,
                          capture_output=True,text=True,timeout=90)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--xbe',required=True,type=Path)
    parser.add_argument('--report',required=True,type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    ordinary,accounted,fixture,sites,rows,entry = prepare(args.xbe)
    command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
               f'-I{ROOT/"src"}',f'-I{ROOT/"include"}',f'-I{scratch}']
    for name,source in (('ordinary',ordinary),('accounted',accounted)):
        cpp = scratch/f'{name}.cpp'
        cpp.write_text(source)
        result = subprocess.run([*command,'-fsyntax-only',str(cpp)],cwd=scratch,text=True,capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    (scratch/'particle_physics_shapes_reference.inc').write_text(fixture)
    comparisons = {}
    for name,count,flags in (('release',900,[]),('sanitized',300,[
            '-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])):
        binary = scratch/name
        subprocess.run([*command,*flags,str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        comparisons[name] = {}
        for mode in ('literal','census','verify','collector'):
            result = run(binary,count,mode)
            if result.returncode:
                raise RuntimeError(f'{name} {mode}:\n{result.stderr}')
            row = comparisons[name][mode] = json.loads(result.stdout)
            if mode == 'verify':
                verification = re.search(r'Particle physics shapes verify: (\d+) calls',result.stderr)
                if not verification or int(verification[1]) != count:
                    raise ValueError(f'Particle physics shape iterator missed reference runs: {result.stderr}')
                row['verified'] = count
    rejected = []
    contents = HEADER.read_text()
    for name,before,after in MUTANTS:
        if contents.count(before) != 1:
            raise ValueError(f'Particle physics shapes mutant {name} needs one source anchor')
        shadow = scratch/name
        target = shadow/HEADER.relative_to(ROOT/'src')
        target.parent.mkdir(parents=True)
        target.write_text(contents.replace(before,after,1))
        binary = shadow/'probe'
        subprocess.run([command[0],f'-I{shadow}',*command[1:],str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        result = run(binary,70,'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle physics shape iterator accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,owned_scratch=str(scratch),retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in
                       (UNIT,HEADER,TEST,Path(binding.__file__),Path(__file__))},
        xbe_sha=IMAGE,machine_body_sha=MACHINE,machine_calls=MACHINE_CALLS,actual_entry_identity=entry,
        instruction_sites=sites,manifest=rows,syntax=dict(ordinary=sha(ordinary),accounted=sha(accounted)),
        comparisons=comparisons,rejected_semantics=rejected),indent=2)+'\n')
    print('TU1.5 particle physics shape iteration and motion boundaries passed.')


if __name__ == '__main__':
    main()
