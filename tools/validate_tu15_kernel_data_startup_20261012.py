#!/usr/bin/env python3
"""Freeze and seal actual-kernel-input TU1.5 startup ownership evidence."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import shutil
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-kernel-hardware-20261012'
CMAKE=('cmake/PhysicsWorldRuntime.cmake','cmake/PhysicsPlatformLocks.cmake',
    'cmake/PhysicsPlatformCall.cmake','cmake/SourceProcessHeapRuntime.cmake','cmake/SourceTu15StartupRecords.cmake')
TOOLS=('tools/capture_xbox_kernel_hardware_20261012.py','tools/capture_xbox_kernel_hardware_boot_20261012.py',
    'tools/oracle_tu15_kernel_data_startup_20261012.py','tools/validate_tu15_kernel_data_startup_20261012.py')
TESTS={'original_tu15_fresh_startup_records','original_tu15_fresh_startup_kernel_data','original_runtime_image_tu15'}

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())

def freeze():
    selected=set(CMAKE+TOOLS+('docs/PHYSICS_TU15_KERNEL_DATA_STARTUP_20261012.md',
        'tests/source_runtime_image_tests.cpp','src/source_runtime_memory.cpp',
        'src/source_runtime_image.cpp','src/source_runtime_host_virtual_memory.cpp'))
    for name in CMAKE:
        selected.update(re.findall(r'\b(?:src|tests)/[\w/]+\.cpp',(ROOT/name).read_text()))
    pending=list(selected)
    while pending:
        name=pending.pop();path=ROOT/name
        if name.endswith(('.hpp','.cpp')):
            for include in re.findall(r'^#include\s+"([^"]+)"',path.read_text(),re.M):
                found=next((p for p in (path.parent/include,ROOT/'include'/include,ROOT/'src'/include) if p.is_file()),None)
                assert found is not None,(name,include)
                relative=str(found.relative_to(ROOT))
                if relative not in selected:selected.add(relative);pending.append(relative)
        if name.endswith('.py'):
            for node in ast.walk(ast.parse(path.read_text())):
                modules=[node.module] if isinstance(node,ast.ImportFrom) and node.module else [v.name for v in node.names] if isinstance(node,ast.Import) else []
                for module in modules:
                    found=next((p for p in (ROOT/'tools'/(module.replace('.','/')+'.py'),
                        ROOT/'tools/reference/halo2-decompiled/tools'/(module.replace('.','/')+'.py')) if p.is_file()),None)
                    if found:
                        relative=str(found.relative_to(ROOT))
                        if relative not in selected:selected.add(relative);pending.append(relative)
    target=E/'source-v3';target.mkdir()
    pins={name:sha(ROOT/name) for name in sorted(selected)}
    for name,pin in pins.items():
        dest=target/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,dest)
        assert sha(dest)==pin==sha(ROOT/name),name
    top='''cmake_minimum_required(VERSION 3.20)
project(halo2_tu15_kernel_data_startup LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument retained native startup graph" OFF)
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/registration.cpp "// Actual CMake consumer\n")
add_library(halo2_systems STATIC ${CMAKE_CURRENT_BINARY_DIR}/registration.cpp)
include(cmake/PhysicsWorldRuntime.cmake)
include(cmake/PhysicsPlatformLocks.cmake)
include(cmake/PhysicsPlatformCall.cmake)
add_executable(source_runtime_image_tests tests/source_runtime_image_tests.cpp
    src/source_runtime_memory.cpp src/source_runtime_image.cpp)
target_include_directories(source_runtime_image_tests PRIVATE include)
target_compile_features(source_runtime_image_tests PRIVATE cxx_std_20)
if(HALO2_SANITIZERS)
    target_compile_options(source_runtime_image_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(source_runtime_image_tests PRIVATE -fsanitize=address,undefined)
endif()
include(cmake/SourceProcessHeapRuntime.cmake)
include(cmake/SourceTu15StartupRecords.cmake)
'''
    (target/'CMakeLists.txt').write_text(top)
    for name in ('analysis','assets'):(target/name).symlink_to(ROOT/name,target_is_directory=True)
    result=dict(schema=1,parity_target='English TU1.5 only',files=pins,top_sha256=sha(target/'CMakeLists.txt'),
        original_archive_reuse=False,tests=sorted(TESTS),production_cmake_sha256=sha(ROOT/'CMakeLists.txt'),
        world_cpp_sha256=sha(ROOT/'src/physics_world_runtime_generated.cpp'),
        scope='Fresh native source compilation with actual retained data input. No title, heap/world creation or live physics claim.')
    (E/'source-manifest-v3.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(files=len(pins),manifest_sha256=sha(E/'source-manifest-v3.json'))))

def validate():
    manifest=read(E/'source-manifest-v3.json')
    for name,pin in manifest['files'].items():assert sha(E/'source-v3'/name)==pin,name
    assert sha(E/'source-v3/CMakeLists.txt')==manifest['top_sha256']
    compiled=read(E/'source-manifest-v2.json')
    assert manifest['top_sha256']==compiled['top_sha256']
    for name,pin in compiled['files'].items():
        assert sha(E/'source-v2'/name)==pin,name
        if name!=TOOLS[-1]:assert manifest['files'][name]==pin,name
    # Shared native dependencies must still match the cohort that was tested.
    for name,pin in manifest['files'].items():
        if name.endswith(('.hpp','.cpp','.cmake')):assert sha(ROOT/name)==pin,name
    first=read(E/'boot-v1/kernel-v1/report.json');repeat=read(E/'boot-v1/kernel-repeat/report.json')
    fresh=read(E/'boot-v3/kernel/report.json');boot=read(E/'boot-v3/validation.json')
    assert first['records']==repeat['records']==fresh['records']
    assert first['metadata_sha256']==repeat['metadata_sha256']==fresh['metadata_sha256']
    assert fresh['tool_sha256']==sha(ROOT/TOOLS[0])
    assert boot['capture_tool_sha256']==sha(ROOT/TOOLS[0]) and boot['tool_sha256']==sha(ROOT/TOOLS[1])
    assert boot['original_resources_unchanged'] and boot['isolated_hdd_overlay'] and boot['private_eeprom_copy']
    assert not boot['dvd_selected'] and not boot['title_executed'] and not boot['physical_xbox_proof']
    assert boot['kernel_report_sha256']==sha(E/'boot-v3/kernel/report.json')
    for folder in ('boot-v1/kernel-v1','boot-v1/kernel-repeat','boot-v3/kernel'):
        report=read(E/folder/'report.json')
        assert not report['guest_memory_writes'] and not report['guest_register_writes'] and not report['instruction_patches']
        assert sha(E/folder/'export-metadata.bin')==report['metadata_sha256']
        for record in report['records'].values():assert sha(E/folder/(record['name']+'.bin'))==record['sha256']
    fixture=read(E/'original-v1/fixture.json');fixture_pin=sha(E/'original-v1/fixture.json')
    assert fixture_pin==sha(E/'original-repeat/fixture.json')
    assert fixture['tool_sha256']==sha(ROOT/TOOLS[2]) and fixture['helper_tool_sha256']==sha(ROOT/'tools/oracle_tu15_startup_records_20261011.py')
    assert fixture['kernel_input']['record']==fresh['records']['322']
    assert fixture['kernel_input']['report_sha256']==sha(E/'boot-v3/kernel/report.json')
    assert fixture['kernel_input']['boot_validation_sha256']==sha(E/'boot-v3/validation.json')
    assert len(fixture['cases'])==24 and len(fixture['source_instructions'])==169
    assert sum(len(c['instructions']) for c in fixture['cases'])==744
    assert sum(len(c['stores']) for c in fixture['cases'])==492
    assert sum(len(v['stores']) for c in fixture['cases'] for v in c['platform_calls'])==408
    assert all(c['boundary']['target']==0x2e0fe2 and c['boundary']['width']==24 for c in fixture['cases'])
    assert fixture['original_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert sha(fixture['original_path'])==fixture['original_sha256']
    assert fixture['hardware_data_supplied'] and not fixture['source_instruction_patches']
    for key in ('older_image_executed','native_expected_data_used','captured_world_input_used','settings_result_supplied',
        'process_heap_installed','full_startup_complete','live_physics_parity','physical_xbox_proof'):assert not fixture[key],key
    builds={}
    for mode in ('release','sanitizer'):
        directory=E/('build-'+mode+'-v2');xml=directory/'tu15-tests.xml';tests=ET.parse(xml).getroot()
        assert {c.attrib['name'] for c in tests.iter('testcase')}==TESTS
        assert not any(list(tests.iter(name)) for name in ('failure','error','skipped'))
        builds[mode]=dict(results_sha256=sha(xml),tests=len(TESTS),
            startup_binary_sha256=sha(directory/'source_tu15_startup_records_tests'),
            image_binary_sha256=sha(directory/'source_runtime_image_tests'))
    tests=ET.parse(E/'production-tests-v4.xml').getroot()
    assert {c.attrib['name'] for c in tests.iter('testcase')}==TESTS-{'original_runtime_image_tu15'}
    assert not any(list(tests.iter(name)) for name in ('failure','error','skipped'))
    provenance=read(E/'build-provenance-v2.json')
    assert provenance['source_manifest_sha256']==sha(E/'source-manifest-v2.json')
    for name,pin in provenance['native_dependency_pins'].items():assert sha(ROOT/name)==pin
    for mode in ('release','sanitizer','production'):
        for name,pin in provenance[mode]['artifacts'].items():assert sha(Path(provenance[mode]['directory'])/name)==pin
    result=dict(schema=1,parity_target='English TU1.5 only',source_manifest_sha256=sha(E/'source-manifest-v3.json'),
        compiled_source_manifest_sha256=sha(E/'source-manifest-v2.json'),validator_sha256=sha(__file__),
        source_cohort_delta='v3 fixes validator hash-variable shadowing only; all native/test/build inputs are byte-identical to compiled v2',
        kernel_report_sha256=sha(E/'boot-v3/kernel/report.json'),original_fixture_sha256=fixture_pin,
        startup_cases=24,source_instructions=744,source_stores=492,primary_platform_stores=408,
        builds=builds,build_provenance_sha256=sha(E/'build-provenance-v2.json'),production_results_sha256=sha(E/'production-tests-v4.xml'),
        physical_xbox_proof=False,live_physics_parity=False,fresh_heap_world_body_ownership_still_required=True)
    (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(result))

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');a=ap.parse_args()
    freeze() if a.freeze else validate()
