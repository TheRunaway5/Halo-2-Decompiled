#!/usr/bin/env python3
"""Observe actual production loop-owner targets without resetting build flags."""
import argparse,json,os,shlex,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
BUILDS=(('normal','build-animation-metadata-store-20261008'),('sanitizer','build-animation-event-asan-20261008'))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();out=a.directory.resolve();assert not out.exists();out.mkdir(parents=True);commands=[]
 inputs=('src/audio/sound_looping.cpp','src/audio/sound_driver_master_gain.cpp','tests/sound_looping_tests.cpp','tests/sound_driver_master_gain_tests.cpp','tests/sound_admission_tests.cpp');dep=subprocess.run(['g++','-std=c++20','-Iinclude', '-Isrc','-MM',*inputs],cwd=ROOT,capture_output=True,text=True,check=True);save(out/'dependencies.txt',dep.stdout.encode());names=sorted(set(dep.stdout.replace('\\\n',' ').split())-set(q for q in dep.stdout.replace('\\\n',' ').split()if q.endswith(':'))|{'CMakeLists.txt','include/halo2/original_data.hpp','src/assets/original_data.cpp'})
 for rel in names:
  dest=out/'source'/rel;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dest)
 def run(argv,label,env=None):
  r=subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,text=True);save(out/(label+'.stdout'),r.stdout.encode());save(out/(label+'.stderr'),r.stderr.encode());commands.append(dict(argv=argv,cwd=str(ROOT),label=label,exit_code=r.returncode,environment_overrides={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')}if env else{},stdout_sha256=sha(out/(label+'.stdout')),stderr_sha256=sha(out/(label+'.stderr'))));(out/'commands-pending.json').write_bytes(encoded(commands));assert r.returncode==0,(label,r.stdout[-3000:],r.stderr[-3000:])
 archives={}
 for tag,name in BUILDS:
  build=ROOT/name;run(['cmake','-S',str(ROOT),'-B',str(build)],tag+'-configure');run(['cmake','--build',str(build),'--target','sound_looping_tests','sound_driver_master_gain_tests','sound_admission_tests','-j2'],tag+'-build')
  env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')if tag=='sanitizer'else None
  run(['ctest','--test-dir',str(build),'-R','^original_sound_(looping|driver_master_gain|admission)$','--output-on-failure','-V'],tag+'-ctest',env);dest=out/tag;dest.mkdir()
  for target in('sound_looping_tests','sound_driver_master_gain_tests','sound_admission_tests'):
   shutil.copy2(build/target,dest/target);link=build/'CMakeFiles'/f'{target}.dir/link.txt';shutil.copy2(link,dest/(target+'-link.txt'));shutil.copy2(build/'CMakeFiles'/f'{target}.dir/flags.make',dest/(target+'-flags.make'))
   for token in shlex.split(link.read_text()):
    if token.endswith('.a'):
     source=Path(token);source=source if source.is_absolute()else build/source;copied=dest/source.name
     if copied.exists():assert sha(copied)==sha(source)
     else:shutil.copy2(source,copied)
     archives[str(source.relative_to(ROOT))if source.is_relative_to(ROOT)else str(source)]=sha(source)
  for rel in('CMakeCache.txt','CMakeFiles/halo2_cache.dir/flags.make','CMakeFiles/halo2_cache.dir/build.make'):shutil.copy2(build/rel,dest/Path(rel).name)
 before={rel:sha(out/'source'/rel)for rel in names};after={rel:sha(ROOT/rel)for rel in names};drift={rel:dict(before=before[rel],after=after[rel])for rel in names if before[rel]!=after[rel]};assert not any(rel in drift for rel in('include/halo2/sound_looping.hpp','src/audio/sound_looping.cpp','tests/sound_looping_tests.cpp'))
 save(out/'commands.json',encoded(dict(schema='sound-looping-production-v1',tool_sha256=sha(__file__),commands=commands,source_snapshot=before,source_after=after,source_drift=drift,observed_archives=archives,scope='Focused actual production targets with original fixtures, existing sanitizer settings preserved. These are observed archives/source snapshots, not a coherent freshly rebuilt full tree, live driver/startup or playback claim.')))
 print(json.dumps(dict(commands_sha256=sha(out/'commands.json'),source_drift=drift,archives=len(archives))))
if __name__=='__main__':main()
