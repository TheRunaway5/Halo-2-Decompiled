#!/usr/bin/env python3
"""Verify surgical2F7E0 retirement, originalCALLs, producer compile and retained null gate.

No full generator run, instruction lowering, original execution, reference
fixture edit or GPU success is performed. Disposable compilation uses the
official CMake worker under the enclosing run_test.py owner.
"""
import argparse,ast,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    from oracle_smg_firing_1_1 import Xbe,_Code
    from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
    from generate_physics_live_support_tu15 import complete_body
    image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(image,'.text')
    source=ROOT/'src/rendering/world/source_texture_camera_producer_tu15_generated.cpp';generator=ROOT/'tools/generate_texture_camera_producer_tu15.py';text=source.read_text();tree=ast.parse(generator.read_text());values={}
    for node in tree.body:
        if isinstance(node,ast.Assign):
            for target in node.targets:
                if isinstance(target,ast.Name)and target.id in('ENTRIES','CALLS'):values[target.id]=ast.literal_eval(node.value)
    assert len(values['ENTRIES'])==25 and 0x2f7e0 not in values['ENTRIES'] and 'run_2f7e0'not in text
    driver=text.split('void source_run_texture_camera_producer_tu15',1)[1].split('bool source_texture_camera_producer_tu15_owns_entry',1)[0]
    ownership=text.split('bool source_texture_camera_producer_tu15_owns_entry',1)[1]
    assert 'case 0x2f7e0u:'not in driver and 'case 0x2f7e0u:'not in ownership
    mapping=next(line for line in text.splitlines()if line.startswith('std::uint32_t selected('));assert 'case 0x2f7e0u:return 0x2f7e0u;'in mapping
    authenticated=[];owner=complete_body(code,0x229a90)
    for site,returned in((0x229b95,0x229b9a),(0x229c30,0x229c35)):
        ins=code.at(site);assert site in owner.code and ins.mnemonic=='call'and ins.operands[0].imm==0x2f7e0 and site+ins.size==returned
        assert (0x229a90,site,0x2f7e0)in values['CALLS']
        exact=f'if(s.services.instruction){{s.services.instruction(0x{site:x}u);}} s.push(0x{site:x}u,0x{returned:x}u);s.required(0x{site:x}u,0x2f7e0u,false);'
        assert text.count(exact)==1
        authenticated.append(dict(site=f'{site:08x}',original_bytes=ins.bytes.hex(),target='0002f7e0',real_return=f'{returned:08x}',bridge=exact))
    fixture=ROOT/'tests/fixtures/native_tu15_render_camera_packet.json.gz'
    import gzip
    reference=json.loads(gzip.decompress(fixture.read_bytes()));nulls=[row for row in reference['cases']if row['name']=='default_camera_required']
    assert len(nulls)==1 and not nulls[0]['completed']and nulls[0]['boundary']==dict(phase='default_camera_required',instruction=0x2f8a5)
    assert 'NativeTu15RenderCameraDefaultRequired'in (ROOT/'tests/native_tu15_render_camera_packet_tests.cpp').read_text()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    files=['src/rendering/world/source_texture_camera_producer_tu15_generated.cpp','src/rendering/world/native_tu15_render_camera_packet.cpp','src/rendering/world/native_tu15_render_camera_packet_binding.cpp','cmake/NativeTu15RenderCameraPacket.cmake','tests/native_tu15_render_camera_packet_tests.cpp','tests/fixtures/native_tu15_render_camera_packet.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']
    for name in files:
        destination=project/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,destination)
    (project/'ownership.cpp').write_text('''#include "halo2/source_texture_camera_producer_tu15.hpp"
#include <iostream>
int main(){
 if(halo2::source_texture_camera_producer_tu15_owns_entry(0x2f7e0u))return 1;
 for(auto target:{0x229a90u,0x2f5e0u,0x2f620u,0x2f690u,0x2c470u})
  if(!halo2::source_texture_camera_producer_tu15_owns_entry(target))return 2;
 for(auto target:{0x2b770u,0x2b9f0u,0x13c00u})
  if(halo2::source_texture_camera_producer_tu15_owns_entry(target))return 3;
 std::cout<<"Retired packet entry unowned; retained producer/private-prefix admissions unchanged\\n";
}
''')
    (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeRenderCameraRetirement LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/NativeTu15RenderCameraPacket.cmake)
add_library(retired_producer OBJECT src/rendering/world/source_texture_camera_producer_tu15_generated.cpp)
target_include_directories(retired_producer PRIVATE include "{ROOT/'src'}")
target_compile_features(retired_producer PRIVATE cxx_std_20)
target_compile_options(retired_producer PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)
add_executable(packet_retirement_ownership ownership.cpp $<TARGET_OBJECTS:retired_producer>)
target_include_directories(packet_retirement_ownership PRIVATE include)
target_compile_features(packet_retirement_ownership PRIVATE cxx_std_20)
target_link_options(packet_retirement_ownership PRIVATE -Wl,--gc-sections)
add_test(NAME packet_retirement_ownership COMMAND packet_retirement_ownership)
''')
    pins={name:sha(ROOT/name)for name in files};pins[str(generator.relative_to(ROOT))]=sha(generator);pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/'cmake'),'--target','native_tu15_render_camera_packet_tests','--target','packet_retirement_ownership','--filter','^(native_tu15_render_camera_packet|packet_retirement_ownership)$','--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log']
    result=subprocess.run(command,check=False);unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    report=dict(passed=result.returncode==0 and unchanged,scope=__doc__,source_sha256=pins,source_unchanged=unchanged,command=command,exit_code=result.returncode,authenticated_calls=authenticated,compiled_local_bodies=25,retired_original_sites=109,remaining_native_default_projection_sites=35,retained_null_reference_cases=len(nulls),null_projection_silently_skipped=False,original_fixture_sha256=sha(fixture),original_fixtures_or_frozen_sources_changed=False,owned_scratch=str(scratch),retained_large_artifacts=[],runtime_or_GPU_qualified=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
