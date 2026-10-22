#!/usr/bin/env python3
"""Repair only the handoff test's incorrect absent-automatic-assets assertion.

Keep the complete frozen production build and twelve passing CTests unchanged.
Compile the corrected runtime test against the same hashed runtime/archives,
then complete both network regressions and scoped recording ASan/UBSan.
The original failing test, executable and CTest log remain preserved.
"""
import concurrent.futures,datetime,difflib,json,os,re,shlex,shutil,subprocess
from pathlib import Path
import verify_king_recording_native as native
ROOT,OUT=native.ROOT,native.OUT
BASE=OUT/'verify-20261012T031259.924802Z'
TEST='tests/multiplayer_outcome_runtime_tests.cpp'
SCRIPT='tools/verify_king_recording_native_repair.py'
sha=native.sha
def link_command(build,target):
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands',target],text=True)
 cmd=shlex.split(next(line for line in reversed(listing.splitlines())if f' -o {target} 'in line))
 if cmd[:2]==[':','&&']:cmd=cmd[2:]
 if cmd[-2:]==['&&',':']:cmd=cmd[:-2]
 return cmd
def main():
 source=BASE/'frozen-source';build=BASE/'build';pins=json.loads((BASE/'source-hashes.json').read_text())
 prior=json.loads((BASE/'commands.json').read_text());assert [c['exit']for c in prior]==[0,0,8]
 assert all(sha(OUT/c['log'])==c['log_sha256']for c in prior)
 old_log=(OUT/prior[-1]['log']).read_text();passed=re.findall(r'Test\s+#\d+:\s+([a-z_]+)\s+\.+\s+Passed',old_log)
 assert len(passed)==12 and '92% tests passed, 1 tests failed out of 13'in old_log
 assert old_log.count('***Failed')==1 and 'native_multiplayer_outcome_handoff'in old_log
 runtime_name='src/gameplay/multiplayer/multiplayer_runtime.cpp'
 assert all(sha(ROOT/p)==pins[p]for p in native.OWNED if p not in (TEST,runtime_name)),'Owned production drift'
 frozen_runtime=(source/runtime_name).read_text();live_runtime=(ROOT/runtime_name).read_text()
 marker='    std::map<std::uint32_t,AnimationClip> fp_clips;'
 assert frozen_runtime.count(marker)==live_runtime.count(marker)==1
 assert frozen_runtime.split(marker)[0]==live_runtime.split(marker)[0],'Actual headless/runtime handoff prefix drift'
 assert sha(ROOT/TEST)!=pins[TEST]
 run=OUT/('repair-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'));run.mkdir(parents=True);overlay=run/'overlay';overlaid={}
 for p in (TEST,SCRIPT):
  target=overlay/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);overlaid[p]=sha(target)
 (run/'overlay-hashes.json').write_text(json.dumps(overlaid,sort_keys=True,indent=2)+'\n')
 (run/'concurrent-render-runtime.patch').write_text(''.join(difflib.unified_diff(frozen_runtime.splitlines(True),live_runtime.splitlines(True),fromfile='frozen/src/multiplayer_runtime.cpp',tofile='live/src/gameplay/multiplayer/multiplayer_runtime.cpp')))
 runtime_drift=dict(frozen_sha256=pins[runtime_name],live_sha256=sha(ROOT/runtime_name),unchanged_through_marker=marker,
  actual_headless_prefix_equal=True,patch_sha256=sha(run/'concurrent-render-runtime.patch'))
 commands=[];env={**os.environ,'SDL_AUDIODRIVER':'dummy','PYTHONDONTWRITEBYTECODE':'1'}
 entries=json.loads((build/'compile_commands.json').read_text());links={t:link_command(build,t)for t in ('multiplayer_outcome_runtime_tests','king_recording_runtime_tests')}
 artifacts={str(build/t):sha(build/t)for t in native.TARGETS}
 for link in links.values():
  for token in link:
   p=Path(token)if Path(token).is_absolute()else build/token
   if token.endswith(('.a','.so','.o'))and p.is_file():artifacts[str(p.resolve())]=sha(p)
 (run/'base-artifact-hashes.json').write_text(json.dumps(artifacts,sort_keys=True,indent=2)+'\n')
 inputs=json.loads((BASE/'input-hashes.json').read_text());assert all(sha(p)==v for p,v in inputs.items())
 def execute(label,cmd,cwd=source,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=cwd,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,cwd=str(cwd),exit=r.returncode,log=str(log.relative_to(OUT)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 def compile_owner(name,label,sanitized):
  row=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(row['command']);obj=run/(label+'.o')
  cmd=[str(overlay/name)if token==str(source/name)and name in overlaid else token for token in cmd]
  cmd[cmd.index('-o')+1]=str(obj)
  for option in ('-MF','-MT'):
   if option in cmd:cmd[cmd.index(option)+1]=str(obj)+('.d'if option=='-MF'else'')
  if sanitized:cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(label,cmd,Path(row['directory']));return obj
 def link_test(target,objects,label,sanitized):
  cmd=[]
  for token in links[target]:
   if token.startswith('-Wl,--dependency-file='):continue
   if token.endswith(target+'.cpp.o'):cmd.extend(map(str,objects))
   else:cmd.append(token)
  binary=run/label;cmd[cmd.index('-o')+1]=str(binary)
  if sanitized:cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('link-'+label,cmd,build);return binary
 obj=compile_owner(TEST,'compile-runtime-test',False);runtime=link_test('multiplayer_outcome_runtime_tests',[obj],'multiplayer_outcome_runtime_tests',False)
 execute('runtime-test',[runtime,source/'assets/disc/maps/lockout.map',source/'assets/disc/default.xbe'])
 for target in ('multiplayer_fp_network_tests','multiplayer_scope_network_tests'):
  execute(target,[build/target,source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',native.U5])
 owners=('src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/multiplayer/source_result_event.cpp','src/gameplay/multiplayer/multiplayer_outcome.cpp','tests/king_recording_runtime_tests.cpp')
 objects=[compile_owner(p,'sanitizer-compile-'+str(i),True)for i,p in enumerate(owners)]
 sanitized=link_test('king_recording_runtime_tests',objects,'king_recording_runtime_tests_asan',True)
 execute('sanitizer-test',[sanitized,source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',native.U5],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),pins))
 assert all(pins[p]==v for p,v in zip(pins,current))and all(sha(p)==v for p,v in inputs.items())and all(sha(p)==v for p,v in artifacts.items())
 assert all(sha(overlay/p)==v and sha(ROOT/p)==v for p,v in overlaid.items())
 drift=[p for p,v in pins.items()if p!='CMakeLists.txt'and(ROOT/p).is_file()and sha(ROOT/p)!=v]
 assert not((set(native.OWNED)-{TEST,runtime_name})&set(drift)),drift
 assert sha(ROOT/runtime_name)==runtime_drift['live_sha256']
 result=dict(schema='king-recording-native-repair-v1',boundary=__doc__,source=str(source),overlay=str(overlay),base=str(BASE),
  base_source_hashes_sha256=sha(BASE/'source-hashes.json'),base_input_hashes_sha256=sha(BASE/'input-hashes.json'),
  base_cmake_overlay_sha256=sha(BASE/'cmake-overlay.json'),base_commands_sha256=sha(BASE/'commands.json'),base_failure_preserved=True,
  base_artifact_hashes_sha256=sha(run/'base-artifact-hashes.json'),overlay_hashes_sha256=sha(run/'overlay-hashes.json'),
  unchanged_passed_ctests=passed,replaced_failed_ctest='native_multiplayer_outcome_handoff',commands=commands,live_checkout_drift=drift,concurrent_render_runtime_drift=runtime_drift,
  binaries={str(p.relative_to(OUT)):sha(p)for p in (runtime,sanitized)},scoped_sanitized_owners=list(owners),ordinary_linked_libraries=True,
  native_king_recording=True,complete_retained_publication=False,normal_king_admission=False,complete_match_flows=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',OUT/'verification.json');print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
