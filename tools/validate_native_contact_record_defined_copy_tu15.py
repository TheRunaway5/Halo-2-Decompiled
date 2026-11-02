#!/usr/bin/env python3
"""Guarded private303BA0/303EE0 definedness; no unknown value is fabricated."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copytree(a.stage/'staged/include',project/'include',dirs_exist_ok=True)
        units=['native_tu15_contact_records.cpp','native_tu15_contact_buffers.cpp']
        pins={}
        for name in units:
            source=a.stage/'staged/src/physics'/name;pins['src/physics/'+name]=sha(source);shutil.copyfile(source,project/name)
        source=ROOT/'src/physics/native_tu15_array_growth.cpp';shutil.copyfile(source,project/'native_tu15_array_growth.cpp');pins[str(source.relative_to(ROOT))]=sha(source)
        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');shutil.copyfile(ROOT/'tools/probe_native_contact_record_defined_copy_tu15.cpp',project/'driver.cpp')
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(definedRecordCopy LANGUAGES CXX)\ninclude(CTest)\nadd_executable(records driver.cpp '+' '.join(units)+' native_tu15_array_growth.cpp)\ntarget_include_directories(records PRIVATE include)\ntarget_compile_features(records PRIVATE cxx_std_20)\ntarget_compile_options(records PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME defined_record_copy COMMAND records)\n')
        results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','records','--filter','^defined_record_copy$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command));assert r.returncode==0,'Native defined record copy differs'
        report.update(passed=True,controls=5,results=results,source_hashes=pins,stage_manifest_sha256=sha(a.stage/'source-pins.json'),scope='Typed private bitwise definedness only. Default public/physical fault matrix and actual304 caller lifetime/World/live remain unqualified.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
