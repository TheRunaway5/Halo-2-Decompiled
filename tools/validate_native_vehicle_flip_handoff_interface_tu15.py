#!/usr/bin/env python3
"""Ownership/admission interface test only; no supplied physics outcome is parity."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();files=[ROOT/'src/physics/native_tu15_vehicle_flip_handoff.cpp',ROOT/'include/halo2/native_tu15_vehicle_flip_handoff.hpp',ROOT/'tests/native_tu15_vehicle_flip_handoff_tests.cpp',Path(__file__)];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();pins={str(p.relative_to(ROOT)):sha(p)for p in files};(project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(FlipHandoffInterface LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
add_executable(handoff "{files[0]}" "{files[2]}")
target_include_directories(handoff PRIVATE "{ROOT}/include")
target_compile_features(handoff PRIVATE cxx_std_20)
target_link_libraries(handoff PRIVATE Threads::Threads)
add_test(NAME handoff_interface COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- $<TARGET_FILE:handoff>)
''');results=[]
 for compiler in ['g++','clang++']:
  cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','handoff','--filter','^handoff_interface$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(cmd);results.append(dict(command=cmd,exit_code=r.returncode,compiler=compiler))
 report=dict(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/p)==h for p,h in pins.items()),interface_only=True,physics_parity_claimed=False,source_hashes=pins,compiler_results=results,owned_scratch=str(scratch),temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file()),retained_large_artifacts=[]);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
