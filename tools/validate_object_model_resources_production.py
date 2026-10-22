#!/usr/bin/env python3
"""Record a focused registered production object-model CPU comparison."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir();out.mkdir(parents=True,exist_ok=False)
    names=['include/halo2/object_model_resources.hpp','src/gameplay/objects/object_model_resources.cpp','tests/object_model_resources_tests.cpp']
    observed={}
    for name in names+['CMakeLists.txt']:
        target=out/'source-before'/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/name,target);observed[name]=sha(ROOT/name)
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target','object_model_resources_tests','-j2']),
        ('test',['ctest','--test-dir',str(build),'-R','^original_object_model_resources$','--output-on-failure','-V'])):
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode)
        (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
    text=(out/'test.log').read_text()
    assert '60 original cases, 96 stores, 21120 full compared bytes'in text and '100% tests passed'in text
    for name in names:assert sha(ROOT/name)==observed[name],name
    files=['object_model_resources_tests','libhalo2_cache.a','libhalo2_systems.a']
    for target in ('object_model_resources_tests','halo2_cache','halo2_systems'):
        files.extend(f'CMakeFiles/{target}.dir/{name}'for name in ('flags.make','link.txt'))
    for name in files:
        source=build/name;target=out/'build'/name;assert source.is_file()
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    if a.sanitizer:
        for target in ('object_model_resources_tests','halo2_cache','halo2_systems'):
            assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{target}.dir/flags.make').read_text(),target
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='object-model-resource-production-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],
        copied_build_files=files,actual_sanitizer_flags_checked=a.sanitizer,commands_sha256=sha(out/'commands.json'),
        scope='Registered complete CPU query and caller output stores; selected source/archive snapshots, no unrelated freshness or live object admission.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))

if __name__=='__main__':main()
