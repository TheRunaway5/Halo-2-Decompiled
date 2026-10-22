#!/usr/bin/env python3
"""Build/run only the native Vista announcer slice against read-only archives.

The selected source overrides are rebuilt; the existing cache/system/platform
archives and FFmpeg/SDL libraries remain uninstrumented. This is a dummy SDL
device/manual mixer test, not speaker audibility or the full live world flow.
"""
import argparse,hashlib,json,os,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['src/ui/vista/native_vista_announcer.cpp','src/audio/vista_announcer_events.cpp',
 'src/audio/vista_announcer_assets.cpp','src/audio/vista_announcer_queue.cpp',
 'src/assets/source_tag_metadata_store.cpp','src/audio/multiplayer_audio.cpp',
 'src/audio/sound_variation.cpp','src/audio/sound_driver_parameters.cpp','src/assets/original_data.cpp',
 'tests/native_vista_announcer_tests.cpp']
ARCHIVES=['build-vista-cache-adapter/libhalo2_systems.a',
 'build-vista-cache-adapter/libhalo2_cache.a','build-vista-cache-adapter/libhalo2_sound_platform.a']
PACKAGES=['sdl2','libavcodec','libavformat','libavutil','libswresample','libswscale']
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--asan',action='store_true');a=p.parse_args()
 target=Path.home()/'.cache'/('native-vista-announcer-tests-asan'if a.asan else'native-vista-announcer-tests')
 command=['c++','-std=c++20','-Wall','-Wextra','-Werror','-pedantic','-fno-fast-math','-ffp-contract=off']
 if a.asan:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 command+=['-Iinclude', '-Isrc']+shlex.split(subprocess.check_output(['pkg-config','--cflags',*PACKAGES],text=True))
 command+=SOURCES+['-Wl,--start-group',*ARCHIVES,'-Wl,--end-group']
 command+=shlex.split(subprocess.check_output(['pkg-config','--libs',*PACKAGES],text=True))+['-lz','-lglm','-pthread','-o',str(target)]
 compiler_tmp=Path.home()/'.cache'/'native-vista-announcer-compiler-tmp';compiler_tmp.mkdir(parents=True,exist_ok=True)
 subprocess.run(command,cwd=ROOT,env=dict(os.environ,TMPDIR=str(compiler_tmp)),check=True)
 run=[str(target),'Halo 2 Vista installer/maps',r'scenarios\multi\ascension\ascension',
  'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03']
 environment=dict(os.environ,SDL_AUDIODRIVER='dummy')
 if a.asan:environment['ASAN_OPTIONS']='detect_leaks=1'
 result=subprocess.run(run,cwd=ROOT,env=environment,text=True,capture_output=True)
 evidence=dict(command=command,run=run,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr,
  source_sha256={s:sha(ROOT/s)for s in SOURCES},archive_sha256={s:sha(ROOT/s)for s in ARCHIVES},
  assets_sha256={s:sha(ROOT/s)for s in ['Halo 2 Vista installer/halo2.exe','Halo 2 Vista installer/maps/shared.map',
   'Halo 2 Vista installer/maps/ascension.map',run[-1]]},scope=__doc__)
 out=ROOT/'analysis/frontend/vista-session-flow-audit/announcer-events-original'
 out.mkdir(parents=True,exist_ok=True);(out/('native-asan-execution.json'if a.asan else'native-execution.json')).write_text(json.dumps(evidence,indent=2)+'\n')
 print(result.stdout,end='');print(result.stderr,end='');raise SystemExit(result.returncode)
if __name__=='__main__':main()
