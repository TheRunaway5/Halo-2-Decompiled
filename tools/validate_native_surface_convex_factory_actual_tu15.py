#!/usr/bin/env python3
"""Actual304→2F class23/24 native factory qualification, with private scratch poison."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference,save
ROOT=Path(__file__).resolve().parents[1]
UNITS=('native_tu15_convex_agent_factory.cpp','native_tu15_contact_agent_factory.cpp',
       'native_tu15_inverse_geometry.cpp','native_tu15_contact_cache.cpp',
       'native_tu15_contact_shape.cpp','native_tu15_geometry.cpp',
       'native_tu15_activation_services.cpp','native_tu15_array_growth.cpp',
       'native_tu15_array_allocator.cpp','native_tu15_agent_cleanup_children.cpp')
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--stage',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False);args.stage=args.stage.resolve()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();candidate=args.stage/'staged/include/halo2/native_tu15_convex_agent_binding.hpp'
    files=['tools/observe_native_surface_convex_factory_actual_tu15.cpp','tools/observe_native_contact_candidate_entry_tu15.cpp',
        'tools/validate_native_surface_convex_factory_actual_tu15.py','tools/native_contact_process_before_input_tu15.py',
        'src/physics/native_tu15_contact_unwind_internal.hpp',*[f'src/physics/{name}'for name in UNITS]]
    pins={name:sha(ROOT/name)for name in files};candidate_pin=sha(candidate)
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],source_sha256=pins,candidate_header_sha256=candidate_pin,
        source_after_used_as_native_input=False,captured_gp_used=False,native_factory_uses_original_services=False)
    try:
        archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst'
        before=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'before',0x304350)
        reference=build_constraint_reference(before,scratch/'reference');save(scratch/'input.json',before.input)
        project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(candidate,project/'include/halo2/native_tu15_convex_agent_binding.hpp')
        for unit in UNITS:shutil.copyfile(ROOT/'src/physics'/unit,project/unit)
        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp')
        prefix=(ROOT/'tools/observe_native_contact_candidate_entry_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0]
        observer=prefix+(ROOT/'tools/observe_native_surface_convex_factory_actual_tu15.cpp').read_text();(project/'driver.cpp').write_text(observer)
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(surfaceConvexFactory LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\n'
        cmake+='add_executable(factory driver.cpp '+' '.join(UNITS)+')\ntarget_include_directories(factory PRIVATE include)\ntarget_compile_features(factory PRIVATE cxx_std_20)\ntarget_compile_options(factory PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options(factory PRIVATE -Wl,--gc-sections)\ntarget_link_libraries(factory PRIVATE nlohmann_json::nlohmann_json)\nfile(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")\n'
        retained=[]
        for mode in range(4):
            name=f'evidence/mode{mode}.json';retained.append(name)
            cmake+=f'add_test(NAME surface_factory_{mode} COMMAND factory "{reference["elf"]}" "{scratch}/input.json" "{before.path}/capture-native-before.json" {mode} "${{CMAKE_BINARY_DIR}}/{name}")\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
                '--output',str(args.output/compiler),'--target','factory','--filter','^surface_factory_',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',
                *sum((['--retain-report',name]for name in retained),[])]
            result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
            assert result.returncode==0,'Actual native23/24 factory differs'
            for mode in range(4):
                p=args.output/compiler/'evidence/evidence'/f'mode{mode}.json';row=json.loads(p.read_text());assert row['passed']and not row.get('error')and len(row['cases'])==2
        assert all(sha(ROOT/name)==value for name,value in pins.items())and sha(candidate)==candidate_pin,'Factory source inputs changed'
        report.update(passed=True,source_inputs_unchanged=True,actual_input=before.receipt,results=results,
            modes=4,scratch_poisons=['a5','5a'],generated_observer_sha256=sha(project/'driver.cpp'),
            scope='Genuine Source304→2F→class23/24 factory BEFORE, ordinary formals/FP, whole89.8MB/agent256B/result/FP comparison. Native factory uses sole pool/cache/support kernels with independently poisoned native-only scratch. No original child service, copied after-image or captured GP enters native kernel. Ordered operands rely on prior component proofs; actual parent/World/live admission remain open.')
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(args.output/'report.json',report)
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
