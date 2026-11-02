#!/usr/bin/env python3
"""Verify nine retired camera bodies, twelve CALLs and the canonical null gate.

No full generator run, instruction lowering, original execution, reference
fixture edit or GPU success is performed. Disposable compilation uses the
official CMake worker under the enclosing run_test.py owner.
"""
import argparse,ast,hashlib,json,os,re,shutil,subprocess,sys
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
    retired=(0x2f7e0,0x2b700,0x2f950,0x2f5e0,0x2f620,0x2f690,0x2fb50,0x2fd70,0x2c470)
    assert len(values['ENTRIES'])==17 and all(e not in values['ENTRIES'] and f'run_{e:x}'not in text for e in retired)
    driver=text.split('void source_run_texture_camera_producer_tu15',1)[1].split('bool source_texture_camera_producer_tu15_owns_entry',1)[0]
    ownership=text.split('bool source_texture_camera_producer_tu15_owns_entry',1)[1].split('\n',1)[0]
    public=set(values['ENTRIES'])-{0x2b770,0x2b9f0,0x13c00}
    assert len(public)==14 and 0x25940 in public
    for section in(driver,ownership):assert {int(v,16)for v in re.findall(r'case 0x([0-9a-f]+)u:',section)}==public
    mapping=next(line for line in text.splitlines()if line.startswith('std::uint32_t selected('))
    for entry in retired:assert f'case 0x{entry:x}u:return 0x{entry:x}u;'in mapping
    bodies={m[1]:m[0].rstrip()for m in re.finditer(r'static void run_([0-9a-f]+)\(WorldState &s\) \{\n.*?\n\}\n',text,re.S)}
    previous_path=ROOT/'analysis/campaign-native-new-game-20261028/camera-leaf-retirement-edit.json';previous_edit=json.loads(previous_path.read_text())
    layout_path=ROOT/'analysis/campaign-native-new-game-20261028/camera-layout-retirement-edit.json';layout_edit=json.loads(layout_path.read_text())
    edit_path=ROOT/'analysis/campaign-native-new-game-20261028/camera-projection-retirement-edit.json';edit=json.loads(edit_path.read_text())
    policy_edit_path=ROOT/'analysis/campaign-native-new-game-20261028/camera-frame-policy-retirement-edit.json';policy_edit=json.loads(policy_edit_path.read_text())
    assert edit['passed']and layout_edit['passed']and previous_edit['passed']and policy_edit['passed']
    assert edit['before_sha256']==layout_edit['after_sha256']and layout_edit['before_sha256']==previous_edit['after_sha256']
    assert previous_edit['before_sha256']=='aed6c7ec072a709d943607f72ceea1d7daa32c4910648c1efc594ea294c60bb9'
    assert policy_edit['before_sha256']==edit['after_sha256']and policy_edit['after_sha256']==sha(source)
    assert policy_edit['selected_address_map_sha256']==edit['selected_address_map_sha256']==hashlib.sha256(mapping.encode()).hexdigest()
    assert len(bodies)==17 and set(policy_edit['retained_bodies'])==set(bodies)and set(policy_edit['changed_retained_parents'])=={'229a90'}
    assert set(edit['retained_bodies'])==set(bodies)|{'2c470'}and set(edit['changed_retained_parents'])=={'13c00','2b9f0'}
    policy_unchanged=[];projection_bodies={}
    for entry,body in bodies.items():
        pins=policy_edit['retained_bodies'][entry];assert pins['after_sha256']==hashlib.sha256(body.encode()).hexdigest()
        prior=body
        for bridge in policy_edit['exact_bridges']:prior=prior.replace(bridge['after'],bridge['before'])
        assert hashlib.sha256(prior.encode()).hexdigest()==pins['before_sha256']==edit['retained_bodies'][entry]['after_sha256']
        projection_bodies[entry]=prior
        if entry!='229a90':assert prior==body;policy_unchanged.append(entry)
    assert len(policy_unchanged)==16 and set(policy_edit['removed_bodies'])=={'2c470'}
    removed=policy_edit['removed_bodies']['2c470']['sha256']
    assert removed==edit['retained_bodies']['2c470']['after_sha256']==layout_edit['retained_bodies']['2c470']['after_sha256']==previous_edit['retained_bodies']['2c470']['after_sha256']
    assert previous_edit['selected_address_map_sha256']==layout_edit['selected_address_map_sha256']==edit['selected_address_map_sha256']
    unchanged_bodies=[];inverse_parents=[]
    for entry,body in projection_bodies.items():
        pins=edit['retained_bodies'][entry];assert pins['after_sha256']==hashlib.sha256(body.encode()).hexdigest()
        assert layout_edit['retained_bodies'][entry]['after_sha256']==pins['before_sha256']
        assert previous_edit['retained_bodies'][entry]['after_sha256']==layout_edit['retained_bodies'][entry]['before_sha256']
        if entry in edit['changed_retained_parents']:
            prior=body
            for bridge in edit['exact_bridges']:prior=prior.replace(bridge['after'],bridge['before'])
            assert prior!=body and hashlib.sha256(prior.encode()).hexdigest()==pins['before_sha256'];inverse_parents.append(entry)
        else:assert pins['before_sha256']==pins['after_sha256'];unchanged_bodies.append(entry)
    assert len(unchanged_bodies)==15 and len(inverse_parents)==2
    # Reconstruct the prior layout parent from the latest unchanged body too;
    # source hashes cannot hide an unrelated body rewrite between receipts.
    for entry in layout_edit['changed_retained_parents']:
        prior=projection_bodies[entry]
        for bridge in layout_edit['exact_bridges']:prior=prior.replace(bridge['after'],bridge['before'])
        assert hashlib.sha256(prior.encode()).hexdigest()==layout_edit['retained_bodies'][entry]['before_sha256']
    for entry,pins in edit['removed_bodies'].items():assert layout_edit['retained_bodies'][entry]['after_sha256']==pins['sha256']
    for entry,pins in layout_edit['removed_bodies'].items():assert previous_edit['retained_bodies'][entry]['after_sha256']==pins['sha256']
    authenticated=[]
    calls=((0x229a90,0x229b95,0x2f7e0,0x229b9a),(0x229a90,0x229c30,0x2f7e0,0x229c35),
        (0x2b770,0x2b7e6,0x2b700,0x2b7eb),(0x2b770,0x2b8b4,0x2f950,0x2b8b9),
        (0x13c00,0x13c8a,0x2f950,0x13c8f),(0x229a90,0x229b44,0x2f5e0,0x229b49),
        (0x229a90,0x229b5f,0x2f620,0x229b64),(0x229a90,0x229b7d,0x2f690,0x229b82),
        (0x13c00,0x13c9b,0x2fd70,0x13ca0),(0x2b9f0,0x2bab7,0x2fd70,0x2babc),
        (0x2b9f0,0x2bafc,0x2fd70,0x2bb01),(0x229a90,0x229b9e,0x2c470,0x229ba3))
    for owner_entry,site,target,returned in calls:
        owner=complete_body(code,owner_entry);ins=code.at(site)
        assert site in owner.code and ins.mnemonic=='call'and ins.operands[0].imm==target and site+ins.size==returned
        assert (owner_entry,site,target)in values['CALLS']
        exact=f'if(s.services.instruction){{s.services.instruction(0x{site:x}u);}} s.push(0x{site:x}u,0x{returned:x}u);s.required(0x{site:x}u,0x{target:x}u,false);'
        assert text.count(exact)==1
        authenticated.append(dict(owner=f'{owner_entry:08x}',site=f'{site:08x}',original_bytes=ins.bytes.hex(),target=f'{target:08x}',real_return=f'{returned:08x}',bridge=exact))
    fixture=ROOT/'tests/fixtures/native_tu15_render_camera_packet.json.gz'
    import gzip
    reference=json.loads(gzip.decompress(fixture.read_bytes()));nulls=[row for row in reference['cases']if row['name']=='default_camera_required']
    assert len(nulls)==1 and not nulls[0]['completed']and nulls[0]['boundary']==dict(phase='default_camera_required',instruction=0x2f8a5)
    assert 'NativeTu15RenderCameraDefaultRequired'in (ROOT/'tests/native_tu15_render_camera_packet_tests.cpp').read_text()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
    files=['src/rendering/world/source_texture_camera_producer_tu15_generated.cpp','src/rendering/world/native_tu15_render_camera_packet.cpp','src/rendering/world/native_tu15_render_camera_packet_binding.cpp','cmake/NativeTu15RenderCameraPacket.cmake','tests/native_tu15_render_camera_packet_tests.cpp','tests/fixtures/native_tu15_render_camera_packet.json.gz','tools/test_native_campaign_screen_lifecycle_tu15.py']
    modules=['cmake/NativeTu15RenderCameraPacket.cmake','cmake/NativeTu15CameraBspSelection.cmake','cmake/CampaignNativeViewRectangle.cmake','cmake/NativeTu15CameraViewLayout.cmake','cmake/NativeTu15CameraProjection.cmake','cmake/NativeTu15CameraFramePolicy.cmake']
    stems=['native_tu15_render_camera_packet','native_tu15_camera_bsp_selection','native_campaign_view_rectangle_tu15','native_tu15_camera_view_layout','native_tu15_camera_projection','native_tu15_camera_frame_policy']
    fixtures=['native_tu15_render_camera_packet.json.gz','native_tu15_camera_bsp_selection.json.gz','native_campaign_view_rectangle_tu15_v2.json.gz','native_tu15_camera_view_layout_v2.json.gz','native_tu15_camera_projection_v2.json.gz','native_tu15_camera_frame_policy.json.gz']
    for stem,fixture_name,module in zip(stems[1:],fixtures[1:],modules[1:]):
        files.extend([f'src/rendering/world/{stem}.cpp',f'src/rendering/world/{stem}_binding.cpp',module,
            f'tests/{stem}_tests.cpp',f'tests/fixtures/{fixture_name}'])
    for name in fixtures:
        original=json.loads(gzip.decompress((ROOT/'tests/fixtures'/name).read_bytes()))
        assert original['image_sha256']==NEW_PIN and original['instruction_patches']==[] and not original['supplied_original_outcomes']
    main_cmake=(ROOT/'CMakeLists.txt').read_text();world=(ROOT/'src/physics/physics_live_world_tu15.cpp').read_text().split('void required_entry_original(',1)[1]
    for module in modules:assert main_cmake.count('include('+module+')')==1
    for owns,run in [('native_tu15_render_camera_packet_owns_entry','native_tu15_render_camera_packet_run'),
        ('native_tu15_camera_bsp_selection_owns_entry','native_tu15_camera_bsp_selection_run'),
        ('native_campaign_view_rectangle_owns_entry_tu15','native_campaign_view_rectangle_run_tu15'),
        ('native_tu15_camera_view_layout_owns_entry','native_tu15_camera_view_layout_run')]:
        assert f'{owns}(c.target)'in world and f'{run}(m,c.target,c.instruction,a,o);return;'in world
    assert 'native_tu15_camera_projection_owns_call(c.target,c.instruction)'in world
    assert 'native_tu15_camera_projection_run(m,c.target,c.instruction,a,o);return;'in world
    assert 'native_tu15_camera_projection_owns_entry(c.target)'not in world
    assert world.index('native_tu15_camera_projection_owns_call')<world.index('source_texture_camera_producer_tu15_owns_entry')
    assert world.count('if(c.target==0x2c470u && c.instruction==0x229b9eu)')==1
    assert world.count('native_tu15_camera_frame_policy_run(m,c.target,c.instruction,a,o);return;')==1
    assert 'native_tu15_camera_frame_policy_owns_entry(c.target)'not in world
    assert world.index('native_tu15_camera_frame_policy_run')<world.index('source_texture_camera_frame_call_tu15')
    #25940 remains a legacy retained-row owner except its existing precise
    #258F7 Renderer capture service. No raw nativequery may shadow that owner.
    assert policy_edit['retained_bodies']['25940']['before_sha256']==policy_edit['retained_bodies']['25940']['after_sha256']
    assert 's.required(0x258f7u,0x25940u,false);'in bodies['25800']
    assert 'if(c.target==0x380450u || (c.target==0x2f7e0u && a.gp[0]==0u))'in world
    assert 'native_tu15_render_camera_default_run(m,c.target,c.instruction,a,o);return;'in world
    assert main_cmake.count('include(cmake/NativeTu15RenderCameraDefault.cmake)')==1
    assert main_cmake.count('include(cmake/CampaignNativeAiAwarenessBinding.cmake)')==1
    assert 'native_campaign_ai_awareness_tu15_run(m,c,a,o);return;'in world
    simulation=(ROOT/'src/gameplay/simulation.cpp').read_text()
    assert '#include "halo2/native_tu15_camera_projection_binding.hpp"'in simulation
    assert '" native_camera_projection=" << native_tu15_camera_projection_calls()'in simulation
    assert '!/tests/fixtures/native_tu15_camera_projection_v2.json.gz'in (ROOT/'.gitignore').read_text()
    assert '#include "halo2/native_tu15_camera_frame_policy_binding.hpp"'in simulation
    assert '" native_camera_frame_policy=" << native_tu15_camera_frame_policy_calls()'in simulation
    assert '!/tests/fixtures/native_tu15_camera_frame_policy.json.gz'in (ROOT/'.gitignore').read_text()
    default_proof=ROOT/'analysis/campaign-native-new-game-20261028/packet-default/component-proof.json'
    default=json.loads(default_proof.read_text());assert default['passed']
    for section in('source_hashes','sealed_nonnull_source_hashes'):
        for name,pin in default[section].items():assert sha(ROOT/name)==pin,name
    projection_proof=ROOT/'analysis/campaign-native-new-game-camera-20261028/projection-proof.json'
    projection=json.loads(projection_proof.read_text());assert projection['passed']
    for name,pin in projection['source_hashes'].items():assert sha(ROOT/name)==pin,name
    policy_proof=ROOT/'analysis/campaign-native-new-game-20261028/frame-policy/component-proof.json'
    policy=json.loads(policy_proof.read_text());assert policy['passed']
    for name,pin in policy['source_hashes'].items():assert sha(ROOT/name)==pin,name
    assert 'add_test(NAME campaign_transparent_resident_owner COMMAND campaign_transparent_resident_owner_tests'in main_cmake
    for name in files:
        destination=project/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,destination)
    (project/'ownership.cpp').write_text('''#include "halo2/source_texture_camera_producer_tu15.hpp"
#include "halo2/native_tu15_render_camera_packet_binding.hpp"
#include "halo2/native_tu15_camera_bsp_selection_binding.hpp"
#include "halo2/native_campaign_view_rectangle_tu15_binding.hpp"
#include "halo2/native_tu15_camera_view_layout_binding.hpp"
#include "halo2/native_tu15_camera_projection_binding.hpp"
#include "halo2/native_tu15_camera_frame_policy_binding.hpp"
#include <iostream>
#include <stdexcept>
struct Memory final:halo2::SourcePhysicsContactMemory {
 mutable unsigned reads{};unsigned writes{};
 void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw std::runtime_error("Unexpected admission read");}
 void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Unexpected admission write");}
};
template<class Call>void reject(Call call){bool denied{};try{call();}catch(const std::invalid_argument&){denied=true;}if(!denied)throw std::runtime_error("Unqualified native admission succeeded");}
int main(){try{
 for(auto target:{0x2f7e0u,0x2b700u,0x2f950u,0x2f5e0u,0x2f620u,0x2f690u,0x2fb50u,0x2fd70u,0x2c470u})
  if(halo2::source_texture_camera_producer_tu15_owns_entry(target))return 1;
 for(auto target:{PUBLIC_ENTRIES})
  if(!halo2::source_texture_camera_producer_tu15_owns_entry(target))return 2;
 for(auto target:{0x2b770u,0x2b9f0u,0x13c00u})
  if(halo2::source_texture_camera_producer_tu15_owns_entry(target))return 3;
 Memory memory;halo2::SourcePhysicsWorldRuntimeAbi abi{};
 if(!halo2::native_tu15_render_camera_packet_owns_entry(0x2f7e0u)||
    !halo2::native_tu15_camera_bsp_selection_owns_entry(0x2b700u)||
    !halo2::native_campaign_view_rectangle_owns_entry_tu15(0x2f950u)||
    !halo2::native_tu15_camera_view_layout_owns_entry(0x2f5e0u)||
    !halo2::native_tu15_camera_view_layout_owns_entry(0x2f620u)||
    !halo2::native_tu15_camera_view_layout_owns_entry(0x2f690u)||
    !halo2::native_tu15_camera_projection_owns_call(0x2fd70u,0x13c9bu)||
    !halo2::native_tu15_camera_projection_owns_call(0x2fd70u,0x2bab7u)||
    !halo2::native_tu15_camera_projection_owns_call(0x2fd70u,0x2bafcu)||
    halo2::native_tu15_camera_projection_owns_call(0x144090u,0x2ffe9u)||
    halo2::native_tu15_camera_projection_owns_entry(0x144090u)||
    halo2::native_tu15_camera_projection_owns_call(0x2fd70u,0)||
    !halo2::native_tu15_camera_frame_policy_owns_entry(0x2c470u)||
    !halo2::source_texture_camera_producer_tu15_owns_entry(0x25940u))return 4;
 reject([&]{halo2::native_tu15_render_camera_packet_run(memory,0,0x229b95,abi);});
 reject([&]{halo2::native_tu15_camera_bsp_selection_run(memory,0,0x2b7e6,abi);});
 reject([&]{halo2::native_campaign_view_rectangle_run_tu15(memory,0,0x2b8b4,abi);});
 reject([&]{halo2::native_tu15_camera_view_layout_run(memory,0,0x229b44,abi);});
 reject([&]{halo2::native_tu15_render_camera_packet_run(memory,0x2f7e0,0,abi);});
 reject([&]{halo2::native_tu15_camera_bsp_selection_run(memory,0x2b700,0,abi);});
 reject([&]{halo2::native_campaign_view_rectangle_run_tu15(memory,0x2f950,0,abi);});
 for(auto target:{0x2f5e0u,0x2f620u,0x2f690u})reject([&]{halo2::native_tu15_camera_view_layout_run(memory,target,0,abi);});
 reject([&]{halo2::native_tu15_camera_projection_run(memory,0,0x2bab7,abi);});
 reject([&]{halo2::native_tu15_camera_projection_run(memory,0x2fd70,0,abi);});
 for(auto site:{0x13c9bu,0x2bab7u,0x2bafcu})reject([&]{halo2::native_tu15_camera_projection_run(memory,0x2fd70,site,abi);});
 for(auto site:{0x229b95u,0x229c30u})reject([&]{halo2::native_tu15_render_camera_packet_run(memory,0x2f7e0,site,abi);});
 reject([&]{halo2::native_tu15_camera_bsp_selection_run(memory,0x2b700,0x2b7e6,abi);});
 for(auto site:{0x2b8b4u,0x13c8au})reject([&]{halo2::native_campaign_view_rectangle_run_tu15(memory,0x2f950,site,abi);});
 reject([&]{halo2::native_tu15_camera_frame_policy_run(memory,0x2c470,0,abi);});
 reject([&]{halo2::native_tu15_camera_frame_policy_run(memory,0x25940,0x258f7,abi);});
 if(memory.reads||memory.writes)return 5;
 std::cout<<"Nine retired entries unowned; all14 public and three private gates unchanged; exact2C470 productioncaller;25940 remains legacy/capture-owned; no global144090 interception; native admission rejects before memory; integer layout has no FP-policy requirement\\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\\n';return 1;}}
'''.replace('PUBLIC_ENTRIES',','.join(f'0x{p:x}u'for p in sorted(public))))
    (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(NativeRenderCameraRetirement LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_library(halo2_physics_world_runtime OBJECT)
target_include_directories(halo2_physics_world_runtime PRIVATE include)
target_compile_features(halo2_physics_world_runtime PRIVATE cxx_std_20)
include(cmake/NativeTu15RenderCameraPacket.cmake)
include(cmake/NativeTu15CameraBspSelection.cmake)
include(cmake/CampaignNativeViewRectangle.cmake)
include(cmake/NativeTu15CameraViewLayout.cmake)
include(cmake/NativeTu15CameraProjection.cmake)
include(cmake/NativeTu15CameraFramePolicy.cmake)
add_library(retired_producer OBJECT src/rendering/world/source_texture_camera_producer_tu15_generated.cpp)
target_include_directories(retired_producer PRIVATE include "{ROOT/'src'}")
target_compile_features(retired_producer PRIVATE cxx_std_20)
target_compile_options(retired_producer PRIVATE -ffunction-sections -fdata-sections -fno-fast-math -ffp-contract=off -frounding-math)
add_executable(packet_retirement_ownership ownership.cpp $<TARGET_OBJECTS:retired_producer> $<TARGET_OBJECTS:halo2_physics_world_runtime>)
target_include_directories(packet_retirement_ownership PRIVATE include)
target_compile_features(packet_retirement_ownership PRIVATE cxx_std_20)
target_link_options(packet_retirement_ownership PRIVATE -Wl,--gc-sections)
add_test(NAME packet_retirement_ownership COMMAND packet_retirement_ownership)
''')
    files.extend([f'include/halo2/{stem}{suffix}.hpp'for stem in stems for suffix in('','_binding')])
    files.extend(['CMakeLists.txt','src/physics/physics_live_world_tu15.cpp','src/gameplay/simulation.cpp','.gitignore',str(edit_path.relative_to(ROOT)),str(layout_path.relative_to(ROOT)),str(previous_path.relative_to(ROOT)),str(policy_edit_path.relative_to(ROOT)),str(default_proof.relative_to(ROOT)),str(projection_proof.relative_to(ROOT)),str(policy_proof.relative_to(ROOT))])
    files.extend(default['source_hashes']);files.extend(default['sealed_nonnull_source_hashes'])
    pins={name:sha(ROOT/name)for name in files};pins[str(generator.relative_to(ROOT))]=sha(generator);pins[str(Path(__file__).relative_to(ROOT))]=sha(__file__)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/'cmake')]
    for target in [*(stem+'_tests'for stem in stems),'packet_retirement_ownership']:command.extend(['--target',target])
    command.extend(['--filter','^(native_tu15_render_camera_packet|native_tu15_camera_bsp_selection|native_campaign_view_rectangle_tu15|native_tu15_camera_view_layout|native_tu15_camera_projection|native_tu15_camera_frame_policy|packet_retirement_ownership)$','--jobs','2','--test-jobs','1','--retain-report','Testing/Temporary/LastTest.log'])
    result=subprocess.run(command,check=False);unchanged=all(sha(ROOT/name)==pin for name,pin in pins.items())
    report=dict(passed=result.returncode==0 and unchanged,scope=__doc__,source_sha256=pins,source_unchanged=unchanged,command=command,exit_code=result.returncode,authenticated_calls=authenticated,compiled_local_bodies=17,retained_public_entries=14,retired_original_sites={'2f7e0':109,'2b700':37,'2f950':124,'2f5e0':28,'2f620':43,'2f690':102,'2fb50':138,'2fd70':315,'2c470':35},retired_local_body_sites=931,typed_internal_inverse_sites=82,unchanged_other_bodies=policy_unchanged,bridge_only_parents_verified_by_inverse_edit=['229a90'],prior_projection_inverse_parents=inverse_parents,previous_retirement_receipt_chain_verified=True,selected_mapping_sha256=hashlib.sha256(mapping.encode()).hexdigest(),retirement_edit_receipts=[str(previous_path.relative_to(ROOT)),str(layout_path.relative_to(ROOT)),str(edit_path.relative_to(ROOT)),str(policy_edit_path.relative_to(ROOT))],canonical_packet_null_boundary_sites=35,additive_native_default_proof_pins_match=True,additive_native_default_route_preserved=True,retained_null_reference_cases=len(nulls),null_projection_silently_skipped=False,universal_inverse_intercepted=False,frame_policy_exact_caller_only=True,target_query_production_routing_added=False,shadow_frame_duplicate_retired=False,transparent_owner_test_registration_preserved=True,original_fixture_sha256={f:sha(ROOT/'tests/fixtures'/f)for f in fixtures},original_fixtures_or_frozen_sources_changed=False,owned_scratch=str(scratch),retained_large_artifacts=[],runtime_or_GPU_qualified=False,whole_native_camera_parent=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])
if __name__=='__main__':raise SystemExit(main())
