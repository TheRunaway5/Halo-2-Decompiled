#!/usr/bin/env python3
"""Record focused production builds without changing existing build flags."""
import argparse
import json
import shutil
import subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizer',action='store_true')
    args=parser.parse_args(); build=args.build.resolve(); out=args.output.resolve()
    assert build.is_dir() and not out.exists(); out.mkdir(parents=True)
    source_names=['include/halo2/object_creation_request.hpp','src/object_creation_request.cpp',
                  'tests/object_creation_request_tests.cpp']
    observed={}
    for name in source_names+['CMakeLists.txt']:
        target=out/'source-before'/name; target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ROOT/name,target); observed[name]=sha(ROOT/name)
    commands={}
    for label,argv in (
        ('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target','object_creation_request_tests','-j2']),
        ('test',['ctest','--test-dir',str(build),'-R','^original_object_creation_request$','--output-on-failure','-V'])):
        with (out/(label+'.log')).open('w') as log:
            result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode)
        (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:
            raise RuntimeError((out/(label+'.log')).read_text()[-4000:])
    test_log=(out/'test.log').read_text()
    assert '62 original cases, 902 stores, 1975940 full compared bytes' in test_log
    assert '100% tests passed' in test_log
    for name in source_names:
        assert sha(ROOT/name)==observed[name],name
    copied=[]
    for name in ['object_creation_request_tests','libhalo2_systems.a','libhalo2_cache.a',
        'CMakeFiles/object_creation_request_tests.dir/flags.make','CMakeFiles/object_creation_request_tests.dir/link.txt',
        'CMakeFiles/halo2_systems.dir/flags.make','CMakeFiles/halo2_systems.dir/link.txt',
        'CMakeFiles/halo2_cache.dir/flags.make','CMakeFiles/halo2_cache.dir/link.txt']:
        original=build/name; target=out/'build'/name
        assert original.is_file(); target.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(original,target)
        copied.append(name)
    if args.sanitizer:
        for target in ('object_creation_request_tests','halo2_systems','halo2_cache'):
            assert '-fsanitize=address,undefined' in (out/'build/CMakeFiles'/f'{target}.dir/flags.make').read_text(),target
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='object-creation-request-production-validation-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],
        copied_build_files=copied,actual_sanitizer_flags_checked=args.sanitizer,
        commands_sha256=sha(out/'commands.json'),scope='Focused current production target; observed archives/flags/CMake only, no whole-tree freshness/launcher/gameplay claim.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=args.sanitizer)))


if __name__=='__main__':
    main()
