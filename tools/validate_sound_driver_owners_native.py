#!/usr/bin/env python3
"""Fresh CPU-owner snapshots with the real retained NativeSoundStorage."""
import argparse,os,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
SOURCES=('include/halo2/sound_driver_owners.hpp','src/sound_driver_owners.cpp','tests/sound_driver_owners_tests.cpp','include/halo2/sound_driver_startup.hpp','include/halo2/sound_map_activation.hpp','include/halo2/native_sound_storage.hpp','src/native_sound_storage.cpp','include/halo2/sound_cache_startup.hpp')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();d=a.directory.resolve();native=d/'native';native.mkdir(parents=True,exist_ok=True)
 for rel in SOURCES:
  dst=native/'source'/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dst)
 commands=[]
 def run(argv,label,env=None):
  r=subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,text=True);save(native/(label+'.stdout'),r.stdout.encode());save(native/(label+'.stderr'),r.stderr.encode());commands.append(dict(argv=argv,cwd=str(ROOT),exit_code=r.returncode,label=label,environment_overrides={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')}if env else{},stdout_sha256=sha(native/(label+'.stdout')),stderr_sha256=sha(native/(label+'.stderr'))));assert r.returncode==0,(label,r.stdout,r.stderr);return r.stdout
 version=run(['g++','--version'],'compiler-version');inputs=[str(native/'source'/q)for q in('src/sound_driver_owners.cpp','src/native_sound_storage.cpp','tests/sound_driver_owners_tests.cpp')]
 normal=native/'sound_driver_owners_tests';san=native/'sound_driver_owners_tests_asan'
 run(['g++','-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-I'+str(native/'source/include'),*inputs,'-o',str(normal)],'normal-build');run([str(normal),str(d/'fixtures.tsv')],'normal')
 run(['g++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Wall','-Wextra','-Wpedantic','-I'+str(native/'source/include'),*inputs,'-o',str(san)],'sanitizer-build');run([str(san),str(d/'fixtures.tsv')],'sanitizer',dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
 for rel in SOURCES:assert(ROOT/rel).read_bytes()==(native/'source'/rel).read_bytes(),rel
 save(native/'commands.json',encoded(dict(schema='sound-driver-owners-native-validation-v1',validator_sha256=sha(__file__),commands=commands,compiler_version=version,observed_source_files={rel:sha(ROOT/rel)for rel in SOURCES},fixture_sha256=sha(d/'fixtures.tsv'),scope='Fresh standalone observed CPU-owner and actual NativeSoundStorage sources. No shared archive freshness or source voice/device/platform result.')))
 print({'commands_sha256':sha(native/'commands.json'),'normal_sha256':sha(normal),'sanitizer_sha256':sha(san)})
if __name__=='__main__':main()
