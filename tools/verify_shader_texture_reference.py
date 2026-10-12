#!/usr/bin/env python3
"""Freeze tag-driven texture references and verify an explicit production archive overlay.

The prior complete material build supplies unchanged objects. The texture
owner, changed world material owner and optional vertex extern owner are
compiled into an explicit archive overlay, with no ABI/header layout changes.
This does not admit framebuffer extern17 or biped camouflage.
"""
import argparse,datetime,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/shader-texture-reference-native-20261011'
BASE=ROOT/'analysis/multiplayer/game-variants/camouflage-material-native-20261011/verify-20261011T225019.485728Z'
FILES=('include/halo2/source_shader_texture.hpp','src/source_shader_texture.cpp',
       'src/world_material.cpp','tests/shader_texture_reference_tests.cpp',
       'tests/camouflage_material_gpu_tests.cpp','tools/inspect_camouflage_pass_contract.py')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def link_command(build,target):
 lines=subprocess.check_output(['ninja','-C',str(build),'-t','commands',target],text=True).splitlines()
 command=shlex.split(next(s for s in reversed(lines)if f' -o {target} 'in s))
 if command[:2]==[':','&&']:command=command[2:]
 if command[-2:]==['&&',':']:command=command[:-2]
 return [t for t in command if not t.startswith('-Wl,--dependency-file=')]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path)
 ap.add_argument('--vertex-extern',action='store_true',help='Include the native19BE0 remapper, its ordinary RGB consumer and original fixture')
 a=ap.parse_args()
 vertex=a.vertex_extern or 'halo2/source_vertex_extern.hpp'in(ROOT/'src/world_material.cpp').read_text()
 if a.output is None:a.output=(ROOT/'analysis/multiplayer/game-variants/vertex-extern-native-20261011'if vertex else OUT)/'verification.json'
 run=a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';source.mkdir(parents=True);build=BASE/'build';base_source=BASE/'frozen-source'
 pins=json.loads((BASE/'source-hashes.json').read_text());assets=json.loads((BASE/'asset-hashes.json').read_text())
 assert all(sha(base_source/n)==h for n,h in pins.items())
 assert all(sha(ROOT/n)==h for n,h in assets.items())
 # A declared union preserves the complete prior source snapshot. Only listed
 # files and the two exact live CMake additions below replace prior content.
 for folder in ('include','src','tests','tools'):
  for p in (base_source/folder).rglob('*'):
   if not p.is_file():continue
   relative=p.relative_to(base_source);q=source/relative;q.parent.mkdir(parents=True,exist_ok=True);q.symlink_to(p)
 for folder in ('cmake','external','resources','shaders','assets','analysis'):
  (source/folder).symlink_to(base_source/folder,target_is_directory=True)
 overlay_pins={}
 selected_files=FILES+(('include/halo2/source_vertex_extern.hpp','src/source_vertex_extern.cpp',
                      'tests/source_vertex_extern_tests.cpp','tests/fixtures/camouflage_vertex_extern_original.json',
                      'tools/oracle_camouflage_vertex_extern.py')if vertex else())
 for name in selected_files:
  q=source/name
  if q.is_symlink():q.unlink()
  q.parent.mkdir(parents=True,exist_ok=True);q.write_bytes((ROOT/name).read_bytes());overlay_pins[name]=sha(q)
 current=(ROOT/'CMakeLists.txt').read_text();cmake=(base_source/'CMakeLists.txt').read_text()
 added_source='target_sources(halo2_cache PRIVATE src/source_shader_texture.cpp)\n'
 if vertex:added_source+='target_sources(halo2_cache PRIVATE src/source_vertex_extern.cpp)\n'
 start=current.index('    add_executable(shader_texture_reference_tests ')
 end=current.index('assets.json)',start)+len('assets.json)\n')
 added_test=current[start:end]
 if vertex:
  start=current.index('    add_executable(source_vertex_extern_tests ')
  end=current.index('camouflage-pass-20261011/report.json)',start)+len('camouflage-pass-20261011/report.json)\n')
  added_test+=current[start:end]
 assert added_source in current and added_source not in cmake and added_test not in cmake
 anchor='target_sources(halo2_cache PRIVATE src/world_material.cpp src/bsp_atmosphere.cpp src/bsp_patchy_fog.cpp src/water.cpp src/scenery_lighting.cpp src/object_lighting.cpp)\n'
 cmake=cmake.replace(anchor,anchor+added_source,1)
 anchor='        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_material_animation_original.json)\n'
 assert anchor in cmake;cmake=cmake.replace(anchor,anchor+added_test,1)
 (source/'CMakeLists.txt').write_text(cmake);overlay_pins['CMakeLists.txt']=sha(source/'CMakeLists.txt')
 (run/'live-CMakeLists.txt').write_text(current)
 (run/'source-hashes.json').write_text(json.dumps(overlay_pins,sort_keys=True,indent=2)+'\n')
 driver=run/'verification-driver.py';driver.write_bytes(Path(__file__).read_bytes())
 layer=ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d';library=ROOT/'.tools/validation/usr/lib'
 validation={str(p):sha(p)for p in(layer/'VkLayer_khronos_validation.json',library/'libVkLayer_khronos_validation.so')}
 env={**os.environ,'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy','HALO2_TEST_VALIDATION':'1',
      'VK_LAYER_PATH':str(layer),'LD_LIBRARY_PATH':str(library)+(':'+os.environ['LD_LIBRARY_PATH']if os.environ.get('LD_LIBRARY_PATH')else'')}
 commands=[]
 def execute(label,command,cwd=build,extra=None):
  command=list(map(str,command));log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(command,cwd=cwd,env={**env,**(extra or{})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=command,cwd=str(cwd),exit=r.returncode,log=str(log),log_sha256=sha(log)))
  (run/'command-journal.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n')
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 # Configure the union to check the exact additions and obtain compiler flags.
 config=run/'configured-build'
 execute('configure',['cmake','-S',source,'-B',config,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
                     '-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
 entries=json.loads((config/'compile_commands.json').read_text())
 def compile_owner(name,sanitized=False):
  entry=next(e for e in entries if Path(e['file'])==source/name)
  command=shlex.split(entry['command']);obj=run/(Path(name).name+('.asan.o'if sanitized else'.o'))
  command[command.index('-o')+1]=str(obj)
  if sanitized:command.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute(('asan-'if sanitized else'compile-')+Path(name).stem,command,Path(entry['directory']))
  return obj
 world=compile_owner('src/world_material.cpp');texture=compile_owner('src/source_shader_texture.cpp')
 test=compile_owner('tests/shader_texture_reference_tests.cpp');gpu=compile_owner('tests/camouflage_material_gpu_tests.cpp')
 vertex_owner=compile_owner('src/source_vertex_extern.cpp')if vertex else None
 vertex_test=compile_owner('tests/source_vertex_extern_tests.cpp')if vertex else None
 archive=run/'libhalo2_cache.a';shutil.copy2(build/'libhalo2_cache.a',archive)
 # Object member names deliberately match the original CMake archive names.
 execute('archive-overlay',['ar','r',archive,world,texture]+([vertex_owner]if vertex else[]))
 def link(target,test_object=None,sanitized=None):
  old='camouflage_material_tests'if target in('shader_texture_reference_tests','source_vertex_extern_tests')else target
  command=link_command(build,old)
  command=[str(test_object)if test_object and t.endswith(old+'.cpp.o')else str(archive)if t=='libhalo2_cache.a'else t for t in command]
  if sanitized:command[1:1]=list(map(str,sanitized));command.extend(['-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  binary=run/(target+('_asan'if sanitized else''));command[command.index('-o')+1]=str(binary)
  execute('link-'+binary.name,command);return binary
 reference=link('shader_texture_reference_tests',test)
 material=link('camouflage_material_tests')
 gpu_binary=link('camouflage_material_gpu_tests',gpu)
 species=link('multiplayer_species_runtime_tests')
 vertex_binary=link('source_vertex_extern_tests',vertex_test)if vertex else None
 report=ROOT/'analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json'
 census=ROOT/'analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json'
 evidence={str(p):sha(p)for p in(report,census,report.with_name('reproduction.json'))}
 original_dependencies=json.loads(report.read_text())['dependencies']
 if vertex:
  vertex_provenance=json.loads((source/'tests/fixtures/camouflage_vertex_extern_original.json').read_text())['provenance']
  for name,digest in vertex_provenance['dependencies'].items():
   if name in original_dependencies:assert original_dependencies[name]==digest
   original_dependencies[name]=digest
 assert all(sha(ROOT/n)==h for n,h in original_dependencies.items())
 args=[ROOT/'assets/disc/maps',ROOT/'assets/map-pack/maps',report,census]
 execute('reference-test',[reference,*args])
 vertex_args=[source/'tests/fixtures/camouflage_vertex_extern_original.json',report]
 if vertex:execute('vertex-extern-test',[vertex_binary,*vertex_args])
 execute('material-test',[material,source/'tests/fixtures/camouflage_material_animation_original.json'])
 execute('gpu-required',[gpu_binary,ROOT/'assets/disc/maps',run/'gpu-captures'])
 execute('native-render-required',[species,source,run/'species-runtime','single'])
 # Re-run the actual unchanged original dispatch/prefix corpus, including its
 # deliberately cold framebuffer cases; do not supply successful cache calls.
 original=run/'original'
 # This inspector records resolved paths relative to its real repository root.
 # Execute that original path and verify every declared dependency unchanged
 # before and after, rather than changing its provenance logic in the union.
 execute('original-pass',[sys.executable,ROOT/'tools/inspect_camouflage_pass_contract.py','--output',original],ROOT)
 assert sha(original/'report.json')==sha(report),'Original pass report failed to reproduce'
 original_hashes={str(p.relative_to(original)):sha(p)for p in original.iterdir()if p.is_file()}
 assert all(sha(report.parent/n)==h for n,h in original_hashes.items())
 vertex_original_hashes={}
 if vertex:
  vertex_original=run/'original-vertex'
  execute('original-vertex',[sys.executable,ROOT/'tools/oracle_camouflage_vertex_extern.py',
          '--output',vertex_original,'--fixture',vertex_original/'fixture.json'],ROOT)
  assert sha(vertex_original/'fixture.json')==overlay_pins['tests/fixtures/camouflage_vertex_extern_original.json']
  prior=ROOT/'analysis/multiplayer/game-variants/camouflage-binder-20261011'
  vertex_original_hashes={p.name:sha(p)for p in vertex_original.iterdir()if p.is_file()}
  assert all(sha(prior/name)==digest for name,digest in vertex_original_hashes.items()if name!='fixture.json')
 asan_world=compile_owner('src/world_material.cpp',True)
 asan_texture=compile_owner('src/source_shader_texture.cpp',True)
 asan_test=compile_owner('tests/shader_texture_reference_tests.cpp',True)
 asan_vertex=compile_owner('src/source_vertex_extern.cpp',True)if vertex else None
 asan_owners=[asan_world,asan_texture]+([asan_vertex]if vertex else[])
 sanitized=link('shader_texture_reference_tests',asan_test,asan_owners)
 execute('sanitizer-test',[sanitized,*args],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 if vertex:
  asan_vertex_test=compile_owner('tests/source_vertex_extern_tests.cpp',True)
  sanitized_vertex=link('source_vertex_extern_tests',asan_vertex_test,[asan_vertex])
  execute('sanitizer-vertex-test',[sanitized_vertex,*vertex_args],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(base_source/n)==h for n,h in pins.items())
 assert all(sha(source/n)==h for n,h in overlay_pins.items())
 assert all(sha(ROOT/n)==h for n,h in assets.items())
 assert all(sha(p)==h for p,h in {**evidence,**validation}.items())
 assert all(sha(ROOT/n)==h for n,h in original_dependencies.items())
 result=dict(schema='native-vertex-extern-verification-v1'if vertex else'native-shader-texture-reference-verification-v1',boundary=__doc__,
  base=str(BASE),base_manifest_sha256=sha(BASE.parent/'verification.json'),
  base_source_hashes_sha256=sha(BASE/'source-hashes.json'),base_asset_hashes_sha256=sha(BASE/'asset-hashes.json'),
  unchanged_base_cache_sha256=sha(build/'libhalo2_cache.a'),source=str(source),overlay_hashes=overlay_pins,
  driver_sha256=sha(driver),commands=commands,evidence=evidence,validation=validation,
  original_outputs=original_hashes,original_vertex_outputs=vertex_original_hashes,
  original_dependencies=original_dependencies,original_texture_prefixes=24,owned_maps=21,
  material_cases=1280,gpu_completed=True,ordinary_native_render_completed=True,
  vertex_extern=vertex,whole_vertex_fixture_cases=len(json.loads(vertex_args[0].read_text())['cases'])if vertex else 0,
  archive_sha256=sha(archive),binaries={str(p):sha(p)for p in(reference,material,gpu_binary,species,sanitized,*((vertex_binary,sanitized_vertex)if vertex else()))},
  captures={str(p):sha(p)for p in run.rglob('*.ppm')},
  sanitizer_scope='New texture/optional vertex extern owners, changed world material owner and complete reference tests; remaining prior libraries built normally.',
  live_overlay_drift=[n for n,h in overlay_pins.items()if n!='CMakeLists.txt'and sha(ROOT/n)!=h],
  native_biped_clone=False,framebuffer_extern17=False,production_king_admission=False,goal_complete=False)
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
