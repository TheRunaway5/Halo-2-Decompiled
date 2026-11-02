#!/usr/bin/env python3
"""Owned private selection-swap DATA controls; no original/live closure claim."""
import argparse,json,os,re,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copytree(a.stage/'staged/include',project/'include',dirs_exist_ok=True)
        def source(name):
            staged=a.stage/'staged/src/physics'/name;return staged if staged.is_file()else ROOT/'src/physics'/name
        units=['native_tu15_contact_records.cpp','native_tu15_contact_buffers.cpp','native_tu15_geometry.cpp','native_tu15_array_growth.cpp']
        pins={}
        for name in units:
            file=source(name);pins[str(file.relative_to(ROOT))]=sha(file);shutil.copyfile(file,project/name)
        file=source('native_tu15_contact_producer.cpp');pins[str(file.relative_to(ROOT))]=sha(file);shutil.copyfile(file,project/'native.cpp')
        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
        driver=ROOT/'tools/probe_native_contact_producer_defined_swap_tu15.cpp';pins[str(driver.relative_to(ROOT))]=sha(driver);shutil.copyfile(driver,project/'driver.cpp')
        (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\nproject(privateSelectedSwap LANGUAGES CXX)\ninclude(CTest)\nadd_executable(swap driver.cpp '+' '.join(units)+')\ntarget_include_directories(swap PRIVATE include)\ntarget_compile_features(swap PRIVATE cxx_std_20)\ntarget_compile_options(swap PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME private_selected_swap COMMAND swap)\n')
        queue=[project/name for name in ('driver.cpp',*units,'native_tu15_contact_unwind_internal.hpp')];compiled_pins={}
        while queue:
            file=queue.pop();key=str(file.relative_to(project))
            if key in compiled_pins:continue
            compiled_pins[key]=sha(file)
            for include in re.findall(r'#include\s+"([^"]+)"',file.read_text()):
                queue.append(project/'include'/include if include.startswith('halo2/')else file.parent/include)
        tool_pins={str(file.relative_to(ROOT)):sha(file)for file in (Path(__file__),driver,ROOT/'tools/build_native_tu15_constraint_before_reference.py',ROOT/'tools/run_tests.py',ROOT/'tools/run_test.py')}
        stage_pin=sha(a.stage/'source-pins.json')
        results=[]
        for compiler in('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','swap','--filter','^private_selected_swap$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command));assert r.returncode==0,'Private selection-swap DATA control failed'
        assert all(sha(ROOT/name)==pin for name,pin in pins.items())
        assert all(sha(project/name)==pin for name,pin in compiled_pins.items())
        assert all(sha(ROOT/name)==pin for name,pin in tool_pins.items())and sha(a.stage/'source-pins.json')==stage_pin
        report.update(passed=True,controls_per_compiler=14,source_hashes=pins,compiled_dependency_pins=compiled_pins,compiled_sources_unchanged=True,tool_pins=tool_pins,stage_manifest_sha256=stage_pin,results=results,scope='Private definedness/alias/admission and known-data default compatibility controls only. Independent Source/full304/defaultcohort/fault/mutant/World/live proof remains required.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
