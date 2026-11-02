#!/usr/bin/env python3
"""Qualify ownership/read-only interface of the typed native contact input observer."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();files=[ROOT/'include/halo2/native_tu15_contact_time_input_observer.hpp',ROOT/'src/physics/native_tu15_contact_time_input_observer.cpp',ROOT/'tests/native_tu15_contact_time_input_observer_tests.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files};(project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeContactTimeInputObserver LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
add_executable(observer "{files[1]}" "{files[2]}")
target_include_directories(observer PRIVATE "{ROOT}/include")
target_compile_features(observer PRIVATE cxx_std_20)
target_link_libraries(observer PRIVATE Threads::Threads)
add_test(NAME observer_interface COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:observer>)
''');results=[]
 for compiler in ('g++','clang++'):
  command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','observer','--filter','^observer_interface$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
 report=dict(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==v for f,v in pins.items()),source_hashes=pins,compiler_results=results,interface_only=True,physics_results_supplied=False,guest_abi_images=False,read_only_typed_input=True,owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
