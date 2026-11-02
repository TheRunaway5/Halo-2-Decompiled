#!/usr/bin/env python3
"""Owned qualification of the actual contact feature ABI/WorldState adapter."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  source=ROOT/'src/physics/native_tu15_contact_features_handoff.cpp';kernel=ROOT/'src/physics/native_tu15_contact_features.cpp';test=ROOT/'tests/native_tu15_contact_features_handoff_tests.cpp';header=ROOT/'include/halo2/native_tu15_contact_features_handoff.hpp';helper=ROOT/'src/physics/runtime/native_tu15_contact_features_handoff_internal.hpp';files=[source,kernel,test,header,helper,ROOT/'include/halo2/native_tu15_contact_features.hpp',ROOT/'src/physics/runtime/physics_world_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_contact_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_solver_internal.hpp',ROOT/'src/physics/runtime/physics_step_internal.hpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};assert 'thread_local'not in source.read_text();project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');(project/'src/physics/runtime').mkdir(parents=True)
  for path in files:
   if path.suffix in('.cpp','.hpp'):
    target=project/path.relative_to(ROOT);target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(path,target)
  variants={}
  text=source.read_text()
  for name,needle,replacement in(('wrong_RET12','state.gp[4]=sp+16u','state.gp[4]=sp+12u'),('wrong_source_call','call.instruction==0x30e96du||call.instruction==0x30e985u','call.instruction==0x30e96eu||call.instruction==0x30e985u'),('manufactured_GP','publish();state.gp[4]=sp+16u;','publish();state.gp[0]^=1u;state.gp[4]=sp+16u;')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and("feature"in r.stderr.lower()or"handoff"in r.stderr.lower()),(r.returncode,r.stdout,r.stderr)\nprint("Actual feature adapter rejects mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_contact_features_handoff LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
add_library(driver OBJECT tests/native_tu15_contact_features_handoff_tests.cpp src/physics/native_tu15_contact_features.cpp)
target_include_directories(driver PRIVATE include src)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
  for mode in('native',*variants):
   filename='src/physics/native_tu15_contact_features_handoff.cpp'if mode=='native'else mode+'.cpp';cmake+=f'add_executable(handoff_{mode} $<TARGET_OBJECTS:driver> {filename})\ntarget_include_directories(handoff_{mode} PRIVATE include src)\ntarget_compile_features(handoff_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(handoff_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries(handoff_{mode} PRIVATE Threads::Threads)\n';negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME handoff_{mode} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:handoff_{mode}>)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','handoff_native',*sum((['--target','handoff_'+name]for name in variants),[]),'--filter','^handoff_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,source_hashes=pins,sources_unchanged=unchanged,negative_controls=list(variants),actual_kernel_and_WorldState_helper_exercised=True,no_thread_local_binding=True,active_failure_uses_fallback=False,physics_parity_claimed=False,production_live_wiring_qualified=False)
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
