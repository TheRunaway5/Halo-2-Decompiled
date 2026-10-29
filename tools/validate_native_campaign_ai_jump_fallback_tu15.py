#!/usr/bin/env python3
"""Owned native jump-fallback original-reference/compiler/mutation qualification."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 names=['cmake/CampaignNativeAiJumpFallback.cmake','src/campaign/ai/native_campaign_ai_jump_fallback_tu15.cpp','tests/native_campaign_ai_jump_fallback_tu15_tests.cpp','tests/fixtures/native_campaign_ai_jump_fallback_tu15.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']
 for name in names:
  dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 core=(project/names[1]).read_text();variants={}
 pairs=[('premature_z_rounding','asm volatile("fsts %0":"=m"(dz)::"memory","st");','asm volatile("fstps %0;flds %0":"=m"(dz)::"memory","st");'),('drop_pending_quantity','if(pending)throw NativeCampaignAiJumpPendingErrorTu15(pending_quantity(*pending),std::current_exception());','if(pending)throw;'),('discard_pending_fp','asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");',''),('wrong_duration','duration=bits(divide(scalar(magnitude),scalar(speed_bits)));','duration=bits(divide(scalar(speed_bits),scalar(magnitude)));'),('zero_default_vector','const auto fallback=read(memory,0x474d10u);\n            direction[0]=read(memory,fallback);direction[1]=read(memory,fallback+4u);direction[2]=read(memory,fallback+8u);','direction={0,0,0};'),('early_optional_outputs','publish(memory,observe,0xff66c,outputs.direction,direction[0]);\n        publish(memory,observe,0xff66e,outputs.direction+4u,direction[1]);\n        publish(memory,observe,0xff671,outputs.direction+8u,direction[2]);\n        if(outputs.distance)publish(memory,observe,0xff676,outputs.distance,magnitude);\n        if(outputs.speed)publish(memory,observe,0xff67e,outputs.speed,speed_bits);\n        if(outputs.duration)publish(memory,observe,0xff688,outputs.duration,duration);','if(outputs.distance)publish(memory,observe,0xff676,outputs.distance,magnitude);\n        if(outputs.speed)publish(memory,observe,0xff67e,outputs.speed,speed_bits);\n        if(outputs.duration)publish(memory,observe,0xff688,outputs.duration,duration);\n        publish(memory,observe,0xff66c,outputs.direction,direction[0]);\n        publish(memory,observe,0xff66e,outputs.direction+4u,direction[1]);\n        publish(memory,observe,0xff671,outputs.direction+8u,direction[2]);')]

 for name,needle,replacement in pairs:
  assert core.count(needle)==1,(name,core.count(needle));variants[name]=core.replace(needle,replacement)
 for name,text in variants.items():(project/f'src/campaign/ai/{name}.cpp').write_text(text)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('retained jump fallback bytes differ','jump fallback ordered reads differ','jump fallback ordered store differs','jump fallback store count differs','jump fallback completion differs','jump fallback FP differs','pending jump presence differs','pending jump quantity differs','pending jump kind differs','actual jump fallback fault operand differs','Missing jump fallback storage','Jump fallback storage extent differs')),(r.returncode,r.stdout,r.stderr)
print('Expected physical-original jump fallback mutation rejection:',r.stderr.strip()[-4000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCampaignJumpFallback LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeAiJumpFallback.cmake)
'''
 for name in variants:
  cmake+=f'''add_executable(jump_{name} tests/native_campaign_ai_jump_fallback_tu15_tests.cpp src/campaign/ai/{name}.cpp)
target_include_directories(jump_{name} PRIVATE include)
target_compile_features(jump_{name} PRIVATE cxx_std_20)
target_compile_options(jump_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(jump_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME jump_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:jump_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_ai_jump_fallback_tu15.json.gz")
'''
 (project/'CMakeLists.txt').write_text(cmake);results=[];pins={name:sha(ROOT/name)for name in names};pins.update({str(path.relative_to(ROOT)):sha(path)for path in(ROOT/'include/halo2/native_campaign_ai_jump_fallback_tu15.hpp',ROOT/'include/halo2/native_campaign_ai_jump_point_tu15.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__))})
 for kind,compiler,sanitizers in [('normal','g++',False),('sanitizers','g++',True),('clang-controls','clang++',False)]:
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_ai_jump_fallback_tu15_tests','--filter','^(native_campaign_ai_jump_fallback_tu15|jump_)'if kind=='clang-controls'else'^native_campaign_ai_jump_fallback_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if kind=='clang-controls':
   for name in variants:command+=['--target','jump_'+name]
  if sanitizers:command+=['--sanitizers']
  run=subprocess.run(command,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=command))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(x['exit_code']==0 for x in results),source_hashes=pins,source_unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
