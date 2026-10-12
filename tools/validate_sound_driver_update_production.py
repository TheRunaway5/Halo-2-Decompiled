#!/usr/bin/env python3
"""Register/build focused actual production targets without resetting flags."""
import argparse,os,shlex,shutil,subprocess
from pathlib import Path
from oracle_mp_title_update_display import ROOT,sha,save,encoded
from validate_sound_driver_update_native import source_files
BUILDS=(('normal','build-animation-metadata-store-20261008'),('sanitizer','build-animation-event-asan-20261008'))
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,required=True);a=p.parse_args();out=a.directory.resolve();commands=[]
    sources=source_files()+['CMakeLists.txt','include/halo2/sound_cache_lifecycle.hpp','src/sound_cache_lifecycle.cpp','tests/sound_cache_lifecycle_tests.cpp']
    for rel in sources:
        dst=out/'source'/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,dst)
    def run(argv,label,env=None):
        r=subprocess.run(argv,cwd=ROOT,env=env,capture_output=True,text=True);save(out/(label+'.stdout'),r.stdout.encode());save(out/(label+'.stderr'),r.stderr.encode());commands.append(dict(argv=argv,cwd=str(ROOT),label=label,exit_code=r.returncode,environment_overrides={k:env[k]for k in('ASAN_OPTIONS','UBSAN_OPTIONS')}if env else{},stdout_sha256=sha(out/(label+'.stdout')),stderr_sha256=sha(out/(label+'.stderr'))));assert r.returncode==0,(label,r.stdout[-4000:],r.stderr[-4000:])
    archives={}
    for tag,name in BUILDS:
        build=ROOT/name;run(['cmake','-S',str(ROOT),'-B',str(build)],tag+'-configure');run(['cmake','--build',str(build),'--target','sound_cache_lifecycle_tests','sound_driver_update_tests','-j2'],tag+'-build')
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')if tag=='sanitizer'else None
        run(['ctest','--test-dir',str(build),'-R','^original_sound_(cache_lifecycle|driver_update)$','--output-on-failure','-V'],tag+'-ctest',env)
        dest=out/tag;dest.mkdir(parents=True,exist_ok=True)
        for target in('sound_cache_lifecycle_tests','sound_driver_update_tests'):
            shutil.copy2(build/target,dest/target)
            link=build/'CMakeFiles'/f'{target}.dir/link.txt';shutil.copy2(link,dest/(target+'-link.txt'))
            flags=build/'CMakeFiles'/f'{target}.dir/flags.make';shutil.copy2(flags,dest/(target+'-flags.make'))
            for token in shlex.split(link.read_text()):
                if token.endswith('.a'):
                    path=Path(token);path=path if path.is_absolute()else build/path
                    copied=dest/path.name
                    if copied.exists():assert sha(copied)==sha(path)
                    else:shutil.copy2(path,copied)
                    key=str(path.relative_to(ROOT))if path.is_relative_to(ROOT)else str(path)
                    archives[key]=sha(path)
        shutil.copy2(build/'CMakeCache.txt',dest/'CMakeCache.txt')
    before={rel:sha(out/'source'/rel)for rel in sources};after={rel:sha(ROOT/rel)for rel in sources};drift={rel:dict(before=before[rel],after=after[rel])for rel in sources if before[rel]!=after[rel]}
    assert not any(rel in drift for rel in('include/halo2/sound_driver_update.hpp','src/sound_driver_update.cpp','tests/sound_driver_update_tests.cpp'))
    save(out/'commands.json',encoded(dict(schema='sound-driver-update-production-validation-v1',tool_sha256=sha(__file__),commands=commands,source_snapshot=before,source_after=after,source_drift=drift,observed_archives=archives,scope='Actual focused CMake production targets and original fixtures, existing actual sanitizer flags preserved. Exact observed archives/source snapshots are pinned, not a reproducible whole-tree or active-device/playback claim.')))
    print(dict(commands_sha256=sha(out/'commands.json'),archives=len(archives),source_drift=drift))
if __name__=='__main__':main()
