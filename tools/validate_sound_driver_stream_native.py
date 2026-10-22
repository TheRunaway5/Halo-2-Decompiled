#!/usr/bin/env python3
"""Build immutable observed source snapshots for the original stream-prefix comparison."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha
FIXTURE=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native/fixtures.tsv'
FIXTURE_PIN='40c2ce8f0ace2fca2df05d58b15f73b9ad3a726556a275d5394ce3cc05fa0b41'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 assert sha(FIXTURE)==FIXTURE_PIN
 inputs=['src/audio/sound_driver_stream.cpp','tests/sound_driver_stream_tests.cpp']
 dependencies=subprocess.run(['g++','-std=c++20','-Iinclude', '-Isrc','-MM',*inputs],cwd=ROOT,capture_output=True,text=True,check=True)
 (out/'dependencies.txt').write_text(dependencies.stdout)
 names=sorted(set(q for q in dependencies.stdout.replace('\\\n',' ').split()if not q.endswith(':')))
 pins={}
 for name in names:
  source=ROOT/name;assert source.is_file()and source.is_relative_to(ROOT)
  pins[name]=sha(source);destination=out/'source'/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
 commands=[]
 for mode in ['normal','sanitizer']:
  folder=out/mode;folder.mkdir(exist_ok=True);exe=folder/'sound_driver_stream_tests'
  common=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(out/'source/include'), '-I'+str(out/'source/src')]
  common+=['-O2']if mode=='normal'else['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
  for label,argv in [('build',common+[*[str(out/'source'/q)for q in inputs],'-o',str(exe)]),('test',[str(exe),str(FIXTURE)])]:
   env=os.environ.copy();delta={}
   if mode=='sanitizer':delta=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1');env.update(delta)
   with(folder/(label+'.log')).open('w')as log:r=subprocess.run(argv,cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT)
   commands.append(dict(mode=mode,label=label,argv=argv,exit_code=r.returncode,environment=delta))
   (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
   if r.returncode:raise RuntimeError((folder/(label+'.log')).read_text()[-4000:])
  print(mode,(folder/'test.log').read_text().strip(),flush=True)
 for name,pin in pins.items():assert sha(ROOT/name)==sha(out/'source'/name)==pin
 report=dict(schema='sound-driver-stream-source-snapshot-validation-v1',validator_sha256=sha(Path(__file__)),source_pins=pins,fixture_sha256=FIXTURE_PIN,commands_sha256=sha(out/'commands.json'),scope='Actual source snapshot normal/ASan+UBSan prefix comparison; native stream constructor stops before supplying any result.')
 (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(out/'report.json'),sources=len(pins))))
if __name__=='__main__':main()
