#!/usr/bin/env python3
"""Registered real component caller/operand integration and prior regressions."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha
from validate_physics_component_rebuild_native import SOURCES

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--sanitizer',action='store_true');a=p.parse_args()
    build=a.build.resolve();out=a.output.resolve();assert build.is_dir()and not out.exists();out.mkdir(parents=True)
    targets=['physics_component_rebuild_tests','physics_construction_operands_tests','physics_component_construction_tests',
        'object_physics_construction_tests','object_preparation_pressure_tests','looping_construction_operands_tests','fp_sound_submit_tests']
    names=SOURCES+['include/halo2/original_data.hpp','src/original_data.cpp','tests/physics_construction_operands_tests.cpp',
        'include/halo2/object_preparation_pressure.hpp'];observed={}
    for name in names+['CMakeLists.txt']:
        dst=out/'source-before'/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dst);observed[name]=sha(ROOT/name)
    commands={}
    pattern='^original_(physics_component_rebuild|physics_construction_operands|physics_component_construction|object_physics_construction|object_preparation_pressure|looping_construction_operands|fp_sound_submit_composition)$'
    for label,argv in [('configure',['cmake','-S',str(ROOT),'-B',str(build)]),
        ('build',['cmake','--build',str(build),'--target',*targets,'-j2']),
        ('test',['ctest','--test-dir',str(build),'-R',pattern,'-V','--output-on-failure'])]:
        with(out/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        commands[label]=dict(argv=argv,exit_code=result.returncode);(out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
        if result.returncode:raise RuntimeError((out/(label+'.log')).read_text()[-5000:])
    log=(out/'test.log').read_text();assert '100% tests passed out of 7'in log
    assert '2 test-only upper-caller lower replays, 109 no-replay required stops, 1 actual native allocator/init/rebuild prefix'in log
    assert '56 scalar bytes, 74 indexed table bytes, two authenticated images; base absent, mutable state excluded'in log
    for name in names:assert sha(ROOT/name)==observed[name],name
    files=targets+['libhalo2_cache.a','libhalo2_systems.a']
    for name in targets+['halo2_cache','halo2_systems']:files.extend(f'CMakeFiles/{name}.dir/{part}'for part in('flags.make','link.txt'))
    for name in files:
        source=build/name;dst=out/'build'/name;assert source.is_file();dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,dst)
    if a.sanitizer:
        for name in targets+['halo2_cache','halo2_systems']:assert '-fsanitize=address,undefined'in(out/'build/CMakeFiles'/f'{name}.dir/flags.make').read_text(),name
    shutil.copy2(ROOT/'CMakeLists.txt',out/'CMakeLists.after.txt')
    report=dict(schema='physics-component-rebuild-production-v1',validator_sha256=sha(Path(__file__)),
        observed_before_source_pins=observed,cmake_after_sha256=sha(ROOT/'CMakeLists.txt'),
        cmake_changed_during_build=sha(ROOT/'CMakeLists.txt')!=observed['CMakeLists.txt'],actual_sanitizer_flags_checked=a.sanitizer,
        commands_sha256=sha(out/'commands.json'),copied_build_files=files,registered_tests=7,
        scope='Registered raw allocation/component/dispatch callers and selected-image immutable imports, prior pressure/constructor/looping/FP request regressions. Required native grouping/world/device startup and live Q/R/collision remain unresolved.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(out/'report.json'),actual_sanitizer_flags_checked=a.sanitizer)))

if __name__=='__main__':main()
