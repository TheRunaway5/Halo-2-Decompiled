#!/usr/bin/env python3
"""Observe ordinary rocket damage and the actual Pelican parent fatal path.

The authored damage contract has no instant responses. Do not require a
particular region/debris effect; require the actual source death/retirement
transition and retain failures when original negative gates prevent it.
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


def fixture_source(before):
    edits=[
        ('  const bool pelican_damage=name=="campaign_pelican_rear_gun" || name=="campaign_pelican_rocket_pod";\n',
         '  const bool pelican_parent_case=name=="campaign_pelican_parent";\n  const bool pelican_damage=pelican_parent_case || name=="campaign_pelican_rear_gun" || name=="campaign_pelican_rocket_pod";\n'),
        ('  const bool campaign_no_damage=name=="campaign_guntower_no_damage" || pelican_damage;\n',
         '  const bool campaign_no_damage=name=="campaign_guntower_no_damage" || (pelican_damage && !pelican_parent_case);\n'),
        ('  const bool campaign=name=="campaign_pelican_children" ||',
         '  const bool campaign=pelican_parent_case || name=="campaign_pelican_children" ||'),
        ('      if(definition && definition->name==attached->second)candidates.push_back(child.object);\n',
         '      if(definition && attached!=attached_targets.end() && definition->name==attached->second)candidates.push_back(child.object);\n'),
        ('    check(candidates.size()==(name=="campaign_pelican_rocket_pod"?2u:1u),"Actual attack attachment count changed");\n',
         '    if(pelican_parent_case)candidates.push_back(pelican_parent);\n    check(candidates.size()==(name=="campaign_pelican_rocket_pod"?2u:1u),"Actual attack family target count changed");\n'),
        ('  if(campaign_no_damage) {\n    const auto aim_definition=',
         '  if(campaign_no_damage || pelican_parent_case) {\n    const auto aim_definition='),
        ('if(capture_model || campaign_scenery || campaign_no_damage || name=="wraith_minigun")campaign_query_observation',
         'if(capture_model || campaign_scenery || campaign_no_damage || pelican_parent_case || name=="wraith_minigun")campaign_query_observation'),
        ('  } else\n  if(campaign_scenery) {\n',
         '''  } else if(pelican_parent_case) {
    const auto object=cache.load_source_tag_metadata(target_definition);
    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38u));
    const Bytes head(model.bytes_at(model.source_address(),model.size()));
    check(head.u32(0x60u)==1,"Parent damage-information count differs from original contract");
    const Bytes info(cache.read_tag_bytes(head.u32(0x64u),0xf8));
    check(info.u32(0xbcu)==1 && info.u32(0)==0 && info.u32(0x28)==0x447a0000u,
        "Parent damage inputs differ from original contract");
    const Bytes section(cache.read_tag_bytes(info.u32(0xc0u),0x38));
    check(section.u32(0xc)==0,"Parent instant-response count differs from original contract");
    report.data["parent_contract"]={{"damage_information_count",1},{"section_count",1},{"instant_response_count",0},
        {"reference","78266495e535efacfde78eea3ad128b1ccf8bc90ec76a898d0f6d822ca4a981c"}};
    report.data["actual_original_area_calls"]=Json::array();
    report.data["original_area_attributed_death_stores"]=Json::array();
    report.data["actual_target_damage_requests"]=Json::array();
    report.data["pelican_family_damage_requests"]=Json::array();
    campaign_area_observer=[&](bool before,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi) {
      if(entry!=0xd91d0u)return;
      if(before) {
        const auto request=word(abi.gp[4]+4u),caller=word(abi.gp[4]);
        Json event{{"caller_return",caller},{"request",request},{"damage",word(request)},
            {"source_player",word(request+8u)},{"source_object",word(request+0xcu)}};
        if(caller==0x101b89u) {
          const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return memory.view(at,size);}};
          const auto header=source_raw_datum_lookup(target_pool,abi.gp[6],storage);
          check(header && word(header+8u)==abi.gp[5],"Parent stimulus lost its actual projectile generation/body");
          event["projectile"]=abi.gp[6];event["projectile_definition"]=word(abi.gp[5]);
        }
        active_area_calls.push_back(event);report.data["actual_original_area_calls"].push_back(event);
      } else {
        check(!active_area_calls.empty(),"Parent area observations became unbalanced");active_area_calls.pop_back();
      }
    };
  } else
  if(campaign_scenery) {
'''),
        ('    if(campaign_no_damage && store.instruction==0xda0eeu) {\n',
         '    if((campaign_no_damage || pelican_parent_case) && store.instruction==0xda0eeu) {\n'),
        ('        if(campaign_scenery || named_default_wraith) {\n',
         '        if(campaign_scenery || named_default_wraith || pelican_parent_case) {\n'),
        ('        if(campaign_scenery || named_default_wraith)death.update(',
         '        if(campaign_scenery || named_default_wraith || pelican_parent_case)death.update('),
        ('        if(campaign_scenery && !active_area_calls.empty()) {\n',
         '        if((campaign_scenery || pelican_parent_case) && !active_area_calls.empty()) {\n'),
        ('  for(unsigned tick=0;tick<(pelican_damage?180u:900u);++tick){\n',
         '  for(unsigned tick=0;tick<((pelican_damage && !pelican_parent_case)?180u:900u);++tick){\n'),
        ('  bool pending_qualified=false;\n',
         '  bool pending_qualified=false;\n  unsigned parent_equipment_replacements=0;\n'),
        ('    if(exists)aim();\n',
         '''    if(pelican_parent_case && exists && !dead && state.players[slot].loaded==0 &&
        state.players[slot].reserve==0 && parent_equipment_replacements<2) {
      const auto previous_weapon=state.players[slot].source_weapon;
      const auto replacement=native.equip_weapon(unit,campaign_weapon);
      check(replacement!=null_datum,"Original equipment owners rejected the declared replacement rocket launcher");
      ++parent_equipment_replacements;
      report.data["declared_parent_equipment_replacements"].push_back({{"tick",state.tick},
          {"unit",unit},{"definition",campaign_weapon},{"actual_weapon",replacement},{"previous_weapon",previous_weapon},
          {"original_entries",{0xcf4d0u,0xb9960u,0xb9b70u,0xcf5a0u}},
          {"depleted_loaded",state.players[slot].loaded},{"depleted_reserve",state.players[slot].reserve}});
    }
    if(exists)aim();
'''),
        ('    if(state.players[slot].shot_tick!=previous_shot)++shots;\n    const auto after=source.live_objects();\n',
         '    if(!pelican_parent_case && state.players[slot].shot_tick!=previous_shot)++shots;\n    const auto after=source.live_objects();\n'),
        ('      if(object.type==5 && projectiles.insert(object.object).second){\n',
         '      if(object.type==5 && projectiles.insert(object.object).second){\n        if(pelican_parent_case)++shots;\n'),
        ('  if(campaign_no_damage)campaign_area_observer={};\n',
         '  if(campaign_no_damage || pelican_parent_case)campaign_area_observer={};\n'),
        ('  destruction_continuation(world,commands,report);explicit_cleanup_receipt(world,target,report);\n',
         '''  if(pelican_parent_case) {
    std::map<std::uint32_t,NativeObjectLifetime> tracked;
    for(const auto &event:world.match.simulation().object_lifetimes().events()) {
      if(event.kind==NativeObjectLifetimeEventKind::release)tracked.erase(event.owner.handle);
      else tracked.insert_or_assign(event.owner.handle,event.owner);
    }
    report.data["native_lifetime_unmatched_owners_before_cut"]=Json::array();
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return memory.view(at,size);}};
    for(const auto &[handle,owner]:tracked)if(!source_raw_datum_lookup(target_pool,handle,storage)) {
      const auto slot=word(target_pool+0x44u)+(handle&65535u)*word(target_pool+0x24u);
      const auto bytes=memory.view(slot,2);
      Json mismatch{{"handle",handle},{"definition",owner.definition},{"type",owner.type},
          {"published",owner.published},{"expected_salt",handle>>16},
          {"actual_slot_salt",std::to_integer<unsigned>(bytes[0])|(std::to_integer<unsigned>(bytes[1])<<8)}};
      if(const auto *definition=cache.query_tag({.handle=TagHandle{owner.definition}}))mismatch["definition_name"]=definition->name;
      report.data["native_lifetime_unmatched_owners_before_cut"].push_back(mismatch);
    }
    report.save();
  }
  destruction_continuation(world,commands,report);explicit_cleanup_receipt(world,target,report);
'''),
        ('  } else if(campaign) {\n    check(regions_changed,"Campaign target death did not change its authored model regions");\n',
         '''  } else if(pelican_parent_case) {
    check(active_area_calls.empty(),"Parent fatal stimulus retained an active original area call");
    check(!report.data["actual_target_damage_requests"].empty(),"Parent has no actual target damage request");
    report.data["campaign_acceptance_scope"]="Actual attack parent and ordinary source rockets, parent damage/death or retirement, observed source flags and full family state, exact continuation; no authored response, region or debris result supplied. Other producers, physical transitions and full definition parity remain open";
  } else if(campaign) {
    check(regions_changed,"Campaign target death did not change its authored model regions");
''')]
    after=before
    for needle,replacement in edits:
        if after.count(needle)!=1:raise ValueError('Parent caller seam changed: '+needle[:90])
        after=after.replace(needle,replacement)
    return after,edits


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',required=True,type=Path)
    parser.add_argument('--inputs',required=True,type=Path)
    parser.add_argument('--radial-probe',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--retirement-observer-candidate',action='store_true',
        help='Compile one isolated native readback candidate; leave the borrowed engine unchanged')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);admission=json.loads(args.admission.read_text())
    frozen=Path(admission['owner']).resolve(strict=True);inputs=frozen/'source';build=frozen/'build'
    if any(sha(inputs/name)!=row['after']for name,row in admission['changes'].items()):raise ValueError('Native source drifted')
    contract=json.loads(args.inputs.read_text());radial=json.loads(args.radial_probe.read_text())
    if not contract['passed']or contract['contract_digest']!='78266495e535efacfde78eea3ad128b1ccf8bc90ec76a898d0f6d822ca4a981c'or not radial['passed']:
        raise ValueError('Parent contract or source radial stimulus unqualified')
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(inputs/fixture).read_text();after,edits=fixture_source(before)
    source=owner/'source';target=source/fixture;target.parent.mkdir(parents=True);target.write_text(after)
    libraries,pins=native_link_inputs(build);security=build/'online-protocol/Release/libhalo2_online_security_ffi.so';pins[str(security)]=sha(security)
    headers={str(p):sha(p)for parent in ('include','src','tests/helpers')for p in (inputs/parent).rglob('*.hpp')if p.is_file()}
    map_path=Path(json.loads((args.radial_probe.parent/'validation/evidence/campaign_pelican_rocket_pod.json').read_text())['map']).resolve(strict=True)
    if sha(map_path)!=radial['mcc_map_sha256']:raise ValueError('Radial MCC map drifted')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2PelicanParentProbe LANGUAGES CXX)
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
    if args.retirement_observer_candidate:
        name='src/gameplay/multiplayer/multiplayer.cpp'
        original=(inputs/name).read_text()
        anchor='  // Original14EFF0 follows object updates and calls153920 only for a current\n'
        addition='  if(simulation_.source_physics_movement_)simulation_.object_lifetimes_.observe_source_retirements();\n'
        if original.count(anchor)!=1 or addition in original:raise ValueError('Native retirement readback seam changed')
        path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(original.replace(anchor,addition+anchor))
        flags_file=build/'CMakeFiles/halo2_systems.dir/flags.make';flags={}
        for line in flags_file.read_text().splitlines():
            for kind in ('CXX_FLAGS','CXX_DEFINES','CXX_INCLUDES'):
                if line.startswith(kind+' = '):flags[kind]=shlex.split(line.split(' = ',1)[1])
        options=flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']
        if any(any(c in token for c in '\n\r";$')for token in options):raise ValueError('Unexpected native compile flags')
        cmake+='add_library(retirement_readback OBJECT '+name+')\n'
        # One SHELL group preserves repeated -isystem options through CMake's
        # compile-option deduplication, retaining the actual native invocation.
        cmake+='target_compile_options(retirement_readback PRIVATE "SHELL:'+shlex.join(options)+'")\n'
        cmake+='target_sources(probe PRIVATE $<TARGET_OBJECTS:retirement_readback>)\n'
        cmake+='target_link_options(probe PRIVATE "-Wl,-Map,'+str(link_map)+'")\n'
        engine_candidate=dict(source=name,before_sha256=hashlib.sha256(original.encode()).hexdigest(),
            after_sha256=sha(path),anchor=anchor,addition=addition,compile_options=options,
            original_flags_file_sha256=sha(flags_file),source_results_supplied=False,
            validator_sha256=sha(inputs/'src/platform/source_pool_continuation.cpp'),
            retirement_observer_sha256=sha(inputs/'src/gameplay/objects/native_object_lifetimes.cpp'),
            all_other_engine_libraries_borrowed_unchanged=True)
    cmake+=f'add_test(NAME campaign_pelican_parent COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:probe> "{map_path}" "{map_path.parent}" campaign_pelican_parent "${{CMAKE_BINARY_DIR}}/parent.json")\n'
    cmake+=f'set_tests_properties(campaign_pelican_parent PROPERTIES TIMEOUT 360 ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;H2_SECURITY_LIBRARY={security}")\n'
    (source/'CMakeLists.txt').write_text(cmake);args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,owned_scratch=str(owner),native_owner=str(frozen),tool_sha256=sha(__file__),
        caller_substitutions=edits,fixture_before_sha256=hashlib.sha256(before.encode()).hexdigest(),fixture_after_sha256=sha(target),
        parent_input_reference_sha256=sha(args.inputs),native_libraries_sha256=pins,native_headers_sha256=headers,mcc_map_sha256=sha(map_path),
        engine_libraries_changed=False,engine_candidate=engine_candidate,full_goal_complete=False,retained_large_artifacts=[])
    def save():(args.output/'probe.json').write_text(json.dumps(report,indent=2)+'\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','probe','--filter','^campaign_pelican_parent$',
        '--jobs','1','--test-jobs','1','--output',str(args.output/'validation'),'--retain-report','parent.json'];report['command']=command;save()
    code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    proof=args.output/'validation/evidence/parent.json';data=json.loads(proof.read_text())if proof.exists()else{}
    report.update(returncode=code,native_libraries_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in pins.items()),
        native_headers_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in headers.items()),mcc_map_unchanged=sha(map_path)==report['mcc_map_sha256'],
        error=data.get('error'),shots=data.get('shots'),minimum_body=data.get('minimum_body'),source_dead=data.get('source_dead'),target_retired=data.get('target_retired'))
    report['passed']=code==0 and data.get('passed')is True and all(report[k]for k in ('native_libraries_unchanged','native_headers_unchanged','mcc_map_unchanged'))
    if engine_candidate:
        text=link_map.read_text()if link_map.exists()else''
        engine_candidate.update(old_multiplayer_archive_member_absent=bool(text)and'libhalo2_systems.a(multiplayer.cpp.o)'not in text,
            replacement_object_linked='CMakeFiles/retirement_readback.dir/src/gameplay/multiplayer/multiplayer.cpp.o'in text,
            link_map_sha256=sha(link_map)if link_map.exists()else None)
        report['passed'] &= all(engine_candidate[k]for k in ('old_multiplayer_archive_member_absent','replacement_object_linked'))
    save();print(json.dumps({k:report[k]for k in ('passed','error','shots','minimum_body','source_dead','target_retired')}),flush=True)
    return 0 if report['passed']else code or 1


if __name__=='__main__':raise SystemExit(main())
