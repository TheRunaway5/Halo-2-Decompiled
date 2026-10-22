#!/usr/bin/env python3
"""Build copied master-gain sources and compare original complete buffers."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

FIXTURE=ROOT/'analysis/multiplayer/integration/sound-driver-master-gain-native/fixtures/fixtures.tsv'
FIXTURE_PIN='ff4c6b9d6eed212764971ad62da7a13c31221e7676e5eac520c1cf9a40d9184a'

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
    assert not out.exists();out.mkdir(parents=True);assert sha(FIXTURE)==FIXTURE_PIN
    inputs=['src/audio/sound_driver_master_gain.cpp','tests/sound_driver_master_gain_tests.cpp']
    dependency=subprocess.run(['g++','-std=c++20','-Iinclude', '-Isrc','-MM',*inputs],cwd=ROOT,capture_output=True,text=True,check=True)
    (out/'dependencies.txt').write_text(dependency.stdout)
    names=sorted(set(q for q in dependency.stdout.replace('\\\n',' ').split()if not q.endswith(':')));pins={}
    for name in names:
        source=ROOT/name;assert source.is_file()and source.is_relative_to(ROOT);pins[name]=sha(source)
        target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
    commands=[]
    for mode in ('normal','sanitizer'):
        folder=out/mode;folder.mkdir();exe=folder/'sound_driver_master_gain_tests'
        argv=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(out/'source/include'), '-I'+str(out/'source/src')]
        argv+=['-O2']if mode=='normal'else['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
        for label,command in (('build',argv+[*[str(out/'source'/q)for q in inputs],'-o',str(exe)]),('test',[str(exe),str(FIXTURE)])):
            delta=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')if mode=='sanitizer'else{}
            environment=os.environ.copy();environment.update(delta)
            with(folder/(label+'.log')).open('w')as log:result=subprocess.run(command,cwd=ROOT,env=environment,stdout=log,stderr=subprocess.STDOUT)
            commands.append(dict(mode=mode,label=label,argv=command,environment=delta,exit_code=result.returncode))
            (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
            if result.returncode:raise RuntimeError((folder/(label+'.log')).read_text()[-4000:])
        print(mode,(folder/'test.log').read_text().strip(),flush=True)
    for name,pin in pins.items():assert sha(ROOT/name)==sha(out/'source'/name)==pin,name
    report=dict(schema='sound-driver-master-gain-source-validation-v1',validator_sha256=sha(Path(__file__)),source_pins=pins,
        fixture_sha256=FIXTURE_PIN,commands_sha256=sha(out/'commands.json'),scope='Complete CPU master gains; no device/map/admission/playback success.')
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(out/'report.json'),sources=len(pins))))
if __name__=='__main__':main()
