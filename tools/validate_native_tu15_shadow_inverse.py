#!/usr/bin/env python3
"""Owned actual-shadow-caller binding/compiler/sanitizer/mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
 binding='src/rendering/shadows/native_tu15_shadow_inverse_binding.cpp';core='src/rendering/world/native_tu15_camera_projection.cpp';fixture='tests/fixtures/native_tu15_shadow_inverse_v2.json.gz'
 names=['cmake/NativeTu15ShadowInverse.cmake',binding,core,'tests/native_tu15_shadow_inverse_tests.cpp',fixture,'tools/test_native_campaign_screen_lifecycle_tu15.py']
 for name in names:dest=project/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
 text=(project/binding).read_text();variants={}
 pairs=[
 ('wrong_caller','profile.call.instruction==0xc4879u','profile.call.instruction==0x2ffe9u'),
 ('allow_MapPack','profile.image==SourcePhysicsContactImage::TitleUpdate15 &&','true &&'),
 ('swap_frames','native_tu15_camera_inverse_frame(memory,source,destination,fp,observe)','native_tu15_camera_inverse_frame(memory,destination,source,fp,observe)'),
 ('drop_success_FP','    abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};\n    std::array','    abi.floating_point=abi.floating_point;\n    std::array'),
 ('drop_fault_FP','catch(...) {abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};throw;}','catch(...) {throw;}'),
 ('wrong_RET','abi.gp[4]+=4u;','abi.gp[4]+=8u;'),
 ('swallow_fault','catch(...) {abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};throw;}','catch(...) {abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};return;}'),
 ('early_RET_read','    calls.fetch_add(1,std::memory_order_relaxed);','    std::array<std::byte,4> early_return{};memory.read(abi.gp[4],early_return);\n    calls.fetch_add(1,std::memory_order_relaxed);'),
 ('ignore_RET_target','if(actual_return!=0xc487eu)','if(false)'),
 ('allow_record_stack_alias','const bool disjoint=record_end<=stack_begin || stack_end<=record_begin;','const bool disjoint=true;')]
 for name,needle,wrong in pairs:assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,wrong)
 for name,variant in variants.items():(project/f'{name}.cpp').write_text(variant)
 (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(s in r.stderr for s in ('shadow inverse','shadow profile','shadow caller','Unqualified native shadow','Missing native shadow')),(r.returncode,r.stdout,r.stderr)
print('Expected actualcaller binding mutation rejection:',r.stderr.strip()[-3000:])
''')
 cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeTu15ShadowInverse LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(HALO2_SANITIZERS)
add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
add_link_options(-fsanitize=address,undefined)
endif()
add_library(halo2_physics_world_runtime OBJECT src/rendering/world/native_tu15_camera_projection.cpp)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/NativeTu15ShadowInverse.cmake)
'''
 for name in variants:cmake+=f'''add_executable(shadow_{name} tests/native_tu15_shadow_inverse_tests.cpp {name}.cpp {core})
target_include_directories(shadow_{name} PRIVATE include)
target_compile_features(shadow_{name} PRIVATE cxx_std_20)
target_compile_options(shadow_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(shadow_{name} PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME shadow_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:shadow_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/{fixture}")
'''
 (project/'CMakeLists.txt').write_text(cmake);names+=['include/halo2/native_tu15_shadow_inverse_binding.hpp','include/halo2/native_tu15_camera_projection.hpp','include/halo2/native_tu15_physics_frame.hpp','include/halo2/physics_world_runtime.hpp','include/halo2/physics_contact_runtime.hpp','include/halo2/physics_contact_record.hpp',str(Path(__file__).relative_to(ROOT))];pins={name:sha(ROOT/name)for name in names};results=[]
 for kind,compiler,san in(('gcc','g++',False),('sanitizers','clang++',True),('clang-controls','clang++',False)):
  cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_tu15_shadow_inverse_tests','--filter','^(native_tu15_shadow_inverse|shadow_)'if kind=='clang-controls'else'^native_tu15_shadow_inverse$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','Testing/Temporary/LastTest.log']
  if san:cmd+=['--sanitizers']
  if kind=='clang-controls':
   for name in variants:cmd+=['--target','shadow_'+name]
  run=subprocess.run(cmd,check=False);results.append(dict(kind=kind,exit_code=run.returncode,command=cmd))
  if run.returncode:break
 report=dict(passed=len(results)==3 and all(r['exit_code']==0 for r in results),source_hashes=pins,source_unchanged=all(sha(ROOT/n)==v for n,v in pins.items()),results=results,negative_controls=list(variants),owned_scratch=str(scratch),retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
