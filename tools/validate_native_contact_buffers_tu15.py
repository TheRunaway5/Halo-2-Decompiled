#!/usr/bin/env python3
"""Native contact buffer lifetimes and append with genuine physical pool calls."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):
 with Path(p).open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);a.output=a.output.resolve();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');bodies={entry:complete_body(code,entry)for entry in(0x303c70,0x303d00,0x303ee0,0x303ba0)};assert [len(b.code)for b in bodies.values()]==[51,53,36,29]
  source=ROOT/'src/physics/native_tu15_contact_buffers.cpp';header=ROOT/'include/halo2/native_tu15_contact_buffers.hpp';observer=ROOT/'tools/observe_native_contact_buffers_tu15.cpp';dependencies=[ROOT/'src/physics/native_tu15_contact_records.cpp',ROOT/'src/physics/native_tu15_array_growth.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in(source,header,observer,*dependencies,ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',ROOT/'include/halo2/native_tu15_contact_records.hpp',ROOT/'include/halo2/native_tu15_array_growth.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',Path(__file__))}
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458';capture=base/'iteration-04-elongation-crates/original/required-failure-inputs';replay=scratch/'original';command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,text=True,capture_output=True,timeout=180);report.update(ordinary_reference_command=command,ordinary_reference_raw_compare_exit=r.returncode);assert(replay/'original-hardware').exists(),r.stderr[-3000:]
  reference=base/'elongation-crates-original-lifetime-final/report.json';assert sha(reference)=='ed7dbc646f209755a538cfa5a4ad18a9f93dd4eaac9601bf1948617ad2579c87';prior=json.loads(reference.read_text());event_index,event=next((n,e)for n,e in enumerate(prior['events'])if e['kind']=='source_record_copy_entry');rows=[]
  def case(name,kind,**args):rows.append(dict(name=name,kind=kind,**args))
  for count in(0,1,7,16):case('initialize_'+str(count),'initialize',count=count);case('destroy_'+str(count),'destroy',count=count)
  case('initialize_exhausted_stack','initialize',count=7,exhaust_stack=True);case('destroy_exhausted_stack','destroy',count=7,exhaust_stack=True);case('destroy_owned_grown_components','destroy',count=7,grow_components=True)
  for count in range(5):case('append_'+str(count),'append',count=count)
  case('append_same_input_output','append',count=0,input_alias=True);case('append_header_inside_input','append',count=0,header_alias=True);case('append_actual_alignment_fault','append',count=0,alignment_fault=True)
  case('initialize_rejected_store','initialize',count=1,stop_before=0x303c92,reject_write_object='buffer',reject_write_offset=4)
  case('initialize_observer_after_store','initialize',count=1,stop_before=0x303c95,observer_after=0x303c92)
  case('initialize_rejected_allocator_input','initialize',count=1,stop_before=0x303ca0,reject_global_allocator_read=True)
  case('append_rejected_capacity_input','append',count=0,stop_before=0x303f02,reject_read_object='header',reject_read_offset=8)
  case('append_rejected_count_store','append',count=0,stop_before=0x303f2e,reject_write_object='header',reject_write_offset=4)
  case('append_observer_after_flag_store','append',count=0,stop_before=0x303efb,observer_after=0x303ef4)
  case('append_observer_after_vector_store','append',count=0,stop_before=0x303bac,observer_after=0x303ba9)
  case('destroy_rejected_marker_input','destroy',count=1,stop_before=0x303d3d,reject_read_object='allocator',reject_read_offset=0x14)
  case('destroy_rejected_cursor_store','destroy',count=1,force_rewind=True,stop_before=0x303d5b,reject_write_object='allocator',reject_write_offset=8)
  case('destroy_observer_after_cursor_store','destroy',count=1,force_rewind=True,stop_before=0x303d5e,observer_after=0x303d5b)
  for rounding in range(4):
   for denorm in(0,0x40,0x8000,0x8040):case(f'append_controls_{rounding}_{denorm:x}','append',count=0,control_word=0x27f|(rounding<<10),status=0x400,mxcsr=0x1f80|(rounding<<13)|denorm)
  for row in rows:
   if row.get('stop_before'):assert row['stop_before']in set().union(*(b.code for b in bodies.values()))
  fixture=scratch/'cases.json';save(fixture,dict(cases=rows,constructor_payload=event['source_payload'],constructor_input_provenance=dict(report=str(reference.relative_to(ROOT)),report_sha256=sha(reference),event_index=event_index,original_source=event['source'],payload_sha256=hashlib.sha256(bytes.fromhex(event['source_payload'])).hexdigest(),scope='Declared opaque input before original leaf entry. Two flags changed toAA/BB before both original/native owners; only native/original owners calculate outputs. No expected packet or caller GP/SSE postimage supplied.')))
  project=scratch/'project';(project/'src').mkdir(parents=True);shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'src/native.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'src/native_tu15_contact_unwind_internal.hpp');shutil.copyfile(observer,project/'src/observer.cpp');shutil.copyfile(dependencies[0],project/'src/records.cpp');shutil.copyfile(dependencies[1],project/'src/growth.cpp');text=source.read_text();variants={};needle='count*256u+15u';assert text.count(needle)==1;variants['wrong_component_stride']=text.replace(needle,'count*128u+15u');needle='m.word(0x303ef4,input.identity+0x7du,0,1);';assert text.count(needle)==1;variants['uninitialized_flag']=text.replace(needle,'m.word(0x303ef4,input.identity+0x7du,0xbb,1);');needle='index+1u);NativeTu15ContactRecordMemory';assert text.count(needle)==1;variants['wrong_append_count']=text.replace(needle,'index+2u);NativeTu15ContactRecordMemory')
  for name,value in variants.items():(project/'src'/f'{name}.cpp').write_text(value)
  (project/'negative.py').write_text('''import json,subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and('semantic reference differs'in j.get('error','')or any(c.get('physics_different_bytes',0)>0 or not c.get('opaque_packets_equal',True)or not c.get('fp_equal',True)for c in j.get('cases',[]))),(r.returncode,r.stdout,r.stderr)
print('Actual original rejects native buffer mutation:',j.get('error'))
''')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeContactBuffersTu15 LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT src/observer.cpp)
add_library(dependencies OBJECT src/records.cpp src/growth.cpp)
foreach(target driver dependencies)
 target_include_directories(${target} PRIVATE include)
 target_compile_features(${target} PRIVATE cxx_std_20)
 target_compile_options(${target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retained=[]
  for name in('native',*variants):
   cmake+=f'add_executable(buffers_{name} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> src/{name}.cpp)\ntarget_include_directories(buffers_{name} PRIVATE include)\ntarget_compile_features(buffers_{name} PRIVATE cxx_std_20)\n';relative='evidence/'+name+'.json';retained.append(relative);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:buffers_{name}> "{replay}/original-hardware" "{capture}/capture.json" "{fixture}" "${{CMAKE_BINARY_DIR}}/{relative}"';cmake+=f'add_test(NAME buffers_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','buffers_native',*sum((['--target','buffers_'+name]for name in variants),[]),'--filter','^buffers_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report',name]for name in(*retained,'Testing/Temporary/LastTest.log')),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items());assert unchanged;report.update(passed=all(row['exit_code']==0 for row in results),physical_cases_per_compiler=len(rows),compiler_results=results,source_sha256=pins,source_snapshot_unchanged=unchanged,original_image_sha256=NEW_PIN,body_sites={hex(e):len(b.code)for e,b in bodies.items()},negative_controls=list(variants),remaining_pool_services_execute_original_cpu=True,production_native_only_closure=False,whole_producer_raw_red_unchanged=True,remaining=['Production pool/kernel service binding','Full producer private-input lifetime/read/future reuse closure','Source unmasked/fault-resume/SEH platform owner','Live native-only multiplayer and full physics closure','Xbox hardware bit identity'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_owner_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());save(a.output/'report.json',report)
 print(json.dumps({k:report.get(k)for k in('passed','physical_cases_per_compiler','negative_controls')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
