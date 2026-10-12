#!/usr/bin/env python3
"""Strict native comparison with retained original player-birth outputs."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['tests/vista_player_birth_tests.cpp','src/vista_player_birth.cpp','src/vista_slayer_publication.cpp']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--asan',action='store_true');args=parser.parse_args()
 target=Path.home()/'.cache'/('vista_player_birth_tests_asan'if args.asan else'vista_player_birth_tests')
 temporary=Path.home()/'.cache'/'native-vista-birth-compiler-tmp';temporary.mkdir(parents=True,exist_ok=True)
 command=['c++','-std=c++20','-Wall','-Wextra','-Werror','-pedantic','-Iinclude']
 if args.asan:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
 command+=SOURCES+['-o',str(target)]
 subprocess.run(command,cwd=ROOT,env=dict(os.environ,TMPDIR=str(temporary)),check=True)
 fixture='tests/fixtures/vista_player_birth_original.json';directory='analysis/frontend/vista-session-flow-audit/player-birth-original'
 run=[str(target),fixture,directory];environment=dict(os.environ)
 if args.asan:environment['ASAN_OPTIONS']='detect_leaks=1'
 result=subprocess.run(run,cwd=ROOT,env=environment,text=True,capture_output=True)
 evidence=dict(command=command,run=run,exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr,
  source_sha256={p:sha(ROOT/p)for p in SOURCES+['include/halo2/vista_player_birth.hpp',fixture]},binary_sha256=sha(target))
 (ROOT/directory/('focused-asan-execution.json'if args.asan else'focused-execution.json')).write_text(json.dumps(evidence,indent=2)+'\n')
 print(result.stdout,end='');print(result.stderr,end='');raise SystemExit(result.returncode)
if __name__=='__main__':main()
