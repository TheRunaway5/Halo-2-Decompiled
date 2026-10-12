#!/usr/bin/env python3
"""Validate registered component construction and raw sound request linkage."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir()and not out.exists();out.mkdir(parents=True)
    targets=['physics_component_construction_tests','fp_sound_submit_tests']
    names=['include/halo2/raw_datum_pool.hpp','src/raw_datum_pool.cpp','include/halo2/physics_component_construction.hpp',
        'src/physics_component_construction.cpp','tests/physics_component_construction_tests.cpp',
        'include/halo2/object_physics_construction.hpp','src/object_physics_construction.cpp',
        'include/halo2/fp_sound_submit.hpp','src/fp_sound_submit.cpp','tests/fp_sound_submit_tests.cpp'];observed={}
    for name in names+['CMakeLists.txt']:
        target=out/'source-before'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target);observed[name]=sha(ROOT/name)
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target',*targets,'-j2']),
        ('test',['ctest','--test-dir',str(build),'-R','^original_(physics_component_construction|fp_sound_submit_composition)$','-V','--output-on-failure'])):
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode);(out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
    text=(out/'test.log').read_text()
    assert '511 original cases and required stops, 1561 calls, 59789 stores, 3775976 full compared bytes, no lower replay, host FP context restored'in text
    assert 'PASS 3 authored FP events, 3 complete world/raw-submit compositions, 30 world stores, 99 raw stores'in text
    assert '100% tests passed out of 2'in text
    for name in names:assert sha(ROOT/name)==observed[name],name
    files=targets+['libhalo2_systems.a','libhalo2_cache.a']
    for name in targets+['halo2_systems','halo2_cache']:files.extend(f'CMakeFiles/{name}.dir/{part}'for part in('flags.make','link.txt'))
    for name in files:
        source=build/name;target=out/'build'/name;assert source.is_file();target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    if a.sanitizer:
        for name in targets+['halo2_systems','halo2_cache']:
            assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{name}.dir/flags.make').read_text(),name
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='physics-component-and-fp-submit-production-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],copied_build_files=files,
        actual_sanitizer_flags_checked=a.sanitizer,commands_sha256=sha(out/'commands.json'),
        scope='Registered real allocation/component initializer/no-replay physical caller prefix and actual FP/world/raw-submit cold composition. No live world/solver/driver readiness, grenade completion or audible all-map proof.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))

if __name__=='__main__':main()
