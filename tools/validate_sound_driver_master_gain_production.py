#!/usr/bin/env python3
"""Record focused production master-gain/import builds with existing flags."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir()and not out.exists();out.mkdir(parents=True)
    source_names=['include/halo2/sound_driver_master_gain.hpp','src/sound_driver_master_gain.cpp',
        'tests/sound_driver_master_gain_tests.cpp','tests/sound_driver_master_gain_constants_tests.cpp',
        'include/halo2/original_data.hpp','src/original_data.cpp']
    observed={}
    for name in source_names+['CMakeLists.txt']:
        target=out/'source-before'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target);observed[name]=sha(ROOT/name)
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target','sound_driver_master_gain_tests','sound_driver_master_gain_constants_tests','-j2']),
        ('test',['ctest','--test-dir',str(build),'-R','^original_sound_driver_master_gain(_constants)?$','--output-on-failure','-V'])):
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode);(out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-4000:])
    text=(out/'test.log').read_text();assert '144 original cases, 2304 stores, 20734272 full compared bytes'in text
    assert 'immutableMasterGainBytes=24 baseUnknown=absent'in text and '100% tests passed'in text
    for name in source_names:assert sha(ROOT/name)==observed[name],name
    copied=['sound_driver_master_gain_tests','sound_driver_master_gain_constants_tests','libhalo2_cache.a']
    for target in ('sound_driver_master_gain_tests','sound_driver_master_gain_constants_tests','halo2_cache'):
        copied.extend(f'CMakeFiles/{target}.dir/{name}'for name in ('flags.make','link.txt'))
    copied.append('CMakeFiles/halo2_cache.dir/build.make')
    for name in copied:
        source=build/name;target=out/'build'/name;assert source.is_file();target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    if a.sanitizer:
        for target in ('sound_driver_master_gain_tests','sound_driver_master_gain_constants_tests','halo2_cache'):
            assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{target}.dir/flags.make').read_text(),target
    strict=(out/'build/CMakeFiles/halo2_cache.dir/build.make').read_text()
    assert 'sound_driver_master_gain.cpp' in strict and '-frounding-math' in strict
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='sound-driver-master-gain-production-v1',validator_sha256=sha(Path(__file__)),observed_before_source_pins=observed,
        cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],
        copied_build_files=copied,actual_sanitizer_flags_checked=a.sanitizer,commands_sha256=sha(out/'commands.json'),
        scope='Focused production CPU master gain/import target only; observed source/archive/flags and CMake drift, no source admission/launcher/playback proof.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))
if __name__=='__main__':main()
