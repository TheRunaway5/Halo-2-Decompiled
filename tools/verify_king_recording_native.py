#!/usr/bin/env python3
"""Freeze native King recording, original event comparison and runtime retention.

Build every selected target on one coherent complete source snapshot. Native
tests use actual Lockout/Xbox data with declared time32/debug-position inputs;
the real headless runtime also exercises its default system wall-clock owner.
Presentation, full retained publication and normal King admission stay gated.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/king-objective-network-native-20261012/verify-20261012T024545.653433Z'
OUT=ROOT/'analysis/multiplayer/game-variants/king-recording-native-20261012'
U5='/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
CORE=ROOT/'analysis/multiplayer/game-variants/result-event-native-20261012/verification.json'
CORE_PIN='1ea1d9e4d59a284cf0b03d996a2dc2af9874e84359bcb1e883c5b6565b660c72'
OWNED=('include/halo2/multiplayer.hpp','src/gameplay/multiplayer/multiplayer.cpp','include/halo2/multiplayer_outcome.hpp','src/gameplay/multiplayer/multiplayer_outcome.cpp',
 'src/gameplay/multiplayer/multiplayer_runtime.cpp','tests/multiplayer_outcome_tests.cpp','tests/multiplayer_outcome_runtime_tests.cpp','tests/king_runtime_tests.cpp',
 'tests/king_objective_network_tests.cpp','tests/king_recording_runtime_tests.cpp','include/halo2/source_result_event.hpp',
 'src/gameplay/multiplayer/source_result_event.cpp','tests/result_event_tests.cpp','tools/verify_king_recording_native.py')
TARGETS=('king_recording_runtime_tests','king_objective_network_tests','king_runtime_tests','multiplayer_tests','camouflage_runtime_tests',
 'multiplayer_outcome_tests','multiplayer_outcome_runtime_tests','result_event_tests','result_statistics_tests','result_capture_tests','result_retention_tests',
 'multiplayer_fp_network_tests','multiplayer_scope_network_tests')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');ap.add_argument('--resume-run',type=Path);a=ap.parse_args()
 assert sha(CORE)==CORE_PIN;core=json.loads(CORE.read_text());base=BASE/'frozen-source'
 run=a.resume_run.resolve()if a.resume_run else a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';journal=run/'commands.json'
 if a.resume_run:
  pins=json.loads((run/'source-hashes.json').read_text());commands=json.loads(journal.read_text());assert all(sha(source/p)==v for p,v in pins.items())
 else:
  run.mkdir(parents=True,exist_ok=False);shutil.copytree(base,source,symlinks=True);commands=[]
  for p in OWNED:
   target=source/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target)
  for p in ('docs/MP_RESULT_EVENT_NATIVE_BINDING.md','docs/MP_KING_RECORDING_EVENT_SOURCE_RESEARCH.md','tools/research_king_recording_event.py'):
   shutil.copy2(ROOT/p,source/p)
  cmake=(source/'CMakeLists.txt').read_text();live=(ROOT/'CMakeLists.txt').read_text();line='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_event.cpp)\n'
  assert live.count(line)==1 and line not in cmake;anchor='target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_capture.cpp)\n';assert cmake.count(anchor)==1;cmake=cmake.replace(anchor,anchor+line)
  declarations=[]
  for target,pattern,anchor in (
   ('result_event_tests',r'        add_executable\(result_event_tests.*?complete/report\.json\)\n','        add_executable(result_retention_tests tests/result_retention_tests.cpp src/gameplay/multiplayer/source_result_retention.cpp)\n'),
   ('king_recording_runtime_tests',r'    add_executable\(king_recording_runtime_tests.*?default\.xbe"\)\n','    target_link_libraries(king_objective_network_tests PRIVATE halo2_systems)\n')):
   declaration=re.search(pattern,live,re.S).group();assert target not in cmake and cmake.count(anchor)==1
   cmake=cmake.replace(anchor,anchor+declaration);declarations.append(declaration)
  (source/'CMakeLists.txt').write_text(cmake);(run/'live-CMakeLists.txt').write_text(live)
  (run/'cmake-overlay.json').write_text(json.dumps(dict(base_sha256=sha(base/'CMakeLists.txt'),live_sha256=sha(run/'live-CMakeLists.txt'),
   frozen_sha256=sha(source/'CMakeLists.txt'),production_source_line=line,test_declarations=declarations),sort_keys=True,indent=2)+'\n')
  paths=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
  with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:pins=dict(zip((str(p.relative_to(source))for p in paths),pool.map(sha,paths)))
  (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 inputs={str(CORE):CORE_PIN}
 for name in ('assets/disc/maps/lockout.map','assets/disc/maps/shared.map','assets/disc/default.xbe',U5):inputs[str(source/name)]=sha(source/name)
 for folder in ('complete','activation/complete'):
  for p in sorted((ROOT/'analysis/multiplayer/game-variants/king-recording-event-source-20261012'/folder).iterdir()):
   if p.is_file():inputs[str(p)]=sha(p)
 inputs[str(ROOT/'analysis/multiplayer/game-variants/king-recording-event-source-20261012/activation/ordering_evidence.py')]='d25566862e59b8da38797b4c22942461aa87102b087eec4d2113ce0dcb72277c'
 assert all(sha(p)==v for p,v in inputs.items());(run/'input-hashes.json').write_text(json.dumps(inputs,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'SDL_AUDIODRIVER':'dummy','PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,cwd=source,extra=None):
  cmd=list(map(str,cmd));prior=next((c for c in commands if c['label']==label and c['exit']==0),None)
  if prior:
   assert prior['command']==cmd and sha(a.output.parent/prior['log'])==prior['log_sha256'];return
  count=sum(c['label']==label for c in commands);log=run/(label+('-retry'+str(count)if count else '')+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=cwd,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,cwd=str(cwd),exit=r.returncode,log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  journal.write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j4'])
 expression='^(native_(king_recording_runtime|king_objective_network|king_runtime|multiplayer|lan_startup|camouflage_runtime|multiplayer_outcome_ownership|multiplayer_outcome_handoff)|retail_multiplayer|original_result_(event|statistics|capture|retention))$'
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R',expression])
 ctest=next(c for c in reversed(commands)if c['label']=='ctest'and c['exit']==0);assert re.search(r'100% tests passed.*out of 13',(a.output.parent/ctest['log']).read_text())
 for target in ('multiplayer_fp_network_tests','multiplayer_scope_network_tests'):execute(target,[build/target,source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',U5])
 entries=json.loads((build/'compile_commands.json').read_text());objects=[]
 owners=('src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/multiplayer/source_result_event.cpp','src/gameplay/multiplayer/multiplayer_outcome.cpp','tests/king_recording_runtime_tests.cpp')
 for i,name in enumerate(owners):
  row=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(row['command']);obj=run/f'sanitized-{i}.o';cmd[cmd.index('-o')+1]=str(obj)
  for option in ('-MF','-MT'):
   if option in cmd:cmd[cmd.index(option)+1]=str(obj)+('.d'if option=='-MF'else'')
  cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']);execute('sanitizer-compile-'+str(i),cmd,Path(row['directory']));objects.append(obj)
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','king_recording_runtime_tests'],text=True)
 link=shlex.split(next(line for line in reversed(listing.splitlines())if ' -o king_recording_runtime_tests 'in line))
 if link[:2]==[':','&&']:link=link[2:]
 if link[-2:]==['&&',':']:link=link[:-2]
 cmd=[]
 for token in link:
  if token.startswith('-Wl,--dependency-file='):continue
  if token.endswith('king_recording_runtime_tests.cpp.o'):cmd.extend(map(str,objects))
  else:cmd.append(token)
 sanitized=run/'king_recording_runtime_tests_asan';cmd[cmd.index('-o')+1]=str(sanitized);cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
 execute('sanitizer-link',cmd,build);execute('sanitizer-test',[sanitized,source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',U5],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(p)==v for p,v in inputs.items())
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current));drift=[p for p,v in pins.items()if p!='CMakeLists.txt'and(ROOT/p).is_file()and sha(ROOT/p)!=v]
 assert not(set(OWNED)&set(drift)),drift
 result=dict(schema='king-recording-native-v1',boundary=__doc__,source=str(source),source_hashes_sha256=sha(run/'source-hashes.json'),
  input_hashes_sha256=sha(run/'input-hashes.json'),cmake_overlay_sha256=sha(run/'cmake-overlay.json'),core_manifest_sha256=CORE_PIN,
  commands=commands,live_checkout_drift=drift,binaries={str(p.relative_to(a.output.parent)):sha(p)for p in (*(build/t for t in TARGETS),sanitized)},
  scoped_sanitized_owners=list(owners),ordinary_linked_libraries=True,native_king_recording=True,
  complete_retained_publication=False,normal_king_admission=False,complete_match_flows=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',a.output);print(a.output,flush=True)
if __name__=='__main__':main()
