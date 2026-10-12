#!/usr/bin/env python3
"""Validate physical-construction CPU owners through registered production."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir()and not out.exists();out.mkdir(parents=True)
    target='object_physics_construction_tests';names=['include/halo2/object_physics_construction.hpp',
        'src/object_physics_construction.cpp','tests/object_physics_construction_tests.cpp'];observed={}
    for name in names+['CMakeLists.txt']:
        destination=out/'source-before'/name;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/name,destination);observed[name]=sha(ROOT/name)
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target',target,'-j2']),
        ('test',['ctest','--test-dir',str(build),'-R','^original_object_physics_construction$','-V','--output-on-failure'])):
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode)
        (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
    text=(out/'test.log').read_text()
    assert '133 original cases, 147 stores, 4556774 full compared bytes, 5 explicitly replayed lower boundaries for upper-caller-only case'in text
    assert '100% tests passed out of 1'in text
    for name in names:assert sha(ROOT/name)==observed[name],name
    files=[target,'libhalo2_systems.a','libhalo2_cache.a']
    for name in(target,'halo2_systems','halo2_cache'):files.extend(f'CMakeFiles/{name}.dir/{item}'for item in('flags.make','link.txt'))
    for name in files:
        source=build/name;destination=out/'build'/name;assert source.is_file()
        destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
    if a.sanitizer:
        for name in(target,'halo2_systems','halo2_cache'):
            assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{name}.dir/flags.make').read_text(),name
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='object-physics-construction-production-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],copied_build_files=files,
        actual_sanitizer_flags_checked=a.sanitizer,commands_sha256=sha(out/'commands.json'),
        scope='Complete five CPU upper/leaf owners. Five explicitly recorded lower callbacks verify the actual upper caller only; no native lower-world construction or live collision proof.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))

if __name__=='__main__':main()
