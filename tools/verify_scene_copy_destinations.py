#!/usr/bin/env python3
"""Freeze genuine destination CPU comparisons and bounded host water-copy delivery.

Only destination tests/oracle, water helper and the exact renderer tap hunk are
overlaid on the verified scene-copy base. Production fullscreen shader execution
uses declared host images and existing Vulkan centers. It does not establish
original GPU sampling, normal water/camouflage scheduling or gamemode admission.
"""
import argparse,datetime,hashlib,json,os,re,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/game-variants/scene-copy-geometry-native-20261012/verify-20261012T004707.124351Z'
OUT=ROOT/'analysis/multiplayer/game-variants/scene-copy-destinations-native-20261012'
FILES=('include/halo2/water.hpp','src/water.cpp','tests/scene_copy_destination_tests.cpp',
 'tests/water_copy_gpu_tests.cpp','tests/fixtures/scene_copy_destinations_original.json',
 'tools/oracle_scene_copy_destinations.py','tools/verify_scene_copy_destinations.py')
TARGETS=('scene_copy_destination_tests','source_scene_copy_tests','water_tests','water_copy_gpu_tests','halo2_renderer')
CHECKS=('original_scene_copy_destinations','original_scene_copy_geometry','retail_multiplayer_water','original_water_copy_host_gpu')
TEST_BLOCK='''    add_executable(scene_copy_destination_tests tests/scene_copy_destination_tests.cpp)
    target_link_libraries(scene_copy_destination_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_scene_copy_destinations COMMAND scene_copy_destination_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_destinations_original.json)
    add_executable(water_copy_gpu_tests tests/water_copy_gpu_tests.cpp)
    target_link_libraries(water_copy_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan nlohmann_json::nlohmann_json)
    add_dependencies(water_copy_gpu_tests halo2_shaders)
    add_test(NAME original_water_copy_host_gpu COMMAND water_copy_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_destinations_original.json
        ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/water-copy-host-framebuffer)
'''
OLD_TAPS='''                const ShaderVector taps{-.25f/extent.width,-.25f/extent.height,.25f/extent.width,.25f/extent.height};
'''
NEW_TAPS='''                const auto source_taps=water_copy_taps(rectangle[2],rectangle[3]);
                const ShaderVector taps{source_taps[0][0],source_taps[0][1],source_taps[1][0],source_taps[2][1]};
'''
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--resume',type=Path);args=ap.parse_args()
 run=args.resume.resolve() if args.resume else OUT/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp'
 if not args.resume:run.mkdir(parents=True);temp.mkdir()
 base_pins=json.loads((BASE/'source-hashes.json').read_text())
 assert all(sha(BASE/'frozen-source'/name)==digest for name,digest in base_pins.items())
 base_result=json.loads((BASE/'verification.json').read_text());assert all(row['exit']==0 for row in base_result['commands'])
 if not args.resume:
  shutil.copytree(BASE/'frozen-source',source,symlinks=True)
  for name in FILES:
   destination=source/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,destination)
  cmake=(source/'CMakeLists.txt').read_text();live=(ROOT/'CMakeLists.txt').read_text();assert TEST_BLOCK in live
  anchor='''        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_geometry_original.json)
'''
  assert cmake.count(anchor)==1;cmake=cmake.replace(anchor,anchor+TEST_BLOCK);(source/'CMakeLists.txt').write_text(cmake)
  renderer=(source/'src/renderer.cpp').read_text();assert renderer.count(OLD_TAPS)==1 and NEW_TAPS in (ROOT/'src/renderer.cpp').read_text()
  (source/'src/renderer.cpp').write_text(renderer.replace(OLD_TAPS,NEW_TAPS))
  pins={name:sha(source/name)for name in sorted(set(base_pins)|set(FILES))}
  (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 else:
  pins=json.loads((run/'source-hashes.json').read_text());assert all(sha(source/n)==h for n,h in pins.items())
  assert all(sha(ROOT/n)==pins[n]for n in FILES if n!='tools/verify_scene_copy_destinations.py')
  shutil.copy2(Path(__file__).resolve(),run/'resume-driver.py')
 evidence_names=('analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json',
  'analysis/multiplayer/game-variants/scene-copy-rectangles-20261011/report.json',
  'analysis/multiplayer/game-variants/scene-copy-destinations-20261012/report.json')
 evidence={name:sha(ROOT/name)for name in evidence_names};report=json.loads((ROOT/evidence_names[2]).read_text())
 assert all(pins[name]==digest for name,digest in report['dependencies'].items())
 assets=base_result['assets'].copy()
 validation={str(path.relative_to(ROOT)):sha(path)for path in(
  ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json',
  ROOT/'.tools/validation/usr/lib/libVkLayer_khronos_validation.so')}
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1','SDL_AUDIODRIVER':'dummy',
  'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation',
  'VK_LAYER_PATH':str(ROOT/'.tools/validation/usr/share/vulkan/explicit_layer.d'),
  'LD_LIBRARY_PATH':str(ROOT/'.tools/validation/usr/lib')+':'+os.environ.get('LD_LIBRARY_PATH','')}
 commands=json.loads((run/'command-journal.json').read_text()) if args.resume else []
 if args.resume:
  assert [q['label']for q in commands]==['configure','build','ctest']
  assert all(q['exit']==0 and sha(Path(q['log']))==q['log_sha256']for q in commands)
 def execute(label,command,cwd=source,extra=None):
  command=list(map(str,command));log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(command,cwd=cwd,env={**env,**(extra or{})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=command,cwd=str(cwd),exit=r.returncode,log=str(log),log_sha256=sha(log)))
  (run/'command-journal.json').write_text(json.dumps(commands,sort_keys=True,indent=2)+'\n');print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 if not args.resume:
  execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo',
   '-DBUILD_TESTING=ON','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'])
  execute('build',['cmake','--build',build,'--target',*TARGETS,'-j2'])
  execute('ctest',['ctest','--test-dir',build,'-R','^('+'|'.join(CHECKS)+')$','--output-on-failure'])
 ctest=(run/'ctest.log').read_text();assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 4',ctest) and 'Skipped' not in ctest
 gpu_log=next((build/'Testing/Temporary').glob('LastTest.log')).read_text()
 assert '24 production water-copy shader calls' in gpu_log and 'VUID-' not in gpu_log and 'Validation Error' not in gpu_log
 original=run/'original'
 execute('original',[sys.executable,source/'tools/oracle_scene_copy_destinations.py','--output',original,'--fixture',original/'fixture.json'])
 outputs={p.name:sha(p)for p in original.iterdir()if p.is_file()}
 assert outputs['fixture.json']==pins['tests/fixtures/scene_copy_destinations_original.json']
 assert outputs['report.json']==evidence[evidence_names[2]]
 entries=json.loads((build/'compile_commands.json').read_text())
 def sanitized(name):
  entry=next(e for e in entries if Path(e['file'])==source/name);command=shlex.split(entry['command'])
  obj=run/(Path(name).stem+'.asan.o');command[command.index('-o')+1]=str(obj)
  command.extend(['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer'])
  execute('asan-compile-'+Path(name).stem,command,Path(entry['directory']));return obj
 owners=[sanitized(name)for name in('src/source_scene_copy.cpp','src/water.cpp','tests/scene_copy_destination_tests.cpp')]
 binary=run/'scene_copy_destination_tests_asan'
 execute('asan-link',['c++','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',*owners,
  build/'libhalo2_cache.a','-lz','-lglm',build/'libhalo2_native_synchronization.a','-o',binary])
 execute('asan-test',[binary,source/'tests/fixtures/scene_copy_destinations_original.json'],
  extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all(sha(ROOT/name)==digest for name,digest in {**assets,**evidence,**validation}.items())
 assert all(sha(BASE/'frozen-source'/name)==digest for name,digest in base_pins.items())
 result=dict(schema='native-scene-copy-destinations-verification-v1',boundary=__doc__,source=str(source),
  base=str(BASE),base_manifest_sha256=sha(BASE/'verification.json'),source_hashes_sha256=sha(run/'source-hashes.json'),
  overlaid_files=FILES,renderer_overlay=dict(before=OLD_TAPS,after=NEW_TAPS),
  cmake_overlay='Only recorded test declarations; unrelated live CMake/renderer drift excluded',
  commands=commands,assets=assets,evidence=evidence,validation=validation,original_outputs=outputs,
  gpu_test_detail_sha256=sha(build/'Testing/Temporary/LastTest.log'),
  resume_driver_sha256=sha(run/'resume-driver.py') if args.resume else None,
  resume_reason='Verifier summary parser accepts both CTest output formats; unchanged passing build/tests retained' if args.resume else None,
  binaries={str(p):sha(p)for p in(*(build/t for t in TARGETS if t!='halo2_renderer'),build/'libhalo2_renderer.a',binary)},
  captures={str(p.relative_to(build)):sha(p)for p in(build/'water-copy-host-framebuffer').iterdir()},
  complete_original_calls=37,emitted_attributes_compared=888,program_words_per_call=60,water_offset_vectors_compared=96,
  host_shader_calls=24,host_shader_channel_checks=12582912,
  sanitizer_scope='Scene-copy geometry, complete water owner and complete destination comparison; remaining libraries built normally',
  renderer_compiled=True,renderer_normal_water_flow=False,original_gpu_sampling=False,
  normal_camouflage_submission=False,concurrent_depth_alias=False,additional_variant_admission=False,goal_complete=False,
  live_checkout_drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest])
 (run/'verification.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');(OUT/'verification.json').write_bytes((run/'verification.json').read_bytes())
 print(OUT/'verification.json',flush=True)
if __name__=='__main__':main()
