#!/usr/bin/env python3
"""Freeze original result-event recording and its native event/ring implementation.

Actual root CMake declarations are overlaid onto a compatible complete frozen
source snapshot. Fresh unchanged Xbox/kernel execution, four focused CTests
and full standalone ASan/UBSan compare the event bytes and complete ring.
Native world time/pose/lifecycle and final retained publication stay separate.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/king-objective-network-native-20261012/verify-20261012T024545.653433Z/frozen-source'
REPORT=ROOT/'analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json'
REPORT_PIN='395b5d08cbcc47bf421349051cec917afc2689882239ff53a044c07c17915da8'
OUT=ROOT/'analysis/multiplayer/game-variants/result-event-native-20261012'
OWNED=('include/halo2/source_result_event.hpp','src/source_result_event.cpp','tests/result_event_tests.cpp','tools/verify_result_event.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');a=ap.parse_args()
 assert sha(REPORT)==REPORT_PIN;report=json.loads(REPORT.read_text())
 run=a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'));run.mkdir(parents=True,exist_ok=False)
 source=run/'frozen-source';shutil.copytree(BASE,source,symlinks=True)
 files=set(OWNED)|set(report['dependencies'])|{'tools/research_king_recording_event.py'};overlaid={}
 for p in sorted(files):
  expected=report['tool_sha256']if p=='tools/research_king_recording_event.py'else report['dependencies'].get(p)
  if expected:assert sha(ROOT/p)==expected,p
  target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);overlaid[p]=sha(target)
 for p in ('include/halo2/source_result_statistics.hpp','include/halo2/source_game_end.hpp','src/source_result_statistics.cpp','src/source_result_capture.cpp','src/source_result_retention.cpp'):
  assert sha(ROOT/p)==sha(BASE/p),('Required retained ABI drift',p)
 cmake=(source/'CMakeLists.txt').read_text();live=(ROOT/'CMakeLists.txt').read_text()
 declaration=re.search(r'        add_executable\(result_event_tests.*?complete/report\.json\)\n',live,re.S).group()
 line='target_sources(halo2_systems PRIVATE src/source_result_event.cpp)\n';assert live.count(line)==1 and line not in cmake
 anchor='target_sources(halo2_systems PRIVATE src/source_result_capture.cpp)\n';assert cmake.count(anchor)==1;cmake=cmake.replace(anchor,anchor+line)
 anchor='        add_executable(result_retention_tests tests/result_retention_tests.cpp src/source_result_retention.cpp)\n';assert cmake.count(anchor)==1;cmake=cmake.replace(anchor,anchor+declaration)
 (source/'CMakeLists.txt').write_text(cmake);(run/'live-CMakeLists.txt').write_text(live)
 (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(BASE/'CMakeLists.txt'),live_sha256=sha(run/'live-CMakeLists.txt'),
  frozen_sha256=sha(source/'CMakeLists.txt'),production_source_line=line,test_declaration=declaration),sort_keys=True,indent=2)+'\n')
 paths=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:pins=dict(zip((str(p.relative_to(source))for p in paths),pool.map(sha,paths)))
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n');commands=[]
 env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=source,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,exit=r.returncode,log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 original=run/'original';execute('original',[sys.executable,source/'tools/research_king_recording_event.py','--output',original])
 outputs={p.name:sha(p)for p in original.iterdir()if p.is_file()}
 assert len(outputs)==65 and all(v==sha(REPORT.parent/p)for p,v in outputs.items())
 build=run/'build';execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON'])
 targets=('result_event_tests','result_statistics_tests','result_capture_tests','result_retention_tests')
 execute('build',['cmake','--build',build,'--target',*targets,'-j4'])
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R','^original_result_(event|statistics|capture|retention)$'])
 assert re.search(r'100% tests passed.*out of 4',(run/'ctest.log').read_text())
 sanitized=run/'result_event_tests_asan';execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(source/'include'),source/'tests/result_event_tests.cpp',source/'src/source_result_event.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,original/'report.json'],{'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current))and sha(REPORT)==REPORT_PIN
 drift=[p for p,v in pins.items()if p!='CMakeLists.txt'and(ROOT/p).is_file()and sha(ROOT/p)!=v];assert not(set(OWNED)&set(drift)),drift
 result=dict(schema='source-result-event-native-v1',boundary=__doc__,source=str(source),base=str(BASE),source_hashes_sha256=sha(run/'source-hashes.json'),
  cmake_overlay_sha256=sha(run/'cmake-overlay.json'),overlaid_files=overlaid,commands=commands,live_checkout_drift=drift,
  original_report_sha256=REPORT_PIN,original_outputs=outputs,original_summary=report['summary'],
  binaries={str(p.relative_to(a.output.parent)):sha(p)for p in (*(build/t for t in targets),sanitized)},
  platform_time_pose_explicit_services=True,native_world_recording=False,additional_variant_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',a.output);print(a.output,flush=True)
if __name__=='__main__':main()
