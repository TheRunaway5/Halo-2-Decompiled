#!/usr/bin/env python3
"""Verify the corrected slot-reuse test on the unchanged frozen production build.

The prior test attempted join after game over. Its failure is preserved. Only
the test is replaced; production objects and five passing baseline checks are
reused with explicit hashes. This proves native objective UDP delivery, not
normal King admission, original Xbox wire compatibility or complete matches.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/king-objective-network-native-20261012/verify-20261012T024545.653433Z'
OUT=BASE.parent
OWNED=('include/halo2/multiplayer.hpp','include/halo2/multiplayer_datagrams.hpp','src/multiplayer.cpp','src/multiplayer_network.cpp','tests/king_runtime_tests.cpp','tools/verify_king_objective_network.py')
TARGETS=('king_objective_network_tests','king_runtime_tests','multiplayer_tests','camouflage_runtime_tests','multiplayer_fp_network_tests','multiplayer_scope_network_tests')
TEST='tests/king_objective_network_tests.cpp'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');a=ap.parse_args()
 source=BASE/'frozen-source';build=BASE/'build';base_pins=json.loads((BASE/'source-hashes.json').read_text())
 assets=json.loads((BASE/'asset-hashes.json').read_text());prior=json.loads((BASE/'commands.json').read_text())
 assert [c['exit']for c in prior]==[0,0,8],prior
 assert all(sha(source/p)==v for p,v in base_pins.items())and all(sha(source/p)==v for p,v in assets.items())
 assert all(sha(ROOT/p)==base_pins[p]for p in OWNED),'Owned production or baseline driver drift'
 assert sha(ROOT/TEST)!=base_pins[TEST],'Expected the isolated test repair'
 run=a.output.parent/('repair-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'));run.mkdir(parents=True)
 overlay=run/'overlay';pins={}
 for p in (TEST,'tools/verify_king_objective_network_repair.py'):
  target=overlay/p;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,target);pins[p]=sha(target)
 (run/'overlay-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 entries=json.loads((build/'compile_commands.json').read_text())
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','king_objective_network_tests'],text=True)
 link=shlex.split(next(line for line in reversed(listing.splitlines())if ' -o king_objective_network_tests 'in line))
 if link[:2]==[':','&&']:link=link[2:]
 if link[-2:]==['&&',':']:link=link[:-2]
 artifacts={str(build/t):sha(build/t)for t in TARGETS}
 for token in link:
  p=Path(token)if Path(token).is_absolute()else build/token
  if token.endswith(('.a','.so','.o'))and p.is_file():artifacts[str(p.resolve())]=sha(p)
 (run/'base-artifact-hashes.json').write_text(json.dumps(artifacts,sort_keys=True,indent=2)+'\n')
 commands=[];env={**os.environ,'SDL_AUDIODRIVER':'dummy','PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,cwd=overlay,extra=None):
  cmd=list(map(str,cmd));log=run/(label+'.log')
  with log.open('w')as out:r=subprocess.run(cmd,cwd=cwd,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=cmd,cwd=str(cwd),exit=r.returncode,log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True)
  assert r.returncode==0,log
 def compile_owner(name,label,sanitized):
  row=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(row['command']);obj=run/(label+'.o')
  cmd=[str(overlay/name)if token==str(source/name)and name in pins else token for token in cmd]
  cmd[cmd.index('-o')+1]=str(obj)
  for option in ('-MF','-MT'):
   if option in cmd:cmd[cmd.index(option)+1]=str(obj)+('.d'if option=='-MF'else'')
  if sanitized:cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  else:cmd.extend(['-Werror'])
  execute(label,cmd,Path(row['directory']));return obj
 def link_test(objects,name,sanitized):
  cmd=[]
  for token in link:
   if token.startswith('-Wl,--dependency-file='):continue
   if token.endswith('king_objective_network_tests.cpp.o'):cmd.extend(map(str,objects))
   else:cmd.append(token)
  binary=run/name;cmd[cmd.index('-o')+1]=str(binary)
  if sanitized:cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('link-'+name,cmd,build);return binary
 obj=compile_owner(TEST,'compile-test',False);binary=link_test([obj],'king_objective_network_tests',False)
 u5=next(p for p in assets if Path(p).is_absolute());args=[source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',u5]
 execute('objective-test',[binary,*args])
 execute('baseline-ctest',['ctest','--test-dir',build,'--verbose','-R','^(native_king_runtime|native_multiplayer|native_lan_startup|retail_multiplayer|native_camouflage_runtime)$'])
 assert re.search(r'100% tests passed.*out of 5',(run/'baseline-ctest.log').read_text())
 for target in ('multiplayer_fp_network_tests','multiplayer_scope_network_tests'):execute(target,[build/target,*args])
 objects=[compile_owner(p,'sanitizer-compile-'+str(i),True)for i,p in enumerate(('src/multiplayer.cpp','src/multiplayer_network.cpp',TEST))]
 sanitized=link_test(objects,'king_objective_network_tests_asan',True)
 execute('sanitizer-test',[sanitized,*args],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),base_pins))
 assert all(base_pins[p]==v for p,v in zip(base_pins,current))and all(sha(source/p)==v for p,v in assets.items())
 assert all(sha(p)==v for p,v in artifacts.items())and all(sha(overlay/p)==v and sha(ROOT/p)==v for p,v in pins.items())
 drift=[p for p,v in base_pins.items()if(ROOT/p).is_file()and sha(ROOT/p)!=v]
 assert not(set(OWNED)&set(drift))
 result=dict(schema='king-objective-network-native-repair-v1',boundary=__doc__,source=str(source),overlay=str(overlay),
  base=str(BASE),base_source_hashes_sha256=sha(BASE/'source-hashes.json'),base_asset_hashes_sha256=sha(BASE/'asset-hashes.json'),
  base_commands_sha256=sha(BASE/'commands.json'),base_failure_preserved=True,overlay_hashes_sha256=sha(run/'overlay-hashes.json'),
  base_artifact_hashes_sha256=sha(run/'base-artifact-hashes.json'),commands=commands,live_checkout_drift=drift,
  binaries={str(p.relative_to(a.output.parent)):sha(p)for p in (binary,sanitized)},
  scoped_sanitized_owners=['src/multiplayer.cpp','src/multiplayer_network.cpp',TEST],ordinary_linked_libraries=True,
  native_king_objective_delivery=True,normal_king_admission=False,complete_match_flows=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',a.output);print(a.output,flush=True)
if __name__=='__main__':main()
