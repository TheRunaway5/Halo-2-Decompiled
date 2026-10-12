#!/usr/bin/env python3
"""Validate copied complete raw allocation/component owners and native prefix."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    names=['include/halo2/raw_datum_pool.hpp','src/raw_datum_pool.cpp','include/halo2/physics_component_construction.hpp',
        'src/physics_component_construction.cpp','tests/physics_component_construction_tests.cpp',
        'include/halo2/object_physics_construction.hpp','src/object_physics_construction.cpp']
    for name in names:
        target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
    commands=[]
    for name,flags in [('normal',['-O2']),('sanitizer',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
        directory=out/name;directory.mkdir();binary=directory/'physics_component_construction_tests'
        argv=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-fno-fast-math','-ffp-contract=off','-frounding-math',*flags,
            '-I',str(out/'source/include')]+[str(out/'source'/n)for n in names if n.endswith('.cpp')]+['-o',str(binary)]
        for label,command in [('build',argv),('test',[str(binary),str(a.fixture.resolve())])]:
            with(directory/(label+'.log')).open('w')as log:result=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
            commands.append(dict(mode=name,phase=label,argv=command,exit_code=result.returncode))
            (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
            if result.returncode:raise RuntimeError((directory/(label+'.log')).read_text()[-5000:])
        assert '511 original cases and required stops, 1561 calls, 59789 stores, 3775976 full compared bytes, no lower replay, host FP context restored'in(directory/'test.log').read_text()
    report=dict(schema='physics-component-construction-native-validation-v1',validator_sha256=sha(Path(__file__)),
        source_pins={name:sha(ROOT/name)for name in names},fixture_sha256=sha(a.fixture),commands_sha256=sha(out/'commands.json'),
        normal_and_asan_ubsan_pass=True,recorded_lower_replay=False,
        scope='Complete16D2C0/1D2AA0/1D3340 plus actual1C6F50 composed allocation prefix; missing1D2B10 remains a hard required stop. Source full x87 stack/sticky state and live world activation remain separate.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'))))

if __name__=='__main__':main()
