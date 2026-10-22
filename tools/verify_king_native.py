#!/usr/bin/env python3
"""Freeze and verify the bounded native King binding without opening menu admission."""
import argparse,datetime,hashlib,json,os,re,shlex,shutil,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DEFAULT=ROOT/'analysis/multiplayer/game-variants/king-native-20261011'
TARGETS=('king_runtime_tests','king_callback_tests','king_engine_tests','multiplayer_tests','source_netgame_weapon_scene_tests')
FIXTURES=('king_engine_original.json','king_scoring_original.json','king_world_original.json','king_callbacks_original.json')
OWNERS=('src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/multiplayer/multiplayer_inventory.cpp','src/network/multiplayer_network.cpp','src/gameplay/multiplayer/source_king.cpp','src/assets/content.cpp')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=DEFAULT/'verification.json')
 ap.add_argument('--resume-run',type=Path,help='Resume an interrupted, unchanged frozen source snapshot; all verification phases rerun.')
 a=ap.parse_args()
 run=a.resume_run.resolve()if a.resume_run else a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp'
 # Copy complete source/configuration trees. Asset and original-analysis inputs
 # are shared read-only references; the executable oracles pin their originals.
 pins={};commands=[]
 if a.resume_run:
  pins=json.loads((run/'source-hashes.json').read_text())
  assert pins and all(sha(source/name)==digest for name,digest in pins.items()),'frozen source changed before resume'
 else:
  source.mkdir(parents=True);temp.mkdir()
  for folder in ('include','src','tests','tools','cmake','external','resources','shaders'):
   for p in sorted((ROOT/folder).rglob('*')):
    if not p.is_file() or '__pycache__'in p.parts or p.suffix in ('.pyc','.pyo'):continue
    relative=p.relative_to(ROOT);target=source/relative;target.parent.mkdir(parents=True,exist_ok=True)
    raw=p.read_bytes();target.write_bytes(raw);pins[str(relative)]=hashlib.sha256(raw).hexdigest()
  for name in ('CMakeLists.txt','README.md'):
   shutil.copy2(ROOT/name,source/name);pins[name]=sha(source/name)
  for name in ('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
  (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 # Pin every map read by the scene regression before tests, including both
 # possible selection roots. The source-only oracles separately pin their XBEs.
 names=re.findall(r'"([a-z_]+)"',(source/'tests/source_netgame_weapon_scene_tests.cpp').read_text().split('constexpr const char*maps[]={',1)[1].split('};',1)[0])
 assets={}
 for name in names:
  for folder in ('assets/disc/maps','assets/map-pack/maps'):
   p=source/folder/(name+'.map')
   if p.exists():assets[str(p.relative_to(source))]=dict(bytes=p.stat().st_size,sha256=sha(p))
 p=source/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 assets[str(p.relative_to(source))]=dict(bytes=p.stat().st_size,sha256=sha(p))
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,cwd=source,extra=None):
  log=run/(label+'.log')
  with log.open('w')as output:result=subprocess.run(list(map(str,cmd)),cwd=cwd,env={**env,**(extra or {})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=result.returncode,
    log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',result.returncode,flush=True)
  if result.returncode:raise RuntimeError(f'{label} failed; see {log}')
 # No copied native implementation participates in original output production.
 original=run/'original-callbacks'
 execute('original-callbacks',[sys.executable,source/'tools/oracle_king_callbacks.py','--output',original/'report.json','--fixture',original/'fixture.json'])
 assert sha(original/'fixture.json')==sha(source/'tests/fixtures/king_callbacks_original.json')
 report=json.loads((original/'report.json').read_text())
 assert report==json.loads((ROOT/'analysis/multiplayer/game-variants/king-callbacks-20261011/report.json').read_text())
 # This oracle embeds all complete listings in the reproduced report.
 assert len(report['instruction_listings'])==21 and all(row['instructions'] for row in report['instruction_listings'])
 execute('original-arena',[sys.executable,source/'tools/oracle_king_runtime_boundary.py','--output',run/'original-arena.json'])
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j2'])
 expression='^(native_king_runtime|original_king_callbacks|original_king_engine|native_multiplayer|native_lan_startup|retail_multiplayer|retail_source_netgame_weapon_scene)$'
 execute('ctest',['ctest','--test-dir',build,'-R',expression,'--output-on-failure'])
 # Instrument every changed owner reached by the native arena test. Remaining
 # runtime libraries retain their ordinary build; this is scoped sanitization.
 compilation=json.loads((build/'compile_commands.json').read_text());objects=[]
 selected=(*OWNERS,'tests/king_runtime_tests.cpp')
 for index,name in enumerate(selected):
  entry=next(e for e in compilation if Path(e['file'])==source/name);cmd=shlex.split(entry['command']);obj=run/(f'sanitized-{index}.o')
  where=cmd.index('-o');cmd[where+1]=str(obj)
  cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('sanitizer-compile-'+str(index),cmd,Path(entry['directory']));objects.append(obj)
 link=shlex.split((build/'CMakeFiles/king_runtime_tests.dir/link.txt').read_text())if(build/'CMakeFiles/king_runtime_tests.dir/link.txt').exists()else None
 if link is None:
  # Ninja emits a concrete link command in its command inventory.
  listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','king_runtime_tests'],text=True)
  line=next(line for line in reversed(listing.splitlines())if ' -o king_runtime_tests 'in line)
  link=shlex.split(line)
  if link[:2]==[':', '&&']:link=link[2:]
  if link[-2:]==['&&', ':']:link=link[:-2]
 binary=run/'king_runtime_tests_asan';cmd=[]
 for token in link:
  if token.startswith('-Wl,--dependency-file='):continue
  if token.endswith('king_runtime_tests.cpp.o'):cmd.extend(map(str,objects));continue
  cmd.append(token)
 where=cmd.index('-o');cmd[where+1]=str(binary);cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
 execute('sanitizer-link',cmd,build)
 execute('sanitizer-test',[binary,source/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 # A separate fully instrumented callback executable checks the new raw-bank
 # and trait owners independently of ordinary native runtime libraries.
 callback=run/'king_callback_tests_asan'
 execute('callback-sanitizer-build',['c++','-std=c++20','-O1','-g','-fno-fast-math','-ffp-contract=off','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(source/'include'), '-I'+str(source/'src'),
   source/'tests/king_callback_tests.cpp',source/'src/gameplay/multiplayer/source_king.cpp',source/'src/gameplay/multiplayer/source_result_statistics.cpp','-o',callback])
 execute('callback-sanitizer-test',[callback,source/'tests/fixtures/king_callbacks_original.json'],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all((source/name).stat().st_size==row['bytes']and sha(source/name)==row['sha256']for name,row in assets.items())
 drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest]
 result=dict(schema='native-king-binding-verification-v1',scope=__doc__,frozen_source=str(source.relative_to(a.output.parent)),
  source_hashes=str((run/'source-hashes.json').relative_to(a.output.parent)),source_hashes_sha256=sha(run/'source-hashes.json'),
  verifier_driver=str(Path(__file__).resolve()),verifier_driver_sha256=sha(__file__),resumed=bool(a.resume_run),
  asset_hashes=str((run/'asset-hashes.json').relative_to(a.output.parent)),asset_hashes_sha256=sha(run/'asset-hashes.json'),
  commands=commands,live_checkout_drift=drift,original_callback_calls=report['whole_calls'],
  binaries={str(p.relative_to(a.output.parent)):sha(p)for p in(*(build/t for t in TARGETS),binary,callback)},
  native_sanitized_owners=list(selected),production_king_admission=False,king_transport_admission=False,goal_complete=False)
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(str(a.output),flush=True)
if __name__=='__main__':main()
