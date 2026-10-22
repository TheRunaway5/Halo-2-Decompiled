#!/usr/bin/env python3
"""Build hash-checked snapshots for whole creation/cache/worker composition."""
import argparse,os,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
INPUTS=('src/audio/sound_create.cpp','src/audio/sound_submit.cpp','src/audio/sound_cache_request.cpp','src/rendering/source_bitmap_async_read.cpp','src/platform/source_async_jobs.cpp','src/gameplay/animation/animation_async_constructor.cpp','src/gameplay/animation/animation_async_worker.cpp','src/audio/native_sound_storage.cpp','src/platform/native_synchronization.cpp','tests/sound_create_cache_tests.cpp')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);p.add_argument('--fixture-directory',type=Path);a=p.parse_args();d=a.directory.resolve();fixture=(a.fixture_directory or a.directory).resolve();native=d/'native';native.mkdir(parents=True,exist_ok=True)
    dependencies=subprocess.run(['g++','-std=c++20','-Iinclude', '-Isrc','-MM',*INPUTS],cwd=ROOT,capture_output=True,text=True,check=True)
    SOURCES=sorted(set(q for q in dependencies.stdout.replace('\\\n',' ').split()if not q.endswith(':')))
    save(native/'dependencies.txt',dependencies.stdout.encode())
    for rel in SOURCES:
        dst=native/'source'/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dst)
    commands=[]
    def run(argv,label,env=None):
        r=subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,text=True)
        save(native/(label+'.stdout'),r.stdout.encode());save(native/(label+'.stderr'),r.stderr.encode())
        commands.append(dict(argv=argv,cwd=str(ROOT),exit_code=r.returncode,label=label,environment_overrides={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')}if env else{},stdout_sha256=sha(native/(label+'.stdout')),stderr_sha256=sha(native/(label+'.stderr'))))
        assert r.returncode==0,(label,r.stdout,r.stderr);return r.stdout
    version=run(['g++','--version'],'compiler-version')
    inputs=[str(native/'source'/q)for q in INPUTS]
    common=['g++','-std=c++20','-pthread','-Wall','-Wextra','-Wpedantic','-ffp-contract=off','-fno-fast-math','-frounding-math','-fno-associative-math','-I'+str(native/'source/include'), '-I'+str(native/'source/src')]
    normal=native/'sound_create_cache_tests';san=native/'sound_create_cache_tests_asan'
    run([*common,'-O2',*inputs,'-o',str(normal)],'normal-build');run([str(normal),str(fixture/'fixtures.tsv'),str(ROOT/'assets/disc/maps')],'normal')
    run([*common,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',*inputs,'-o',str(san)],'sanitizer-build')
    run([str(san),str(fixture/'fixtures.tsv'),str(ROOT/'assets/disc/maps')],'sanitizer',dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
    for rel in SOURCES:assert(ROOT/rel).read_bytes()==(native/'source'/rel).read_bytes(),rel
    save(native/'commands.json',encoded(dict(schema='sound-create-cache-native-validation-v1',validator_sha256=sha(__file__),commands=commands,compiler_version=version,observed_source_files={rel:sha(ROOT/rel)for rel in SOURCES},fixture_sha256=sha(fixture/'fixtures.tsv'),scope='Fresh standalone observed source-owner + actual NativeSoundStorage, whole creation and actual pending cache pin; no shared archive freshness or map/device/playback admission.')))
    print({'commands_sha256':sha(native/'commands.json'),'normal_sha256':sha(normal),'sanitizer_sha256':sha(san)})
if __name__=='__main__':main()
