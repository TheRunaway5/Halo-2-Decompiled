#!/usr/bin/env python3
"""Freeze and verify the shared player-damage consumer in actual native combat."""
import argparse,datetime,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/king-damage-native-20261011'
TARGETS=('player_damage_tests','king_runtime_tests','multiplayer_tests','prop_sandbox_tests','multiplayer_grenade_delivery_tests')
OWNERS=('src/source_player_damage.cpp','src/damage_amount.cpp','src/source_king.cpp','src/multiplayer.cpp','src/simulation.cpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json')
 ap.add_argument('--resume-run',type=Path);a=ap.parse_args()
 run=a.resume_run.resolve()if a.resume_run else a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp';commands=[]
 if a.resume_run:
  pins=json.loads((run/'source-hashes.json').read_text())
  assert pins and all(sha(source/name)==digest for name,digest in pins.items()),'frozen source changed'
 else:
  source.mkdir(parents=True);temp.mkdir()
  pins={}
  for folder in ('include','src','tests','tools','cmake','external','resources','shaders'):
   for path in sorted((ROOT/folder).rglob('*')):
    if not path.is_file()or '__pycache__'in path.parts or path.suffix in('.pyc','.pyo'):continue
    name=path.relative_to(ROOT);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(path.read_bytes());pins[str(name)]=sha(target)
  for name in ('CMakeLists.txt','README.md'):
   shutil.copy2(ROOT/name,source/name);pins[name]=sha(source/name)
  for name in ('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
  (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 assets={}
 for folder in ('assets/disc/maps','assets/map-pack/maps'):
  for path in sorted((source/folder).glob('*.map')):
   assets[str(path.relative_to(source))]=dict(bytes=path.stat().st_size,sha256=sha(path))
 for name in ('assets/disc/default.xbe','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'):
  path=source/name;assets[name]=dict(bytes=path.stat().st_size,sha256=sha(path))
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy'}
 def execute(label,cmd,cwd=source,extra=None):
  log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(list(map(str,cmd)),cwd=cwd,env={**env,**(extra or {})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=r.returncode,
      log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j2'])
 expression='^(original_player_damage|native_king_runtime|native_multiplayer|native_lan_startup|retail_multiplayer|live_prop_sandbox|multiplayer_grenade_delivery)$'
 execute('ctest',['ctest','--test-dir',build,'-R',expression,'--output-on-failure'])
 original=run/'original'
 execute('original-consumers',[sys.executable,source/'tools/oracle_king_trait_consumers.py','--output',original/'report.json','--fixture',original/'fixture.json'])
 assert sha(original/'fixture.json')==sha(source/'tests/fixtures/king_trait_consumers_original.json')
 report=json.loads((original/'report.json').read_text())
 entries=json.loads((build/'compile_commands.json').read_text());objects=[]
 for index,name in enumerate((*OWNERS,'tests/king_runtime_tests.cpp')):
  entry=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(entry['command']);obj=run/(f'sanitized-{index}.o')
  cmd[cmd.index('-o')+1]=str(obj);cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('sanitizer-compile-'+str(index),cmd,Path(entry['directory']));objects.append(obj)
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','king_runtime_tests'],text=True)
 line=next(line for line in reversed(listing.splitlines())if ' -o king_runtime_tests 'in line)
 link=shlex.split(line)
 if link[:2]==[':', '&&']:link=link[2:]
 if link[-2:]==['&&', ':']:link=link[:-2]
 cmd=[]
 for token in link:
  if token.startswith('-Wl,--dependency-file='):continue
  if token.endswith('king_runtime_tests.cpp.o'):cmd.extend(map(str,objects))
  else:cmd.append(token)
 binary=run/'king_runtime_tests_asan';cmd[cmd.index('-o')+1]=str(binary);cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
 execute('sanitizer-link',cmd,build)
 sanitizer={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
 execute('sanitizer-test',[binary,source/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'],extra=sanitizer)
 kernel=run/'player_damage_tests_asan'
 execute('kernel-sanitizer-build',['c++','-std=c++20','-O1','-g','-fno-fast-math','-ffp-contract=off','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(source/'include'),
     source/'tests/player_damage_tests.cpp',source/'src/source_player_damage.cpp',source/'src/damage_amount.cpp',source/'src/source_king.cpp','-o',kernel])
 execute('kernel-sanitizer-test',[kernel,source/'tests/fixtures/king_trait_consumers_original.json'],extra=sanitizer)
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all((source/name).stat().st_size==row['bytes']and sha(source/name)==row['sha256']for name,row in assets.items())
 result=dict(schema='native-player-damage-verification-v1',scope=__doc__,frozen_source=str(source.relative_to(a.output.parent)),
     source_hashes=str((run/'source-hashes.json').relative_to(a.output.parent)),source_hashes_sha256=sha(run/'source-hashes.json'),
     asset_hashes=str((run/'asset-hashes.json').relative_to(a.output.parent)),asset_hashes_sha256=sha(run/'asset-hashes.json'),
     verifier_driver=str(Path(__file__).resolve()),verifier_driver_sha256=sha(__file__),resumed=bool(a.resume_run),commands=commands,
     original_report_sha256=sha(original/'report.json'),original_damage_calls=5004,original_consumer_report=report,
     binaries={str(p.relative_to(a.output.parent)):sha(p)for p in(*(build/t for t in TARGETS),binary,kernel)},
     sanitized_owners=list(OWNERS),production_king_admission=False,king_transport_admission=False,goal_complete=False,
     live_checkout_drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest])
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
