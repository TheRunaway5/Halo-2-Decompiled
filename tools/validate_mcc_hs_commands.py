"""Freeze the native MCC HS modules for focused original-parser CMake tests.

Run inside run_test.py. The official run_tests.py owns every compiler tree;
the outer owner removes this code-only project. Only bounded reports remain.
This is component evidence; it does not enable gameplay or qualify a build.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',default='g++');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--negative-controls',action='store_true')
    p.add_argument('--unit-controls-hs',action='store_true')
    a=p.parse_args();root=Path(__file__).resolve().parents[1]
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(scratch)
    project=scratch/'project';project.mkdir()
    sources=[
        'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
        'src/campaign/source_campaign_runtime_tu15_generated.cpp',
        'src/campaign/scripting/native_mcc_script_builtin.cpp',
        'src/campaign/scripting/mcc_script_layout.cpp',
        'src/campaign/scripting/scripts.cpp','src/campaign/scripting/source_script.cpp','src/core/kernels.cpp',
        'src/assets/cache.cpp',
        'src/runtime/source_runtime_memory.cpp',
        'src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp']
    if a.unit_controls_hs:sources+=['src/campaign/scripting/source_workshop_unit_controls_tu15_generated.cpp']
    paths=sources+[
        'src/runtime/source_runtime_write_protection.inc',
        'src/campaign/scripting/mcc_script_layout.inc','src/campaign/mcc_cairo_vanilla_scripts.inc',
        'src/assets/cache_file_snapshot.cpp','src/runtime/source_runtime_snapshot_bytes.cpp',
        'cmake/TestStorage.cmake','cmake/SnapshotStorage.cmake','tools/run_test.py',
        'tests/native_mcc_script_command_tests.cpp','tests/native_mcc_script_builtin_tests.cpp',
        'tests/fixtures/workshop_mcc_blur_installed.json',
        'tests/script_mcc_legacy_command_tests.cpp',
        'tools/mcc_script_builtin_hooks.py','tools/validate_mcc_hs_commands.py']
    if a.unit_controls_hs:paths+=['tests/workshop_unit_controls_hs_tests.cpp']
    paths += [str(f.relative_to(root))for f in (root/'include').rglob('*')if f.is_file()]
    paths += [str(f.relative_to(root))for f in (root/'src').rglob('*.hpp')]
    paths=sorted(set(paths));pins={name:hashlib.sha256((root/name).read_bytes()).hexdigest()for name in paths}
    for name in paths:
        out=project/name;out.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(root/name,out)
        assert hashlib.sha256(out.read_bytes()).hexdigest()==pins[name],name
    sys.path.insert(0,str(project/'tools'))
    import mcc_script_builtin_hooks as hooks
    for name in sources[:2]:
        text=(project/name).read_text()
        assert hooks.add_hooks(hooks.remove_hooks(text))==text,name
    variants=[]
    if a.negative_controls:
        profile=(project/'src/campaign/scripting/native_mcc_script_builtin.cpp').read_text()
        campaign=(project/sources[1]).read_text()
        changes=[
            ('missing_command_owner',profile,'auto profile=std::make_shared<Impl>();',
                'enable_mcc_blur=false;auto profile=std::make_shared<Impl>();','Genuine command receipt not owned'),
            ('wrong_command_return',profile,'return found.receipt->opcode==0x2b3u?4:8;',
                'return found.receipt->opcode==0x2b3u?8:8;','MCC command return declaration differs'),
            ('command_as_getter',profile,'if(found.receipt->opcode!=0x38fu && found.receipt->opcode!=0x390u)return {};',
                'if(found.receipt->opcode!=0x38fu && found.receipt->opcode!=0x390u)return 0u;',
                'Argumentful command leaked through getter seam'),
            ('stale_argument_identity',profile,'if(expected.argument_datum!=0xffffffffu &&',
                'if(false && expected.argument_datum!=0xffffffffu &&','Invalid MCC command receipt admitted'),
            ('wide_argument_read',campaign,'std::uint16_t(r(s.memory,s.gp[0],2))',
                'std::uint16_t(r(s.memory,s.gp[0],4))','MCC blur did not read exactly its original two-byte SHORT argument'),
            ('wrong_argument_signature',campaign,'w(s.memory,s.observe,0x211c1bu,signature,2,7u);',
                'w(s.memory,s.observe,0x211c1bu,signature,2,8u);','Deferred command completed before actual argument evaluation')]
        for name,source,needle,replacement,diagnostic in changes:
            assert source.count(needle)==1,(name,source.count(needle))
            (project/(name+'.cpp')).write_text(source.replace(needle,replacement))
            variants.append(dict(name=name,expected_diagnostic=diagnostic))
        (project/'negative.py').write_text('''import subprocess,sys
expected=sys.argv[1];r=subprocess.run(sys.argv[2:],capture_output=True,text=True,timeout=30)
assert r.returncode!=0 and expected in r.stderr,(r.returncode,r.stdout,r.stderr)
print('Expected original-backed rejection:',r.stderr.strip())
''')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeMccHsCommands LANGUAGES CXX)
include(CTest)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include(cmake/TestStorage.cmake)
include(cmake/SnapshotStorage.cmake)
add_library(hs STATIC '''+' '.join(sources)+''')
target_include_directories(hs PUBLIC include src)
target_compile_features(hs PUBLIC cxx_std_20)
target_compile_definitions(hs PRIVATE HALO2_CACHE_BYTES_ONLY)
target_compile_options(hs PRIVATE -O0 -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)
target_link_libraries(hs PUBLIC halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
target_link_options(hs PUBLIC -Wl,--gc-sections)
if(HALO2_SANITIZERS)
target_compile_options(hs PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(hs PUBLIC -fsanitize=address,undefined)
endif()
add_executable(native_mcc_script_command_tests tests/native_mcc_script_command_tests.cpp)
target_link_libraries(native_mcc_script_command_tests PRIVATE hs)
add_test(NAME native_mcc_script_commands COMMAND native_mcc_script_command_tests)
add_test(NAME native_mcc_script_installed_commands COMMAND native_mcc_script_command_tests
 "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_mcc_blur_installed.json")
add_executable(native_mcc_script_builtin_tests tests/native_mcc_script_builtin_tests.cpp)
target_link_libraries(native_mcc_script_builtin_tests PRIVATE hs)
add_test(NAME native_mcc_script_clock_getters COMMAND native_mcc_script_builtin_tests)
add_executable(script_mcc_legacy_command_tests tests/script_mcc_legacy_command_tests.cpp)
target_link_libraries(script_mcc_legacy_command_tests PRIVATE hs)
add_test(NAME native_mcc_script_cpp_commands COMMAND script_mcc_legacy_command_tests
 "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/workshop_mcc_blur_installed.json")
'''
    for variant in variants:
        name=variant['name']
        cmake+=f'''add_executable({name} {name}.cpp tests/native_mcc_script_command_tests.cpp)
target_link_libraries({name} PRIVATE hs)
target_compile_options({name} PRIVATE -O0 -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME native_mcc_script_negative_{name} COMMAND "${{Python3_EXECUTABLE}}" -B
 "${{CMAKE_CURRENT_SOURCE_DIR}}/negative.py" "{variant['expected_diagnostic']}" "$<TARGET_FILE:{name}>")
'''
    if a.unit_controls_hs:
        cmake+='''add_executable(workshop_unit_controls_hs_tests tests/workshop_unit_controls_hs_tests.cpp)
target_link_libraries(workshop_unit_controls_hs_tests PRIVATE hs)
add_test(NAME native_mcc_script_unit_controls_hs COMMAND workshop_unit_controls_hs_tests)
'''
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(root/'tools/run_tests.py'),'--source',str(project),
        '--scratch-root',str(scratch/'builds'),'--output',str(a.output/'cmake'),
        '--target','native_mcc_script_command_tests','--target','native_mcc_script_builtin_tests',
        '--target','script_mcc_legacy_command_tests',
        '--filter','^native_mcc_script_','--cmake-arg=-DCMAKE_CXX_COMPILER='+a.compiler,'--jobs','2',
        '--retain-report','Testing/Temporary/LastTest.log']
    if a.sanitizers:command+=['--sanitizers']
    for variant in variants:command+=['--target',variant['name']]
    if a.unit_controls_hs:command+=['--target','workshop_unit_controls_hs_tests']
    result=subprocess.run(command)
    changed=[name for name,pin in pins.items()if hashlib.sha256((root/name).read_bytes()).hexdigest()!=pin]
    critical=sources+['include/halo2/native_mcc_script_builtin.hpp',
        'include/halo2/script_mcc_legacy_command.hpp','tests/script_mcc_legacy_command_tests.cpp',
        'tests/fixtures/workshop_mcc_blur_installed.json',
        'tests/native_mcc_script_command_tests.cpp','tests/native_mcc_script_builtin_tests.cpp',
        'tools/mcc_script_builtin_hooks.py']
    unchanged=not changed;critical_unchanged=not set(changed).intersection(critical)
    report=dict(scope=__doc__,command=command,exit_code=result.returncode,owned_scratch=str(scratch),
        source_hashes=pins,root_unchanged=unchanged,changed_root_files=changed,
        critical_root_unchanged=critical_unchanged,negative_controls=variants,
        generator_roundtrip=True,unit_controls_hs=a.unit_controls_hs,gameplay_enabled=False,retained_large_artifacts=[])
    a.output.mkdir(parents=True,exist_ok=True);(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    assert critical_unchanged,'MCC HS inputs changed during this frozen component test'
    return result.returncode


if __name__=='__main__':raise SystemExit(main())
