#!/usr/bin/env python3
"""Freeze native equipped-weapon camouflage binding, original material order and GPU owner isolation.

This does not admit the separate biped camouflage clone or any gated variant.
"""
import argparse,datetime,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-material-native-20261011'
TARGETS=('camouflage_material_tests','camouflage_material_gpu_tests','multiplayer_species_runtime_tests',
 'camouflage_context_tests','camouflage_tests','camouflage_runtime_tests','player_damage_tests','king_runtime_tests',
 'multiplayer_tests','prop_sandbox_tests','multiplayer_grenade_delivery_tests')
CHECKS=('original_camouflage_material_animation','camouflage_material_owner_isolation','original_camouflage_context',
 'original_camouflage','native_camouflage_runtime','original_player_damage','native_king_runtime','native_multiplayer',
 'native_lan_startup','retail_multiplayer','live_prop_sandbox','multiplayer_grenade_delivery','native_multiplayer_species_single')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json')
 ap.add_argument('--resume-run',type=Path)
 ap.add_argument('--gpu-test-source',type=Path,help='Freeze a corrected test-only fixture against unchanged resumed production libraries')
 a=ap.parse_args()
 if a.gpu_test_source and not a.resume_run:ap.error('--gpu-test-source requires --resume-run')
 run=a.resume_run.resolve()if a.resume_run else a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp';commands=[]
 if a.resume_run:
  pins=json.loads((run/'source-hashes.json').read_text());assert all(sha(source/n)==h for n,h in pins.items())
  prior=run/'prior-attempts'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ');prior.mkdir(parents=True)
  for path in run.glob('*.log'):shutil.copy2(path,prior/path.name)
  if(run/'verification-driver.py').exists():shutil.copy2(run/'verification-driver.py',prior/'verification-driver.py')
  for name in('camouflage-material-captures','Testing'):
   path=build/name
   if path.exists():shutil.copytree(path,prior/name)
 else:
  source.mkdir(parents=True);temp.mkdir()
  pins={}
  for folder in('include','src','tests','tools','cmake','external','resources','shaders'):
   for path in sorted((ROOT/folder).rglob('*')):
    if not path.is_file()or'__pycache__'in path.parts or path.suffix in('.pyc','.pyo'):continue
    name=path.relative_to(ROOT);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(path.read_bytes());pins[str(name)]=sha(target)
  for name in('CMakeLists.txt','README.md'):
   shutil.copy2(ROOT/name,source/name);pins[name]=sha(source/name)
  for name in('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
  (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 driver=run/'verification-driver.py';driver.write_bytes(Path(__file__).read_bytes())
 asset_paths=['assets/disc/default.xbe','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03']
 asset_paths.extend(str(p.relative_to(source))for folder in('assets/disc/maps','assets/map-pack/maps')for p in(source/folder).glob('*.map'))
 assets={name:sha(source/name)for name in asset_paths if(source/name).exists()}
 if(run/'asset-hashes.json').exists():
  old_assets=json.loads((run/'asset-hashes.json').read_text());assert all(assets[n]==h for n,h in old_assets.items())
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 validation=ROOT/'.tools/validation/usr';layer=validation/'share/vulkan/explicit_layer.d';library=validation/'lib'
 validation_assets={str(p):sha(p)for p in(layer/'VkLayer_khronos_validation.json',library/'libVkLayer_khronos_validation.so')}
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy',
  'VK_LAYER_PATH':str(layer),'LD_LIBRARY_PATH':str(library)+(':'+os.environ['LD_LIBRARY_PATH']if os.environ.get('LD_LIBRARY_PATH')else'')}
 def execute(label,cmd,cwd=source,extra=None):
  log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(list(map(str,cmd)),cwd=cwd,env={**env,**(extra or{})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=r.returncode,
   log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
  '-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j4'])
 selected_checks=tuple(n for n in CHECKS if not(a.gpu_test_source and n=='camouflage_material_owner_isolation'))
 execute('ctest',['ctest','--test-dir',build,'-R','^('+'|'.join(selected_checks)+')$','--output-on-failure'])
 ctests=json.loads(subprocess.check_output(['ctest','--test-dir',str(build),'--show-only=json-v1'],text=True))
 gpu_binary=build/'camouflage_material_gpu_tests';gpu_overlay=None
 if a.gpu_test_source:
  overlay=run/('gpu-fixture-overlay-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
  overlay.mkdir();test_source=overlay/'camouflage_material_gpu_tests.cpp';shutil.copy2(a.gpu_test_source,test_source)
  entry=next(e for e in json.loads((build/'compile_commands.json').read_text())if Path(e['file'])==source/'tests/camouflage_material_gpu_tests.cpp')
  cmd=shlex.split(entry['command']);obj=overlay/'camouflage_material_gpu_tests.o'
  cmd[cmd.index('-o')+1]=str(obj);cmd[cmd.index(str(source/'tests/camouflage_material_gpu_tests.cpp'))]=str(test_source)
  execute('gpu-overlay-compile',cmd,Path(entry['directory']))
  listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','camouflage_material_gpu_tests'],text=True)
  line=next(s for s in reversed(listing.splitlines())if' -o camouflage_material_gpu_tests 'in s);link=shlex.split(line)
  if link[:2]==[':', '&&']:link=link[2:]
  if link[-2:]==['&&', ':']:link=link[:-2]
  cmd=[str(obj)if token.endswith('camouflage_material_gpu_tests.cpp.o')else token for token in link if not token.startswith('-Wl,--dependency-file=')]
  gpu_binary=overlay/'camouflage_material_gpu_tests';cmd[cmd.index('-o')+1]=str(gpu_binary)
  execute('gpu-overlay-link',cmd,build)
  gpu_overlay=dict(source=str(test_source),source_sha256=sha(test_source),object_sha256=sha(obj),
   boundary='Only the GPU test fixture is replaced. Frozen production source, headers and libraries are unchanged; the old configured GPU test remains excluded and its failed logs are retained.')
 # CTest regards a missing-display77 as a skip, so verify the GPU test itself
 # actually completed. An unavailable display is not a successful GPU proof.
 execute('gpu-required',[gpu_binary,source/'assets/disc/maps',run/'gpu-required'])
 original=run/'original'
 execute('original-material',[sys.executable,source/'tools/oracle_camouflage_material_animation.py',
  '--output',original/'report.json','--fixture',original/'fixture.json'])
 fixture='tests/fixtures/camouflage_material_animation_original.json'
 assert sha(original/'fixture.json')==pins[fixture],'Original material fixture did not reproduce'
 entries=json.loads((build/'compile_commands.json').read_text());objects=[]
 for index,name in enumerate(('src/world_material.cpp','tests/camouflage_material_tests.cpp')):
  entry=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(entry['command']);obj=run/(f'sanitized-{index}.o')
  cmd[cmd.index('-o')+1]=str(obj);cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('sanitizer-compile-'+str(index),cmd,Path(entry['directory']));objects.append(obj)
 listing=subprocess.check_output(['ninja','-C',str(build),'-t','commands','camouflage_material_tests'],text=True)
 line=next(s for s in reversed(listing.splitlines())if' -o camouflage_material_tests 'in s);link=shlex.split(line)
 if link[:2]==[':', '&&']:link=link[2:]
 if link[-2:]==['&&', ':']:link=link[:-2]
 cmd=[]
 for token in link:
  if token.startswith('-Wl,--dependency-file='):continue
  if token.endswith('camouflage_material_tests.cpp.o'):cmd.extend(map(str,objects))
  else:cmd.append(token)
 binary=run/'camouflage_material_tests_asan';cmd[cmd.index('-o')+1]=str(binary);cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
 execute('sanitizer-link',cmd,build)
 execute('sanitizer-test',[binary,source/fixture],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(source/n)==h for n,h in pins.items());assert all(sha(source/n)==h for n,h in assets.items())
 assert all(sha(n)==h for n,h in validation_assets.items())
 result=dict(schema='native-camouflage-material-verification-v1',boundary=__doc__,frozen_source=str(source),
  source_hashes_sha256=sha(run/'source-hashes.json'),asset_hashes_sha256=sha(run/'asset-hashes.json'),commands=commands,
  driver_sha256=sha(driver),resumed=bool(a.resume_run),validation_assets=validation_assets,
  configured_checks=selected_checks,gpu_test_overlay=gpu_overlay,
  validation_environment={n:env[n]for n in('VK_LAYER_PATH','LD_LIBRARY_PATH')},
  fixture_sha256=sha(original/'fixture.json'),original_material_cases=1280,gpu_test_completed=True,
  sanitizer_scope='Changed production world_material owner and complete material-order test; remaining libraries built normally.',
  binaries={str(p.relative_to(a.output.parent)):sha(p)for p in(*(build/t for t in TARGETS),binary,gpu_binary)},
  captures={str(p.relative_to(a.output.parent)):sha(p)for p in sorted((run/'gpu-required').glob('*.ppm'))},
  native_equipped_weapon_binding=True,native_biped_clone=False,original_xbox_framebuffer_parity=False,
  production_king_admission=False,king_transport_admission=False,goal_complete=False,
  live_checkout_drift=[n for n,h in pins.items()if not(ROOT/n).exists()or sha(ROOT/n)!=h])
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
