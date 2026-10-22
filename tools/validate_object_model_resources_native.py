#!/usr/bin/env python3
"""Build copied source model-query CPU owners against original-only fixtures."""
import argparse,json,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    names=['include/halo2/object_model_resources.hpp','src/gameplay/objects/object_model_resources.cpp','tests/object_model_resources_tests.cpp']
    for name in names:
        target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,target)
    commands=[]
    for name,flags in [('normal',['-O2']),('sanitizer',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
        directory=out/name;directory.mkdir();binary=directory/'object_model_resources_tests'
        for label,argv in [('build',['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror',*flags,
            '-I',str(out/'source/include'), '-I'+str(out/'source/src'),str(out/'source/src/gameplay/objects/object_model_resources.cpp'),
            str(out/'source/tests/object_model_resources_tests.cpp'),'-o',str(binary)]),
            ('test',[str(binary),str(a.fixture.resolve())])]:
            with(directory/(label+'.log')).open('w')as log:result=subprocess.run(argv,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
            commands.append(dict(mode=name,phase=label,argv=argv,exit_code=result.returncode))
            (out/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
            if result.returncode:raise RuntimeError((directory/(label+'.log')).read_text()[-4000:])
        assert '60 original cases, 96 stores, 21120 full compared bytes'in(directory/'test.log').read_text()
    report=dict(schema='object-model-resource-native-validation-v1',validator_sha256=sha(Path(__file__)),
        source_pins={name:sha(ROOT/name)for name in names},fixture_sha256=sha(a.fixture),
        commands_sha256=sha(out/'commands.json'),normal_and_asan_ubsan_pass=True,
        scope='Complete CPU query/ordered caller outputs only; retained source fields and explicit query controls, no live model admission.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'))))

if __name__=='__main__':main()
