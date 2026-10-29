#!/usr/bin/env python3
"""Disposable production transition-request compiler/sanitizer/mutation proof."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--gcc-only',action='store_true');a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    core='src/campaign/native_campaign_transition_request_tu15.cpp'
    names=['cmake/CampaignNativeTransitionRequest.cmake',core,'tests/native_campaign_transition_request_tu15_tests.cpp','tools/test_native_campaign_screen_lifecycle_tu15.py','tests/fixtures/native_campaign_transition_request_tu15.json.gz']
    for name in names:
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
    text=(project/core).read_text();variants={}
    for name,before,after,count in [
        ('truncated_copy','offset<0x1118u','offset<0x1114u',1),
        ('copy_preload','for(std::uint32_t offset=0;offset<0x1118u;offset+=4u)\n            write(m,0x4f9c78u+offset,options.read_at(offset));','std::array<std::uint32_t,0x446> staged{};\n        for(std::uint32_t offset=0;offset<0x1118u;offset+=4u)staged[offset/4u]=options.read_at(offset);\n        for(std::uint32_t offset=0;offset<0x1118u;offset+=4u)write(m,0x4f9c78u+offset,staged[offset/4u]);',1),
        ('inverted_absent_flag','write(m,0x4f9c6eu,!present,1);','write(m,0x4f9c6eu,present,1);',1),
        ('late_clock_producer','clock(cell);\n    const auto tick=read(m,cell);','const auto tick=read(m,cell);\n    clock(cell);',1),
        ('swallowed_required_owner','throw NativeCampaignTransitionRequestRequiredOwnerTu15();','return;',1),
        ('wrong_session_guard','state!=3u&&state!=8u','state!=3u&&state!=7u',1),
        ('missing_signed_count_minimum','if(count>=2&&count<=6)return;','if(count<=6)return;',1)]:
        assert text.count(before)==count,(name,text.count(before));path=f'src/campaign/{name}.cpp';(project/path).write_text(text.replace(before,after));variants[name]=path
    (project/'negative.py').write_text('''import subprocess,sys
r=subprocess.run([sys.executable,sys.argv[1],'--probe',sys.argv[2],'--fixture',sys.argv[3]],text=True,capture_output=True)
assert r.returncode!=0 and any(v in r.stderr for v in ('ordered transition reads differ','ordered transition read count differs','ordered transition stores differ','ordered transition store count differs','retained transition publication bytes differ','clock preparation timing differs','transition required owner differs','transition publication completion differs','unexpected transition source read','absent clock producer')),(r.returncode,r.stdout,r.stderr)
print('Independent original rejects semantic mutation:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.24)
project(NativeTransitionRequest LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime STATIC)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/CampaignNativeTransitionRequest.cmake)
add_library(mutation_driver OBJECT tests/native_campaign_transition_request_tu15_tests.cpp)
target_include_directories(mutation_driver PRIVATE include)
target_compile_features(mutation_driver PRIVATE cxx_std_20)
target_link_libraries(mutation_driver PRIVATE nlohmann_json::nlohmann_json)
'''
    for name,path in variants.items():cmake+=f'''add_executable(policy_{name} $<TARGET_OBJECTS:mutation_driver> {path})
target_include_directories(policy_{name} PRIVATE include)
target_compile_features(policy_{name} PRIVATE cxx_std_20)
add_test(NAME policy_{name} COMMAND "${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/test_native_campaign_screen_lifecycle_tu15.py" "$<TARGET_FILE:policy_{name}>" "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/native_campaign_transition_request_tu15.json.gz")
'''
    (project/'CMakeLists.txt').write_text(cmake);pins={name:sha(ROOT/name)for name in names}
    for header in ['native_campaign_transition_request_tu15.hpp','native_tu15_physics_frame.hpp','physics_contact_record.hpp']:pins['include/halo2/'+header]=sha(ROOT/'include/halo2'/header)
    pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__);results=[];instrumented=[]
    configs=[('gcc','g++',False)]if a.gcc_only else [('gcc','g++',False),('clang-controls','clang++',False),('sanitizers','clang++',True)]
    for kind,compiler,san in configs:
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/kind),'--target','native_campaign_transition_request_tu15_tests','--filter','^(native_campaign_transition_request_tu15|policy_)'if kind=='clang-controls'else'^native_campaign_transition_request_tu15$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','Testing/Temporary/LastTest.log','--retain-report','compile_commands.json']
        if kind=='clang-controls':
            for name in variants:command+=['--target','policy_'+name]
        if san:command+=['--sanitizers']
        status=subprocess.run(command).returncode;results.append(dict(kind=kind,exit_code=status,command=command))
        if san:
            commands=json.loads((a.output/kind/'evidence/compile_commands.json').read_text())
            for path in [core]:
                found=[c.get('command',' '.join(c.get('arguments',[])))for c in commands if c['file'].endswith('/'+path)and'native_campaign_transition_request_tu15_tests.dir'in c.get('command',' '.join(c.get('arguments',[])))];assert found;instrumented.append(dict(source=path,all_objects_instrumented=all('-fsanitize=address,undefined'in c for c in found),commands=found))
        if status:break
    report=dict(passed=len(results)==len(configs)and all(r['exit_code']==0 for r in results)and all(x['all_objects_instrumented']for x in instrumented),full_matrix=not a.gcc_only,source_sha256=pins,source_unchanged=all(sha(ROOT/n)==h for n,h in pins.items()),results=results,negative_controls=list(variants)if not a.gcc_only else[],actual_sanitizer_objects=instrumented,lifted_fallback=False,retained_large_artifacts=[]);(a.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
