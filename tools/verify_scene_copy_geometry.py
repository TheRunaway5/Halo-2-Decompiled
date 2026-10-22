#!/usr/bin/env python3
"""Freeze and verify original scene-copy CPU geometry on the verified fog base.

Only the new geometry owner, tests, source oracles and their explicit dependencies
are overlaid. No renderer/shader/water edit is included. Complete original CPU
calls and bit comparisons do not prove GPU commands, sampling, framebuffer
output, normal camouflage scheduling or additional game-mode admission.
"""
import datetime,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/shader-fog-native-20261012/verify-20261012T002720.611133Z'
OUT=ROOT/'analysis/multiplayer/game-variants/scene-copy-geometry-native-20261012'
FILES=('include/halo2/source_scene_copy.hpp','src/rendering/source_scene_copy.cpp','tests/source_scene_copy_tests.cpp',
 'tests/fixtures/scene_copy_geometry_original.json','tools/oracle_camouflage_scene_copy.py',
 'tools/oracle_scene_copy_rectangles.py','tools/oracle_camouflage_pass_services.py',
 'tools/differential_source_object_admission_snapshot.py','tools/verify_scene_copy_geometry.py')
TARGETS=('source_scene_copy_tests','source_shader_fog_tests','camouflage_pass_bank_tests','water_tests')
CHECKS=('original_scene_copy_geometry','original_shader_fog','original_camouflage_pass_banks','retail_multiplayer_water')
SOURCE_BLOCK='''target_sources(halo2_cache PRIVATE src/rendering/source_scene_copy.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_scene_copy.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_scene_copy.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
'''
TEST_BLOCK='''    add_executable(source_scene_copy_tests tests/source_scene_copy_tests.cpp)
    target_link_libraries(source_scene_copy_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_scene_copy_geometry COMMAND source_scene_copy_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_geometry_original.json)
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
 source_anchor='target_sources(halo2_cache PRIVATE src/rendering/source_shader_fog.cpp)\n'
 test_anchor='''    add_test(NAME original_shader_fog COMMAND source_shader_fog_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fog_externs_original.json)
'''
 assert cmake.count(source_anchor)==1 and cmake.count(test_anchor)==1
 cmake=cmake.replace(source_anchor,source_anchor+SOURCE_BLOCK).replace(test_anchor,test_anchor+TEST_BLOCK)
 (source/'CMakeLists.txt').write_text(cmake)
 pins={name:sha(source/name)for name in sorted(set(base_pins)|set(FILES))}
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 evidence_names=('analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json',
  'analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json')
 evidence={name:sha(ROOT/name)for name in evidence_names}
 for name in evidence_names:
  report=json.loads((ROOT/name).read_text())
  assert all(pins[path]==digest for path,digest in report['dependencies'].items()if path.startswith('tools/'))
 assets=base_result['assets'].copy()
 snapshot=Path('analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry')
 for name in (snapshot/'export.json',snapshot/'exports/physical-ram.bin'):
  assets[str(name)]=sha(ROOT/name)
 assert assets[str(snapshot/'exports/physical-ram.bin')]==report['snapshot_ram_sha256']
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
 original_outputs={}
 for label,tool,reference,fixture in (
  ('original-captured','oracle_camouflage_scene_copy.py','camouflage-scene-copy-20261011',False),
  ('original-rectangles','oracle_scene_copy_rectangles.py','scene-copy-rectangles-20261011',True)):
  destination=run/label
  arguments=[sys.executable,source/'tools'/tool,'--output',destination]
  if fixture:arguments.extend(['--fixture',destination/'fixture.json'])
  execute(label,arguments)
  outputs={p.name:sha(p)for p in destination.iterdir()if p.is_file()}
  for name,digest in outputs.items():
   expected=source/'tests/fixtures/scene_copy_geometry_original.json'if name=='fixture.json'else ROOT/'analysis/multiplayer/game-variants'/reference/name
   assert sha(expected)==digest,(label,name,'original output differs')
  original_outputs[label]=outputs
 entries=json.loads((build/'compile_commands.json').read_text())
 def sanitized(name):
  entry=next(e for e in entries if Path(e['file'])==source/name);command=shlex.split(entry['command'])
  obj=run/(Path(name).stem+'.asan.o');command[command.index('-o')+1]=str(obj)
  command.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('asan-compile-'+Path(name).stem,command,Path(entry['directory']));return obj
 owners=[sanitized(name)for name in('src/rendering/source_scene_copy.cpp','src/rendering/vertex_program.cpp','tests/source_scene_copy_tests.cpp')]
 binary=run/'source_scene_copy_tests_asan'
 execute('asan-link',['c++','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',*owners,
  build/'libhalo2_cache.a','-lz','-lglm',build/'libhalo2_native_synchronization.a','-o',binary])
 execute('asan-test',[binary,ROOT/evidence_names[0],source/'tests/fixtures/scene_copy_geometry_original.json'],
  extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all(sha(ROOT/name)==digest for name,digest in {**assets,**evidence}.items())
 assert all(sha(BASE/'frozen-source'/name)==digest for name,digest in base_pins.items())
 result=dict(schema='native-scene-copy-geometry-verification-v1',boundary=__doc__,source=str(source),
  base=str(BASE),base_manifest_sha256=sha(BASE/'verification.json'),source_hashes_sha256=sha(run/'source-hashes.json'),
  overlaid_files=FILES,cmake_overlay='Only recorded source/test declarations; unrelated live CMake drift excluded',
  commands=commands,assets=assets,evidence=evidence,original_outputs=original_outputs,
  binaries={str(p):sha(p)for p in(*(build/t for t in TARGETS),binary)},
  sanitizer_scope='New scene-copy owner, native vertex-program decoder/evaluator and complete scene-copy comparison; remaining libraries built normally',
  complete_original_calls=35,emitted_attributes_compared=840,program_words_per_call=60,
  captured_resource_dimensions={'source':[640,480],'destination':[320,240]},
  depth_range_bits={'near':0x3c800000,'far':0x44800000},
  gpu_submission=False,normal_pass_submission=False,scene_copy_framebuffer=False,water_renderer_change=False,
  concurrent_depth_alias=False,additional_variant_admission=False,goal_complete=False,
  live_checkout_drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest])
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
 (OUT/'verification.json').write_bytes((run/'verification.json').read_bytes())
 print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
