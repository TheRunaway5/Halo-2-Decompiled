#!/usr/bin/env python3
"""Probe ordinary rocket routing to actual Pelican attack-variant attachments.

Use a private caller fixture with the active native cohort's read-only libraries.
Preserve bounded recipient/model-gate diagnostics even when a case rejects.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from qualify_live_damage_material_tu15 import native_link_inputs,sha

ROOT=Path(__file__).resolve().parents[1]


def fixture_source(before):
    edits=[
        ('  const bool campaign_no_damage=name=="campaign_guntower_no_damage";\n',
         '''  const bool pelican_damage=name=="campaign_pelican_rear_gun" || name=="campaign_pelican_rocket_pod";
  const bool campaign_no_damage=name=="campaign_guntower_no_damage" || pelican_damage;
'''),
        ('(!campaign_no_damage || Bytes(source.memory().view(abi.gp[4],4)).u32(0)==0xda180u);',
         '(!campaign_no_damage || Bytes(source.memory().view(abi.gp[4],4)).u32(0)==(pelican_damage?0xda875u:0xda180u));'),
        ('  const auto campaign_parent_suffix=name=="campaign_pelican_children"?',
         '  const auto campaign_parent_suffix=(name=="campaign_pelican_children" || pelican_damage)?'),
        ('      {"wraith_minigun","objects\\\\vehicles\\\\wraith\\\\turrets\\\\minigun\\\\minigun"},\n',
         '''      {"wraith_minigun","objects\\\\vehicles\\\\wraith\\\\turrets\\\\minigun\\\\minigun"},
      {"campaign_pelican_rear_gun","objects\\\\vehicles\\\\pelican\\\\pelican_rear_gun\\\\pelican_rear_gun"},
      {"campaign_pelican_rocket_pod","objects\\\\vehicles\\\\pelican\\\\pelican_rocket_pod\\\\pelican_rocket_pod"},
'''),
        ('      if(name=="campaign_pelican_children") {\n',
         '      if(name=="campaign_pelican_children" || pelican_damage) {\n'),
        ('  const auto target_suffix=attached!=attached_targets.end()?',
         '  std::uint32_t pelican_parent=null_datum,pelican_stimulus_anchor=null_datum;\n  std::vector<std::uint32_t> pelican_family;\n  const auto target_suffix=attached!=attached_targets.end()?'),
        ('  if(named_default_wraith) {\n',
         '''  if(pelican_damage) {
    check(candidates.empty(),"Unarmed scenario Pelican unexpectedly created an attack attachment");
    const auto position=world.match.state().players[slot].position+Vec3{0,0,.5f};
    pelican_parent=test::pelican_children_constructor_tu15(world,cache,*campaign_placement,position,report.data);
    report.data["construction_only"]=false;report.data["attack_parent"]=pelican_parent;
    pelican_family.push_back(pelican_parent);
    for(const auto &child:report.data.at("actual_children"))pelican_family.push_back(child.at("object").get<std::uint32_t>());
    for(const auto &child:source.live_objects())if(child.parent==pelican_parent && child.type==1) {
      const auto definition=cache.query_tag({.handle=TagHandle{child.definition}});
      if(definition && definition->name.ends_with("\\\\pelican_rocket_pod") && pelican_stimulus_anchor==null_datum)
        pelican_stimulus_anchor=child.object;
      if(definition && definition->name==attached->second)candidates.push_back(child.object);
    }
    check(candidates.size()==(name=="campaign_pelican_rocket_pod"?2u:1u),"Actual attack attachment count changed");
    check(pelican_stimulus_anchor!=null_datum,"Attack family lacks its actual rocket-pod firing-lane anchor");
    report.data["scene_stimulus_anchor"]=pelican_stimulus_anchor;
  }
  if(named_default_wraith) {
'''),
        ('  const auto sphere=source.object_sphere(target);const auto root=source.object_transform(target);\n',
         '  const auto sphere=source.object_sphere(pelican_damage?pelican_stimulus_anchor:target);const auto root=source.object_transform(pelican_damage?pelican_stimulus_anchor:target);\n'),
        ('    const auto object=cache.load_source_tag_metadata(target_definition);\n    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38u));\n    check(model.group()=="hlmt","Negative target lost its authored model owner");\n',
         '''    const auto aim_definition=pelican_damage?source.object_definition(pelican_parent):target_definition;
    const auto object=cache.load_source_tag_metadata(aim_definition);
    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38u));
    check(model.group()=="hlmt","Negative target lost its authored model owner");
    if(pelican_damage)report.data["nearby_explosion_aim"]={{"collision_object",pelican_parent},
        {"collision_definition",aim_definition},{"target_attachment",target},
        {"scope","Aim at actual parent collision near the attachment; source projectile and radial owners decide all hits and damage"}};
'''),
        ('  const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};\n  Vec3 current_eye{},current_target{};\n',
         '''  const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
  const auto pelican_family_state=[&] {
    Json result=Json::array();const auto live=source.live_objects();
    for(const auto handle:pelican_family) {
      const auto object=std::ranges::find_if(live,[&](const auto &row){return row.object==handle;});
      Json row{{"object",handle},{"live",object!=live.end()}};
      if(object!=live.end()) {
        const auto body=source.source_object_body(handle);const auto vitality=source.object_vitality(handle);
        const auto pose=source.object_transform(handle);
        row.update({{"definition",object->definition},{"parent",object->parent},{"body_address",body},
            {"body_bits",std::bit_cast<std::uint32_t>(vitality.body)},
            {"shield_bits",std::bit_cast<std::uint32_t>(vitality.shield)},
            {"damage_flags",word(body+0x108u)},{"permutations",source.object_model_regions(handle).selection.permutations},
            {"position",{pose.position.x,pose.position.y,pose.position.z}}});
      }
      result.push_back(row);
    }
    return result;
  };
  if(pelican_damage)report.data["pelican_family_before_settling"]=pelican_family_state();
  Vec3 current_eye{},current_target{};
'''),
        ('    current_eye=position;const auto bound=source.object_sphere(target);current_target=bound.center;\n',
         '    current_eye=position;const auto bound=source.object_sphere(pelican_damage?pelican_parent:target);current_target=bound.center;\n'),
        ('      const auto nodes=source.world_nodes(target);\n      const auto active=source.object_model_regions(target).selection.permutations;\n',
         '      const auto nodes=source.world_nodes(pelican_damage?pelican_parent:target);\n      const auto active=source.object_model_regions(pelican_damage?pelican_parent:target).selection.permutations;\n'),
        ('      const bool center_hit=admit(bound.center);\n',
         '      const bool center_hit=admit(pelican_damage?source.object_sphere(pelican_stimulus_anchor).center:bound.center);\n'),
        ('    campaign_area_observer=[&,active=false](bool before,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi)mutable{\n',
         '''    report.data["pelican_family_model_queries"]=Json::array();
    report.data["pelican_family_damage_requests"]=Json::array();
    report.data["actual_original_area_calls"]=Json::array();
    campaign_area_observer=[&,active=false,family_active=false](bool before,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi)mutable{
      if(pelican_damage && entry==0xd91d0u) {
        if(before) {
          const auto request=word(abi.gp[4]+4u),caller=word(abi.gp[4]);
          Json event{{"request",request},{"caller_return",caller},{"damage",word(request)},
              {"source_player",word(request+8u)},{"source_object",word(request+0xcu)}};
          if(caller==0x101b89u) {
            const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return memory.view(at,size);}};
            const auto header=source_raw_datum_lookup(target_pool,abi.gp[6],storage);
            check(header && word(header+8u)==abi.gp[5],"Actual projectile area call lost its full generation/body");
            event["projectile"]=abi.gp[6];event["projectile_definition"]=word(abi.gp[5]);
          }
          active_area_calls.push_back(event);report.data["actual_original_area_calls"].push_back(event);
        } else {
          check(!active_area_calls.empty(),"Actual Pelican area observations became unbalanced");active_area_calls.pop_back();
        }
      }
      if(pelican_damage && entry==0xd80a0u) {
        if(before) {
          family_active=std::ranges::find(pelican_family,abi.gp[1])!=pelican_family.end() &&
              (word(abi.gp[4])==0xda875u || word(abi.gp[4])==0xda180u);
          if(family_active) {
            const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return memory.view(at,size);}};
            const auto header=source_raw_datum_lookup(target_pool,abi.gp[1],storage);
            check(header!=0,"Family model query lost its actual full generation");
            report.data["pelican_family_model_queries"].push_back({{"object",abi.gp[1]},
                {"definition",word(word(header+8u))},{"caller_return",word(abi.gp[4])},{"returned",false}});
          }
        } else if(family_active) {
          auto &event=report.data["pelican_family_model_queries"].back();
          event["returned"]=true;event["actual_result"]=abi.gp[0];family_active=false;
        }
      }
'''),
        ('        active=abi.gp[1]==target && word(abi.gp[4])==0xda180u;\n        if(!active)return;\n',
         '''        active=abi.gp[1]==target && word(abi.gp[4])==(pelican_damage?0xda875u:0xda180u);
        if(!active)return;
        if(pelican_damage) {
          // Authenticated DA830 prologue: D80A0 entry ESP is caller
          // entry ESP-160; the actual request argument is at caller+4.
          const auto request=word(abi.gp[4]+0x164u),damage=word(request);
          const auto definition=cache.query_tag({.handle=TagHandle{damage}});
          check(definition && definition->group=="jpt!","Actual per-target area request lost its damage effect");
          check(!active_area_calls.empty() && active_area_calls.back().contains("projectile") &&
              active_area_calls.back().at("damage")==damage,"Target rejection is outside its actual projectile area owner");
          auto event=active_area_calls.back();event.update({{"target",target},{"damage",damage},
              {"damage_name",definition->name},{"source_player",word(request+8u)},
              {"source_object",word(request+0xcu)},{"per_target_owner",0xda830u},{"lookup_caller_return",0xda875u}});
          report.data["actual_target_damage_requests"].push_back(event);
        }
'''),
        ('      const auto request=word(stack+0x100u),recipient=word(stack+0x104u);\n      if(recipient==target) {\n',
         '''      const auto request=word(stack+0x100u),recipient=word(stack+0x104u);
      if(pelican_damage && std::ranges::find(pelican_family,recipient)!=pelican_family.end() &&
          report.data["pelican_family_damage_requests"].size()<128) {
        const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return memory.view(at,size);}};
        const auto header=source_raw_datum_lookup(target_pool,recipient,storage);
        check(header!=0,"Family damage request lost its actual full generation");
        const auto damage=word(request);const auto tag=cache.query_tag({.handle=TagHandle{damage}});
        check(tag && tag->group=="jpt!","Actual family request has no damage-effect owner");
        report.data["pelican_family_damage_requests"].push_back({{"object",recipient},
            {"definition",word(word(header+8u))},{"damage",damage},{"damage_name",tag->name},
            {"source_player",word(request+8u)},{"source_object",word(request+0xcu)},
            {"request_flags",word(request+4u)},{"full_generation_live",true}});
      }
      if(recipient==target) {
'''),
        ('  bool pending_qualified=false;\n  for(unsigned tick=0;tick<900;++tick){\n',
         '  bool pending_qualified=false;\n  if(pelican_damage)report.data["pelican_family_before_stimulus"]=pelican_family_state();\n  for(unsigned tick=0;tick<(pelican_damage?180u:900u);++tick){\n'),
        ('  report.data.update({{"shots",shots},{"minimum_body",minimum}',
         '  if(pelican_damage)report.data["pelican_family_after_stimulus"]=pelican_family_state();\n  report.data.update({{"shots",shots},{"minimum_body",minimum}'),
    ]
    after=before
    for needle,replacement in edits:
        if after.count(needle)!=1:raise ValueError('Pelican damage caller seam changed: '+needle[:90])
        after=after.replace(needle,replacement)
    def apply(needle,replacement):
        nonlocal after
        if after.count(needle)!=1:raise ValueError('Pelican radial gate seam changed: '+needle[:90])
        after=after.replace(needle,replacement);edits.append((needle,replacement))
    # The original radial candidate owner rejects these children before the
    # lower direct dispatcher. Keep the guntower's distinct DA180 expectation.
    needle='std::ranges::all_of(report.data["actual_target_model_rejections"],[](const auto &event){'
    apply(needle,needle.replace('[](','[&]('))
    apply('event.at("caller_return")==0xda180u &&','event.at("caller_return")== (pelican_damage?0xda875u:0xda180u) &&')
    needle='    report.data["original_no_damage_information_gate_verified"]=true;\n'
    apply(needle,'''    if(pelican_damage) {
      check(active_area_calls.empty(),"Original projectile area owner has not returned");
      const auto &before=report.data.at("pelican_family_before_stimulus");
      const auto &after=report.data.at("pelican_family_after_stimulus");
      check(before.size()==4 && after.size()==4,"Actual attack family census changed");
      for(unsigned index=1;index<4;++index) {
        check(after[index].at("live").get<bool>() && after[index].at("object")==before[index].at("object") &&
            after[index].at("parent")==before[index].at("parent") && after[index].at("definition")==before[index].at("definition") &&
            after[index].at("body_bits")==before[index].at("body_bits") && after[index].at("shield_bits")==before[index].at("shield_bits") &&
            after[index].at("damage_flags")==before[index].at("damage_flags") && after[index].at("permutations")==before[index].at("permutations"),
            "Actual radial no-information gate changed a child generation, parent, vitality, flags or regions");
      }
      check(after[0].at("live").get<bool>() && std::bit_cast<float>(after[0].at("body_bits").get<std::uint32_t>())<
          std::bit_cast<float>(before[0].at("body_bits").get<std::uint32_t>()),"Actual rocket scene supplied no damaged parent positive control");
      check(std::ranges::any_of(report.data["actual_target_damage_requests"],[&](const auto &request) {
        return request.contains("projectile") && std::ranges::any_of(report.data["detonations"],[&](const auto &event) {
          return event.at("projectile")==request.at("projectile") && event.at("definition")==request.at("projectile_definition");
        });
      }),"Attachment's actual radial rejection has no matching live projectile detonation");
      report.data["original_radial_no_information_gate_verified"]=true;
    }
'''+needle)
    needle='    report.data["campaign_acceptance_scope"]="Original campaign placement, ordinary rockets, actual full-target damage dispatcher and native model lookup returning no authored damage information, unchanged vitality/regions and exact continuation; other producers and full-definition gates remain open";'
    apply(needle,needle+'\n    if(pelican_damage)report.data["campaign_acceptance_scope"]="Actual attack constructor; ordinary rocket area calls; actual full-target DA830 null-information gate before the lower dispatcher; unchanged children and damaged parent positive control; coupled continuation. Direct collision, other producers and full-definition parity remain open";')
    return after,edits


def run_original(args,owner):
    if not(args.binary and args.map and args.case and args.security_library and args.native_report):
        raise ValueError('Original entry probe requires binary, map, case, security library and native report')
    command=[str(args.binary),str(args.map),str(args.map.parent),args.case,str(args.native_report)]
    log=owner/'native-capture.log'
    with log.open('w')as output:
        code=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',
            HALO2_DESTRUCTION_CAPTURE_MODEL='1',HALO2_DESTRUCTION_QUERY_CAPTURE_ONLY='1',
            H2_SECURITY_LIBRARY=str(args.security_library)),stdout=output,stderr=subprocess.STDOUT,timeout=240).returncode
    native=json.loads(args.native_report.read_text())if args.native_report.exists()else{}
    capture=owner/'model-lookup-capture';metadata=capture/'capture.json'
    args.output.mkdir(parents=True,exist_ok=True)
    if code or not native.get('capture_only')or not metadata.exists():
        (args.output/'failure.log').write_text(log.read_text()[-16000:]);raise ValueError('Ordinary rocket scene produced no actual target lookup capture')
    data=json.loads(metadata.read_text());events=native.get('actual_target_model_rejections',[])
    if data['entry']!=0xd80a0 or data['before']['gp'][1]!=native['target']or not events or not all(
            row.get('returned')is True and row.get('actual_result')==0 and row.get('caller_return')==0xda875 for row in events):
        raise ValueError('Actual capture missed the full attachment radial no-information lookup')
    (args.output/'native-entry.json').write_text(json.dumps(native,indent=2)+'\n')
    code=subprocess.run([sys.executable,str(ROOT/'tools/qualify_destruction_original_entry_tu15.py'),
        '--capture-name',capture.name,'--output',str(args.output/'original')]).returncode
    return code


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',type=Path)
    parser.add_argument('--constructor-probe',type=Path)
    parser.add_argument('--area-model-gate',type=Path)
    parser.add_argument('--capture-original-model',action='store_true')
    parser.add_argument('--run-original',action='store_true')
    parser.add_argument('--binary',type=Path)
    parser.add_argument('--map',type=Path)
    parser.add_argument('--case',choices=('campaign_pelican_rear_gun','campaign_pelican_rocket_pod'))
    parser.add_argument('--security-library',type=Path)
    parser.add_argument('--native-report',type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output=args.output.resolve()
    if args.run_original:return run_original(args,owner)
    if not(args.admission and args.constructor_probe and args.area_model_gate):parser.error('Build requires native admission, constructor probe and original area gate')
    admission=json.loads(args.admission.read_text());probe=json.loads(args.constructor_probe.read_text())
    gate=json.loads(args.area_model_gate.read_text())
    if gate.get('passed')is not True or gate.get('lookup_entry_stack_request_offset')!=0x164:
        raise ValueError('Original per-target area gate is unqualified')
    frozen=Path(admission['owner']).resolve(strict=True);inputs=frozen/'source';build=frozen/'build'
    if not probe['passed']or not probe.get('engine_candidate'):raise ValueError('Constructor/readback context unqualified')
    if any(sha(inputs/name)!=row['after']for name,row in admission['changes'].items()):raise ValueError('Native source drifted')
    if sha(inputs/probe['engine_candidate']['source'])!=probe['engine_candidate']['after_sha256']:
        raise ValueError('Native library source lacks qualified player readback')
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(inputs/fixture).read_text();after,edits=fixture_source(before)
    constructor=json.loads((args.constructor_probe.parent/'validation/evidence/pelican-children.json').read_text())
    map_path=Path(constructor['map']).resolve(strict=True)
    if sha(map_path)!=probe['mcc_map_sha256']:raise ValueError('Constructor map drifted')
    source=owner/'source';target=source/fixture;target.parent.mkdir(parents=True);target.write_text(after)
    libraries,pins=native_link_inputs(build)
    security=build/'online-protocol/Release/libhalo2_online_security_ffi.so';pins[str(security)]=sha(security)
    headers={str(p):sha(p)for parent in ('include','src','tests/helpers')for p in (inputs/parent).rglob('*.hpp')if p.is_file()}
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2PelicanDamageProbe LANGUAGES CXX)
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
    cases=['campaign_pelican_rear_gun','campaign_pelican_rocket_pod']
    for case in cases:
        if args.capture_original_model:
            cmake+=f'add_test(NAME {case} COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- "{sys.executable}" -B "{Path(__file__).resolve()}" --run-original --binary $<TARGET_FILE:probe> --map "{map_path}" --case {case} --security-library "{security}" --native-report "${{CMAKE_BINARY_DIR}}/{case}.json" --output "{args.output/("actual-"+case)}")\n'
        else:
            cmake+=f'add_test(NAME {case} COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:probe> "{map_path}" "{map_path.parent}" {case} "${{CMAKE_BINARY_DIR}}/{case}.json")\n'
        cmake+=f'set_tests_properties({case} PROPERTIES TIMEOUT 360 ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;H2_SECURITY_LIBRARY={security}")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,owned_scratch=str(owner),native_owner=str(frozen),tool_sha256=sha(__file__),
        fixture_original_sha256=hashlib.sha256(before.encode()).hexdigest(),fixture_probe_sha256=sha(target),
        caller_substitutions=edits,native_libraries_sha256=pins,native_headers_sha256=headers,mcc_map_sha256=sha(map_path),
        original_area_gate_reference=dict(path=str(args.area_model_gate.resolve()),sha256=sha(args.area_model_gate)),
        capture_original_model=args.capture_original_model,engine_libraries_changed=False,full_goal_complete=False,retained_large_artifacts=[])
    def save():(args.output/'probe.json').write_text(json.dumps(report,indent=2)+'\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','probe',
        '--filter','^campaign_pelican_(rear_gun|rocket_pod)$','--jobs','1','--test-jobs','1','--output',str(args.output/'validation')]
    for case in cases:command+=['--retain-report',case+'.json']
    report['command']=command;save()
    code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    results={}
    for case in cases:
        path=args.output/'validation/evidence'/(case+'.json');data=json.loads(path.read_text())if path.exists()else{}
        results[case]=dict(passed=data.get('passed')is True and data.get('original_no_damage_information_gate_verified')is True,
            error=data.get('error'),target=data.get('target'),parent=data.get('attack_parent'),
            family_requests=len(data.get('pelican_family_damage_requests',[])),target_requests=len(data.get('actual_target_damage_requests',[])),
            target_model_rejections=len(data.get('actual_target_model_rejections',[])))
        if args.capture_original_model:
            original=args.output/('actual-'+case)/'original/report.json'
            reference=json.loads(original.read_text())if original.exists()else{}
            results[case].update(passed=data.get('capture_only')is True and reference.get('passed')is True,
                scope='One actual attachment D80A0 call only; no new world acceptance case',
                original_instructions=reference.get('original_instructions'),compared_memory_bytes=reference.get('compared_memory_bytes'))
    report.update(returncode=code,cases=results,native_libraries_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in pins.items()),
        native_headers_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in headers.items()),mcc_map_unchanged=sha(map_path)==report['mcc_map_sha256'])
    report['passed']=code==0 and all(row['passed']for row in results.values())and all(report[k]for k in ('native_libraries_unchanged','native_headers_unchanged','mcc_map_unchanged'))
    save();print(json.dumps(dict(passed=report['passed'],cases=results,report=str(args.output/'probe.json'))),flush=True)
    return 0 if report['passed']else code or 1


if __name__=='__main__':raise SystemExit(main())
