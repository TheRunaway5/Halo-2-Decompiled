#!/usr/bin/env python3
"""Build observed source snapshots and execute normal/sanitized cache owners."""
import argparse,json,os,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded

SOURCES=('include/halo2/sound_cache_startup.hpp','src/sound_cache_startup.cpp','tests/sound_cache_startup_tests.cpp',
         'include/halo2/sound_startup.hpp','src/sound_startup.cpp')
STARTUP=ROOT/'analysis/multiplayer/integration/sound-startup-native/fixtures.tsv'

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();native=d/'native';native.mkdir(parents=True,exist_ok=True)
 existing=native/'sound_cache_startup_tests'
 if existing.exists():
  previous=native/'preliminary-before-pinning';previous.mkdir(exist_ok=True);shutil.copy2(existing,previous/f'{sha(existing)}.bin')
 for rel in SOURCES:
  destination=native/'source'/rel;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,destination)
 commands=[]
 def run(argv,label,env=None):
  result=subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,text=True)
  save(native/(label+'.stdout'),result.stdout.encode());save(native/(label+'.stderr'),result.stderr.encode())
  commands.append(dict(argv=argv,cwd=str(ROOT),label=label,exit_code=result.returncode,
      environment_overrides={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')}if env else{},stdout_sha256=sha(native/(label+'.stdout')),stderr_sha256=sha(native/(label+'.stderr'))))
  assert result.returncode==0,(label,result.stdout,result.stderr)
  return result.stdout
 version=run(['g++','--version'],'compiler-version')
 inputs=[str(native/'source/src'/q)for q in('sound_cache_startup.cpp','sound_startup.cpp')]+[str(native/'source/tests/sound_cache_startup_tests.cpp')]
 normal=native/'sound_cache_startup_tests';san=native/'sound_cache_startup_tests_asan'
 run(['g++','-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-I'+str(native/'source/include'),*inputs,'-o',str(normal)],'normal-build')
 run([str(normal),str(d/'fixtures.tsv'),str(STARTUP)],'normal')
 run(['g++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Wpedantic','-I'+str(native/'source/include'),*inputs,'-o',str(san)],'sanitizer-build')
 env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
 run([str(san),str(d/'fixtures.tsv'),str(STARTUP)],'sanitizer',env)
 for rel in SOURCES:assert (ROOT/rel).read_bytes()==(native/'source'/rel).read_bytes(),rel
 save(native/'commands.json',encoded(dict(schema='source-sound-cache-startup-native-validation-v1',validator_sha256=sha(__file__),commands=commands,compiler_version=version,
   observed_source_files={rel:sha(ROOT/rel)for rel in SOURCES},cache_fixture_sha256=sha(d/'fixtures.tsv'),immutable_startup_fixture_sha256=sha(STARTUP),
   scope='Standalone fresh source snapshot builds only. Complete cache constructor + portable CPU startup composition stops at real platform constructor request without a return. No current shared archive freshness, ordinary map activation, voice/readiness or runtime audio claim.')))
 print(json.dumps(dict(normal_sha256=sha(normal),sanitizer_sha256=sha(san),commands_sha256=sha(native/'commands.json'))))

if __name__=='__main__':main()
