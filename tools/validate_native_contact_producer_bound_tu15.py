#!/usr/bin/env python3
"""Same typed production304 helper against independently executed original DATA."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference,save
ROOT=Path(__file__).resolve().parents[1]

WRAPPER='''#include "halo2/native_tu15_contact_producer_binding.hpp"
namespace {
using ParentActivities=halo2::NativeTu15ContactProducerBindingActivity;
constexpr std::uint32_t PARENT_DATA=0x74000000u;
void run_native_parent(halo2::SourcePhysicsContactMemory&storage,const J&input,
 halo2::NativeTu15PhysicsFrameFp&fp,ParentActivities&activity){
 halo2::NativeTu15PairDeletionNativeScope scope;
 halo2::native_tu15_produce_bound_contacts(storage,{input.at("agent")},
  {input.at("first")},{input.at("second")},{input.at("configuration")},
  {input.at("output")},{PARENT_DATA,0x10000u},fp,0,{},scope,&activity);
 require(!scope.active()&&activity.attempts==1&&activity.completed==1&&
  !activity.refusals&&!activity.unprovided_reads,"Native producer binding activity differs");
}
}
'''

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--modes',type=int,nargs='+',choices=range(4),default=list(range(4)));a=p.parse_args();a.stage=a.stage.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],native_original_executor_calls=0)
    try:
        stage=json.loads((a.stage/'source-pins.json').read_text());manifest_pin=sha(a.stage/'source-pins.json')
        assert all(sha(a.stage/'staged'/n)==row['sha256']for n,row in stage['files'].items())
        names=['tools/observe_native_contact_candidate_entry_tu15.cpp','tools/observe_native_contact_producer_actual_tu15.cpp',str(Path(__file__).relative_to(ROOT)),'tools/native_contact_process_before_input_tu15.py','tools/build_native_tu15_constraint_before_reference.py'];pins={n:sha(ROOT/n)for n in names}
        archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst'
        before=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'before',0x304350)
        reference=build_constraint_reference(before,scratch/'reference');save(scratch/'input.json',before.input)
        project=scratch/'project';shutil.copytree(a.stage/'staged',project)
        prefix=(ROOT/names[0]).read_text().split('int main(int argc,char**argv)')[0]
        driver=prefix+WRAPPER+(ROOT/names[1]).read_text()
        (project/'driver.cpp').write_text(driver)
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualBoundParent LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\nadd_executable(parent driver.cpp '+' '.join(stage['units'])+')\ntarget_include_directories(parent PRIVATE include)\ntarget_compile_features(parent PRIVATE cxx_std_20)\ntarget_compile_options(parent PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options(parent PRIVATE -Wl,--gc-sections)\ntarget_link_libraries(parent PRIVATE nlohmann_json::nlohmann_json)\nfile(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")\n'
        retained=[]
        for mode in a.modes:
            n=f'evidence/mode{mode}.json';retained.append(n);cmake+=f'add_test(NAME actual_parent_{mode} COMMAND parent "{reference["elf"]}" "{scratch}/input.json" "{before.path}/capture-native-before.json" {mode} "${{CMAKE_BINARY_DIR}}/{n}")\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in a.compilers:
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','parent','--filter','^actual_parent_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',*sum((['--retain-report',n]for n in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
        report['results']=results;assert all(row['exit_code']==0 for row in results),'Actual native304 binding differs; inspect retained diagnostics'
        for compiler in a.compilers:
            for mode in a.modes:
                file=a.output/compiler/'evidence/evidence'/f'mode{mode}.json';d=json.loads(file.read_text());assert d['passed']and not d.get('error')and len(d['cases'])==2
        assert all(sha(ROOT/n)==pin for n,pin in pins.items())and sha(a.stage/'source-pins.json')==manifest_pin
        assert all(sha(a.stage/'staged'/n)==row['sha256']for n,row in stage['files'].items())
        report.update(passed=True,actual_input=before.receipt,source_inputs_unchanged=True,stage_manifest_sha256=manifest_pin,tool_pins=pins,generated_driver_sha256=sha(project/'driver.cpp'),production_helper_sha256=sha(project/'include/halo2/native_tu15_contact_producer_binding.hpp'),scope='Selected same typed production304 helper only; strict captured banks/FP, zero initially provided private DATA. Ordered access/observer, admission/mutant/default/fault, peer, World/live remain separate requirements.')
    except BaseException as e:report['error']=str(e)or repr(e);raise
    finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
if __name__=='__main__':main()
