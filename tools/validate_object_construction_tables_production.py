#!/usr/bin/env python3
"""Validate immutable construction/admission imports in the existing builds."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir()and not out.exists();out.mkdir(parents=True)
    names=['include/halo2/object_construction_tables.hpp','src/gameplay/objects/object_construction_tables.cpp',
        'tests/object_construction_tables_tests.cpp','include/halo2/original_data.hpp','src/assets/original_data.cpp']
    observed={}
    for name in names+['CMakeLists.txt']:
        target=out/'source-before'/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/name,target);observed[name]=sha(ROOT/name)
    targets=['object_construction_tables_tests','sound_create_constants_tests','sound_driver_master_gain_constants_tests']
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target']+targets+['-j2']),
        ('test',['ctest','--test-dir',str(build),'-R',
            '^original_(object_construction_tables|sound_create_constants|sound_driver_master_gain_constants)$',
            '--output-on-failure','-V'])):
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode)
        (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
    text=(out/'test.log').read_text()
    assert 'immutableConstructionBytes=960 extraAdmissionBytes=8 originalGroupQueries=24 baseUnknown=absent boundedFieldsAndOwners=pass'in text
    assert '100% tests passed out of 3'in text
    for name in names:assert sha(ROOT/name)==observed[name],name
    files=targets+['libhalo2_cache.a','libhalo2_systems.a']
    for target in targets+['halo2_cache','halo2_systems']:
        files.extend(f'CMakeFiles/{target}.dir/{name}'for name in ('flags.make','link.txt'))
    for name in files:
        source=build/name;target=out/'build'/name;assert source.is_file()
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    if a.sanitizer:
        for target in targets+['halo2_cache','halo2_systems']:
            assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{target}.dir/flags.make').read_text(),target
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='object-construction-provider-production-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],
        copied_build_files=files,actual_sanitizer_flags_checked=a.sanitizer,
        commands_sha256=sha(out/'commands.json'),
        scope='Immutable selected-image data/adapter and original group-query ABI; no live type callback, full object/world construction or sound admission.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))

if __name__=='__main__':main()
