#!/usr/bin/env python3
"""Validate copied native request/lookup owners against original whole buffers."""
import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT, sha

FIXTURE=ROOT/'analysis/multiplayer/integration/object-creation-request-native/fixtures/fixtures.tsv'
FIXTURE_PIN='63a0d97e641d3572c3c07ecf21122db21a55a54971124f901241a255a6dfbfba'


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args(); out=args.output.resolve()
    assert not out.exists(); out.mkdir(parents=True)
    assert sha(FIXTURE)==FIXTURE_PIN
    inputs=['src/object_creation_request.cpp','tests/object_creation_request_tests.cpp']
    dependency=subprocess.run(['g++','-std=c++20','-Iinclude','-MM',*inputs],cwd=ROOT,
        capture_output=True,text=True,check=True)
    (out/'dependencies.txt').write_text(dependency.stdout)
    names=sorted(set(q for q in dependency.stdout.replace('\\\n',' ').split() if not q.endswith(':')))
    pins={}
    for name in names:
        path=ROOT/name; assert path.is_file() and path.is_relative_to(ROOT)
        pins[name]=sha(path); destination=out/'source'/name
        destination.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(path,destination)
    commands=[]
    for mode in ('normal','sanitizer'):
        folder=out/mode; folder.mkdir(); executable=folder/'object_creation_request_tests'
        argv=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(out/'source/include')]
        argv+=['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
        argv += [*[str(out/'source'/q) for q in inputs],'-o',str(executable)]
        for label, command in (('build',argv),('test',[str(executable),str(FIXTURE)])):
            delta=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1') if mode=='sanitizer' else {}
            environment=os.environ.copy(); environment.update(delta)
            with (folder/(label+'.log')).open('w') as log:
                result=subprocess.run(command,cwd=ROOT,env=environment,stdout=log,stderr=subprocess.STDOUT)
            commands.append(dict(mode=mode,label=label,argv=command,exit_code=result.returncode,environment=delta))
            (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
            if result.returncode:
                raise RuntimeError((folder/(label+'.log')).read_text()[-4000:])
        print(mode,(folder/'test.log').read_text().strip(),flush=True)
    for name,pin in pins.items():
        assert sha(ROOT/name)==sha(out/'source'/name)==pin
    report=dict(schema='object-creation-request-source-validation-v1',validator_sha256=sha(Path(__file__)),
        source_pins=pins,fixture_sha256=FIXTURE_PIN,commands_sha256=sha(out/'commands.json'),
        scope='Complete original generic request and typed lookup CPU comparison; no projectile allocation/native gameplay admission.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),sources=len(pins))))


if __name__=='__main__':
    main()
