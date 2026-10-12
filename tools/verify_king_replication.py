#!/usr/bin/env python3
"""Freeze and verify the King engine-bank implementation against whole XBEs.

Build the three actual root King CMake targets, reproduce every original output,
and run the complete new bank test under ASan/UBSan. This scoped build does not
admit King, test its wire codec or establish a complete local/network match.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
import oracle_king_replication
import capstone,unicorn

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012/verify-20261012T010411.708711Z/frozen-source'
OUT=ROOT/'analysis/multiplayer/game-variants/king-replication-native-20261012'
OWNED=('include/halo2/source_king_replication.hpp','src/source_king_replication.cpp',
 'tests/king_replication_tests.cpp','tests/fixtures/king_replication_original.json','tools/oracle_king_replication.py','tools/verify_king_replication.py')
NATIVE=('include/halo2/source_king.hpp','src/source_king.cpp','include/halo2/source_game_over.hpp',
 'src/source_game_over.cpp','include/halo2/sound_classes.hpp','src/sound_classes.cpp',
 'tests/king_engine_tests.cpp','tests/king_callback_tests.cpp','src/slayer.cpp','src/source_result_statistics.cpp',
 'tests/fixtures/king_engine_original.json','tests/fixtures/king_scoring_original.json','tests/fixtures/king_world_original.json',
 'tests/fixtures/king_callbacks_original.json')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');args=ap.parse_args()
 run=args.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';source.parent.mkdir(parents=True,exist_ok=False)
 shutil.copytree(BASE,source,symlinks=True)
 py={str(Path(m.__file__).resolve().relative_to(ROOT))for m in tuple(sys.modules.values())if getattr(m,'__file__',None)
  and str(Path(m.__file__).resolve()).startswith(str(ROOT/'tools')+os.sep) and str(m.__file__).endswith('.py')}
 files=set(OWNED)|set(NATIVE)|py
 pins={p:sha(ROOT/p)for p in sorted(files)}
 for p in sorted(files):
  target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);assert sha(target)==pins[p]
 live_cmake=(ROOT/'CMakeLists.txt').read_text();base_cmake=(source/'CMakeLists.txt').read_text()
 declaration=re.search(r'        add_executable\(king_replication_tests.*?king_replication_original\.json\)\n',live_cmake,re.S).group()
 owned_line='target_sources(halo2_systems PRIVATE src/source_king_replication.cpp)\n'
 assert live_cmake.count(owned_line)==1 and owned_line not in base_cmake and 'add_executable(king_replication_tests'not in base_cmake
 source_cmake=base_cmake.replace('target_sources(halo2_systems PRIVATE src/source_king.cpp)\n',
  'target_sources(halo2_systems PRIVATE src/source_king.cpp)\n'+owned_line)
 anchor='        add_executable(king_engine_tests tests/king_engine_tests.cpp src/source_king.cpp src/slayer.cpp)\n'
 assert source_cmake.count(anchor)==1
 source_cmake=source_cmake.replace(anchor,anchor+declaration)
 (source/'CMakeLists.txt').write_text(source_cmake);(run/'live-CMakeLists.txt').write_text(live_cmake)
 (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(BASE/'CMakeLists.txt'),
  live_sha256=sha(run/'live-CMakeLists.txt'),frozen_sha256=sha(source/'CMakeLists.txt'),
  production_source_line=owned_line,test_declaration=declaration),indent=2)+'\n')
 tracked=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:
  frozen_pins=dict(zip((str(p.relative_to(source))for p in tracked),pool.map(sha,tracked)))
 (run/'source-hashes.json').write_text(json.dumps(frozen_pins,sort_keys=True,indent=2)+'\n')
 commands=[];env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,command,extra_env=None):
  path=run/f'{label}.log'
  with path.open('w')as output:
   result=subprocess.run(list(map(str,command)),cwd=source,env={**env,**(extra_env or {})},stdout=output,stderr=subprocess.STDOUT)
  row=dict(label=label,command=list(map(str,command)),exit=result.returncode,log=str(path.relative_to(args.output.parent)),log_sha256=sha(path))
  commands.append(row);(run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
  print(label,'exit',result.returncode,flush=True);assert result.returncode==0,path
 original=run/'original';original.mkdir()
 execute('original',[sys.executable,source/'tools/oracle_king_replication.py','--output',original/'report.json','--fixture',original/'fixture.json'])
 published=oracle_king_replication.OUT
 outputs=['report.json']+[p.name for p in original.glob('*.asm')]
 assert sha(original/'fixture.json')==pins['tests/fixtures/king_replication_original.json']
 for filename in outputs:assert sha(original/filename)==sha(published/filename),filename
 build=run/'build'
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo'])
 execute('build',['cmake','--build',build,'--target','king_replication_tests','king_engine_tests','king_callback_tests','-j2'])
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R','^original_king_(replication|engine|callbacks)$'])
 text=(run/'ctest.log').read_text();assert re.search(r'100% tests passed.*out of 3',text)
 sanitized=run/'king_replication_tests_asan'
 execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fno-fast-math','-ffp-contract=off','-fsanitize=address,undefined','-fno-omit-frame-pointer',
  '-I'+str(source/'include'),source/'tests/king_replication_tests.cpp',source/'src/source_king_replication.cpp',
  source/'src/source_king.cpp',source/'src/source_game_over.cpp',source/'src/sound_classes.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,source/'tests/fixtures/king_replication_original.json'],
  {'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:
  hashes=list(pool.map(lambda p:sha(source/p),frozen_pins))
 assert all(frozen_pins[p]==digest for p,digest in zip(frozen_pins,hashes))
 drift=[p for p in frozen_pins if p!='CMakeLists.txt'and(ROOT/p).is_file()and sha(ROOT/p)!=frozen_pins[p]]
 assert not(set(OWNED)&set(drift)),drift
 fixture=json.loads((original/'fixture.json').read_text())
 for m in fixture['maps'][:12]:assert sha(ROOT/'assets/disc/maps'/f'{m["name"]}.map')==m['cache_sha256']
 report=json.loads((original/'report.json').read_text())
 result=dict(schema='king-replication-native-verification-v1',boundary=__doc__,source=str(source),base=str(BASE),
  source_hashes_sha256=sha(run/'source-hashes.json'),cmake_overlay_sha256=sha(run/'cmake-overlay.json'),
  overlaid_files=pins,live_checkout_drift=drift,commands=commands,original_summary=report['summary'],
  original_outputs={p:sha(original/p)for p in outputs},fixture_sha256=sha(original/'fixture.json'),
  binaries={str(p.relative_to(args.output.parent)):sha(p)for p in [build/'king_replication_tests',build/'king_engine_tests',build/'king_callback_tests',sanitized]},
  python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__,
  production_world_callbacks=False,network_codec=False,native_session_delivery=False,additional_variant_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 shutil.copy2(run/'verification.json',args.output);print(args.output,flush=True)
if __name__=='__main__':main()
