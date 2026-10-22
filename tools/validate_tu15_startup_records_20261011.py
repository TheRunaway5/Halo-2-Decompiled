#!/usr/bin/env python3
"""Seal TU1.5-only fresh startup prefixes and their native record owners."""
import argparse,ast,hashlib,json,re,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
E=ROOT/'analysis/multiplayer/sandbox-source/tu15-startup-records-20261011'
OWNED=('include/halo2/source_tu15_startup_records.hpp','src/platform/source_tu15_startup_records.cpp',
    'tests/source_tu15_startup_records_tests.cpp','cmake/SourceTu15StartupRecords.cmake',
    'tools/oracle_tu15_startup_records_20261011.py','tools/validate_tu15_startup_records_20261011.py',
    'docs/PHYSICS_TU15_STARTUP_RECORDS_20261011.md')
SOURCES=('tests/source_tu15_startup_records_tests.cpp','src/platform/source_tu15_startup_records.cpp',
    'src/platform/source_process_heap_runtime_generated.cpp','src/platform/source_process_heap_platform.cpp',
    'src/platform/source_process_shutdown.cpp','src/physics/physics_platform_call.cpp','src/physics/physics_platform_locks.cpp',
    'src/runtime/source_runtime_memory.cpp','src/runtime/source_runtime_image.cpp','src/runtime/source_runtime_host_virtual_memory.cpp')
REGISTRATION='include(cmake/SourceTu15StartupRecords.cmake)'
TOP='''cmake_minimum_required(VERSION 3.20)
project(tu15_fresh_startup_records LANGUAGES CXX)
include(CTest)
option(HALO2_SANITIZERS "Instrument every reachable startup owner" OFF)
find_package(nlohmann_json CONFIG REQUIRED)
add_executable(source_tu15_startup_records_tests
'''+''.join('  '+p+'\n' for p in SOURCES)+''')
target_include_directories(source_tu15_startup_records_tests PRIVATE include src)
target_compile_features(source_tu15_startup_records_tests PRIVATE cxx_std_20)
target_link_libraries(source_tu15_startup_records_tests PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(source_tu15_startup_records_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)
target_link_options(source_tu15_startup_records_tests PRIVATE -Wl,--gc-sections)
if(HALO2_SANITIZERS)
  target_compile_options(source_tu15_startup_records_tests PRIVATE -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(source_tu15_startup_records_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME original_tu15_fresh_startup_records COMMAND source_tu15_startup_records_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/tu15-startup-records-20261011/original-verified/fixture.json
  "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
set_tests_properties(original_tu15_fresh_startup_records PROPERTIES SKIP_RETURN_CODE 77)
'''
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def read(p):return json.loads(Path(p).read_text())
def dependencies():
    files=set(OWNED)|set(SOURCES);pending=list(files);seen=set()
    while pending:
        name=pending.pop()
        if name in seen:continue
        seen.add(name);p=ROOT/name;found=[]
        if p.suffix in ('.cpp','.hpp'):
            for inc in re.findall(r'^#include "([^"]+)"',p.read_text(),re.M):
                matches=[q.resolve() for q in (p.parent/inc,ROOT/'include'/inc,ROOT/'src'/inc) if q.is_file()]
                assert matches,(p,inc);found.append(matches[0])
        elif p.suffix=='.py':
            for n in ast.walk(ast.parse(p.read_text())):
                modules=[n.module] if isinstance(n,ast.ImportFrom) and n.module else [v.name for v in n.names] if isinstance(n,ast.Import) else []
                for module in modules:
                    q=ROOT/'tools'/(module.replace('.','/')+'.py')
                    if q.is_file():found.append(q)
        for q in found:
            relative=str(q.relative_to(ROOT));files.add(relative)
            if relative not in seen:pending.append(relative)
    return sorted(files)
def freeze():
    assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
    target=E/'source-v1';target.mkdir();files={p:sha(ROOT/p) for p in dependencies()}
    for p,pin in files.items():
        q=target/p;q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/p,q)
        assert sha(q)==pin==sha(ROOT/p)
    (target/'CMakeLists.txt').write_text(TOP);(target/'analysis').symlink_to(ROOT/'analysis',target_is_directory=True)
    evidence=ROOT/'analysis/damage-shields-goal-20261011/process-heap-startup-generated'
    manifest=dict(schema=1,parity_target='English TU1.5 only',files=files,owned=OWNED,sources=SOURCES,
        top_sha256=sha(target/'CMakeLists.txt'),registration=REGISTRATION,
        parent_generated_manifest_sha256=sha(evidence/'manifest.json'),parent_correspondence_sha256=sha(evidence/'normalized-correspondence.json'))
    (E/'source-manifest-v1.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(files=len(files),source_manifest_sha256=sha(E/'source-manifest-v1.json'))))
def validate():
    m=read(E/'source-manifest-v1.json');assert sorted(m['files'])==dependencies() and m['sources']==list(SOURCES) and m['owned']==list(OWNED)
    for p,pin in m['files'].items():assert sha(ROOT/p)==pin==sha(E/'source-v1'/p),p
    assert REGISTRATION in (ROOT/'CMakeLists.txt').read_text()
    assert (E/'source-v1/CMakeLists.txt').read_text()==TOP and sha(E/'source-v1/CMakeLists.txt')==m['top_sha256']
    for name in ('fixture.json','primary/manifest.json','primary/records.c','primary/records.bin',
            'registry/manifest.json','registry/registry.c','registry/registry.bin','registry/head.bin'):
        assert sha(E/'original-verified'/name)==sha(E/'original-repeat'/name),name
    f=read(E/'original-verified/fixture.json');assert f['tool_sha256']==sha(ROOT/OWNED[4])
    assert f['parity_target']=='English TU1.5 only' and len(f['cases'])==24 and len(f['component_cases'])==36
    for k in ('older_image_executed','captured_world_input_used','native_expected_data_used','hardware_data_supplied',
            'settings_result_supplied','timer_armed','process_heap_installed','full_startup_complete','live_physics_parity'):assert not f[k],k
    assert f['source_instruction_patches']==[]
    from oracle_smg_firing_1_1 import Xbe,_Code,_trace
    from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
    assert sha(NEW_DEFAULT)==NEW_PIN==f['original_sha256'];image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert len(f['source_instructions'])==169
    for p,row in f['source_instructions'].items():
        i=code.at(int(p,16));assert i.bytes.hex()==row['raw'] and i.mnemonic==row['mnemonic'] and i.op_str==row['operand']
    entries={int(p,16) for p in f['source_bodies']}
    for p,sites in f['source_bodies'].items():assert sorted(_trace(code,int(p,16),entries).code)==sites
    generated=ROOT/'analysis/damage-shields-goal-20261011/process-heap-startup-generated'
    assert sha(generated/'manifest.json')==m['parent_generated_manifest_sha256']==f['generated_parent_manifest_sha256']
    assert sha(generated/'normalized-correspondence.json')==m['parent_correspondence_sha256']
    g=read(generated/'manifest.json');pair=read(generated/'normalized-correspondence.json')
    assert g['generated_cpp_sha256']==sha(ROOT/'src/platform/source_process_heap_runtime_generated.cpp')
    assert pair['later_owned_xbe_sha256']==NEW_PIN
    for p,row in f['source_instructions'].items():assert pair['instruction_map'][f'{row["canonical"]:08x}']==int(p,16)
    for label in ('primary_record_owner','primary_registry_owner'):
        for p,pin in f[label]['primary_sources'].items():assert sha(ROOT/p)==pin,p
    assert len({p for c in f['cases'] for p in c['instructions']})==28
    assert sum(len(c['instructions']) for c in f['cases'])==504 and sum(len(c['stores']) for c in f['cases'])==348
    assert sum(len(v['stores']) for c in f['cases'] for v in c['platform_calls'])==408
    assert sum(len(c['stores']) for c in f['component_cases'])==270
    for c in f['cases']:
        assert c['image']=='1.5' and c['instructions'][-1]==0x2e00e7
        assert c['boundary']['ordinal']==322 and c['boundary']['address']==0x80000142
        assert all(f'{p:08x}' in f['source_instructions'] for p in c['instructions'])
    expected='PASS TU1.5 fresh startup records: 24 prefixes, 504 source instructions, 348 source stores, 360 record stores, 48 registry stores; 36 independent primary cases, 270 component stores; hardware322 remains required'
    logs={};binaries={}
    for mode in ('release','sanitizer','production'):
        for stage in ('configure','build','test'):logs[mode+'-'+stage]=sha(E/(mode+'-'+stage+'.log'))
        output=(E/(mode+'-test.log')).read_text();assert output.count(expected)==1 and '100% tests passed' in output and 'Skipped' not in output
        assert re.search(r'original_tu15_fresh_startup_records\s+\.+\s+Passed',output)
        directory=ROOT/'build-vehicle-control-20261011' if mode=='production' else E/('build-'+mode)
        binaries[mode]=sha(directory/'source_tu15_startup_records_tests')
        ninja=(directory/'build.ninja').read_text()
        for p in ('src/platform/source_tu15_startup_records.cpp','src/platform/source_process_heap_runtime_generated.cpp'):
            section=ninja.split('/'+p+'.o:')[1].split('\n\n')[0]
            if mode=='sanitizer':assert '-fsanitize=address,undefined' in section
            if p.endswith('_generated.cpp'):
                for flag in ('-fno-fast-math','-ffp-contract=off','-frounding-math'):assert flag in section
    result=dict(schema=1,parity_target='English TU1.5 only',validator_sha256=sha(__file__),
        source_manifest_sha256=sha(E/'source-manifest-v1.json'),source_files=len(m['files']),fixture_sha256=sha(E/'original-verified/fixture.json'),
        source_instructions_authenticated=169,source_sites_exercised=28,prefix_cases=24,component_cases=36,
        source_instruction_observations=504,source_stores=348,record_stores=360,registry_stores=48,component_stores=270,
        repeated_original_and_primary_evidence_identical=True,release_sanitizer_production_passed=True,
        older_image_executed=False,old_checkpoint_used=False,original_code_used_in_production=False,
        native_platform_void_abi_declared=True,original_xbox_kernel_abi_proven=False,required_next_data_ordinal=322,
        hardware_settings_results_supplied=False,timer_scheduling_implemented=False,process_heap_installed=False,
        full_tu15_startup_complete=False,full_world_complete=False,live_physics_parity=False,logs=logs,binaries=binaries)
    (E/'validation.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(cases=24,component_cases=36,validation_sha256=sha(E/'validation.json'))))
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--freeze',action='store_true');a=ap.parse_args()
    freeze() if a.freeze else validate()
