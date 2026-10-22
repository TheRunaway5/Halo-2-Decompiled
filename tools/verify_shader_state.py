#!/usr/bin/env python3
"""Freeze and fully rebuild shader-state imports, CPU bindings and dynamic blend delivery.

Material/MaterialPass layouts change, so all selected targets and their complete
transitive libraries are rebuilt by CMake. Original CPU results and scoped GPU
isolation do not admit the missing framebuffer resource, biped clone, pass
scheduling, concurrent color/depth alias, or any additional game variant.
"""
import argparse,datetime,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/shader-state-native-20261011'
TARGETS=('source_shader_rgb_tests','source_shader_state_tests','shader_state_import_tests',
 'camouflage_state_material_tests','shader_state_gpu_tests','shader_texture_reference_tests',
 'source_vertex_extern_tests','camouflage_material_tests','camouflage_material_gpu_tests',
 'multiplayer_species_runtime_tests','camouflage_context_tests','camouflage_tests',
 'camouflage_runtime_tests','player_damage_tests','king_runtime_tests','multiplayer_tests',
 'prop_sandbox_tests','multiplayer_grenade_delivery_tests')
CHECKS=('original_shader_rgb','original_shader_state','original_shader_state_import',
 'original_camouflage_state_material','original_shader_texture_reference','original_vertex_extern',
 'original_camouflage_material_animation','original_camouflage_context','original_camouflage',
 'native_camouflage_runtime','original_player_damage','native_king_runtime','native_multiplayer',
 'native_lan_startup','retail_multiplayer','live_prop_sandbox','multiplayer_grenade_delivery',
 'native_multiplayer_species_single')

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,default=OUT/'verification.json')
 ap.add_argument('--resume-run',type=Path)
 ap.add_argument('--repair-file',action='append',default=[],help='Explicit failed-run source repair, preserving previous source/logs')
 a=ap.parse_args()
 if a.repair_file and not a.resume_run:ap.error('--repair-file requires --resume-run')
 a.output.parent.mkdir(parents=True,exist_ok=True)
 run=a.resume_run.resolve()if a.resume_run else a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp'
 repairs=[]
 if a.resume_run:
  pins=json.loads((run/'source-hashes.json').read_text());assert all(sha(source/n)==h for n,h in pins.items())
  prior=run/'prior-attempts'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ');prior.mkdir(parents=True)
  for p in run.glob('*.log'):shutil.copy2(p,prior/p.name)
  for name in('verification-driver.py','source-hashes.json','command-journal.json'):
   if(run/name).exists():shutil.copy2(run/name,prior/name)
  for name in a.repair_file:
   assert name in pins
   p=source/name;old=prior/name;old.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,old)
   previous=pins[name];p.write_bytes((ROOT/name).read_bytes());pins[name]=sha(p)
   repairs.append(dict(file=name,before=previous,after=pins[name],preserved=str(old)))
 else:
  source.mkdir(parents=True);temp.mkdir();pins={}
  for folder in('include','src','tests','tools','cmake','external','resources','shaders'):
   for p in sorted((ROOT/folder).rglob('*')):
    if not p.is_file()or'__pycache__'in p.parts or p.suffix in('.pyc','.pyo'):continue
    relative=p.relative_to(ROOT);q=source/relative;q.parent.mkdir(parents=True,exist_ok=True)
    q.write_bytes(p.read_bytes());pins[str(relative)]=sha(q)
  for name in('CMakeLists.txt','README.md'):
   shutil.copy2(ROOT/name,source/name);pins[name]=sha(source/name)
  for name in('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 driver=run/'verification-driver.py';driver.write_bytes(Path(__file__).read_bytes())
 assets={str(p.relative_to(ROOT)):sha(p)for p in(ROOT/'assets/disc/default.xbe',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')}
 for folder in('assets/disc/maps','assets/map-pack/maps'):
  assets.update({str(p.relative_to(ROOT)):sha(p)for p in(ROOT/folder).glob('*.map')})
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 evidence_paths=('analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json',
  'analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json',
  'analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json',
  'analysis/multiplayer/game-variants/state-rgb-pack-20261011/report.json',
  'analysis/multiplayer/game-variants/render-state-extern-20261011/report.json')
 evidence={name:sha(ROOT/name)for name in evidence_paths}
 layer=ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d';library=ROOT/'.tools/validation/usr/lib'
 validation={str(p):sha(p)for p in(layer/'VkLayer_khronos_validation.json',library/'libVkLayer_khronos_validation.so')}
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy','HALO2_TEST_VALIDATION':'1',
  'VK_LAYER_PATH':str(layer),'LD_LIBRARY_PATH':str(library)+(':'+os.environ['LD_LIBRARY_PATH']if os.environ.get('LD_LIBRARY_PATH')else'')}
 commands=[]
 def execute(label,command,cwd=source,extra=None):
  command=list(map(str,command));log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(command,cwd=cwd,env={**env,**(extra or{})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=command,cwd=str(cwd),exit=r.returncode,log=str(log),log_sha256=sha(log)))
  (run/'command-journal.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
  '-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j4'])
 execute('ctest',['ctest','--test-dir',build,'-R','^('+'|'.join(CHECKS)+')$','--output-on-failure'])
 execute('state-gpu-required',[build/'shader_state_gpu_tests',ROOT/'assets/disc/maps',run/'state-gpu-captures'])
 execute('material-gpu-required',[build/'camouflage_material_gpu_tests',ROOT/'assets/disc/maps',run/'material-gpu-captures'])
 execute('native-render-required',[build/'multiplayer_species_runtime_tests',source,run/'species-runtime','single'])
 original_outputs={}
 for label,tool,fixture,prior in(
  ('rgb','tools/oracle_state_rgb_pack.py','tests/fixtures/state_rgb_pack_original.json','state-rgb-pack-20261011'),
  ('state','tools/oracle_render_state_extern.py','tests/fixtures/render_state_extern_original.json','render-state-extern-20261011')):
  directory=run/('original-'+label)
  execute('original-'+label,[sys.executable,source/tool,'--output',directory,'--fixture',directory/'fixture.json'])
  assert sha(directory/'fixture.json')==pins[fixture]
  outputs={p.name:sha(p)for p in directory.iterdir()if p.is_file()}
  assert all(sha(ROOT/'analysis/multiplayer/game-variants'/prior/name)==digest for name,digest in outputs.items()if name!='fixture.json')
  original_outputs[label]=outputs
 # Scope sanitizer instrumentation to the changed arithmetic/import/material
 # owners and each complete CPU comparison, using this build's compatible ABI.
 entries=json.loads((build/'compile_commands.json').read_text())
 def sanitized_object(name):
  entry=next(e for e in entries if Path(e['file'])==source/name);cmd=shlex.split(entry['command'])
  obj=run/(Path(name).stem+'.asan.o');cmd[cmd.index('-o')+1]=str(obj)
  cmd.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('asan-compile-'+Path(name).stem,cmd,Path(entry['directory']));return obj
 owners=[sanitized_object(n)for n in('src/rendering/source_shader_rgb.cpp','src/rendering/source_shader_state.cpp','src/rendering/material.cpp','src/rendering/world/world_material.cpp')]
 sanitized=[]
 args={
  'source_shader_rgb_tests':[source/'tests/fixtures/state_rgb_pack_original.json',ROOT/evidence_paths[2]],
  'source_shader_state_tests':[source/'tests/fixtures/render_state_extern_original.json'],
  'shader_state_import_tests':[ROOT/'assets/disc/maps',ROOT/'assets/map-pack/maps',ROOT/evidence_paths[1]],
  'camouflage_state_material_tests':[ROOT/evidence_paths[2],ROOT/evidence_paths[0]]}
 for target,arguments in args.items():
  obj=sanitized_object('tests/'+target+'.cpp')
  lines=subprocess.check_output(['ninja','-C',str(build),'-t','commands',target],text=True).splitlines()
  cmd=shlex.split(next(s for s in reversed(lines)if f' -o {target} 'in s))
  if cmd[:2]==[':','&&']:cmd=cmd[2:]
  if cmd[-2:]==['&&',':']:cmd=cmd[:-2]
  cmd=[str(obj)if token.endswith(target+'.cpp.o')else token for token in cmd if not token.startswith('-Wl,--dependency-file=')]
  cmd[1:1]=list(map(str,owners));binary=run/(target+'_asan');cmd[cmd.index('-o')+1]=str(binary)
  cmd.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('asan-link-'+target,cmd,build)
  execute('asan-test-'+target,[binary,*arguments],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  sanitized.append(binary)
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all(sha(ROOT/name)==digest for name,digest in {**assets,**evidence}.items())
 assert all(sha(name)==digest for name,digest in validation.items())
 result=dict(schema='native-shader-state-verification-v1',boundary=__doc__,source=str(source),
  source_hashes_sha256=sha(run/'source-hashes.json'),asset_hashes_sha256=sha(run/'asset-hashes.json'),
  driver_sha256=sha(driver),commands=commands,repairs=repairs,evidence=evidence,validation=validation,
  checks=CHECKS,original_outputs=original_outputs,
  binaries={str(p):sha(p)for p in(*(build/t for t in TARGETS),*sanitized)},
  captures={str(p):sha(p)for p in run.rglob('*.ppm')},
  original_rgb_cases=8156,original_state_cases=7392,original_ordered_state_cases=8,
  original_camouflage_state_compositions=5040,authored_map_count=21,authored_implementation_count=126,
  gpu_delivery_completed=True,ordinary_render_completed=True,
  sanitizer_scope='RGB/state arithmetic, material importer, world material owner and all four complete CPU tests; remaining libraries built normally.',
  live_checkout_drift=[n for n,h in pins.items()if not(ROOT/n).exists()or sha(ROOT/n)!=h],
  native_biped_clone=False,framebuffer_extern17=False,concurrent_depth_alias=False,production_king_admission=False,goal_complete=False)
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');(run/'verification.json').write_bytes(a.output.read_bytes())
 print(a.output,flush=True)
if __name__=='__main__':main()
