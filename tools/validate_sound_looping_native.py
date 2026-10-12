#!/usr/bin/env python3
"""Snapshot and independently compile the complete loop-owner native slice."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
FIXTURE=ROOT/'analysis/multiplayer/integration/sound-looping-native/fixtures-v3/fixtures.tsv'
FIXTURE_PIN='025d84f70f2a0877677c2a1dde70029ea9f731c86cd1522a462d498d197daccf'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True);assert sha(FIXTURE)==FIXTURE_PIN
 inputs=['tests/sound_looping_tests.cpp','src/sound_looping.cpp','src/sound_driver_master_gain.cpp'];d=subprocess.run(['g++','-std=c++20','-Iinclude','-MM',*inputs],cwd=ROOT,capture_output=True,text=True,check=True);save(out/'dependencies.txt',d.stdout.encode())
 names=sorted(set(q for q in d.stdout.replace('\\\n',' ').split()if not q.endswith(':')));pins={}
 for name in names:
  source=ROOT/name;assert source.is_file();pins[name]=sha(source);target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,target)
 commands=[]
 for mode in('normal','sanitizer'):
  folder=out/mode;folder.mkdir();exe=folder/'sound_looping_tests';argv=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(out/'source/include')]
  argv+=['-O2']if mode=='normal'else['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
  for label,cmd in(('build',argv+[*[str(out/'source'/q)for q in inputs],'-o',str(exe)]),('test',[str(exe),str(FIXTURE)])):
   env=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')if mode=='sanitizer'else{}
   r=subprocess.run(cmd,cwd=ROOT,env=dict(os.environ,**env),capture_output=True,text=True);save(folder/(label+'.stdout'),r.stdout.encode());save(folder/(label+'.stderr'),r.stderr.encode());commands.append(dict(mode=mode,label=label,argv=cmd,environment=env,exit_code=r.returncode));(out/'commands.json').write_bytes(encoded(commands));assert r.returncode==0,(label,r.stdout[-2500:],r.stderr[-2500:])
  print(mode,(folder/'test.stdout').read_text().strip(),flush=True)
 for name,pin in pins.items():assert sha(ROOT/name)==sha(out/'source'/name)==pin,name
 save(out/'report.json',encoded(dict(schema='sound-looping-native-validation-v1',validator_sha256=sha(__file__),source_pins=pins,fixture_sha256=FIXTURE_PIN,commands_sha256=sha(out/'commands.json'),source_drift=[],source_success_results_supplied=[])))
 print(json.dumps(dict(report_sha256=sha(out/'report.json'),source_files=len(pins))))
if __name__=='__main__':main()
