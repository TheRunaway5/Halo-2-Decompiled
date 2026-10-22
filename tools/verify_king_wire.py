#!/usr/bin/env python3
"""Freeze the King CPU wire codec, reproduce originals and run CMake/ASan checks.

Actual root King targets are used on a compatible frozen source snapshot.
Packet setup/acceptance, entity scheduling, native delivery and complete matches
remain separate; this verifier does not admit additional variants.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
import oracle_king_wire
import capstone,unicorn

ROOT=Path(__file__).resolve().parents[1]
BASE_MANIFEST=ROOT/'analysis/multiplayer/game-variants/king-replication-native-20261012/verify-20261012T014047.910516Z/verification.json'
BASE_PIN='e26168337409827cf18408575a6b595ff55f769fb4c62e74a26e62ec5a213434'
OUT=ROOT/'analysis/multiplayer/game-variants/king-wire-native-20261012'
OWNED=('include/halo2/source_king_wire.hpp','src/gameplay/multiplayer/source_king_wire.cpp','tests/king_wire_tests.cpp',
 'tests/fixtures/king_wire_original.json','tools/oracle_king_wire.py','tools/verify_king_wire.py')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');args=ap.parse_args()
 assert sha(BASE_MANIFEST)==BASE_PIN
 base=Path(json.loads(BASE_MANIFEST.read_text())['source'])
 run=args.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';run.mkdir(parents=True,exist_ok=False);shutil.copytree(base,source,symlinks=True)
 py={str(Path(m.__file__).resolve().relative_to(ROOT))for m in tuple(sys.modules.values())if getattr(m,'__file__',None)
  and str(Path(m.__file__).resolve()).startswith(str(ROOT/'tools')+os.sep)and str(m.__file__).endswith('.py')}
 files=set(OWNED)|py|{'docs/MP_KING_WIRE_SOURCE_RESEARCH.md','docs/MP_KING_REPLICATION_NATIVE_BINDING.md'}
 pins={p:sha(ROOT/p)for p in sorted(files)}
 for p in sorted(files):
  target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);assert sha(target)==pins[p]
 for p in ('include/halo2/source_king.hpp','include/halo2/source_king_replication.hpp','src/gameplay/multiplayer/source_king_replication.cpp'):
  assert sha(ROOT/p)==sha(base/p),('Required bank ABI drift',p)
 live=(ROOT/'CMakeLists.txt').read_text();cmake=(source/'CMakeLists.txt').read_text()
 declaration=re.search(r'        add_executable\(king_wire_tests.*?king_wire_original\.json\)\n',live,re.S).group()
 line='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_wire.cpp)\n'
 assert live.count(line)==1 and line not in cmake and 'add_executable(king_wire_tests'not in cmake
 anchor='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_replication.cpp)\n';assert cmake.count(anchor)==1
 cmake=cmake.replace(anchor,anchor+line)
 anchor='            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_replication_original.json)\n';assert cmake.count(anchor)==1
 cmake=cmake.replace(anchor,anchor+declaration);(source/'CMakeLists.txt').write_text(cmake)
 (run/'live-CMakeLists.txt').write_text(live)
 (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(base/'CMakeLists.txt'),
  live_sha256=sha(run/'live-CMakeLists.txt'),frozen_sha256=sha(source/'CMakeLists.txt'),
  production_source_line=line,test_declaration=declaration),sort_keys=True,indent=2)+'\n')
 files_in_source=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:
  frozen=dict(zip((str(p.relative_to(source))for p in files_in_source),pool.map(sha,files_in_source)))
 (run/'source-hashes.json').write_text(json.dumps(frozen,sort_keys=True,indent=2)+'\n')
 commands=[];env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,argv,extra=None):
  log=run/f'{label}.log'
  with log.open('w')as output:
   result=subprocess.run(list(map(str,argv)),cwd=source,env={**env,**(extra or {})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,argv)),exit=result.returncode,
   log=str(log.relative_to(args.output.parent)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
  print(label,'exit',result.returncode,flush=True);assert result.returncode==0,log
 original=run/'original';original.mkdir()
 execute('original',[sys.executable,source/'tools/oracle_king_wire.py','--output',original/'report.json','--fixture',original/'fixture.json'])
 outputs=['report.json']+[p.name for p in original.glob('*.asm')]
 assert sha(original/'fixture.json')==pins['tests/fixtures/king_wire_original.json']
 for filename in outputs:assert sha(original/filename)==sha(oracle_king_wire.OUT/filename),filename
 build=run/'build'
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo'])
 targets=['king_wire_tests','king_replication_tests','king_engine_tests','king_callback_tests']
 execute('build',['cmake','--build',build,'--target',*targets,'-j2'])
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R','^original_king_(wire|replication|engine|callbacks)$'])
 assert re.search(r'100% tests passed.*out of 4',(run/'ctest.log').read_text())
 sanitized=run/'king_wire_tests_asan'
 execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(source/'include'), '-I'+str(source/'src'),
  source/'tests/king_wire_tests.cpp',source/'src/gameplay/multiplayer/source_king_wire.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,source/'tests/fixtures/king_wire_original.json'],
  {'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),frozen))
 assert all(frozen[p]==digest for p,digest in zip(frozen,current))
 drift=[p for p in frozen if p!='CMakeLists.txt'and(ROOT/p).is_file()and sha(ROOT/p)!=frozen[p]]
 assert not(set(OWNED)&set(drift)),drift
 for p in (oracle_king_wire.CONTRACTS,oracle_king_wire.WIRE_CONTRACTS):
  assert sha(p)==json.loads((original/'report.json').read_text())['static_contracts_sha256'if p==oracle_king_wire.CONTRACTS else'wire_contracts_sha256']
 report=json.loads((original/'report.json').read_text())
 manifest=dict(schema='king-wire-native-verification-v1',boundary=__doc__,base_manifest=str(BASE_MANIFEST),base_manifest_sha256=BASE_PIN,
  source=str(source),source_hashes_sha256=sha(run/'source-hashes.json'),cmake_overlay_sha256=sha(run/'cmake-overlay.json'),
  overlaid_files=pins,live_checkout_drift=drift,commands=commands,original_summary=report['summary'],
  original_outputs={p:sha(original/p)for p in outputs},fixture_sha256=sha(original/'fixture.json'),
  binaries={str(p.relative_to(args.output.parent)):sha(p)for p in [*(build/t for t in targets),sanitized]},
  python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__,
  native_session_delivery=False,original_packet_setup=False,outer_packet_acceptance=False,additional_variant_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 shutil.copy2(run/'verification.json',args.output);print(args.output,flush=True)
if __name__=='__main__':main()
