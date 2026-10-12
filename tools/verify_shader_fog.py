#!/usr/bin/env python3
"""Freeze fog-provider/pass-bank changes on the verified compatible shader-state source.

Only the listed owned files and CMake declarations are overlaid. Unrelated
concurrent checkout changes are not incorporated. Fresh CMake cache targets,
original reproduction and scoped sanitizers prove CPU bank preparation, not
normal camouflage scheduling, view/default production or framebuffer output.
"""
import datetime,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/shader-state-native-20261011/verify-20261011T235858.006932Z'
OUT=ROOT/'analysis/multiplayer/game-variants/shader-fog-native-20261012'
FILES=('include/halo2/source_shader_fog.hpp','include/halo2/world_material.hpp','src/source_shader_fog.cpp',
 'tests/source_shader_fog_tests.cpp','tests/camouflage_pass_bank_tests.cpp','tests/fixtures/fog_externs_original.json',
 'tools/oracle_fog_externs.py','tools/inspect_render_primitives_title_updates.py','tools/verify_shader_fog.py')
TARGETS=('source_shader_fog_tests','camouflage_pass_bank_tests','source_shader_rgb_tests','source_shader_state_tests',
 'source_vertex_extern_tests','shader_state_import_tests','camouflage_state_material_tests','camouflage_material_tests',
 'shader_texture_reference_tests')
CHECKS=('original_shader_fog','original_camouflage_pass_banks','original_shader_rgb','original_shader_state',
 'original_vertex_extern','original_shader_state_import','original_camouflage_state_material',
 'original_camouflage_material_animation','original_shader_texture_reference')
SOURCE_BLOCK='''target_sources(halo2_cache PRIVATE src/source_shader_fog.cpp)
if(MSVC)
    set_source_files_properties(src/source_shader_fog.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/source_shader_fog.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
'''
TEST_BLOCK='''    add_executable(camouflage_pass_bank_tests tests/camouflage_pass_bank_tests.cpp src/source_camouflage_context.cpp)
    target_link_libraries(camouflage_pass_bank_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_camouflage_pass_banks COMMAND camouflage_pass_bank_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-pass-services-20261011/report.json)
    add_executable(source_shader_fog_tests tests/source_shader_fog_tests.cpp)
    target_link_libraries(source_shader_fog_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_shader_fog COMMAND source_shader_fog_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fog_externs_original.json)
'''

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 run=OUT/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp'
 run.mkdir(parents=True);temp.mkdir()
 base_pins=json.loads((BASE/'source-hashes.json').read_text())
 assert all(sha(BASE/'frozen-source'/name)==digest for name,digest in base_pins.items())
 base_result=json.loads((BASE/'verification.json').read_text())
 assert all(row['exit']==0 for row in base_result['commands'])
 shutil.copytree(BASE/'frozen-source',source,symlinks=True)
 for name in FILES:
  destination=source/name;destination.parent.mkdir(parents=True,exist_ok=True)
  shutil.copy2(ROOT/name,destination)
 cmake=(source/'CMakeLists.txt').read_text();live=(ROOT/'CMakeLists.txt').read_text()
 assert SOURCE_BLOCK in live and TEST_BLOCK in live
 source_anchor='target_sources(halo2_cache PRIVATE src/source_shader_state.cpp)\n'
 test_anchor='''    add_test(NAME original_shader_state COMMAND source_shader_state_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/render_state_extern_original.json)
'''
 assert cmake.count(source_anchor)==1 and cmake.count(test_anchor)==1
 cmake=cmake.replace(source_anchor,source_anchor+SOURCE_BLOCK).replace(test_anchor,test_anchor+TEST_BLOCK)
 (source/'CMakeLists.txt').write_text(cmake)
 pins={name:sha(source/name)for name in sorted(set(base_pins)|set(FILES))}
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 evidence_names=('analysis/multiplayer/game-variants/camouflage-pass-services-20261011/report.json',
  'analysis/multiplayer/game-variants/fog-externs-20261011/report.json',
  'analysis/multiplayer/game-variants/render-primitives-title-updates-20261011/report.json')
 evidence={name:sha(ROOT/name)for name in evidence_names}
 assets=json.loads((BASE/'asset-hashes.json').read_text())
 from_oracle=json.loads((ROOT/evidence_names[1]).read_text())
 # Reproduce against the exact source dependencies admitted by the oracle.
 for name,digest in from_oracle.get('dependencies',{}).items():
  assert pins[name]==digest
 sys.path.insert(0,str(source/'tools'))
 from oracle_game_over import SPECS
 for version,spec in SPECS.items():
  path=Path(spec['path']);assert sha(ROOT/path)==from_oracle['versions'][version]['image_sha256']
  assets[str(path)]=sha(ROOT/path)
 commands=[];env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy'}
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
 original=run/'original'
 execute('original',[sys.executable,source/'tools/oracle_fog_externs.py','--output',original,'--fixture',original/'fixture.json'])
 outputs={p.name:sha(p)for p in original.iterdir()if p.is_file()}
 assert outputs['fixture.json']==pins['tests/fixtures/fog_externs_original.json']
 assert all(sha(ROOT/'analysis/multiplayer/game-variants/fog-externs-20261011'/name)==digest
  for name,digest in outputs.items()if name!='fixture.json')
 entries=json.loads((build/'compile_commands.json').read_text())
 def sanitized(name):
  entry=next(e for e in entries if Path(e['file'])==source/name);command=shlex.split(entry['command'])
  obj=run/(Path(name).stem+'.asan.o');command[command.index('-o')+1]=str(obj)
  command.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('asan-compile-'+Path(name).stem,command,Path(entry['directory']));return obj
 owners=[sanitized(name)for name in('src/source_shader_fog.cpp','src/source_camouflage_context.cpp',
  'src/source_vertex_extern.cpp','src/world_material.cpp')]
 binaries=[]
 arguments={
  'source_shader_fog_tests':[source/'tests/fixtures/fog_externs_original.json'],
  'camouflage_pass_bank_tests':[ROOT/'assets/disc/maps/lockout.map',ROOT/evidence_names[0]]}
 for target,args in arguments.items():
  obj=sanitized('tests/'+target+'.cpp');binary=run/(target+'_asan')
  execute('asan-link-'+target,['c++','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',obj,*owners,
   build/'libhalo2_cache.a','-lz','-lglm',build/'libhalo2_native_synchronization.a','-o',binary])
  execute('asan-test-'+target,[binary,*args],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
  binaries.append(binary)
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all(sha(ROOT/name)==digest for name,digest in {**assets,**evidence}.items())
 assert all(sha(BASE/'frozen-source'/name)==digest for name,digest in base_pins.items())
 result=dict(schema='native-shader-fog-verification-v1',boundary=__doc__,source=str(source),
  base=str(BASE),base_manifest_sha256=sha(BASE/'verification.json'),source_hashes_sha256=sha(run/'source-hashes.json'),
  overlaid_files=FILES,cmake_overlay='Only recorded source/test declarations; unrelated live CMake drift excluded',
  commands=commands,assets=assets,evidence=evidence,original_outputs=outputs,
  binaries={str(p):sha(p)for p in(*(build/t for t in TARGETS),*binaries)},
  sanitizer_scope='Fog provider/binder, first-biped context, vertex extern owner, world material and both complete tests; remaining libraries built normally',
  original_provider_contexts=768,original_provider_calls=4608,complete_original_pass_banks=64,
  normal_pass_submission=False,scene_copy=False,concurrent_depth_alias=False,additional_variant_admission=False,goal_complete=False,
  live_checkout_drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest])
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 (OUT/'verification.json').write_bytes((run/'verification.json').read_bytes())
 print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
