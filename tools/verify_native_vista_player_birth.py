#!/usr/bin/env python3
"""Rebuild the native birth/match slice against current read-only archives.

Only listed overrides are instrumented. Existing dependency archives remain
uninstrumented. The CPU arena and actual original Cache world own real native
allocation/spawn; this is not a GPU capture or a complete PC simulation port.
"""
import argparse,hashlib,json,os,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/native_vista_player_birth_tests.cpp','src/vista_player_birth.cpp',
 'src/vista_slayer_publication.cpp','src/multiplayer.cpp','src/multiplayer_world.cpp']
PACKAGES=['sdl2','libavcodec','libavformat','libavutil','libswresample','libswscale']
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--asan',action='store_true')
 p.add_argument('--build',type=Path,default=ROOT/'build-frontend');a=p.parse_args()
 archives=[a.build/f'libhalo2_{name}.a'for name in ('systems','cache','sound_platform')]
 target=Path.home()/'.cache'/('native_vista_player_birth_tests_asan'if a.asan else'native_vista_player_birth_tests')
 command=['c++','-std=c++20','-Wall','-Wextra','-Werror','-Wno-missing-field-initializers',
  '-pedantic','-fno-fast-math','-ffp-contract=off','-Iinclude']
 if a.asan:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 command+=shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True))
 command+=SOURCES+['-Wl,--start-group',*[str(x)for x in archives],'-Wl,--end-group']
 command+=shlex.split(subprocess.check_output(['pkg-config','--libs',*PACKAGES],text=True))
 command+=['-lz','-lglm','-pthread','-o',str(target)]
 compiler_tmp=Path.home()/'.cache'/'native-vista-birth-compiler-tmp';compiler_tmp.mkdir(parents=True,exist_ok=True)
 subprocess.run(command,cwd=ROOT,env=dict(os.environ,TMPDIR=str(compiler_tmp)),check=True)
 run=[str(target),'assets/disc/maps','Halo 2 Vista installer/maps','assets/disc/default.xbe']
 environment=dict(os.environ)
 if a.asan:environment['ASAN_OPTIONS']='detect_leaks=1'
 result=subprocess.run(run,cwd=ROOT,env=environment,text=True,capture_output=True)
 evidence=dict(command=command,run=run,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr,
  source_sha256={x:sha(ROOT/x)for x in SOURCES},archive_sha256={str(x):sha(x)for x in archives},scope=__doc__)
 out=ROOT/'analysis/frontend/vista-session-flow-audit/player-birth-original'
 (out/('native-asan-execution.json'if a.asan else'native-execution.json')).write_text(json.dumps(evidence,indent=2)+'\n')
 print(result.stdout,end='');print(result.stderr,end='');raise SystemExit(result.returncode)
if __name__=='__main__':main()
