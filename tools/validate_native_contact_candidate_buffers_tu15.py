#!/usr/bin/env python3
"""Native TU1.5 key sorting and entry-buffer ownership, against physical original owners."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from validate_native_contact_buffers_tu15 import sha,save

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');owners=(0x2f33b0,0x2f3430,0x2f34d0,0x317170);bodies={e:complete_body(code,e)for e in owners};assert [len(bodies[e].code)for e in owners]==[59,50,54,17]
  source=ROOT/'src/physics/native_tu15_contact_candidate_buffers.cpp';header=ROOT/'include/halo2/native_tu15_contact_candidate_buffers.hpp';observer=ROOT/'tools/observe_native_contact_candidate_buffers_tu15.cpp';dependencies=[ROOT/'src/physics/native_tu15_array_reserve.cpp',ROOT/'src/physics/native_tu15_array_growth.cpp'];files=[source,header,observer,*dependencies,ROOT/'include/halo2/native_tu15_contact_buffers.hpp',ROOT/'include/halo2/native_tu15_array_reserve.hpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
  capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-gemini-column0/original/required-failure-inputs';replay=scratch/'original';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];reference_command=command;r=subprocess.run(command,capture_output=True,text=True,timeout=180);reference_exit=r.returncode;assert(replay/'original-hardware').exists(),r.stderr[-2000:]
  rows=[]
  def case(name,kind,**kw):rows.append(dict(name=name,kind=kind,**kw))
  case('sort_genuine_original_query','sort');case('sort_genuine_reverse','sort',reverse=True)
  for name,keys in [('single',[0xffffffff]),('unsigned_extrema',[0xffffffff,0,0x80000000,1,0x7fffffff,0xfffffffe]),('duplicates',[3,1,3,2,1,0,3,1]),('equal',[7]*9),('ascending',list(range(32))),('descending',list(reversed(range(32))))]:case('sort_'+name,'sort',keys=keys)
  case('sort_subrange','sort',keys=[99,88,7,3,6,1,5,77,66],first=2,last=6)
  for forwarded in(0,0x80000000,0xffffffff):case('sort_forwarded_'+hex(forwarded),'sort',forwarded_comparator=forwarded)
  case('sort_read_prefix','sort',keys=[9,8,7,6,5,4,3,2,1],stop_before=0x2f33c8,reject_pivot_read=True)
  case('sort_prewrite_prefix','sort',keys=[9,8,7,6,5,4,3,2,1],stop_before=0x2f33f2,reject_first_write=True)
  case('sort_committed_observer_prefix','sort',keys=[9,8,7,6,5,4,3,2,1],stop_before=0x2f33f5,observer_after=0x2f33f2)
  for count in(0,1,3,4,7,16,0x80000000,0x7fffffff):
   case('initialize_'+str(count),'initialize',count=count);case('destroy_'+str(count),'destroy',count=count)
  case('initialize_exhausted_stack','initialize',count=7,exhaust_stack=True);case('destroy_exhausted_stack','destroy',count=7,exhaust_stack=True);case('destroy_owned_growth','destroy',count=7,grow_entries=True)
  case('base_owned_growth','base',count=7);case('base_borrowed','base',count=7,borrowed=True)
  case('initialize_prewrite','initialize',count=1,stop_before=0x2f3452,reject_write_offset=4)
  case('initialize_observer','initialize',count=1,stop_before=0x2f345c,observer_after=0x2f3455)
  case('initialize_global_allocator_read','initialize',count=1,stop_before=0x2f3460,reject_global_allocator_read=True)
  case('initialize_remaining_read','initialize',count=1,stop_before=0x2f3466,reject_read_object='allocator',reject_read_offset=12)
  case('initialize_late_prewrite','initialize',count=1,stop_before=0x2f34a9,reject_write_offset=16)
  case('destroy_marker_read','destroy',count=1,stop_before=0x2f3511,reject_read_object='allocator',reject_read_offset=20)
  case('destroy_cursor_observer','destroy',count=1,force_rewind=True,stop_before=0x2f3532,observer_after=0x2f352f)
  for rounding in range(4):
   for denormal in(0,0x40,0x8000,0x8040):case(f'initialize_controls_{rounding}_{denormal:x}','initialize',count=3,control_word=0x27f|(rounding<<10),status=0x400,mxcsr=0x1f80|(rounding<<13)|denormal)
  fixture=scratch/'cases.json';save(fixture,dict(cases=rows,input_policy='Actual query-created keys are obtained in the physical driver. Explicit numeric/reorder/duplicate/control variations are inputs before each original/native leaf; no callback outcome or expected output is supplied.'))
  project=scratch/'project';(project/'src').mkdir(parents=True);shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(observer,project/'src/observer.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'src/native_tu15_contact_unwind_internal.hpp')
  for path in dependencies:shutil.copyfile(path,project/'src'/path.name)
  text=source.read_text();variants={}
  for name,needle,replacement in [('wrong_stride','count*12u+15u','count*8u+15u'),('wrong_unsigned_order','m.word(keys+low*4u)<pivot','less(m.word(keys+low*4u),pivot)'),('wrong_ctor_order','m.word(0x2f34a2u,p+8u,count|0x80000000u);m.word(0x2f34a9u,p+0x10u,count);','m.word(0x2f34a9u,p+0x10u,count);m.word(0x2f34a2u,p+8u,count|0x80000000u);')]:
   assert text.count(needle)==(2 if name=='wrong_unsigned_order'else 1);variants[name]=text.replace(needle,replacement);(project/'src'/f'{name}.cpp').write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and (j.get("error")or any(c.get("physics_different_bytes",0)>0 or not c.get("packets_equal",True)or not c.get("fp_equal",True)for c in j.get("cases",[]))),(r.returncode,j.get("error"))\nprint("Actual original candidate helper rejects mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeCandidateBuffers LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT src/observer.cpp)
add_library(dependencies OBJECT src/native_tu15_array_reserve.cpp src/native_tu15_array_growth.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  for variant in('native',*variants):
   cmake+=f'add_executable(helpers_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> src/{variant}.cpp)\ntarget_include_directories(helpers_{variant} PRIVATE include)\ntarget_compile_features(helpers_{variant} PRIVATE cxx_std_20)\n';path='evidence/'+variant+'.json';retain.append(path);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:helpers_{variant}> "{replay}/original-hardware" "{capture}/capture.json" "{fixture}" "${{CMAKE_BINARY_DIR}}/{path}"';cmake+=f'add_test(NAME helpers_{variant} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','helpers_native',*sum((['--target','helpers_'+name]for name in variants),[]),'--filter','^helpers_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in(*retain,'Testing/Temporary/LastTest.log')),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());assert unchanged
  report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,physical_cases_per_compiler=len(rows),source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,body_sites={hex(e):len(bodies[e].code)for e in owners},negative_controls=list(variants),ordinary_reference_command=reference_command,ordinary_reference_raw_compare_exit=reference_exit,production_native_only_closure=False,source_fs_raw_gate_unchanged=True,remaining=['Independent native unwind/cleanup qualification','Full candidate composition refresh','Production native-only services and live multiplayer','Broad alias/fault/observer qualification','Xbox hardware identity'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
