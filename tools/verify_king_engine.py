#!/usr/bin/env python3
"""Freeze the King core, reproduce original fixtures, build and sanitize it.

This validates the scoped core only. It does not rebuild the entire game or
admit King variants. Original asset files remain read-only external inputs.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
import oracle_king_engine,oracle_king_scoring,oracle_king_world
import capstone,unicorn

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/king-engine-20261011'
NATIVE=('include/halo2/source_king.hpp','include/halo2/slayer.hpp','include/halo2/source_game_end.hpp',
 'src/source_king.cpp','src/slayer.cpp','tests/king_engine_tests.cpp',
 'tests/fixtures/king_engine_original.json','tests/fixtures/king_scoring_original.json','tests/fixtures/king_world_original.json')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');a=ap.parse_args()
 run=a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';source.mkdir(parents=True);build=run/'build';commands=[]
 py_files={Path(m.__file__).resolve()for m in tuple(sys.modules.values())if getattr(m,'__file__',None)
  and str(Path(m.__file__).resolve()).startswith(str(ROOT/'tools')+os.sep) and str(m.__file__).endswith('.py')}
 files=set(NATIVE)|{str(p.relative_to(ROOT))for p in py_files}|{'tools/verify_king_engine.py','CMakeLists.txt',
  'docs/MP_KING_ENGINE_SOURCE_RESEARCH.md','docs/MP_KING_ENGINE_IMPLEMENTATION.md','docs/GAME_MODE_PROGRESS.md'}
 pins={p:sha(ROOT/p)for p in sorted(files)}
 for p in sorted(files):
  target=source/('root-CMakeLists.txt'if p=='CMakeLists.txt'else p);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target)
  assert sha(target)==pins[p]
 for p in ('assets','analysis'):(source/p).symlink_to(ROOT/p,target_is_directory=True)
 (source/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(halo2_king_core_verification LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()
add_executable(king_engine_tests tests/king_engine_tests.cpp src/source_king.cpp src/slayer.cpp)
target_include_directories(king_engine_tests PRIVATE include)
target_compile_options(king_engine_tests PRIVATE -Wall -Wextra -Werror -fno-fast-math -ffp-contract=off)
add_test(NAME original_king_engine COMMAND king_engine_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_engine_original.json
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_scoring_original.json
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_world_original.json)
''')
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,command,extra_env=None):
  log=run/f'{label}.log'
  with log.open('w')as output:
   result=subprocess.run(list(map(str,command)),cwd=source,env={**env,**(extra_env or {})},stdout=output,stderr=subprocess.STDOUT)
  record=dict(label=label,command=list(map(str,command)),exit=result.returncode,log=str(log.relative_to(a.output.parent)),log_sha256=sha(log))
  commands.append(record);print(label,'exit',result.returncode,flush=True)
  if result.returncode:raise RuntimeError(f'{label} failed; see {log}')
  return record
 def original(kind):
  folder=run/f'original-{kind}';folder.mkdir();fixture=folder/'fixture.json';report=folder/'report.json'
  execute('original-'+kind,[sys.executable,source/'tools'/f'oracle_king_{kind}.py','--output',report,'--fixture',fixture])
  expected=source/'tests/fixtures'/f'king_{kind}_original.json';assert sha(fixture)==sha(expected),(kind,'Fixture reproduction differs')
  measured=json.loads(report.read_text());published=json.loads((OUT/('report.json'if kind=='engine'else f'{kind}/report.json')).read_text())
  assert measured==published,(kind,'Original report reproduction differs')
  for listing in measured['instruction_listings']:
   assert sha(folder/'instructions'/listing['file'])==listing['sha256']
  return dict(kind=kind,fixture_sha256=sha(fixture),report_sha256=sha(report),summary=measured['summary'],
   report=str(report.relative_to(a.output.parent)))
 # Original executions are independent from each other and from native code.
 with concurrent.futures.ThreadPoolExecutor(max_workers=3)as pool:originals=list(pool.map(original,('engine','scoring','world')))
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo'])
 execute('build',['cmake','--build',build,'-j2'])
 execute('ctest',['ctest','--test-dir',build,'--verbose'])
 binary=build/'king_engine_tests';sanitized=run/'king_engine_tests_asan'
 compiler='/usr/bin/c++'
 flags=['-std=c++20','-O1','-g','-Wall','-Wextra','-Werror','-fno-fast-math','-ffp-contract=off',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(source/'include')]
 execute('sanitizer-build',[compiler,*flags,source/'tests/king_engine_tests.cpp',source/'src/source_king.cpp',source/'src/slayer.cpp','-o',sanitized])
 fixture_args=[source/'tests/fixtures'/f'king_{kind}_original.json'for kind in ('engine','scoring','world')]
 execute('sanitizer-test',[sanitized,*fixture_args],{'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 for p in files:
  target=source/('root-CMakeLists.txt'if p=='CMakeLists.txt'else p);assert sha(target)==pins[p],p
 # Record live-checkout drift separately: immutable snapshot results stay valid.
 drift=[p for p in sorted(files)if sha(ROOT/p)!=pins[p]]
 assert not any(p.startswith(('include/halo2/source_king','src/source_king','tools/oracle_king','tests/king_'))for p in drift),drift
 maps=json.loads((source/'tests/fixtures/king_engine_original.json').read_text())['maps'][:12]
 for m in maps:assert sha(ROOT/'assets/disc/maps'/f'{m["name"]}.map')==m['cache_sha256']
 manifest=dict(schema='king-core-verification-v1',scope=__doc__,frozen_source=str(source.relative_to(a.output.parent)),
  source_hashes=str((run/'source-hashes.json').relative_to(a.output.parent)),source_hashes_sha256=sha(run/'source-hashes.json'),
  live_checkout_drift=drift,originals=originals,commands=commands,
  binaries={str(p.relative_to(a.output.parent)):sha(p)for p in(binary,sanitized)},
  python=sys.version,capstone=capstone.__version__,unicorn=unicorn.__version__,
  root_cmake_target_verified='king_engine_tests / original_king_engine',
  production_king_admission=False,goal_complete=False)
 a.output.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n');print(str(a.output),flush=True)
if __name__=='__main__':main()
