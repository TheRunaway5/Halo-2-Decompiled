#!/usr/bin/env python3
"""Native producer unwind ordering against actual original state actions."""
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
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_contact_producer.cpp';observer=ROOT/'tools/observe_native_contact_producer_unwind_tu15.cpp';guard=ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp';deps={name:ROOT/('src/physics/native_tu15_'+name+'.cpp')for name in('geometry','contact_records','contact_buffers','array_growth')};files=[source,observer,guard,*deps.values(),*(ROOT/('include/halo2/native_tu15_'+name+'.hpp')for name in('contact_producer','geometry','contact_records','contact_buffers','array_growth','physics_frame')),ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
  ownership=ROOT/'analysis/physics/native-contact-cleanup-ownership-tu15-20261026/report.json';auth=json.loads(ownership.read_text());assert auth['completed']and auth['image_sha256']==NEW_PIN;capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458/iteration-04-elongation-crates/original/required-failure-inputs';replay=scratch/'original';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,text=True,capture_output=True,timeout=180);report.update(ordinary_reference_command=command,ordinary_reference_raw_compare_exit=r.returncode);assert(replay/'original-hardware').exists(),r.stderr[-3000:]
  specs=[('update_after',0x3043e0,-1),('initialize_before',0x304411,0),('initialize_after',0x304416,0),('post_constructor_count_read',0x304416,0),('post_component_arm_store',0x30442e,1),('query_before',0x3044dd,1),('query_after',0x3044e0,1),('append_after',0x3044ec,1),('publish_after',0x3049ca,1),('distance_after',0x304a11,1),('destroy_before',0x304a6a,0),('destroy_after',0x304a6f,0),('final_capacity_read',0x304a6f,0)];rows=[dict(name=name,injection=name,source_stop_before=pc,source_state=state)for name,pc,state in specs];fixture=scratch/'cases.json';save(fixture,dict(cases=rows,ownership_provenance=dict(report=str(ownership.relative_to(ROOT)),report_sha256=sha(ownership),producer_map=auth['maps'][1],stages=auth['stages'][2],scope='Actual original inline producer call and actual source funclets; dynamic opaque-record gate and exception-dispatch/platform remain open.')))
  project=scratch/'project';(project/'src').mkdir(parents=True);shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(observer,project/'src/observer.cpp');shutil.copyfile(guard,project/'src/native_tu15_contact_unwind_internal.hpp')
  for name,path in deps.items():shutil.copyfile(path,project/'src'/(name+'.cpp'))
  text=source.read_text();variants={};lines=text.splitlines(keepends=True);ai=next(n for n,line in enumerate(lines)if'native_contact_detail::UnwindGuard array_cleanup'in line);ci=next(n for n,line in enumerate(lines)if'native_contact_detail::UnwindGuard component_cleanup'in line);wrong=lines.copy();wrong[ai],wrong[ci]=wrong[ci],wrong[ai];variants['reverse_cleanup_order']=''.join(wrong)
  needle='const auto components=m.word(agent+0x10u),second_transform=m.word(second.identity+8u),resolved_container=m.word(p+0x30u);component_cleanup.arm();';assert text.count(needle)==1;variants['early_component_arm']=text.replace(needle,'component_cleanup.arm();const auto components=m.word(agent+0x10u),second_transform=m.word(second.identity+8u),resolved_container=m.word(p+0x30u);')
  needle='component_cleanup.disarm(); // Actual304A62 before explicit component destruction.';assert text.count(needle)==1;late=text.replace(needle,'// Incorrect delayed component disarm for negative control.');needle='environment.dependency([&]{require(s.destroy_component_buffer,"Native TU1.5 contact producer requires actual303D00 destroy owner");s.destroy_component_buffer(p+0x6cu,fp);});';assert late.count(needle)==1;variants['late_component_disarm']=late.replace(needle,needle+'component_cleanup.disarm();')
  for name,value in variants.items():(project/'src'/f'{name}.cpp').write_text(value)
  (project/'negative.py').write_text('''import json,subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and any(not c['native_cleanup_semantics_passed']for c in j.get('cases',[])),(r.returncode,r.stdout,r.stderr)
print('Actual source producer actions reject ownership mutation:',','.join(c['name']for c in j['cases']if not c['native_cleanup_semantics_passed']))
''')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeProducerUnwindTu15 LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT src/observer.cpp)
add_library(dependencies OBJECT src/geometry.cpp src/contact_records.cpp src/contact_buffers.cpp src/array_growth.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retained=[]
  for name in('native',*variants):
   cmake+=f'add_executable(unwind_{name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> src/{name}.cpp)\ntarget_include_directories(unwind_{name} PRIVATE include)\ntarget_compile_features(unwind_{name} PRIVATE cxx_std_20)\n';relative='evidence/'+name+'.json';retained.append(relative);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:unwind_{name}> "{replay}/original-hardware" "{capture}/capture.json" "{fixture}" "${{CMAKE_BINARY_DIR}}/{relative}"';cmake+=f'add_test(NAME producer_unwind_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','unwind_native',*sum((['--target','unwind_'+name]for name in variants),[]),'--filter','^producer_unwind_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report',name]for name in(*retained,'Testing/Temporary/LastTest.log')),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());assert unchanged;report.update(passed=all(row['exit_code']==0 for row in results),native_cleanup_semantics_only=True,physical_cases_per_compiler=len(rows),compiler_results=results,source_sha256=pins,source_snapshot_unchanged=unchanged,original_image_sha256=NEW_PIN,negative_controls=list(variants),source_fs_registration_recreated=False,source_exception_dispatch_emulated=False,old_raw_fs_prefix_gate='RED unchanged',producer_dynamic_opaque_raw_gate='RED unchanged',production_native_only_closure=False,remaining=['Dynamic owned-record producer cleanup and opaque future-read/lifetime closure','Secondary-exception/termination and platform exception dispatch','Native production remaining service binding/live multiplayer','Xbox hardware identity'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_owner_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','physical_cases_per_compiler','negative_controls')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
