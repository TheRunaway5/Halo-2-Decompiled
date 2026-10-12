#!/usr/bin/env python3
"""Build and verify the selected-image immutable constructor/looping adapter."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir()and not out.exists();out.mkdir(parents=True)
    target='looping_construction_operands_tests'
    names=['include/halo2/original_data.hpp','src/original_data.cpp',
        'include/halo2/object_construction.hpp','include/halo2/object_type_construction.hpp',
        'include/halo2/sound_looping.hpp','tests/looping_construction_operands_tests.cpp']
    observed={}
    for name in names+['CMakeLists.txt']:
        destination=out/'source-before'/name;destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/name,destination);observed[name]=sha(ROOT/name)
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target',target,'-j2']),
        ('test',['ctest','--test-dir',str(build),'-R','^original_looping_construction_operands$','-V','--output-on-failure'])):
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode)
        (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
    log=(out/'test.log').read_text()
    assert 'originalLoopingStrings=114 originalStringBytes=1244 extraConstructorBytes=4 baseUnknown=absent sharedOperandsAndHeaderCoexistence=pass'in log
    assert '100% tests passed out of 1'in log
    for name in names:assert sha(ROOT/name)==observed[name],name
    files=[target,'libhalo2_cache.a']
    for name in(target,'halo2_cache'):files.extend(f'CMakeFiles/{name}.dir/{item}'for item in('flags.make','link.txt'))
    for name in files:
        source=build/name;destination=out/'build'/name;assert source.is_file()
        destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
    if a.sanitizer:
        for name in(target,'halo2_cache'):
            assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{name}.dir/flags.make').read_text(),name
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='looping-construction-operands-production-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],
        copied_build_files=files,actual_sanitizer_flags_checked=a.sanitizer,commands_sha256=sha(out/'commands.json'),
        scope='Immutable owned-image strings, arithmetic, method identity and two-byte sentinel. No mutable source state, world construction, Q completion or live playback.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))

if __name__=='__main__':main()
