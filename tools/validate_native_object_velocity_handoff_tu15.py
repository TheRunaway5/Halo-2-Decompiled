#!/usr/bin/env python3
"""Own disposable source/interface checks for the exact F2091 native handoff."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 source=ROOT/'src/physics/native_tu15_object_velocity_handoff.cpp';header=ROOT/'include/halo2/native_tu15_object_velocity_handoff.hpp';generated=ROOT/'src/physics/vehicles/physics_vehicle_update_generated.cpp';test=ROOT/'tests/native_tu15_object_velocity_handoff_tests.cpp';files=[source,header,generated,test,ROOT/'src/physics/vehicles/physics_vehicle_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_world_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_contact_runtime_internal.hpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  text=generated.read_text();first=text.index('static bool handoff_native_object_velocity(VehicleState&s) {');last=text.index('\n}',first)+2;helper=text[first:last];assert helper.count('native_tu15_handoff_object_velocity(s.memory,{0xf2091u,0xb9800u,false},actual,s.observe)')==1;assert text.count('if(!handoff_native_object_velocity(s))run_b86c0(s);')==1;assert'thread_local'not in source.read_text();project=scratch/'project';project.mkdir();body=test.read_text();assert body.count('// HALO2_HANDOFF_SOURCE_HELPER')==1;(project/'test.cpp').write_text(body.replace('// HALO2_HANDOFF_SOURCE_HELPER',helper));shutil.copyfile(source,project/'native.cpp');variants={}
  for name,old,new in(('wrong_source_site','{0xf2091u,0xb9800u,false}','{0xf2092u,0xb9800u,false}'),('lost_state_load','s.load(actual);return true;','return true;')):
   assert helper.count(old)==1;variants[name]=helper.replace(old,new);(project/(name+'.cpp')).write_text(body.replace('// HALO2_HANDOFF_SOURCE_HELPER',variants[name]))
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and("handoff"in r.stderr.lower()or"generated helper"in r.stderr.lower()),(r.returncode,r.stdout,r.stderr)\nprint("Actual source-helper transport rejects mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeObjectVelocityHandoff LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
add_library(binding OBJECT native.cpp)
target_include_directories(binding PRIVATE "{ROOT}/include")
target_compile_features(binding PRIVATE cxx_std_20)
'''
  for name in('native',*variants):
   filename='test.cpp'if name=='native'else name+'.cpp';cmake+=f'add_executable(handoff_{name} $<TARGET_OBJECTS:binding> {filename})\ntarget_include_directories(handoff_{name} PRIVATE "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features(handoff_{name} PRIVATE cxx_std_20)\ntarget_compile_options(handoff_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(handoff_{name} PRIVATE Threads::Threads)\n';command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:handoff_{name}>';cmake+=f'add_test(NAME handoff_{name} COMMAND {command})\nset_tests_properties(handoff_{name} PROPERTIES TIMEOUT 45)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','handoff_native',*sum((['--target','handoff_'+name]for name in variants),[]),'--filter','^handoff_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,source_hashes=pins,current_sources_unchanged=unchanged,source_helper_sha256=hashlib.sha256(helper.encode()).hexdigest(),negative_controls=list(variants),interface_validation_only=True,physics_parity_claimed=False,actual_generated_helper_exercised=True,production_live_wiring_qualified=False,no_thread_local_binding=True,active_failure_uses_fallback=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
