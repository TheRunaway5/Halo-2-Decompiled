#!/usr/bin/env python3
"""Exercise the authored Pelican attack constructor with the active native cohort.

Only the diagnostic caller fixture differs. Native libraries and engine headers
are borrowed read-only and pinned. All builds live in tools/run_test.py scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

from qualify_live_damage_material_tu15 import native_link_inputs,sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',required=True,type=Path)
    parser.add_argument('--inputs',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--campaign-unit-readback-candidate',action='store_true',
        help='Compile the isolated one-line native player readback correction')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve()
    admission=json.loads(args.admission.read_text())
    frozen=Path(admission['owner']).resolve(strict=True);inputs=frozen/'source';build=frozen/'build'
    if any(sha(inputs/name)!=row['after']for name,row in admission['changes'].items()):
        raise ValueError('Native source cohort drifted')
    authored=json.loads(args.inputs.read_text())
    if authored.get('passed')is not True or authored.get('mcc')!=authored.get('original'):
        raise ValueError('Pelican original/MCC constructor inputs are unqualified')
    map_path=Path(authored['source_owner']['mcc_path']).resolve(strict=True)
    if sha(map_path)!=authored['source_owner']['mcc_sha256']:raise ValueError('Pelican MCC input drifted')
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(inputs/fixture).read_text();after=before
    changes=[
        ('#include "halo2/multiplayer_world.hpp"\n',
         '#include "halo2/multiplayer_world.hpp"\n#include "helpers/pelican_children_constructor_tu15.hpp"\n'),
        ('  const bool campaign=name=="campaign_c_turret_ap"',
         '  const bool campaign=name=="campaign_pelican_children" || name=="campaign_c_turret_ap"'),
        ('  const auto campaign_parent_suffix=name=="campaign_scorpion_cannon"?',
         '  const auto campaign_parent_suffix=name=="campaign_pelican_children"?std::string("pelican"):name=="campaign_scorpion_cannon"?'),
        ('      if(!campaign_no_damage)\n',
         '''      if(name=="campaign_pelican_children") {
        // Scripted aircraft can be outside walkable BSP geometry. Reuse the
        // existing isolated vehicle-variant fixture's independent authored
        // observer survey; this selects caller inputs, never a birth result.
        std::vector<ScenarioObject> candidates;
        for(const auto &placement:content.objects)if(placement.type==1 && placement.placement>=0 &&
            placement.origin_bsp>=0 && !placement.model_attachment)candidates.push_back(placement);
        std::stable_partition(candidates.begin(),candidates.end(),[&](const auto &placement){
          return placement.definition==campaign_placement->definition;
        });
        report.data["pelican_observer_survey"]=Json::array();
        auto loaded_bsp=options.bsp;
        for(const auto &placement:candidates) {
          options.bsp=std::size_t(placement.origin_bsp);
          if(loaded_bsp!=options.bsp) {content=cache.load_content(options.bsp,false);loaded_bsp=options.bsp;}
          Json row{{"bsp",options.bsp},{"placement",placement.placement},{"definition",placement.definition}};
          std::array<test::CampaignObserverStartProof,1> proof;
          try {
            const auto result=test::campaign_vehicle_observer_starts<1>(cache,content,options.bsp,placement,&proof);
            row.update({{"admitted",true},{"triangle",proof[0].triangle},{"observer_distance",proof[0].observer_distance}});
            report.data["pelican_observer_survey"].push_back(row);report.data["declared_observer_bsp"]=options.bsp;
            return result;
          } catch(const FormatError &error) {
            if(std::string_view(error.what())!="Authored vehicle has no clear nearby cached BSP observer start")throw;
            row.update({{"admitted",false},{"reason",error.what()}});report.data["pelican_observer_survey"].push_back(row);
          }
        }
        throw FormatError("No authored campaign vehicle has a clear Pelican constructor observer");
      }
      if(!campaign_no_damage)
'''),
        ('  if(name=="coil_to_warthog" || name=="warthog_to_coil"){',
         '''  if(name=="campaign_pelican_children") {
    destruction_continuation(world,commands,report,"pre_constructor_continuation",true);
    const auto original_player_unit=world.match.state().players[slot].source_unit;
    const auto player_state=[&](std::string_view phase) {
      const auto state=world.match.state().players[slot];
      const auto view=source.first_person_view(0);
      Json row{{"phase",phase},{"current_unit",state.source_unit},{"view_unit",view.unit},{"dead",state.dead}};
      const auto pool=Bytes(source.memory().view(0x4f54f4u,4)).u32(0);
      const SourceRawDatumPoolServices player_storage{.read=[&](auto at,auto size){return source.memory().view(at,size);}};
      const auto record=source_raw_datum_lookup(pool,state.source_player,player_storage);
      check(record!=0,"Constructor player observation lost the original record");
      row["raw_current_unit"]=Bytes(source.memory().view(record+0x2cu,4)).u32(0);
      row["raw_previous_unit"]=Bytes(source.memory().view(record+0x30u,4)).u32(0);
      for(const auto &object:source.live_objects())if(object.object==view.unit) {
        row["view_unit_type"]=object.type;row["view_unit_parent"]=object.parent;
        const auto definition=cache.query_tag({.handle=TagHandle{object.definition}});
        if(definition)row["view_unit_definition"]=definition->name;
      }
      if(std::ranges::any_of(source.live_objects(),[&](const auto &object){return object.object==original_player_unit;})) {
        const auto vitality=source.object_vitality(original_player_unit);
        row.update({{"original_unit_body",vitality.body},{"original_unit_shield",vitality.shield}});
      }
      report.data["constructor_player_state"].push_back(row);
    };
    player_state("before_birth");
    const auto position=world.match.state().players[slot].position+Vec3{0,0,.5f};
    const auto parent=test::pelican_children_constructor_tu15(world,cache,*campaign_placement,position,report.data);
    report.save();
    for(unsigned tick=0;tick<8;++tick) {world.match.step(commands);player_state("after_ordinary_frame");}
    const auto objects=source.live_objects();
    for(const auto &child:report.data.at("actual_children")) {
      const auto handle=child.at("object").get<std::uint32_t>();
      check(std::ranges::any_of(objects,[&](const auto &object){return object.object==handle && object.parent==parent &&
          object.definition==child.at("definition").get<std::uint32_t>();}),
          "Original world retired or reparented the Pelican child during the construction probe");
    }
    report.data["ordinary_frames_after_construction"]=8;
    report.save();
    try {destruction_continuation(world,commands,report,"post_constructor_continuation");}
    catch(...) {player_state("continuation_failure");report.save();throw;}
    player_state("after_continuation");
    const auto &final_player=report.data["constructor_player_state"].back();
    check(final_player.at("current_unit")==final_player.at("raw_current_unit") &&
        final_player.at("current_unit")==final_player.at("view_unit"),
        "Completed original player birth lost its native match or first-person binding");
    check(final_player.at("current_unit").get<std::uint32_t>()!=null_datum &&
        final_player.at("current_unit").get<std::uint32_t>()!=original_player_unit &&
        final_player.at("original_unit_body").get<float>()<=0,
        "Constructor regression did not traverse its actual source death and respawn");
    report.data["source_death_and_respawn_readback_qualified"]=true;
    report.data["passed"]=true;
    std::cout<<"PASS original Pelican attack construction and complete-world continuation; damage gate unqualified\\n";
    return 0;
  }
  if(name=="coil_to_warthog" || name=="warthog_to_coil"){''')]
    for needle,replacement in changes:
        if after.count(needle)!=1:raise ValueError('Pelican diagnostic caller seam changed')
        after=after.replace(needle,replacement)
    source=owner/'source';target=source/fixture;target.parent.mkdir(parents=True);target.write_text(after)
    helper='tests/helpers/pelican_children_constructor_tu15.hpp'
    helper_target=source/helper;helper_target.parent.mkdir(parents=True);helper_target.write_bytes((ROOT/helper).read_bytes())
    libraries,pins=native_link_inputs(build)
    security=build/'online-protocol/Release/libhalo2_online_security_ffi.so';pins[str(security)]=sha(security)
    headers={str(p):sha(p)for parent in ('include','src','tests/helpers')for p in (inputs/parent).rglob('*.hpp')if p.is_file()}
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2PelicanChildrenProbe LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(probe tests/destruction_runtime_tu15_tests.cpp)
target_compile_features(probe PRIVATE cxx_std_20)
target_compile_options(probe PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(probe PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+='target_include_directories(probe PRIVATE '+' '.join('"'+str(inputs/p)+'"'for p in ('include','src','tests'))+' /usr/include/SDL2)\n'
    cmake+='target_link_libraries(probe PRIVATE '+' '.join('"'+token+'"'for token in libraries)+')\n'
    engine_candidate=None
    link_map=owner/'build/probe.map'
    if args.campaign_unit_readback_candidate:
        name='src/gameplay/multiplayer/multiplayer.cpp'
        original=(inputs/name).read_text()
        needle='    if(source_engine_owned_)adopt_source_player_units();\n'
        if original.count(needle)!=1:raise ValueError('Native player readback seam changed')
        candidate=original.replace(needle,'    adopt_source_player_units();\n')
        path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(candidate)
        flags_file=build/'CMakeFiles/halo2_systems.dir/flags.make'
        flags={}
        for line in flags_file.read_text().splitlines():
            for kind in ('CXX_FLAGS','CXX_DEFINES','CXX_INCLUDES'):
                if line.startswith(kind+' = '):flags[kind]=shlex.split(line.split(' = ',1)[1])
        options=flags['CXX_FLAGS']+flags['CXX_DEFINES']+flags['CXX_INCLUDES']
        if any(any(c in token for c in '\n\r";$')for token in options):raise ValueError('Unexpected native compile flags')
        cmake+='add_library(campaign_unit_readback OBJECT '+name+')\n'
        cmake+='target_compile_options(campaign_unit_readback PRIVATE '+' '.join('"'+token+'"'for token in options)+')\n'
        cmake+='target_sources(probe PRIVATE $<TARGET_OBJECTS:campaign_unit_readback>)\n'
        cmake+='target_link_options(probe PRIVATE "-Wl,-Map,'+str(link_map)+'")\n'
        engine_candidate=dict(source=name,before_sha256=hashlib.sha256(original.encode()).hexdigest(),
            after_sha256=sha(path),before=needle,after='    adopt_source_player_units();\n',
            original_flags_file_sha256=sha(flags_file),compile_options=options,
            source_results_supplied=False,all_other_engine_libraries_borrowed_unchanged=True)
    cmake+=f'add_test(NAME native_pelican_children_constructor COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:probe> "{map_path}" "{map_path.parent}" campaign_pelican_children "${{CMAKE_BINARY_DIR}}/pelican-children.json")\n'
    cmake+=f'set_tests_properties(native_pelican_children_constructor PROPERTIES TIMEOUT 360 ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;H2_SECURITY_LIBRARY={security}")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,owned_scratch=str(owner),native_owner=str(frozen),tool_sha256=sha(__file__),
        fixture_original_sha256=hashlib.sha256(before.encode()).hexdigest(),fixture_probe_sha256=sha(target),
        diagnostic_caller_substitutions=changes,helper_sha256=sha(helper_target),
        native_libraries_sha256=pins,native_headers_sha256=headers,authored_input_report_sha256=sha(args.inputs),
        mcc_map_sha256=sha(map_path),engine_libraries_changed=False,damage_or_invulnerability_qualified=False,
        engine_candidate=engine_candidate,full_goal_complete=False,retained_large_artifacts=[])
    def save():(args.output/'probe.json').write_text(json.dumps(report,indent=2)+'\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','probe',
        '--filter','^native_pelican_children_constructor$','--jobs','1','--test-jobs','1','--output',str(args.output/'validation'),
        '--retain-report','pelican-children.json']
    report['command']=command;save()
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(returncode=result,native_libraries_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in pins.items()),
        native_headers_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in headers.items()),
        mcc_map_unchanged=sha(map_path)==report['mcc_map_sha256'],helper_unchanged=sha(helper_target)==report['helper_sha256'])
    proof=args.output/'validation/evidence/pelican-children.json'
    native=json.loads(proof.read_text())if proof.exists()else{}
    continuation=native.get('post_constructor_continuation',{})
    if engine_candidate:
        text=link_map.read_text()if link_map.exists()else''
        report['engine_candidate']['old_multiplayer_archive_member_absent']='libhalo2_systems.a(multiplayer.cpp.o)'not in text and bool(text)
        report['engine_candidate']['replacement_object_linked']='CMakeFiles/campaign_unit_readback.dir/src/gameplay/multiplayer/multiplayer.cpp.o'in text
        report['engine_candidate']['link_map_sha256']=sha(link_map)if link_map.exists()else None
    report['passed']=result==0 and native.get('passed')is True and native.get('construction_only')is True and (
        continuation.get('passed')is True and continuation.get('complete_encoded_world_sha256_comparisons')==16)and all(
            report[key]for key in ('native_libraries_unchanged','native_headers_unchanged','mcc_map_unchanged','helper_unchanged'))
    if engine_candidate:report['passed'] &= all(report['engine_candidate'][key]for key in ('old_multiplayer_archive_member_absent','replacement_object_linked'))
    save();print(json.dumps(dict(passed=report['passed'],report=str(args.output/'probe.json'))),flush=True)
    return 0 if report['passed']else result or 1


if __name__=='__main__':raise SystemExit(main())
