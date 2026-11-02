#!/usr/bin/env python3
"""Freeze and compare complete native collection filtering with original CPU inputs.

The fixture contains sparse declared input fields and independent original
read/store/return evidence, never callback outcomes consumed by native. Native
services execute qualified constant/get-child owners. Build/test products stay
in the enclosing run_test.py owner, with nested CMake lifecycle cleanup.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();project=scratch/'project';project.mkdir()
    files=['src/physics/native_tu15_collection_filter.cpp','src/physics/native_tu15_activation_services.cpp',
           'src/physics/native_tu15_contact_agent_factory.cpp','src/physics/native_tu15_contact_unwind_internal.hpp',
           'tests/workshop_collection_filter_tests.cpp','tests/fixtures/workshop_collection_filter_tu15.json.zlib',
           'include/halo2/native_tu15_collection_filter.hpp',str(Path(__file__).relative_to(ROOT))]
    pins={path:sha(ROOT/path)for path in files};report=dict(passed=False,owned_scratch=str(scratch),source_hashes=pins,retained_large_artifacts=[])
    def save():(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    try:
        # Copy headers rather than borrowing a mutable shared checkout during
        # the compiler/mutant cohort. No original asset bank is staged here.
        shutil.copytree(ROOT/'include',project/'include')
        source=(ROOT/files[0]).read_text();(project/'native.cpp').write_text(source)
        for original,target in ((files[1],'activation.cpp'),(files[2],'factory.cpp'),(files[3],'native_tu15_contact_unwind_internal.hpp'),(files[4],'test.cpp'),(files[5],'reference.zlib')):
            shutil.copyfile(ROOT/original,project/target)
        controls={
            'wrong_motion':('services.motion_type(motion,owner,fp)==7u','services.motion_type(motion,owner,fp)==6u'),
            'wrong_property_bit':('m.word(user+31u,1)&1u','m.word(user+31u,1)&2u'),
            'missing_translate_unwrap':('if(classify(shape)==21u)shape=m.word(shape+12u);','if(classify(shape)==20u)shape=m.word(shape+12u);'),
            'wrong_group_row':('(receiver-12u)+(row&31u)*4u+28u','(receiver-12u)+(column&31u)*4u+28u'),
            'wrong_child_stride':('children+index*8u','children+index*4u'),
        }
        for name,(old,new) in controls.items():
            assert source.count(old)==1,old;(project/(name+'.cpp')).write_text(source.replace(old,new))
        (project/'negative.py').write_text('import json,subprocess,sys\nfrom pathlib import Path\nr=subprocess.run(sys.argv[1:3],capture_output=True,text=True)\nj={"return_code":r.returncode,"stdout":r.stdout[-2000:],"stderr":r.stderr[-2000:]}\nPath(sys.argv[3]).write_text(json.dumps(j,indent=2)+"\\n")\nassert r.returncode==1 and ("differ"in r.stderr or "outside declared"in r.stderr),j\nprint("Original reference rejects semantic mutant")\n')
        cmake='''cmake_minimum_required(VERSION 3.20)
project(CollectionFilterNative LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
'''
        for name in ('native',*controls):
            cmake+=f'''add_executable(filter_{name} test.cpp {name}.cpp activation.cpp factory.cpp)
target_include_directories(filter_{name} PRIVATE include)
target_compile_features(filter_{name} PRIVATE cxx_std_20)
target_compile_options(filter_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(filter_{name} PRIVATE nlohmann_json::nlohmann_json ZLIB::ZLIB)
'''
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if name!='native':command+=f'"{sys.executable}" "{project}/negative.py" '
            command+=f'$<TARGET_FILE:filter_{name}> "{project}/reference.zlib"'
            if name!='native':command+=f' "${{CMAKE_BINARY_DIR}}/evidence/{name}.json"'
            cmake+=f'add_test(NAME collection_filter_{name} COMMAND {command})\n'
        (project/'CMakeLists.txt').write_text(cmake)
        results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                     *sum((['--target','filter_'+name]for name in ('native',*controls)),[]),'--filter','^collection_filter_',
                     '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',
                     *sum((['--retain-report','evidence/'+name+'.json']for name in controls),[])]
            result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
        report.update(passed=all(row['exit_code']==0 for row in results),compiler_results=results,cases_per_compiler=381,negative_controls=list(controls),
                      current_sources_unchanged=all(sha(ROOT/path)==pin for path,pin in pins.items()),frozen_header_files=sum(1 for path in (project/'include').rglob('*')if path.is_file()),supplied_callback_outcomes=False,original_reference='collection-filter-original-host.json',production_native_only_closure=False)
        report['passed']=report['passed']and report['current_sources_unchanged']
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(path.stat().st_blocks*512 for path in scratch.rglob('*')if path.is_file());save()
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
