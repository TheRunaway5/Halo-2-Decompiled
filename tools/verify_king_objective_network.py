#!/usr/bin/env python3
"""Freeze and verify native King objective snapshots and real UDP delivery.

Explicit debug positions and observed recording/presentation boundaries isolate
transport from normal King admission. This is the native port protocol, not
Xbox wire interoperability or a complete playable-match claim.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/king-objective-network-native-20261012'
TARGETS=('king_objective_network_tests','king_runtime_tests','multiplayer_tests','camouflage_runtime_tests','multiplayer_fp_network_tests','multiplayer_scope_network_tests')
OWNED=('include/halo2/multiplayer.hpp','include/halo2/multiplayer_datagrams.hpp','src/gameplay/multiplayer/multiplayer.cpp','src/network/multiplayer_network.cpp','tests/king_runtime_tests.cpp','tests/king_objective_network_tests.cpp','tools/verify_king_objective_network.py')
U5='/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');ap.add_argument('--resume-run',type=Path);args=ap.parse_args()
 run=args.resume_run.resolve()if args.resume_run else args.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';journal=run/'commands.json'
 if args.resume_run:
  pins=json.loads((run/'source-hashes.json').read_text());commands=json.loads(journal.read_text())
  assert all(sha(source/p)==v for p,v in pins.items()),'Frozen source drift'
 else:
  run.mkdir(parents=True,exist_ok=False);source.mkdir();commands=[]
  for folder in ('include','src','tests','tools','cmake','external','resources','shaders','docs'):
   for path in sorted((ROOT/folder).rglob('*')):
    if not path.is_file()or '__pycache__'in path.parts or path.suffix in('.pyc','.pyo'):continue
    target=source/path.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)
  for name in ('CMakeLists.txt','README.md'):shutil.copy2(ROOT/name,source/name)
  for name in ('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
  files=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
  with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:pins=dict(zip((str(p.relative_to(source))for p in files),pool.map(sha,files)))
  (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 assets={name:sha(source/name)for name in('assets/disc/maps/lockout.map','assets/disc/maps/shared.map','assets/disc/maps/mainmenu.map','assets/disc/maps/single_player_shared.map','assets/disc/default.xbe',U5)}
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'SDL_AUDIODRIVER':'dummy','PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,argv,cwd=source,extra=None):
  argv=list(map(str,argv))
  prior=next((c for c in commands if c['label']==label and c['exit']==0),None)
  if prior:
   assert prior['command']==argv and sha(args.output.parent/prior['log'])==prior['log_sha256'];return
  log=run/(label+('-retry'+str(sum(c['label']==label for c in commands))if any(c['label']==label for c in commands)else '')+'.log')
  with log.open('w')as output:r=subprocess.run(argv,cwd=cwd,env={**env,**(extra or {})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=argv,cwd=str(cwd),exit=r.returncode,log=str(log.relative_to(args.output.parent)),log_sha256=sha(log)))
  journal.write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j4'])
 expression='^(native_king_objective_network|native_king_runtime|native_multiplayer|native_lan_startup|retail_multiplayer|native_camouflage_runtime)$'
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R',expression])
 ctest=next(c for c in reversed(commands)if c['label']=='ctest'and c['exit']==0)
 assert re.search(r'100% tests passed.*out of 6',(args.output.parent/ctest['log']).read_text())
 for target in('multiplayer_fp_network_tests','multiplayer_scope_network_tests'):
  execute(target,[build/target,source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',U5])
 entries=json.loads((build/'compile_commands.json').read_text());objects=[]
 for index,name in enumerate(('src/gameplay/multiplayer/multiplayer.cpp','src/network/multiplayer_network.cpp','tests/king_objective_network_tests.cpp')):
  row=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(row['command']);obj=run/f'sanitized-{index}.o'
  cmd[cmd.index('-o')+1]=str(obj);cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('sanitizer-compile-'+str(index),cmd,Path(row['directory']));objects.append(obj)
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','king_objective_network_tests'],text=True)
 link=shlex.split(next(line for line in reversed(listing.splitlines())if ' -o king_objective_network_tests 'in line))
 if link[:2]==[':','&&']:link=link[2:]
 if link[-2:]==['&&',':']:link=link[:-2]
 cmd=[]
 for token in link:
  if token.startswith('-Wl,--dependency-file='):continue
  if token.endswith('king_objective_network_tests.cpp.o'):cmd.extend(map(str,objects))
  else:cmd.append(token)
 sanitized=run/'king_objective_network_tests_asan';cmd[cmd.index('-o')+1]=str(sanitized);cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
 execute('sanitizer-link',cmd,build)
 execute('sanitizer-test',[sanitized,source/'assets/disc/maps/lockout.map',source/'assets/disc/maps',U5],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(source/p)==v for p,v in pins.items())and all(sha(source/p)==v for p,v in assets.items())
 drift=[p for p,v in pins.items()if(ROOT/p).is_file()and sha(ROOT/p)!=v];assert not(set(OWNED)&set(drift)),drift
 manifest=dict(schema='king-objective-network-native-v1',boundary=__doc__,source=str(source),source_hashes_sha256=sha(run/'source-hashes.json'),
  asset_hashes_sha256=sha(run/'asset-hashes.json'),commands=commands,live_checkout_drift=drift,
  binaries={str(p.relative_to(args.output.parent)):sha(p)for p in(*(build/t for t in TARGETS),sanitized)},
  scoped_sanitized_owners=['src/gameplay/multiplayer/multiplayer.cpp','src/network/multiplayer_network.cpp','tests/king_objective_network_tests.cpp'],
  ordinary_linked_libraries=True,native_king_objective_delivery=True,normal_king_admission=False,complete_match_flows=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n');shutil.copy2(run/'verification.json',args.output);print(args.output,flush=True)
if __name__=='__main__':main()
