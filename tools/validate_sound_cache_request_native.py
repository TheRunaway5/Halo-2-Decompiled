#!/usr/bin/env python3
"""Fresh normal/sanitizer source snapshots for complete create ownership."""
import argparse,os,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
SOURCES=('include/halo2/sound_cache_request.hpp','src/sound_cache_request.cpp','tests/sound_cache_request_tests.cpp','include/halo2/sound_cache_startup.hpp','include/halo2/source_bitmap_async_read.hpp','src/source_bitmap_async_read.cpp','include/halo2/source_async_jobs.hpp','src/source_async_jobs.cpp','include/halo2/animation_async_constructor.hpp','src/animation_async_constructor.cpp','include/halo2/animation_async_worker.hpp','src/animation_async_worker.cpp','include/halo2/unit_animation_resolution.hpp','include/halo2/unit_animation_selection.hpp','include/halo2/unit_animation.hpp','include/halo2/native_sound_storage.hpp','src/native_sound_storage.cpp','include/halo2/native_synchronization.hpp','src/native_synchronization.cpp')
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);p.add_argument('--fixture-directory',type=Path);a=p.parse_args();d=a.directory.resolve();fixture=(a.fixture_directory or a.directory).resolve();native=d/'native';native.mkdir(parents=True,exist_ok=True)
    for rel in SOURCES:
        dst=native/'source'/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dst)
    commands=[]
    def run(argv,label,env=None):
        r=subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,text=True)
        save(native/(label+'.stdout'),r.stdout.encode());save(native/(label+'.stderr'),r.stderr.encode())
        commands.append(dict(argv=argv,cwd=str(ROOT),exit_code=r.returncode,label=label,environment_overrides={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')}if env else{},stdout_sha256=sha(native/(label+'.stdout')),stderr_sha256=sha(native/(label+'.stderr'))))
        assert r.returncode==0,(label,r.stdout,r.stderr);return r.stdout
    version=run(['g++','--version'],'compiler-version')
    inputs=[str(native/'source'/q)for q in tuple(q for q in SOURCES if q.endswith('.cpp'))]
    common=['g++','-std=c++20','-pthread','-Wall','-Wextra','-Wpedantic','-ffp-contract=off','-fno-fast-math','-frounding-math','-fno-associative-math','-I'+str(native/'source/include')]
    normal=native/'sound_cache_request_tests';san=native/'sound_cache_request_tests_asan'
    run([*common,'-O2',*inputs,'-o',str(normal)],'normal-build');run([str(normal),str(fixture/'fixtures.tsv'),str(ROOT/'assets/disc/maps')],'normal')
    run([*common,'-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',*inputs,'-o',str(san)],'sanitizer-build')
    run([str(san),str(fixture/'fixtures.tsv'),str(ROOT/'assets/disc/maps')],'sanitizer',dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'))
    for rel in SOURCES:assert(ROOT/rel).read_bytes()==(native/'source'/rel).read_bytes(),rel
    save(native/'commands.json',encoded(dict(schema='sound-cache-request-native-validation-v1',validator_sha256=sha(__file__),commands=commands,compiler_version=version,observed_source_files={rel:sha(ROOT/rel)for rel in SOURCES},fixture_sha256=sha(fixture/'fixtures.tsv'),scope='Fresh standalone observed source-owner + actual NativeSoundStorage, no shared archive freshness or map/device/playback admission.')))
    print({'commands_sha256':sha(native/'commands.json'),'normal_sha256':sha(normal),'sanitizer_sha256':sha(san)})
if __name__=='__main__':main()
