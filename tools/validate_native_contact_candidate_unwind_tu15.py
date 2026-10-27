#!/usr/bin/env python3
"""Qualify native contact exception guards against genuine original funclets.

This does not recreate Xbox FS registration or exception dispatch. Their raw
prefix differences remain visible; only native resource cleanup is qualified.
"""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);a.output=a.output.resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_contact_candidate_buffers.cpp';header=ROOT/'include/halo2/native_tu15_contact_candidate_buffers.hpp';observer=ROOT/'tools/observe_native_contact_candidate_unwind_tu15.cpp';guard=ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp';dependencies=[ROOT/'src/physics/native_tu15_array_growth.cpp'];files=[source,header,observer,guard,*dependencies,ROOT/'include/halo2/native_tu15_contact_records.hpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
  ownership=ROOT/'analysis/physics/native-contact-candidate-cleanup-ownership-tu15-20261026/report.json';auth=json.loads(ownership.read_text());assert auth['completed']and auth['image_sha256']==NEW_PIN and auth['maps'][0]['actions']==[dict(state=0,next_state=-1,entry='0x37e600')]
  capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-gemini-column0/original/required-failure-inputs';replay=scratch/'original';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,text=True,capture_output=True,timeout=180);report.update(ordinary_reference_command=command,ordinary_reference_raw_compare_exit=r.returncode);assert(replay/'original-hardware').exists(),r.stderr[-3000:]
  rows=[]
  def case(name,kind,**kw):rows.append(dict(name=name,kind=kind,**kw))
  case('constructor_success_disarm','constructor',count=7,cleanup_reads=0)
  case('constructor_before_arm_observer','constructor',count=7,exception=True,observer_after=0x2f3455,source_stop_before=0x2f345c,source_action=0,cleanup_reads=0)
  case('constructor_before_arm_input_failure','constructor',count=7,exception=True,reject_object='global',reject_read=True,source_stop_before=0x2f3460,source_action=0,cleanup_reads=0)
  case('constructor_armed_before_actual_allocation','constructor',count=7,exhaust_stack=True,exception=True,injection='allocate_before',source_stop_before=0x2f3481,source_action=0x37e600,cleanup_reads=1)
  case('constructor_armed_after_actual_allocation','constructor',count=7,exhaust_stack=True,exception=True,injection='allocate_after',source_stop_before=0x2f3484,source_action=0x37e600,cleanup_reads=1)
  case('constructor_armed_before_late_flag_store','constructor',count=7,exception=True,reject_object='buffer',reject_write=True,reject_offset=8,write_occurrence=1,source_stop_before=0x2f34a2,source_action=0x37e600,cleanup_reads=1)
  case('constructor_armed_after_last_store','constructor',count=7,exception=True,observer_after=0x2f34ae,source_stop_before=0x2f34b1,source_action=0x37e600,cleanup_reads=1)
  case('destructor_success_disarm_owned','destructor',count=7,owned=True,cleanup_reads=0)
  case('destructor_armed_before_stack_release','destructor',count=7,owned=True,exhaust_stack=True,exception=True,injection='release_stack_before',source_stop_before=0x2f3520,source_action=0x37e600,cleanup_reads=5)
  case('destructor_armed_after_actual_stack_release','destructor',count=7,owned=True,exhaust_stack=True,exception=True,injection='release_stack_after',source_stop_before=0x2f3523,source_action=0x37e600,cleanup_reads=5)
  case('destructor_armed_before_marker_read','destructor',count=7,owned=True,exception=True,reject_object='allocator',reject_read=True,reject_offset=0x14,source_stop_before=0x2f3511,source_action=0x37e600,cleanup_reads=5)
  case('destructor_armed_before_capacity_read','destructor',count=7,owned=True,exception=True,reject_object='buffer',reject_read=True,reject_offset=8,source_stop_before=0x2f3535,source_action=0x37e600,cleanup_reads=5)
  case('destructor_disarmed_before_owned_allocator_read','destructor',count=7,owned=True,exception=True,reject_object='global',reject_read=True,read_occurrence=1,source_stop_before=0x2f3544,source_action=0,cleanup_reads=0)
  case('destructor_disarmed_before_owned_release','destructor',count=7,owned=True,exception=True,injection='owned_before',source_stop_before=0x2f355d,source_action=0,cleanup_reads=0)
  case('destructor_armed_before_cursor_write','destructor',count=7,owned=True,exception=True,reject_object='allocator',reject_write=True,reject_offset=8,source_stop_before=0x2f352f,source_action=0x37e600,cleanup_reads=5)
  case('destructor_armed_after_committed_cursor_write','destructor',count=7,owned=True,exception=True,observer_after=0x2f352f,source_stop_before=0x2f3532,source_action=0x37e600,cleanup_reads=5)
  fixture=scratch/'cases.json';provenance=dict(report=str(ownership.relative_to(ROOT)),report_sha256=sha(ownership),maps=auth['maps'],runtime_dispatch='Independently authenticated source330D50/3315C0 dispatch; see contact cleanup ownership audit',scope='Original prefix reaches actual source state; original funclet is then executed directly on real pool-backed objects. Native failure is a C++ exception, never a virtual source dispatcher.');save(fixture,dict(cases=rows,source_ownership_provenance=provenance))
  project=scratch/'project';(project/'src').mkdir(parents=True);shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(observer,project/'src/observer.cpp');shutil.copyfile(guard,project/'src/native_tu15_contact_unwind_internal.hpp');shutil.copyfile(dependencies[0],project/'src/growth.cpp');text=source.read_text();variants={}
  needle='const auto p=buffer.identity;m.word(0x2f3450u';assert text.count(needle)==1;variants['early_constructor_arm']=text.replace(needle,'const auto p=buffer.identity;cleanup.arm();m.word(0x2f3450u')
  needle='const auto capacity=m.word(p+8u);cleanup.disarm();';assert text.count(needle)==1;variants['early_destructor_disarm']=text.replace(needle,'cleanup.disarm();const auto capacity=m.word(p+8u);')
  late=text.replace(needle,'const auto capacity=m.word(p+8u);');needle='s.release_owned_components(actual,owner,data,bytes_owned,0x12u,fp);';assert late.count(needle)==1;variants['late_destructor_disarm']=late.replace(needle,needle+'cleanup.disarm();')
  for name,value in variants.items():(project/'src'/f'{name}.cpp').write_text(value)
  (project/'negative.py').write_text('''import json,subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and any(not c['native_cleanup_semantics_passed']for c in j.get('cases',[])),(r.returncode,r.stdout,r.stderr)
print('Actual source unwind actions reject ownership mutation:',','.join(c['name']for c in j['cases']if not c['native_cleanup_semantics_passed']))
''')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeContactUnwindTu15 LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT src/observer.cpp)
add_library(dependencies OBJECT src/growth.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retained=[]
  for name in('native',*variants):
   cmake+=f'add_executable(unwind_{name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> src/{name}.cpp)\ntarget_include_directories(unwind_{name} PRIVATE include)\ntarget_compile_features(unwind_{name} PRIVATE cxx_std_20)\n';relative='evidence/'+name+'.json';retained.append(relative);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:unwind_{name}> "{replay}/original-hardware" "{capture}/capture.json" "{fixture}" "${{CMAKE_BINARY_DIR}}/{relative}"';cmake+=f'add_test(NAME unwind_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','unwind_native',*sum((['--target','unwind_'+name]for name in variants),[]),'--filter','^unwind_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report',name]for name in(*retained,'Testing/Temporary/LastTest.log')),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());assert unchanged;report.update(passed=all(row['exit_code']==0 for row in results),native_cleanup_semantics_only=True,physical_cases_per_compiler=len(rows),compiler_results=results,source_sha256=pins,source_snapshot_unchanged=unchanged,original_image_sha256=NEW_PIN,ownership_report_sha256=sha(ownership),negative_controls=list(variants),source_fs_registration_recreated=False,source_exception_dispatch_emulated=False,old_raw_fs_prefix_gate='RED unchanged',producer_opaque_raw_gate='RED unchanged',production_native_only_closure=False,remaining=['Secondary-exception/termination and platform exception dispatch','Full candidate exception-action ordering and opaque future-read/lifetime closure','Native production pool binding and live multiplayer','Xbox hardware bit identity'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_owner_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','physical_cases_per_compiler','negative_controls')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
