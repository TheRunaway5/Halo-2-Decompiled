#!/usr/bin/env python3
"""Prove the complete native Effect Particle Draw 174BF0 observer lens."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import native_particle_observer_lens_replacements as binding
from generated_routine_ownership import qualified_dispatch_entries
from instrument_native_execution import instrument_translation
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
CALLER = ROOT / 'src/rendering/effects/source_world_particle_draws_tu15.cpp'
HEADER = ROOT / 'src/rendering/effects/native_tu15_particle_observer_lens.hpp'
TEST = ROOT / 'tests/native_tu15_particle_observer_lens_tests.cpp'
IMAGE = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
MACHINE = '591a38f13896d68cdf3480b0d3b4d6ec0bc9dbeef288209e75c84be283d6651a'
CALLER_PIN = '990f3d62b16a40d3184c531ff6f794c1e59fcb34fcef38ab034f5b4773796e88'
HELPERS = {
    'native_read64':'5365e17a8ead608ed6628a60f2943303f8e10222a5162aadc6561e6e0f00bdc0',
    'native_single_spill':'e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1',
    'native_fsin':'610539eda393c4b305d8f76738a9ddd0408a21ede2fd054fdb1c7d8f4c05fe42',
    'native_fcos':'cf89f62db28bc71dd32f516c2c4f9cfaa508914770c6b8b8101b6869383cfb9c',
    'native_fptan':binding.TANGENT_SHA,
    'native_fpatan':'19772bcc99997a9061fc1f801b4e43949dbefff8e69b4df5c915170458890d33',
}
MUTANTS = (
    ('aspect_ratio','extended(0x174c39u, gp[esi] + aspect_ratio)',
     'extended(0x174c39u, gp[esi] + yaw_scale)'),
    ('tilt_axis','gp[ecx] = word(0x174c2du, reference_axis);',
     'gp[ecx] = gp[eax];'),
    ('tangent_stack','if (!(status & 0x400u)) fp.push(1.L);','if (status & 0x400u) fp.push(1.L);'),
    ('rotation_lane','scalar(0x175004u, gp[edx] + 4u)','scalar(0x175004u, gp[edx] + 8u)'),
    ('lens_floor','if (flags.cf || flags.zf) x[0] = x[4];','if (flags.cf && flags.zf) x[0] = x[4];'),
    ('projection_store','spill_pop(0x1751a1u, gp[esi] + 0x48u);','spill_pop(0x1751a1u, gp[esi] + 0x44u);'),
    ('caller_stack','port.ret(0x1751a7u, 0u);','port.ret(0x1751a7u, 4u);'),
)


def sha(value):
    return hashlib.sha256(value.encode() if isinstance(value,str) else value).hexdigest()


def prepare(xbe):
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe

    reader = Xbe(xbe)
    if sha(reader.data) != IMAGE or sha(reader.read(0x174bf0,0x5b8)) != MACHINE:
        raise ValueError('Particle observer lens TU1.5 executable or owner drifted')
    caller = re.search(r'^void SourceWorldParticleDrawsTu15::publish_observer\(.*?^}',CALLER.read_text(),re.S|re.M)
    if not caller or sha(caller[0]) != CALLER_PIN:
        raise ValueError('Particle observer lens production caller drifted')
    canonical = UNIT.read_text()
    owner_entry = qualified_dispatch_entries(canonical,'tu15').get(binding.OWNER)
    if not owner_entry or owner_entry['entry'] != 0x174bf0:
        raise ValueError('No original TU1.5 particle observer dispatcher entry')
    ordinary,receipt = specialize(canonical)
    if (not binding.admitted(canonical) or receipt.get('native_spatial_owners',[]).count(binding.OWNER) != 1 or
            binding.install(canonical,ordinary) != (ordinary,[])):
        raise ValueError('Expected one installed Effect Particle Draw particle observer lens owner')
    original = binding.world.body(ordinary,binding.OWNER)[0]
    replay = binding.reference(original.replace(binding.HOOK,'',1))
    if original.count(binding.HOOK) != 1 or ordinary.count(replay) != 1:
        raise ValueError('Particle observer lens reference does not exactly retain the ordinary owner')
    sites = lambda body: re.findall(r's\.instruction_literal\(0x([0-9a-f]+)u\)',body)
    ordered_sites = sites(replay)
    if (ordered_sites != sites(original) or len(ordered_sites) != 359 or len(set(ordered_sites)) != 359):
        raise ValueError('Particle observer lens lost an original literal owner')
    helpers = {}
    for name,pin in HELPERS.items():
        fragment = re.search(rf'^(?:long double|std::uint32_t|std::uint64_t) {name}\(.*?^}}',canonical,re.M|re.S)
        if not fragment or sha(fragment[0]) != pin:
            raise ValueError(f'Expected the original {name} helper')
        helpers[name] = binding.fixed_tangent(fragment[0]) if name == 'native_fptan' else fragment[0]
    if binding.TANGENT.search(ordinary)[0] != helpers['native_fptan']:
        raise ValueError('Particle observer lens FPTAN status register can clobber its store address')
    fixture = ('namespace particle_observer_lens_source {\nusing namespace frame_misc_harness;\n'
               'using namespace halo2::physics_solver_detail;\n' + '\n'.join(helpers.values()) + '\n' + replay + '\n}\n')
    accounted,manifest,_ = instrument_translation(ordinary,canonical,UNIT.relative_to(ROOT).as_posix(),ROOT)
    rows = {row['symbol']:row for row in manifest['functions']
            if row['symbol'] == binding.OWNER or row.get('canonical_reference_owner') == binding.OWNER}
    if {key:row['literal_sites'] for key,row in rows.items()} != {binding.OWNER:359,binding.REFERENCE:359}:
        raise ValueError('Particle observer lens execution accounting owner drifted')
    return ordinary,accounted,fixture,ordered_sites,rows,owner_entry


def run(binary, count, mode):
    env = os.environ.copy()
    for flag in ('HALO2_REFERENCE_PARTICLE_OBSERVER_LENS','HALO2_REFERENCE_PHYSICS_WORLD_LEAVES',
                 'HALO2_VERIFY_PHYSICS_WORLD_LEAVES','HALO2_CENSUS_NATIVE_BRIDGES'):
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
        cpp = scratch / f'{name}.cpp'
        cpp.write_text(source)
        result = subprocess.run([*command,'-fsyntax-only',str(cpp)],cwd=scratch,text=True,capture_output=True)
        if result.returncode:
            raise RuntimeError(result.stderr)
    (scratch/'particle_observer_lens_reference.inc').write_text(fixture)
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
            expected = int(re.search(r'Particle observer lens fixture returns: (\d+)',result.stderr)[1])
            if not 0 < expected < count:
                raise ValueError('Particle observer lens fixture did not exercise full and x87-faulting runs')
            row['normal_returns'] = expected
            if mode == 'verify':
                verification = re.search(r'Particle observer lens verify: (\d+) calls',result.stderr)
                if not verification or int(verification[1]) != expected:
                    raise ValueError(f'Particle observer lens missed reference verification: {result.stderr}')
                row['verified'] = int(verification[1])
    rejected = []
    contents = HEADER.read_text()
    for name,before,after in MUTANTS:
        if contents.count(before) != 1:
            raise ValueError(f'Particle observer lens mutant {name} needs one source anchor')
        shadow = scratch/name
        target = shadow / HEADER.relative_to(ROOT/'src')
        target.parent.mkdir(parents=True)
        target.write_text(contents.replace(before,after,1))
        binary = shadow/'probe'
        subprocess.run([command[0],f'-I{shadow}',*command[1:],str(TEST),'-o',str(binary)],cwd=scratch,check=True)
        result = run(binary,64,'literal')
        if result.returncode != 1 or 'differential mismatch' not in result.stderr:
            raise ValueError(f'Particle observer lens accepted mutant {name}: {result.stderr}')
        rejected.append(name)
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(dict(passed=True,owned_scratch=str(scratch),retained_large_artifacts=[],
        source_hashes={str(path.relative_to(ROOT)):sha(path.read_bytes()) for path in
                       (UNIT,CALLER,HEADER,TEST,Path(binding.__file__),Path(__file__))},
        xbe_sha=IMAGE,machine_body_sha=MACHINE,live_caller_body_sha=CALLER_PIN,authentic_helpers=HELPERS,
        prepared_tangent_sha=sha(binding.TANGENT.search(ordinary)[0]),
        actual_entry_identity=entry,instruction_sites=sites,manifest=rows,
        syntax=dict(ordinary=sha(ordinary),accounted=sha(accounted)),comparisons=comparisons,
        rejected_semantics=rejected),indent=2)+'\n')
    print('TU1.5 particle observer lens passed pinned ABI, x87 and projection comparisons.')


if __name__ == '__main__':
    main()
