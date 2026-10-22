#!/usr/bin/env python3
"""Freeze the shared score-change event producer against whole original calls.

Fresh unchanged retail/TU1.1/TU1.5 producers execute their ordinary real dispatch
with declared nonlocal users and actual tags. Native prefix, gate, ordering and
reload checks run through root CMake and full standalone ASan/UBSan. Live
network, captions, audio and normal game-mode admission remain separate.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE_MANIFEST=ROOT/'analysis/multiplayer/game-variants/result-event-native-20261012/verification.json'
BASE_PIN='1ea1d9e4d59a284cf0b03d996a2dc2af9874e84359bcb1e883c5b6565b660c72'
REPORT=ROOT/'analysis/multiplayer/game-variants/score-events-20261012/complete/report.json'
REPORT_PIN='a03d858a312d4ae06552b7edd36a88bff7f2df5285467abfd70e13368aac3c58'
OUT=ROOT/'analysis/multiplayer/game-variants/score-event-native-20261012'
OWNED=('include/halo2/source_score_event.hpp','src/gameplay/multiplayer/source_score_event.cpp','tests/score_event_tests.cpp','tools/oracle_score_events.py','tools/verify_score_event.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');a=ap.parse_args()
 assert sha(BASE_MANIFEST)==BASE_PIN and sha(REPORT)==REPORT_PIN;base=Path(json.loads(BASE_MANIFEST.read_text())['source']);report=json.loads(REPORT.read_text())
 run=a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'));run.mkdir(parents=True,exist_ok=False)
 source=run/'frozen-source';shutil.copytree(base,source,symlinks=True);overlaid={}
 for p in sorted(set(OWNED)|set(report['dependencies'])):
  expected=report['dependencies'].get(p)
  if expected:assert sha(ROOT/p)==expected,p
  target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);overlaid[p]=sha(target)
 assert overlaid['tools/oracle_score_events.py']==report['tool_sha256']
 assert sha(ROOT/'include/halo2/vista_announcer_events.hpp')==sha(base/'include/halo2/vista_announcer_events.hpp')
 cmake=(source/'CMakeLists.txt').read_text();live=(ROOT/'CMakeLists.txt').read_text()
 line='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_score_event.cpp)\n';assert live.count(line)==1 and line not in cmake
 anchor='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_event.cpp)\n';assert cmake.count(anchor)==1;cmake=cmake.replace(anchor,anchor+line)
 declaration=re.search(r'        add_executable\(score_event_tests.*?complete/report\.json\)\n',live,re.S).group();assert 'add_executable(score_event_tests'not in cmake
 anchor='            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json)\n';assert cmake.count(anchor)==1;cmake=cmake.replace(anchor,anchor+declaration)
 (source/'CMakeLists.txt').write_text(cmake);(run/'live-CMakeLists.txt').write_text(live)
 (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(base/'CMakeLists.txt'),live_sha256=sha(run/'live-CMakeLists.txt'),
  frozen_sha256=sha(source/'CMakeLists.txt'),production_source_line=line,test_declaration=declaration),sort_keys=True,indent=2)+'\n')
 paths=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:pins=dict(zip((str(p.relative_to(source))for p in paths),pool.map(sha,paths)))
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n');commands=[];env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=source,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,exit=r.returncode,log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 original=run/'original';execute('original',[sys.executable,source/'tools/oracle_score_events.py','--output',original/'report.json'])
 outputs={p.name:sha(p)for p in original.iterdir()if p.is_file()};assert len(outputs)==10 and all(v==sha(REPORT.parent/p)for p,v in outputs.items())
 build=run/'build';execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON'])
 targets=('score_event_tests','result_event_tests');execute('build',['cmake','--build',build,'--target',*targets,'-j2'])
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R','^original_(score_event|result_event)$']);assert re.search(r'100% tests passed.*out of 2',(run/'ctest.log').read_text())
 sanitized=run/'score_event_tests_asan';execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(source/'include'), '-I'+str(source/'src'),source/'tests/score_event_tests.cpp',source/'src/gameplay/multiplayer/source_score_event.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,original/'report.json'],{'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current))and sha(REPORT)==REPORT_PIN
 drift=[p for p,v in pins.items()if p!='CMakeLists.txt'and(ROOT/p).is_file()and sha(ROOT/p)!=v];assert not(set(OWNED)&set(drift)),drift
 result=dict(schema='source-score-event-native-v1',boundary=__doc__,base_manifest=str(BASE_MANIFEST),base_manifest_sha256=BASE_PIN,
  source=str(source),source_hashes_sha256=sha(run/'source-hashes.json'),cmake_overlay_sha256=sha(run/'cmake-overlay.json'),
  overlaid_files=overlaid,commands=commands,live_checkout_drift=drift,original_report_sha256=REPORT_PIN,original_outputs=outputs,original_summary=report['summary'],
  binaries={str(p.relative_to(a.output.parent)):sha(p)for p in (*(build/t for t in targets),sanitized)},
  native_live_publication=False,additional_variant_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',a.output);print(a.output,flush=True)
if __name__=='__main__':main()
