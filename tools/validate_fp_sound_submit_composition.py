#!/usr/bin/env python3
"""Snapshot new source and compose against observed production Cache archives."""
import argparse,json,os,shutil,subprocess,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def encoded(v):return(json.dumps(v,sort_keys=True,indent=2)+'\n').encode()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,('Existing validation changed',str(p))
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
 inputs=['src/gameplay/weapons/fp_sound_submit.cpp','tests/fp_sound_submit_tests.cpp'];dep=subprocess.run(['g++','-std=c++20','-Iinclude', '-Isrc','-MM',*inputs],cwd=ROOT,capture_output=True,text=True,check=True)
 save(out/'dependencies.txt',dep.stdout.encode());names=sorted(set(q for q in dep.stdout.replace('\\\n',' ').split()if not q.endswith(':')));pins={}
 for name in names:
  path=ROOT/name;assert path.is_file();pins[name]=sha(path);target=out/'source'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)
 fixture=ROOT/'analysis/multiplayer/integration/fp-sound-submit-composition/fixtures/fixtures.tsv';cache=ROOT/'assets/disc/maps/lockout.map';xbe=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';commands=[];archivepins={}
 for mode,build in(('normal','build-animation-metadata-store-20261008'),('sanitizer','build-animation-event-asan-20261008')):
  folder=out/mode;folder.mkdir();archives=[]
  for name in('libhalo2_cache.a','libhalo2_native_synchronization.a'):
   source=ROOT/build/name;target=folder/name;before=sha(source);shutil.copy2(source,target);assert sha(source)==sha(target)==before
   archivepins[str(source.relative_to(ROOT))]=before;archives.append(str(target))
  exe=folder/'fp_sound_submit_tests';flags=['-O2']if mode=='normal'else['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
  cmd=['g++','-std=c++20','-Wall','-Wextra','-Wpedantic','-Werror','-fno-fast-math','-ffp-contract=off','-frounding-math',*flags,'-I'+str(out/'source/include'), '-I'+str(out/'source/src'),*[str(out/'source'/q)for q in inputs],*archives,'/usr/lib/libglm.a','-pthread','-o',str(exe)]
  for label,argv in(('build',cmd),('test',[str(exe),str(fixture),str(cache),str(xbe)])):
   env=dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1')if mode=='sanitizer'else{}
   r=subprocess.run(argv,cwd=ROOT,env=dict(os.environ,**env),capture_output=True,text=True);save(folder/(label+'.stdout'),r.stdout.encode());save(folder/(label+'.stderr'),r.stderr.encode());commands.append(dict(mode=mode,label=label,argv=argv,environment=env,exit_code=r.returncode));(out/'commands.json').write_bytes(encoded(commands));assert r.returncode==0,(mode,label,r.stdout[-3000:],r.stderr[-5000:])
  print(mode,(folder/'test.stdout').read_text().strip(),flush=True)
 for name,h in pins.items():assert sha(ROOT/name)==sha(out/'source'/name)==h,name
 save(out/'report.json',encoded(dict(schema='fp-world-raw-submit-native-composition-validation-v1',validator_sha256=sha(__file__),source_pins=pins,observed_archive_pins=archivepins,fixture_sha256=sha(fixture),cache_sha256=sha(cache),xbe_sha256=sha(xbe),commands_sha256=sha(out/'commands.json'),scope='New source compiled from copied snapshots and linked to copied observed production archives. This does not assert every archive member was freshly built from the observed current shared tree. No source availability, callback result or live provider is fabricated.')))
 print(json.dumps(dict(report_sha256=sha(out/'report.json'),source_files=len(pins))))
if __name__=='__main__':main()
