#!/usr/bin/env python3
"""Genuine304 query with the frozen shared native solver; no native CPU child."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput,sha
from build_native_tu15_constraint_before_reference import build_constraint_reference,save
ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--stage',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    parser.add_argument('--modes',nargs='+',type=int,choices=range(4),default=list(range(4)))
    parser.add_argument('--mutants',action='store_true')
    parser.add_argument('--query-ordinal',type=int,choices=range(3),default=0)
    args=parser.parse_args();args.stage=args.stage.resolve();args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    manifest=args.stage/'source-pins.json';stage=json.loads(manifest.read_text())
    tool_names=['tools/observe_native_contact_query_actual_tu15.cpp','tools/observe_native_contact_candidate_entry_tu15.cpp',
                'tools/validate_native_contact_query_actual_tu15.py','tools/native_contact_process_before_input_tu15.py']
    tool_pins={name:sha(ROOT/name)for name in tool_names}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],stage_manifest_sha256=sha(manifest),
                source_after_used_as_native_input=False,captured_gp_used=False,native_original_executor_calls=0,tool_pins=tool_pins)
    try:
        for name,row in stage['files'].items():assert sha(args.stage/'staged'/name)==row['sha256'],name
        archive=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-304350-driving-vehicle-camera/required-native-contact-process-304350-before/actual-native-before.tar.zst'
        before=NativeContactProcessBeforeInput(archive,'5ef6aba031fc184d390fcc65ad6da055e16a279212232da4252a36e0b69d5c9c',scratch/'before',0x304350)
        reference=build_constraint_reference(before,scratch/'reference');declared_input=dict(before.input,declared_query_ordinal=args.query_ordinal);save(scratch/'input.json',declared_input)
        project=scratch/'project';project.mkdir();shutil.copytree(args.stage/'staged/include',project/'include')
        for file in (args.stage/'staged/src/physics').iterdir():shutil.copyfile(file,project/file.name)
        prefix=(ROOT/tool_names[1]).read_text().split('int main(int argc,char**argv)')[0]
        (project/'driver.cpp').write_text(prefix+(ROOT/tool_names[0]).read_text())
        kernel='native_tu15_convex_simplex_solver.cpp';variants={}
        if args.mutants:
            original=(project/kernel).read_text()
            edits={
                'wrong_second_direction':('xor_bits(add(add(tx,ty),tz),sign)','add(add(tx,ty),tz)'),
                'wrong_normalization':('0x3f000000u','0x3f800000u'),
                'wrong_second_transform':('add(add(x_term,y_term),zt));\n              m.word(0x30f451u','subtract(add(x_term,y_term),zt));\n              m.word(0x30f451u')}
            for name,(old,new)in edits.items():
                assert original.count(old)>=1,name
                variant=name+'.cpp';(project/variant).write_text(original.replace(old,new));variants[name]=variant
            (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\np=sys.argv[-1];d=json.load(open(p))\nassert r.returncode!=0 and d["passed"]is False,(r.returncode,r.stdout,r.stderr)\nassert not d.get("error"),d\nassert d.get("cases") and any(not c["passed"]for c in d["cases"]),d\nprint("Actual query rejects numeric mutation:",p)\n')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualContactQuery LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json REQUIRED)\n'
        common=[unit for unit in stage['units']if unit!=kernel]
        cmake+='add_library(common OBJECT driver.cpp '+' '.join(common)+')\ntarget_include_directories(common PRIVATE include)\ntarget_compile_features(common PRIVATE cxx_std_20)\ntarget_compile_options(common PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_libraries(common PRIVATE nlohmann_json::nlohmann_json)\n'
        for name,unit in {'query':kernel,**variants}.items():
            cmake+='add_executable('+name+' $<TARGET_OBJECTS:common> '+unit+')\ntarget_include_directories('+name+' PRIVATE include)\ntarget_compile_features('+name+' PRIVATE cxx_std_20)\ntarget_compile_options('+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_options('+name+' PRIVATE -Wl,--gc-sections)\ntarget_link_libraries('+name+' PRIVATE nlohmann_json::nlohmann_json)\n'
        cmake+='file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")\n'
        retained=[]
        for mode in args.modes:
            name=f'evidence/mode{mode}.json';retained.append(name)
            cmake+=f'add_test(NAME actual_query_{mode} COMMAND query "{reference["elf"]}" "{scratch}/input.json" "{before.path}/capture-native-before.json" {mode} "${{CMAKE_BINARY_DIR}}/{name}")\n'
        for name in variants:
            retained.append('evidence/'+name+'.json')
            cmake+=f'add_test(NAME actual_query_mutant_{name} COMMAND "{sys.executable}" "{project}/negative.py" $<TARGET_FILE:{name}> "{reference["elf"]}" "{scratch}/input.json" "{before.path}/capture-native-before.json" 0 "${{CMAKE_BINARY_DIR}}/evidence/{name}.json")\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in args.compilers:
            command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
                '--output',str(args.output/compiler),'--target','query',*sum((['--target',name]for name in variants),[]),'--filter','^actual_query_',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1',
                *sum((['--retain-report',name]for name in retained),[])]
            result=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=result.returncode,command=command))
        report['results']=results
        assert all(r['exit_code']==0 for r in results),'Actual query build or comparison failed; inspect retained diagnostics'
        for compiler in args.compilers:
            for mode in args.modes:
                p=args.output/compiler/'evidence/evidence'/f'mode{mode}.json';row=json.loads(p.read_text())
                assert row['passed']and not row.get('error')and len(row['cases'])==2,(p,row.get('error'))
        assert all(sha(ROOT/name)==pin for name,pin in tool_pins.items())
        assert all(sha(args.stage/'staged'/name)==row['sha256']for name,row in stage['files'].items())
        report.update(passed=all(r['exit_code']==0 for r in results),source_inputs_unchanged=True,
            compilers=args.compilers,modes=args.modes,
            declared_query_ordinal=args.query_ordinal,
            mutants_rejected_per_compiler=list(variants),
            actual_input=before.receipt,generated_observer_sha256=sha(project/'driver.cpp'),
            scope='Selected genuine304→2E4FE0 query, native2F8630 and frozen sharedc58solver. Full captured banks, defined output/valid/result and FP strict. All owned scratch undefined. No original child or after result enters native services. Ordered read/store and wider domains not claimed; no World/live admission.')
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(args.output/'report.json',report)
    return 0 if report['passed']else 1

if __name__=='__main__':raise SystemExit(main())
