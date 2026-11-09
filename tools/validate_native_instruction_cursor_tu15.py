#!/usr/bin/env python3
"""Qualify live instruction-cursor dispatch after cached policy changes."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=('src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tests/native_tu15_instruction_cursor_tests.cpp','tools/validate_native_instruction_cursor_tu15.py')
def hashes():return {n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in FILES}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--sanitizers',action='store_true');a=p.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
 r=dict(passed=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  project=scratch/'project';project.mkdir()
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(instruction_cursor LANGUAGES CXX)
include(CTest)
add_executable(instruction_cursor "{ROOT/'tests/native_tu15_instruction_cursor_tests.cpp'}")
target_include_directories(instruction_cursor PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(instruction_cursor PRIVATE cxx_std_20)
add_test(NAME instruction_cursor COMMAND instruction_cursor)
'''
  if a.sanitizers:cmake+='target_compile_options(instruction_cursor PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(instruction_cursor PRIVATE -fsanitize=address,undefined)\n'
  (project/'CMakeLists.txt').write_text(cmake);r['compilers']=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','instruction_cursor','--filter','^instruction_cursor$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   result=subprocess.run(command,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
   r['compilers'].append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-3000:]
  r.update(passed=True,cursor_policy_cases_per_compiler=68,sanitizers=a.sanitizers)
 except BaseException as e:r['error']=str(e) or repr(e);raise
 finally:
  r['source_after']=hashes()
  if r['source_before']!=r['source_after']:r.update(passed=False,source_drift=True)
  (a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
 return 0 if r['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
