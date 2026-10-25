#!/usr/bin/env python3
"""Exercise an additional authored campaign vehicle without changing the engine.

Frozen native libraries are read-only. Current original/MCC metadata identity
is checked before ordinary source creation, weapon commands and continuation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
from audit_destruction_contracts_tu15 import Reader
from inspect_multiplayer import Cache as XboxCache
from mcc_content import MccCache
from qualify_live_damage_material_tu15 import native_link_inputs,sha

ROOT=Path(__file__).resolve().parents[1]
CASES={
    'campaign_sentinel_enforcer':dict(name=r'objects\characters\sentinel_enforcer\sentinel_enforcer',
        map='06a_sentinelwalls',contract='5ce57752ce553d564d01b46e531c69bd2bf09e46f28e5c4199ab06cd4c61744f'),
    'campaign_plug_absorber':dict(name=r'scenarios\objects\solo\sentinelhq\plug_absorber\plug_absorber',
        map='06a_sentinelwalls',contract='265301dba2975630b14d56ea961fa746e2a90a347a4f672ccb3a9e180b4fca16'),
}


def fixture_source(before,case):
    existing='  const bool extended_campaign=name=="campaign_sentinel_enforcer";\n'
    if existing in before:
        if case=='campaign_sentinel_enforcer':return before,[]
        if case!='campaign_plug_absorber' or before.count(existing)!=1:raise ValueError('Campaign extension baseline drifted')
        edits=[(existing,existing.replace(';',' || name=="campaign_plug_absorber";')),
            ('  const auto target_pool=source.source_object_pool();\n',
             '''  const auto target_pool=source.source_object_pool();
  const auto absorber_section_state=[&] {
    const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return memory.view(at,size);}};
    const auto header=source_raw_datum_lookup(target_pool,target,storage);
    check(header && word(header+8u)==target_body && word(target_body)==target_definition,
        "Absorber section observation lost its full target generation");
    const Bytes descriptor(memory.view(target_body+0x120u,4));
    const auto size=std::bit_cast<std::int16_t>(descriptor.u16(0));
    const auto offset=std::bit_cast<std::int16_t>(descriptor.u16(2));
    const auto extent=Bytes(memory.view(header+6u,2)).u16(0);
    report.data["absorber_section_descriptor"]={{"bytes",size},{"body_relative_offset",offset},{"body_extent",extent}};
    check(size==8 && offset>=0 && unsigned(offset)+unsigned(size)<=extent,
        "Absorber actual section storage differs from its one-section original contract");
    const auto vitality=source.object_vitality(target);
    return Json{{"object",target},{"definition",target_definition},{"full_generation_live",true},
        {"word0",word(target_body+unsigned(offset))},{"word4",word(target_body+unsigned(offset)+4u)},
        {"global_body_bits",std::bit_cast<std::uint32_t>(vitality.body)},
        {"global_shield_bits",std::bit_cast<std::uint32_t>(vitality.shield)}};
  };
  if(name=="campaign_plug_absorber")report.data["absorber_section_before_stimulus"]=absorber_section_state();
'''),
            ('  report.data.update({{"shots",shots},{"minimum_body",minimum}',
             '  if(name=="campaign_plug_absorber")report.data["absorber_section_after_stimulus"]=absorber_section_state();\n  report.data.update({{"shots",shots},{"minimum_body",minimum}'),
            ('  if(legacy_bullet_collision) {\n    const Bytes descriptor(memory.view(target_body+0x120u,4));\n',
             '  if(legacy_bullet_collision || name=="campaign_plug_absorber") {\n    const Bytes descriptor(memory.view(target_body+0x120u,4));\n'),
            ('    check(regions_changed,"Campaign target death did not change its authored model regions");\n',
             '''    if(name=="campaign_plug_absorber") {
      const auto object=cache.load_source_tag_metadata(target_definition);
      const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38u));
      const Bytes head(model.bytes_at(model.source_address(),model.size()));
      check(head.u32(0x60u)==1,"Absorber damage-information count differs from its original contract");
      const Bytes info(cache.read_tag_bytes(head.u32(0x64u),0xf8));
      check(info.u32(0)==0 && info.u32(0xbcu)==1,"Absorber damage/section inputs differ from original");
      const Bytes section(cache.read_tag_bytes(info.u32(0xc0u),0x38));
      check(section.u32(4)==1 && section.u32(0xcu)==0,"Absorber authored zero-response section differs from original");
      report.data["authored_zero_response_section_verified"]=true;
    } else check(regions_changed,"Campaign target death did not change its authored model regions");
'''),
            ('    report.data["campaign_acceptance_scope"]="Original campaign mode, authored runtime creation, ordinary rocket death and region changes; debris, occupants, lifetime, story, transport and presentation remain separate";\n',
             '    report.data["campaign_acceptance_scope"]=name=="campaign_plug_absorber"?"Actual authored absorber and ordinary source rockets, original death transition and exact continuation; authored zero-response section preserved. All other producers/physics/presentation and full-definition gates remain open":"Original campaign mode, authored runtime creation, ordinary rocket death and region changes; debris, occupants, lifetime, story, transport and presentation remain separate";\n')]
        after=before
        for needle,replacement in edits:
            if after.count(needle)!=1:raise ValueError('Absorber caller seam drifted: '+needle[:100])
            after=after.replace(needle,replacement)
        return after,edits
    if case!='campaign_sentinel_enforcer':raise ValueError('Additional campaign case requires the qualified Enforcer baseline')
    edits=[
        ('  const bool campaign=pelican_parent_case ||',
         '  const bool extended_campaign=name=="campaign_sentinel_enforcer";\n  const bool campaign=extended_campaign || pelican_parent_case ||'),
        ('  const auto target_pool=source.source_object_pool();\n',
         '  const auto target_pool=source.source_object_pool();\n  if(extended_campaign)report.data["actual_target_damage_requests"]=Json::array();\n'),
        ('  const bool surveyed_vehicle_lane=campaign_disposable_turret || campaign_no_damage || name=="campaign_sentinel_emitter";\n',
         '  const bool surveyed_vehicle_lane=extended_campaign || campaign_disposable_turret || campaign_no_damage || name=="campaign_sentinel_emitter";\n'),
        ('    if((campaign_no_damage || pelican_parent_case) && store.instruction==0xda0eeu) {\n',
         '    if((campaign_no_damage || pelican_parent_case || extended_campaign) && store.instruction==0xda0eeu) {\n'),
        ('        if(campaign_scenery || named_default_wraith || pelican_parent_case) {\n',
         '        if(campaign_scenery || named_default_wraith || pelican_parent_case || extended_campaign) {\n'),
        ('        if(campaign_scenery || named_default_wraith || pelican_parent_case)death.update(',
         '        if(campaign_scenery || named_default_wraith || pelican_parent_case || extended_campaign)death.update('),
        ('    if(pelican_parent_case && exists && !dead && state.players[slot].loaded==0 &&\n',
         '    if((pelican_parent_case || extended_campaign) && exists && !dead && state.players[slot].loaded==0 &&\n'),
        ('    if(!pelican_parent_case && state.players[slot].shot_tick!=previous_shot)++shots;\n',
         '    if(!(pelican_parent_case || extended_campaign) && state.players[slot].shot_tick!=previous_shot)++shots;\n'),
        ('        if(pelican_parent_case)++shots;\n',
         '        if(pelican_parent_case || extended_campaign)++shots;\n'),
    ]
    after=before
    for needle,replacement in edits:
        if after.count(needle)!=1:raise ValueError('Campaign vehicle caller seam drifted: '+needle[:100])
        after=after.replace(needle,replacement)
    return after,edits


def capture_source(before):
    edits=[
        ('  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");\n',
         '  const bool capture_children=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL_CHILDREN");\n  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");\n'),
        ('  const auto campaign_capture_entry=capture_model?',
         '  const auto campaign_capture_entry=capture_children?0xdc540u:capture_model?'),
        ('  if(capture_model || capture_record || (campaign_scenery &&',
         '  if(capture_children || capture_model || capture_record || (campaign_scenery &&'),
        ('        (capture_model?"model-lookup-capture":',
         '        (capture_children?"model-children-capture":capture_model?"model-lookup-capture":'),
        ('  if(capture_model || campaign_scenery || campaign_no_damage || pelican_parent_case || name=="wraith_minigun")campaign_query_observation',
         '  if(capture_children || capture_model || campaign_scenery || campaign_no_damage || pelican_parent_case || name=="wraith_minigun")campaign_query_observation'),
        ('    if(campaign_query_capture && entry==campaign_capture_entry) {\n',
         '''    if(campaign_query_capture && entry==campaign_capture_entry) {
      if(capture_children && before) {
        model_capture_active=model_capture_target!=null_datum && abi.gp[0]==model_capture_target;
        if(model_capture_active) {
          const auto word=[&](auto at){return Bytes(source.memory().view(at,4)).u32(0);};
          const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return source.memory().view(at,size);}};
          const auto header=source_raw_datum_lookup(source.source_object_pool(),abi.gp[0],storage);
          check(header && word(header+8u),"Actual model-child call lost its full object generation");
          report.data["actual_model_children_entries"].push_back({{"object",abi.gp[0]},
              {"definition",word(word(header+8u))},{"full_generation_live",true},
              {"caller_return",word(abi.gp[4])},{"request_key",word(abi.gp[4]+4u)}});
        }
      }
'''),
        ('      if(!capture_model || model_capture_active)campaign_query_capture->observe(',
         '      if((!capture_model && !capture_children) || model_capture_active)campaign_query_capture->observe('),
        ('      if(capture_model && !before)model_capture_active=false;\n',
         '      if((capture_model || capture_children) && !before)model_capture_active=false;\n'),
        ('  const auto target_definition=source.object_definition(target);\n',
         '  const auto target_definition=source.object_definition(target);\n  if(capture_children)model_capture_target=target;\n'),
    ]
    after=before
    for needle,replacement in edits:
        if after.count(needle)!=1:raise ValueError('Actual child capture seam drifted: '+needle[:100])
        after=after.replace(needle,replacement)
    return after,edits


def capture_body_source(before):
    """Observe the complete original DD5E0 boundary without supplying results."""
    edits=[
        ('  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");\n',
         '  const bool capture_body=std::getenv("HALO2_DESTRUCTION_CAPTURE_BODY");\n  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");\n'),
        ('  const auto campaign_capture_entry=capture_model?',
         '  const auto campaign_capture_entry=capture_body?0xdd5e0u:capture_model?'),
        ('  if(capture_model || capture_record || (campaign_scenery &&',
         '  if(capture_body || capture_model || capture_record || (campaign_scenery &&'),
        ('        (capture_model?"model-lookup-capture":',
         '        (capture_body?"absorber-body-capture":capture_model?"model-lookup-capture":'),
        ('  const auto target_definition=source.object_definition(target);\n',
         '  const auto target_definition=source.object_definition(target);\n  if(capture_body)model_capture_target=target;\n'),
        ('    if(campaign_query_capture && entry==campaign_capture_entry) {\n',
         '''    if(campaign_query_capture && entry==campaign_capture_entry) {
      if(capture_body && before) {
        const auto word=[&](auto at){return Bytes(source.memory().view(at,4)).u32(0);};
        model_capture_active=model_capture_target!=null_datum && word(abi.gp[4]+4u)==model_capture_target;
        if(model_capture_active) {
          const SourceRawDatumPoolServices storage{.read=[&](auto at,auto size){return source.memory().view(at,size);}};
          const auto header=source_raw_datum_lookup(source.source_object_pool(),model_capture_target,storage);
          check(header && word(header+8u),"Actual body gate lost its full target generation");
          const auto info=abi.gp[1],request=abi.gp[2];
          const auto selected=std::bit_cast<std::int16_t>(Bytes(source.memory().view(request?request+6u:info+8u,2)).u16(0));
          check(selected>=0 && unsigned(selected)<word(info+0xbcu),"Actual body gate has no selected original section");
          const auto section=word(info+0xc0u)+unsigned(selected)*0x38u;
          report.data["actual_body_gate_entries"].push_back({{"object",model_capture_target},
              {"definition",word(word(header+8u))},{"full_generation_live",true},
              {"caller_return",word(abi.gp[4])},{"selected_section",selected},
              {"section_flags",word(section+4u)},{"section_maximum_bits",word(section+8u)},
              {"amount_bits",word(abi.gp[4]+0xcu)},{"scale_bits",word(abi.gp[4]+0x10u)}});
        }
      }
'''),
        ('      if(!capture_model || model_capture_active)campaign_query_capture->observe(',
         '      if((!capture_model && !capture_body) || model_capture_active)campaign_query_capture->observe('),
        ('      if(capture_model && !before)model_capture_active=false;\n',
         '''      if(capture_body && !before && model_capture_active)
        report.data["actual_body_gate_entries"].back()["result_bits"]=abi.xmm[0][0];
      if((capture_model || capture_body) && !before)model_capture_active=false;
''')]
    after=before
    for needle,replacement in edits:
        if after.count(needle)!=1:raise ValueError('Actual body capture seam drifted: '+needle[:100])
        after=after.replace(needle,replacement)
    return after,edits


def run_body_original(args,owner):
    if not all((args.binary,args.map,args.security_library,args.native_report)):raise ValueError('Body capture needs its actual native caller')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    log=owner/'native-body-capture.log'
    with log.open('w')as output:
        status=subprocess.run([str(args.binary),str(args.map),str(args.map.parent),args.case,str(args.native_report)],
            env=dict(os.environ,HALO2_DESTRUCTION_CAPTURE_BODY='1',HALO2_DESTRUCTION_FIXED_CLOCK='1',
                HALO2_DESTRUCTION_QUERY_CAPTURE_ONLY='1',H2_SECURITY_LIBRARY=str(args.security_library)),
            stdout=output,stderr=subprocess.STDOUT,timeout=240).returncode
    native=json.loads(args.native_report.read_text())if args.native_report.exists()else{}
    capture=owner/'absorber-body-capture';metadata=capture/'capture.json'
    if not metadata.exists():
        (args.output/'failure.log').write_text(log.read_text()[-16000:]);raise ValueError('Native scene produced no completed body gate capture')
    data=json.loads(metadata.read_text());events=native.get('actual_body_gate_entries',[])
    if status or not native.get('capture_only')or data['entry']!=0xdd5e0 or not events or \
        events[0]['caller_return']!=0xdb33f or data['return']!=0xdb33f or not events[0]['full_generation_live']or \
        events[0]['section_flags']!=1 or events[0]['amount_bits']==0 or events[0]['result_bits']!=0:
        raise ValueError('Body capture missed the actual positive-input, zero-output original section gate')
    (args.output/'native-entry.json').write_text(json.dumps(native,indent=2)+'\n')
    return subprocess.run([sys.executable,str(ROOT/'tools/qualify_destruction_original_entry_tu15.py'),
        '--capture-name',capture.name,'--output',str(args.output/'original')]).returncode


def run_original(args,owner):
    if not all((args.binary,args.map,args.security_library,args.native_report)):raise ValueError('Original child capture requires its actual native caller')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=True)
    command=[str(args.binary),str(args.map),str(args.map.parent),args.case,str(args.native_report)]
    log=owner/'native-child-capture.log'
    with log.open('w')as output:
        status=subprocess.run(command,env=dict(os.environ,HALO2_DESTRUCTION_CAPTURE_MODEL_CHILDREN='1',
            HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_QUERY_CAPTURE_ONLY='1',
            H2_SECURITY_LIBRARY=str(args.security_library)),stdout=output,stderr=subprocess.STDOUT,timeout=240).returncode
    native=json.loads(args.native_report.read_text())if args.native_report.exists()else{}
    capture=owner/'model-children-capture';metadata=capture/'capture.json'
    if not metadata.exists():
        (args.output/'failure.log').write_text(log.read_text()[-16000:]);raise ValueError('Native scene produced no completed actual child-owner capture')
    data=json.loads(metadata.read_text());events=native.get('actual_model_children_entries',[])
    if data['entry']!=0xdc540 or not events or data['before']['gp'][0]!=events[0]['object']or \
       not events[0]['full_generation_live']or data['return']!=0xdcd77 or events[0]['caller_return']!=0xdcd77:
        raise ValueError('Child capture missed its actual full-target original caller')
    (args.output/'native-entry.json').write_text(json.dumps(native,indent=2)+'\n')
    (args.output/'native-scene.json').write_text(json.dumps(dict(returncode=status,scene_error=native.get('error'),
        scope='Completed actual DC540 entry/return only; a later scene failure does not certify the surrounding frame'),indent=2)+'\n')
    return subprocess.run([sys.executable,str(ROOT/'tools/qualify_destruction_original_entry_tu15.py'),
        '--capture-name',capture.name,'--output',str(args.output/'original')]).returncode


def bv_candidate(source,inputs,build,qualification,cmake,after,edits):
    proof=json.loads(qualification.read_text())
    if not proof['passed']or not proof['regenerated_source_matches']or proof['native']['instructions']!=453:
        raise ValueError('Existing BV owner lacks its frozen/original qualification')
    if any(sha(ROOT/name)!=pin for name,pin in proof['root_source_pins'].items())or \
       any(sha(name)!=pin for name,pin in proof['frozen_headers'].items())or any(sha(name)!=pin for name,pin in proof['frozen_libraries'].items()):
        raise ValueError('Existing qualified BV sources or frozen context drifted')
    files=('include/halo2/source_vehicle_bv_agent_tu15.hpp','src/physics/collision/source_vehicle_bv_agent_tu15_generated.cpp')
    for name in files:
        path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes((ROOT/name).read_bytes())
    name='src/physics/physics_live_world_tu15.cpp';before=(inputs/name).read_text()
    include='#include "halo2/physics_live_world_tu15.hpp"\n';anchor='        if(source_particle_swarm_tu15_owns_entry(c.target)) {\n'
    addition='''        if(source_vehicle_bv_agent_tu15_owns_entry(c.target)) {
            SourcePhysicsEntryObservation diagnostic(m,c.target,[&]{return a;});
            source_run_vehicle_bv_agent_tu15(m,c.target,a,fp,services(),o);return;
        }
'''
    if before.count(include)!=1 or before.count(anchor)!=1 or '#include "halo2/source_vehicle_bv_agent_tu15.hpp"'in before:
        raise ValueError('BV world route baseline drifted')
    path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(before.replace(include,include+'#include "halo2/source_vehicle_bv_agent_tu15.hpp"\n').replace(anchor,addition+anchor))
    flags_file=build/'CMakeFiles/halo2_systems.dir/flags.make';flags={};custom=None
    prefix='# Custom options: CMakeFiles/halo2_systems.dir/'+name+'.o_OPTIONS = '
    for line in flags_file.read_text().splitlines():
        for kind in ('CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS'):
            if line.startswith(kind+' = '):flags[kind]=shlex.split(line.split(' = ',1)[1])
        if line.startswith(prefix):custom=line[len(prefix):].split(';')
    if custom!=['-fno-fast-math','-ffp-contract=off','-frounding-math']:raise ValueError('Native world FP options drifted')
    options=flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+custom
    if any(any(c in token for c in '\n\r";$')for token in options):raise ValueError('Unexpected native BV compiler flags')
    cmake+='add_library(native_bv_world OBJECT '+name+' '+files[1]+')\n'
    cmake+='target_include_directories(native_bv_world PRIVATE "'+str(source/'include')+'" "'+str(source/'src')+'")\n'
    cmake+='target_compile_options(native_bv_world PRIVATE "SHELL:'+shlex.join(options)+'")\n'
    cmake+='target_sources(probe PRIVATE $<TARGET_OBJECTS:native_bv_world>)\n'
    cmake+='target_include_directories(probe BEFORE PRIVATE "'+str(source/'include')+'" "'+str(source/'src')+'")\n'
    counter_edits=[('  if(capture_model || campaign_scenery || campaign_no_damage || pelican_parent_case || name=="wraith_minigun")campaign_query_observation',
        '  if(extended_campaign || capture_model || campaign_scenery || campaign_no_damage || pelican_parent_case || name=="wraith_minigun")campaign_query_observation'),
        ('    if(campaign_query_capture && entry==campaign_capture_entry) {\n',
         '''    if(before && entry==0x2f18d0u) {
      const auto count=report.data.value("native_bv_agent_entries",0u)+1u;
      report.data["native_bv_agent_entries"]=count;
      if(count<=16)report.data["native_bv_agent_entry_samples"].push_back({{"gp",abi.gp},
          {"caller_return",Bytes(source.memory().view(abi.gp[4],4)).u32(0)}});
    }
    if(campaign_query_capture && entry==campaign_capture_entry) {
''')]
    for needle,replacement in counter_edits:
        if after.count(needle)!=1:raise ValueError('BV entry diagnostic seam drifted')
        after=after.replace(needle,replacement)
    edits+=counter_edits
    candidate=dict(kind='existing_compiled_BV_owner',object_target='native_bv_world',source=name,
        before_sha256=hashlib.sha256(before.encode()).hexdigest(),after_sha256=sha(path),
        source_sha256={n:sha(source/n)for n in files},compile_options=options,original_flags_sha256=sha(flags_file),
        qualification=str(qualification.resolve()),qualification_sha256=sha(qualification),world_addition=addition,
        diagnostic_entry_observation_only=True,semantic_kernel_conversion_qualified=False,runtime_results_or_missing_callee_supplied=False)
    return candidate,cmake,after,edits


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',type=Path)
    parser.add_argument('--contracts',type=Path)
    parser.add_argument('--runtime-inputs',type=Path)
    parser.add_argument('--case',required=True,choices=CASES)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--baseline-in-progress',action='store_true',
        help='Observe an already-built frozen engine while its independent full cohort is running')
    parser.add_argument('--native-child-owner-candidate',action='store_true')
    parser.add_argument('--children-qualification',type=Path)
    parser.add_argument('--children-negative',type=Path)
    parser.add_argument('--native-bv-agent-candidate',action='store_true')
    parser.add_argument('--bv-qualification',type=Path)
    parser.add_argument('--capture-children-original',action='store_true')
    parser.add_argument('--capture-body-original',action='store_true')
    parser.add_argument('--run-original',action='store_true')
    parser.add_argument('--binary',type=Path)
    parser.add_argument('--map',type=Path)
    parser.add_argument('--security-library',type=Path)
    parser.add_argument('--native-report',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.run_original:return run_body_original(args,owner)if args.capture_body_original else run_original(args,owner)
    if not all((args.admission,args.contracts,args.runtime_inputs)):parser.error('Build requires native admission, contracts and MCC input references')
    if args.capture_children_original and not args.native_child_owner_candidate:parser.error('Original capture requires its isolated native child candidate')
    if args.capture_body_original and (not args.native_bv_agent_candidate or args.case!='campaign_plug_absorber'):
        parser.error('Body capture requires its actual absorber and qualified BV candidate')
    if args.native_bv_agent_candidate and args.native_child_owner_candidate:parser.error('Candidate source epochs must be qualified separately')
    admission=json.loads(args.admission.read_text())
    frozen=Path(admission['owner']).resolve(strict=True);inputs=frozen/'source';build=frozen/'build'
    baseline_pending=args.baseline_in_progress and admission.get('status')=='running'
    if (not admission['passed']and not baseline_pending)or any(sha(inputs/name)!=row['after']for name,row in admission['changes'].items()):
        raise ValueError('Native cohort is unqualified or drifted')
    declared=CASES[args.case];census=json.loads(args.contracts.read_text());runtime=json.loads(args.runtime_inputs.read_text())
    definition=census['definitions']['vehi:'+declared['name']]
    if set(definition['xbox'])!={declared['contract']}or set(definition['mcc'])!={declared['contract']}:
        raise ValueError('Campaign vehicle has a changed authored contract')
    context=next(row for row in census['source_files']if Path(row['path']).stem==declared['map'])
    reference_pins={context['path']:context['sha256'],context['mcc_path']:context['mcc_sha256']}
    if any(sha(p)!=pin for p,pin in reference_pins.items()):raise ValueError('Authored campaign map drifted')
    map_path=Path(context['mcc_path']).resolve(strict=True)
    if str(map_path)not in runtime['files']or sha(map_path)!=runtime['files'][str(map_path)]['sha256']:
        raise ValueError('Runtime map outside qualified MCC cohort')
    contracts={}
    for platform,cache_type in (('original',XboxCache),('mcc',MccCache)):
        cache=cache_type(Path(context['path']if platform=='original'else context['mcc_path']))
        try:
            obj=next(tag for tag in cache.tags.values()if tag['group']=='vehi'and tag['name']==declared['name'])
            reader=Reader(cache,platform=='mcc');model=cache.tags[reader.word(obj['offset']+0x38)]
            contracts[platform]=reader.model(model)
            digest=hashlib.sha256(json.dumps(contracts[platform],sort_keys=True,separators=(',',':')).encode()).hexdigest()
            if digest!=declared['contract']:raise ValueError('Current authored model differs from original census')
        finally:
            if hasattr(cache,'close'):cache.close()
            else:
                cache.data.close();cache.stream.close()
    fixture='tests/destruction_runtime_tu15_tests.cpp';before=(inputs/fixture).read_text();after,edits=fixture_source(before,args.case)
    if args.capture_children_original:
        after,capture_edits=capture_source(after);edits+=capture_edits
    if args.capture_body_original:
        after,capture_edits=capture_body_source(after);edits+=capture_edits
    source=owner/'source';target=source/fixture;target.parent.mkdir(parents=True);target.write_text(after)
    libraries,pins=native_link_inputs(build);security=build/'online-protocol/Release/libhalo2_online_security_ffi.so';pins[str(security)]=sha(security)
    headers={str(p):sha(p)for parent in ('include','src','tests/helpers')for p in (inputs/parent).rglob('*.hpp')if p.is_file()}
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2CampaignVehicleProbe LANGUAGES CXX)
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
    candidate=None;link_map=owner/'build/probe.map'
    if args.native_child_owner_candidate:
        if not args.children_qualification or not args.children_negative:parser.error('The native child candidate requires both original and negative qualifications')
        qualified=json.loads(args.children_qualification.read_text());negative=json.loads(args.children_negative.read_text())
        if not qualified['passed']or qualified['no_call_original_i386_cases']!=45 or not negative['passed']or not negative.get('private_child_gate_mutation_rejected'):
            raise ValueError('Native child qualification failed')
        files=('include/halo2/source_damage_model_children_tu15.hpp','src/gameplay/objects/source_damage_model_children_native_tu15_internal.hpp',
            'src/gameplay/objects/source_damage_model_children_tu15.cpp')
        for name in files:
            if sha(ROOT/name)!=qualified['source_hashes'][name]:raise ValueError('Qualified child source drifted')
            path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes((ROOT/name).read_bytes())
        name='src/physics/physics_live_world_tu15.cpp';world_before=(inputs/name).read_text()
        include='#include "halo2/physics_live_world_tu15.hpp"\n'
        anchor='        if(source_damage_receiver_tu15_owns_entry(c.target)) {\n'
        addition='        if(c.target==0xdc540u) {\n            source_run_damage_model_children_tu15(m,a,fp,services(),o);return;\n        }\n'
        if world_before.count(include)!=1 or world_before.count(anchor)!=1:raise ValueError('Original child world route seam drifted')
        world_after=world_before.replace(include,include+'#include "halo2/source_damage_model_children_tu15.hpp"\n').replace(anchor,addition+anchor)
        world_path=source/name;world_path.parent.mkdir(parents=True,exist_ok=True);world_path.write_text(world_after)
        flags_file=build/'CMakeFiles/halo2_systems.dir/flags.make';flags={};custom=None
        prefix='# Custom options: CMakeFiles/halo2_systems.dir/'+name+'.o_OPTIONS = '
        for line in flags_file.read_text().splitlines():
            for kind in ('CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS'):
                if line.startswith(kind+' = '):flags[kind]=shlex.split(line.split(' = ',1)[1])
            if line.startswith(prefix):custom=line[len(prefix):].split(';')
        if custom!=['-fno-fast-math','-ffp-contract=off','-frounding-math']:raise ValueError('Original native world custom flags drifted')
        options=flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+custom
        if any(any(c in token for c in '\n\r";$')for token in options):raise ValueError('Unexpected native world compile flags')
        cmake+='add_library(native_children_world OBJECT '+name+' src/gameplay/objects/source_damage_model_children_tu15.cpp)\n'
        cmake+='target_include_directories(native_children_world PRIVATE "'+str(source/'include')+'" "'+str(source/'src')+'")\n'
        cmake+='target_compile_options(native_children_world PRIVATE "SHELL:'+shlex.join(options)+'")\n'
        cmake+='target_sources(probe PRIVATE $<TARGET_OBJECTS:native_children_world>)\n'
        cmake+='target_include_directories(probe BEFORE PRIVATE "'+str(source/'include')+'" "'+str(source/'src')+'")\n'
        cmake+='target_link_options(probe PRIVATE "-Wl,-Map,'+str(link_map)+'")\n'
        # Counter observation supplies no native gameplay input or callee result.
        counter_edits=[('#include "halo2/source_damage_material_native_tu15.hpp"\n',
            '#include "halo2/source_damage_material_native_tu15.hpp"\n#include "halo2/source_damage_model_children_tu15.hpp"\n'),
            ('    auto recorded=data;const auto rates=',
             '    auto recorded=data;recorded["native_model_children_activity"]=source_damage_model_children_native_activity();const auto rates=')]
        for needle,replacement in counter_edits:
            if after.count(needle)!=1:raise ValueError('Child activity diagnostic seam drifted')
            after=after.replace(needle,replacement)
        target.write_text(after);edits+=counter_edits
        candidate=dict(kind='native_model_children',object_target='native_children_world',source=name,before_sha256=hashlib.sha256(world_before.encode()).hexdigest(),after_sha256=sha(world_path),
            source_sha256={name:sha(source/name)for name in files},compile_options=options,original_flags_sha256=sha(flags_file),
            qualification=str(args.children_qualification.resolve()),qualification_sha256=sha(args.children_qualification),
            negative=str(args.children_negative.resolve()),negative_sha256=sha(args.children_negative),world_addition=addition,
            runtime_results_or_missing_callee_supplied=False)
    if args.native_bv_agent_candidate:
        if not args.bv_qualification:parser.error('BV candidate requires its existing original/frozen qualification')
        candidate,cmake,after,edits=bv_candidate(source,inputs,build,args.bv_qualification,cmake,after,edits)
        target.write_text(after);cmake+='target_link_options(probe PRIVATE "-Wl,-Map,'+str(link_map)+'")\n'
    if args.capture_body_original:
        name='src/gameplay/units/source_damage_delivery_runtime_generated.cpp';original=(inputs/name).read_text()
        needle='static void run_dc690(ObservedDamageState &s) {\n'
        addition='    SourcePhysicsEntryObservation diagnostic(s.memory,s.image==SourcePhysicsContactImage::TitleUpdate15?0xdd5e0u:0xdc690u,[&]{return s.save();});\n'
        if original.count(needle)!=1:raise ValueError('Original body boundary diagnostic seam drifted')
        path=source/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(original.replace(needle,needle+addition))
        flags_file=build/'CMakeFiles/halo2_systems.dir/flags.make';flags={};custom=[]
        prefix='# Custom options: CMakeFiles/halo2_systems.dir/'+name+'.o_OPTIONS = '
        for line in flags_file.read_text().splitlines():
            for kind in ('CXX_DEFINES','CXX_INCLUDES','CXX_FLAGS'):
                if line.startswith(kind+' = '):flags[kind]=shlex.split(line.split(' = ',1)[1])
            if line.startswith(prefix):custom=line[len(prefix):].split(';')
        options=flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+custom
        if any(any(c in token for c in '\n\r";$')for token in options):raise ValueError('Unexpected original damage compile flags')
        cmake+='add_library(body_boundary_diagnostic OBJECT '+name+')\n'
        cmake+='target_compile_options(body_boundary_diagnostic PRIVATE "SHELL:'+shlex.join(options)+'")\n'
        cmake+='target_sources(probe PRIVATE $<TARGET_OBJECTS:body_boundary_diagnostic>)\n'
        candidate['body_boundary_diagnostic']=dict(source=name,original_sha256=hashlib.sha256(original.encode()).hexdigest(),
            diagnostic_sha256=sha(path),addition=addition,compile_options=options,gameplay_instructions_changed=False)
    if args.capture_children_original or args.capture_body_original:
        flag=' --capture-body-original'if args.capture_body_original else''
        cmake+=f'add_test(NAME {args.case} COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- "{sys.executable}" -B "{Path(__file__).resolve()}" --run-original{flag} --binary $<TARGET_FILE:probe> --map "{map_path}" --case {args.case} --security-library "{security}" --native-report "${{CMAKE_BINARY_DIR}}/vehicle.json" --output "{args.output.resolve()/"actual-entry"}")\n'
    else:
        cmake+=f'add_test(NAME {args.case} COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:probe> "{map_path}" "{map_path.parent}" {args.case} "${{CMAKE_BINARY_DIR}}/vehicle.json")\n'
    cmake+=f'set_tests_properties({args.case} PROPERTIES TIMEOUT 360 ENVIRONMENT "HALO2_DESTRUCTION_FIXED_CLOCK=1;HALO2_DESTRUCTION_CONTINUATION=1;HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL=1;HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL=1;H2_SECURITY_LIBRARY={security}")\n'
    (source/'CMakeLists.txt').write_text(cmake);args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,owned_scratch=str(owner),native_owner=str(frozen),tool_sha256=sha(__file__),case=args.case,
        independent_full_cohort_pending=baseline_pending,baseline_admission=str(args.admission.resolve()),
        caller_substitutions=edits,fixture_before_sha256=hashlib.sha256(before.encode()).hexdigest(),fixture_after_sha256=sha(target),
        contract_digest=declared['contract'],contract=contracts['original'],current_original_and_mcc_contracts_equal=True,
        authority_reference_pins=reference_pins,native_libraries_sha256=pins,native_headers_sha256=headers,
        engine_libraries_changed=False,engine_candidate=candidate,capture_children_original=args.capture_children_original,
        capture_body_original=args.capture_body_original,
        full_definition_accepted=False,full_goal_complete=False,retained_large_artifacts=[])
    def save():(args.output/'probe.json').write_text(json.dumps(report,indent=2)+'\n')
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','probe','--filter','^'+args.case+'$',
        '--jobs','1','--test-jobs','1','--output',str(args.output/'validation'),'--retain-report','vehicle.json'];report['command']=command;save()
    code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    proof=args.output/'validation/evidence/vehicle.json';data=json.loads(proof.read_text())if proof.exists()else{}
    report.update(returncode=code,native_libraries_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in pins.items()),
        native_headers_unchanged=all(Path(p).exists()and sha(p)==pin for p,pin in headers.items()),
        authority_maps_unchanged=all(sha(p)==pin for p,pin in reference_pins.items()),
        error=data.get('error'),shots=data.get('shots'),minimum_body=data.get('minimum_body'),source_dead=data.get('source_dead'),target_retired=data.get('target_retired'))
    report['passed']=code==0 and data.get('passed')is True and data.get('post_destruction_continuation',{}).get('complete_encoded_world_sha256_comparisons')==16 and all(
        report[k]for k in ('native_libraries_unchanged','native_headers_unchanged','authority_maps_unchanged'))
    if candidate:
        text=link_map.read_text()if link_map.exists()else''
        candidate.update(old_world_archive_member_absent=bool(text)and'libhalo2_systems.a(physics_live_world_tu15.cpp.o)'not in text,
            replacement_object_linked=('CMakeFiles/'+candidate['object_target']+'.dir/src/physics/physics_live_world_tu15.cpp.o')in text,
            link_map_sha256=sha(link_map)if link_map.exists()else None,actual_native_calls=data.get('native_model_children_activity'))
        activity='native_bv_agent_entries'if candidate['kind']=='existing_compiled_BV_owner'else'native_model_children_activity'
        candidate['observed_activity_field']=activity;candidate['observed_activity']=data.get(activity)
        report['passed'] &= all(candidate[k]for k in ('old_world_archive_member_absent','replacement_object_linked'))and bool(data.get(activity))
    if args.capture_children_original or args.capture_body_original:
        path=args.output/'actual-entry/original/report.json';reference=json.loads(path.read_text())if path.exists()else{}
        report['passed']=code==0 and reference.get('passed')is True and all(report[k]for k in ('native_libraries_unchanged','native_headers_unchanged','authority_maps_unchanged'))and all(
            candidate[k]for k in ('old_world_archive_member_absent','replacement_object_linked'))
        report['actual_original']=dict(report=str(path),passed=reference.get('passed')is True,
            original_instructions=reference.get('original_instructions'),compared_memory_bytes=reference.get('compared_memory_bytes'))
        if args.capture_body_original:
            diagnostic=candidate['body_boundary_diagnostic']
            diagnostic.update(original_archive_member_absent=bool(text)and'libhalo2_systems.a(source_damage_delivery_runtime_generated.cpp.o)'not in text,
                replacement_object_linked='CMakeFiles/body_boundary_diagnostic.dir/'+diagnostic['source']+'.o'in text)
            report['passed'] &= diagnostic['original_archive_member_absent']and diagnostic['replacement_object_linked']
    save();print(json.dumps({k:report[k]for k in ('passed','error','shots','minimum_body','source_dead','target_retired')}),flush=True)
    return 0 if report['passed']else code or 1


if __name__=='__main__':raise SystemExit(main())
