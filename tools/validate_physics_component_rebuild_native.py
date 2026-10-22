#!/usr/bin/env python3
"""Strict normal and real ASan/UBSan checks of copied component callers."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

SOURCES=['include/halo2/physics_component_rebuild.hpp','src/physics/physics_component_rebuild.cpp',
 'tests/physics_component_rebuild_tests.cpp','include/halo2/physics_component_construction.hpp',
 'src/physics/physics_component_construction.cpp','include/halo2/raw_datum_pool.hpp','src/gameplay/objects/raw_datum_pool.cpp',
 'include/halo2/object_physics_construction.hpp','src/gameplay/objects/object_physics_construction.cpp']

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    for name in SOURCES:
        dst=out/'source'/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dst)
    commands=[]
    for mode,flags in [('normal',['-O2']),('sanitizer',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
        directory=out/mode;directory.mkdir();binary=directory/'physics_component_rebuild_tests'
        argv=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-fno-fast-math','-ffp-contract=off','-frounding-math',*flags,
            '-I',str(out/'source/include'), '-I'+str(out/'source/src')]+[str(out/'source'/name)for name in SOURCES if name.endswith('.cpp')]+['-o',str(binary)]
        for label,command in [('build',argv),('test',[str(binary),str(a.fixture.resolve())])]:
            with(directory/(label+'.log')).open('w')as log:result=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
            commands.append(dict(mode=mode,phase=label,argv=command,exit_code=result.returncode));(out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
            if result.returncode:raise RuntimeError((directory/(label+'.log')).read_text()[-5000:])
        assert '2 test-only upper-caller lower replays, 109 no-replay required stops, 1 actual native allocator/init/rebuild prefix'in(directory/'test.log').read_text()
    report=dict(schema='physics-component-rebuild-native-validation-v1',validator_sha256=sha(Path(__file__)),
        source_pins={name:sha(ROOT/name)for name in SOURCES},fixture_sha256=sha(a.fixture),commands_sha256=sha(out/'commands.json'),
        normal_and_actual_asan_ubsan_pass=True,complete_upper_caller_recorded_replays=2,native_composed_prefix_replay=False,
        scope='Complete1D2B10+1D9090 upper callers with labeled actual lower test replay; paired original first-lower controls and real outer/allocator/init/rebuild hard stop. No native grouping/world/contact success or live collision.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(out/'report.json'))))

if __name__=='__main__':main()
