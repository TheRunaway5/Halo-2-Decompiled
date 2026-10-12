#!/usr/bin/env python3
"""Freeze the settings-selected hill material and immediate RGBA11 binding.

All linked libraries are rebuilt because the native Vertex layout gains alpha.
Checks retain all12 actual disc materials and compare1452 original emitted
vertices plus1452 uniform-color controls through production CPU/GLSL inputs.
This is host shader evidence, not an original pixel or whole preparation proof.
"""
import argparse,concurrent.futures,datetime,hashlib,json,os,re,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/king-boundary-material-native-20261012'
OWNED=('include/halo2/cache.hpp','src/material.cpp','include/halo2/source_vertex_inputs.hpp',
 'src/source_vertex_declaration.cpp','include/halo2/world_material.hpp','src/world_material.cpp',
 'shaders/world_vertex.glsl','shaders/vertex_program.glsl','shaders/source_vertex_declaration.comp',
 'src/renderer.cpp','tests/king_boundary_material_tests.cpp','tools/verify_king_boundary_material_native.py',
 'include/halo2/source_transparent_draw.hpp','include/halo2/source_transparent_queue.hpp','src/source_transparent_queue.cpp',
 'include/halo2/source_king_draw.hpp','src/source_king_draw.cpp','tests/king_boundary_renderer_tests.cpp','tests/king_queue_submission_tests.cpp')
TARGETS=('halo2_renderer','king_boundary_material_tests','source_vertex_declaration_tests',
 'king_boundary_tests','king_visual_tests','king_runtime_tests','world_material_tests',
 'renderer_material_residency_tests','king_boundary_renderer_tests','king_queue_submission_tests','source_transparent_queue_tests')
REPORTS=('king-visual-consumers-source-20261012/complete/report.json',
 'king-hill-emission-source-20261012/complete/report.json','king-boundary-native-20261012/verification.json',
 'king-queue-submission-source-20261012/complete/report.json','king-hill-preparation-source-20261012/complete/report.json')
PINS=('5921a37a3dc5c5e4af9539bf770f3455885222e1d08505c78473e174b887288a',
 '96cae012aec928d6f9e23cb756e11bfcfe189e062ec70cb54e0754b0adcddcb8',
 '2fc56b9f37fca6a4f9c1270373c3a3f5efc0f151e454b608335c059f51b9a545',
 'b353e634302890aa55a85ecff59a2f3000c22b7e92e4fa6a5ab0ebeb709abab4',
 '35c002396b71e5b7e001f1c55cdb43ca0f2b23d2e037cbe5d1478add7ba2c881')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--resume-run',type=Path);args=ap.parse_args()
 run=args.resume_run.resolve()if args.resume_run else OUT/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source'
 if args.resume_run:
  hashes=json.loads((run/'source-hashes.json').read_text());assert all(sha(source/p)==pin for p,pin in hashes.items())
 else:
  run.mkdir(parents=True);source.mkdir()
  for folder in('include','src','tests','tools','cmake','external','resources','shaders','docs'):
   for p in sorted((ROOT/folder).rglob('*')):
    if not p.is_file()or '__pycache__'in p.parts or p.suffix in('.pyc','.pyo'):continue
    dest=source/p.relative_to(ROOT);dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
  for name in('CMakeLists.txt','README.md'):shutil.copy2(ROOT/name,source/name)
  for name in('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
  files=[p for p in source.rglob('*')if p.is_file()and not p.is_symlink()]
  with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:
   hashes=dict(zip((str(p.relative_to(source))for p in files),pool.map(sha,files)))
  (run/'source-hashes.json').write_text(json.dumps(hashes,sort_keys=True,indent=2)+'\n')
 inputs={str(ROOT/'analysis/multiplayer/game-variants'/p):pin for p,pin in zip(REPORTS,PINS)}
 visual=json.loads((ROOT/'analysis/multiplayer/game-variants'/REPORTS[0]).read_text())
 for row in visual['cache_inventory']:
  p=ROOT/row['path'];inputs[str(p)]=sha(p)
 for p in(ROOT/'assets/disc/maps/shared.map',ROOT/'assets/disc/maps/mainmenu.map',ROOT/'assets/disc/default.xbe'):
  inputs[str(p)]=sha(p)
 assert all(sha(p)==pin for p,pin in inputs.items())
 if args.resume_run:assert json.loads((run/'input-hashes.json').read_text())==inputs
 else:(run/'input-hashes.json').write_text(json.dumps(inputs,sort_keys=True,indent=2)+'\n')
 scratch=run/'compiler-tmp';scratch.mkdir(exist_ok=True)
 env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy',
  'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation','VK_LAYER_VALIDATE_SYNC':'1','TMPDIR':str(scratch)}
 commands=json.loads((run/'commands.json').read_text())if args.resume_run else []
 def execute(label,argv,cwd=source,extra=None):
  argv=list(map(str,argv));previous=next((c for c in commands if c['label']==label and c['exit']==0),None)
  if previous:
   assert previous['command']==argv and sha(OUT/previous['log'])==previous['log_sha256'];return
  attempts=sum(c['label']==label for c in commands);log=run/(label+('-retry'+str(attempts)if attempts else '')+'.log')
  with log.open('w')as out:r=subprocess.run(argv,cwd=cwd,env={**env,**(extra or {})},stdout=out,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=argv,cwd=str(cwd),exit=r.returncode,
   log=str(log.relative_to(OUT)),log_sha256=sha(log)))
  (run/'commands.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
  print(label,'exit',r.returncode,flush=True);assert r.returncode==0,log
  assert not re.search(r'Validation Error:|SYNC-HAZARD|ERROR: AddressSanitizer|runtime error:',log.read_text()),log
 build=run/'build'
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
  '-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 execute('build',['cmake','--build',build,'--target',*TARGETS,'-j4'])
 expression='^(native_king_boundary_material_(cpu|gpu)|source_world_vertex_declaration_(cpu|gpu)|original_king_(boundary|visual|queue_submission)|native_king_runtime|retail_multiplayer_materials|original_source_transparent_queue)$'
 execute('ctest',['ctest','--test-dir',build,'--verbose','-R',expression])
 assert re.search(r'100% tests passed.*out of 10',(run/'ctest.log').read_text())
 execute('renderer-checks',['xvfb-run','-a','ctest','--test-dir',build,'--verbose','-R','^native_(renderer_material_residency|king_boundary_renderer)$'],extra={'SDL_VIDEODRIVER':'x11'})
 assert re.search(r'100% tests passed.*out of 2',(run/'renderer-checks.log').read_text())
 sanitized=run/'king_queue_submission_tests_asan'
 execute('sanitizer-build',['/usr/bin/c++','-std=c++20','-O1','-g','-Wall','-Wextra','-Werror',
  '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-fast-math','-ffp-contract=off','-I'+str(source/'include'),
  source/'tests/king_queue_submission_tests.cpp',source/'src/source_transparent_queue.cpp',source/'src/source_transparent_sort.cpp','-o',sanitized])
 execute('sanitizer-test',[sanitized,source/'analysis/multiplayer/game-variants'/REPORTS[3]],
  extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(p)==pin for p,pin in inputs.items())
 with concurrent.futures.ThreadPoolExecutor(max_workers=8)as pool:current=list(pool.map(lambda p:sha(source/p),hashes))
 assert all(hashes[p]==v for p,v in zip(hashes,current))
 drift=[p for p in hashes if(ROOT/p).is_file()and sha(ROOT/p)!=hashes[p]]
 assert not((set(OWNED)-{'tools/verify_king_boundary_material_native.py'})&set(drift)),drift
 result=dict(schema='king-boundary-material-native-v1',scope=__doc__,source=str(source),
  source_hashes_sha256=sha(run/'source-hashes.json'),input_hashes_sha256=sha(run/'input-hashes.json'),
  commands=commands,live_checkout_drift=drift,orchestrator_sha256=sha(__file__),compiler_temporary_directory=str(scratch),
  binaries={**{str((build/t).relative_to(OUT)):sha(build/t)for t in TARGETS if(build/t).is_file()},str(sanitized.relative_to(OUT)):sha(sanitized)},
  disc_materials=12,original_emitted_vertices=1452,uniform_color_controls=1452,
  full_vertex_abi_rebuild=True,original_whole_positive_preparation=False,
  declared_hill_world_submission=True,whole_original_queue_calls=135,original_pixels=False,normal_king_admission=False,goal_complete=False)
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 shutil.copy2(run/'verification.json',OUT/'verification.json');print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
