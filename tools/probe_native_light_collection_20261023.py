#!/usr/bin/env python3
"""Observe original native light/PVS inputs after the actual main-view fade.

Private runtime clone only: the pStencil raw native snapshot/collection reader
is used as a diagnostic, with deferred selection and no rendering/publication.
This is neither a Classic renderer repair nor original collection execution.
"""
import hashlib,json,os,re,subprocess,sys,tempfile
from pathlib import Path
from build_launcher_engine import source_hashes
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/combat-presentation-audit-20261022'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=owner/'source'
    pins=json.loads((BASE/'repair-followup-20261023/source-hashes.json').read_text())
    assert source_hashes(source)==pins
    base=BASE/'repair-native-attached-light-collection';out=base;attempt=2
    while (out/'receipt.json').exists():out=base/('attempt-'+str(attempt));attempt+=1
    out.mkdir(parents=True,exist_ok=True)
    original=source/'src/gameplay/multiplayer/multiplayer_runtime.cpp';text=original.read_text()
    text='#include "halo2/pstencil/campaign_lights.hpp"\n#include "halo2/source_physics_entry_observation.hpp"\n'+text
    old='  auto all_effect_light_definitions=world_effects.light_definitions();'
    assert text.count(old)==1
    text=text.replace(old,'  pstencil::CampaignLights combat_native_light_query(cache,options.bsp);\n'+old)
    old='''        if(options.shadow_admission_observer)models.view_calls=[&,i](bool before,auto entry,
            const auto& abi,const auto& memory) {
          options.shadow_admission_observer(before,i,current.tick,0u,entry,abi,memory);
        };'''
    new='''        models.view_calls=[&,i](bool before,auto entry,
            const auto& abi,const auto& memory) {
          if(options.shadow_admission_observer)
            options.shadow_admission_observer(before,i,current.tick,0u,entry,abi,memory);
          if(!before && entry==0x3d2b0u)
            std::cout<<"[NATIVE-LIGHT-AUDIT] first-person-cache view="<<i<<" tick="<<current.tick
                <<" owner="<<Bytes(memory.view(0x4ce3f0u,4)).u32(0)
                <<" models="<<Bytes(memory.view(0x4ce3f4u,4)).u32(0)<<'\\n';
          if(!before && entry==0xc35d0u)try {
            const auto audit=combat_native_light_query.view(source,camera.position,
                light_collection,light_candidates,std::int32_t(frames+1),false,true);
            unsigned active{},visible{};
            for(const auto& row:audit.records) {
              if(!row.definition || !(row.definition->flags&0x20u))continue;
              const bool live=source_effect_light_live(row.runtime);
              const bool collected=std::find(audit.visible.begin(),audit.visible.end(),row.runtime.identity)!=audit.visible.end();
              active+=live;visible+=collected;
              if(live) {
                const auto word=[&](auto a){return Bytes(source.memory().view(a,4)).u32(0);};
                const auto record=word(word(0x4ecbdcu)+0x44u)+(row.runtime.identity&65535u)*272u;
                std::cout<<"[NATIVE-LIGHT-AUDIT] actual id="<<row.runtime.identity
                    <<" fade="<<std::bit_cast<float>(word(record+0x100u))
                    <<" diffuse="<<std::bit_cast<float>(word(record+0xd4u))<<','<<std::bit_cast<float>(word(record+0xd8u))<<','<<std::bit_cast<float>(word(record+0xdcu))<<'\\n';
              }
              auto priority=source.prepare_source_call();priority.gp[6]=row.runtime.owner;
              source.run_actual(0x3e9b0u,priority);
              std::cout<<"[NATIVE-LIGHT-AUDIT] view="<<i<<" tick="<<current.tick
                  <<" id="<<row.runtime.identity<<" definition="<<row.definition_identity
                  <<" owner="<<row.runtime.owner<<" flags="<<row.definition->flags
                  <<" node="<<row.runtime.attached_node<<" cluster="<<row.runtime.cluster
                  <<" live="<<live<<" collected="<<collected<<" priority="<<(priority.gp[0]&255u)<<'\\n';
            }
            std::cout<<"[NATIVE-LIGHT-AUDIT] summary view="<<i<<" tick="<<current.tick
                <<" native_records="<<audit.records.size()<<" native_visible="<<audit.visible.size()
                <<" fp_active="<<active<<" fp_visible="<<visible<<'\\n';
          }catch(const std::exception& error) {
            std::cout<<"[NATIVE-LIGHT-AUDIT] error="<<error.what()<<'\\n';
          }
        };'''
    assert text.count(old)==1;text=text.replace(old,new)
    marker_view='          auto lights=world_effects.source_light_view(source,options.bsp,light_collection,light_candidates,'
    assert text.count(marker_view)==1
    trace='          std::uint32_t audit_shape{};\n          SourcePhysicsEntryObserverBinding entries(source.memory(),[&](bool before,auto entry,const auto& abi) {\n            if(entry!=0x31570u && entry!=0xc3a20u && entry!=0xc3ab0u)return;\n            const auto word=[&](auto a){return Bytes(source.memory().view(a,4)).u32(0);};\n            if(before && entry==0x31570u)audit_shape=word(abi.gp[4]+4u);\n            std::cout<<"[NATIVE-LIGHT-AUDIT] entry="<<entry<<" before="<<before<<" tick="<<current.tick<<" view="<<i\n                <<" eax="<<abi.gp[0]<<" ecx="<<abi.gp[1]<<" edx="<<abi.gp[2]<<" edi="<<abi.gp[7];\n            if(!before && entry==0x31570u)std::cout<<" type="<<Bytes(source.memory().view(audit_shape,2)).u16(0)\n                <<" shape_radius="<<std::bit_cast<float>(word(audit_shape+0x44u));\n            std::cout<<\'\\n\';\n          });\n'
    text=text.replace(marker_view,trace+marker_view)
    marker='          effect_light_views[i]=lights;effect_light_receivers[i]=receivers;'
    assert text.count(marker)==1
    audit='          if(!lights.visible.empty()) {\n            std::cout<<"[NATIVE-LIGHT-AUDIT] selection view="<<i<<" tick="<<current.tick\n                <<" visible="<<lights.visible.size()<<" candidates="<<lights.candidates.size()\n                <<" selected="<<lights.selected.size()<<" receivers="<<receivers.size()<<\'\\n\';\n            for(const auto& row:lights.records)if(std::find(lights.visible.begin(),lights.visible.end(),row.runtime.identity)!=lights.visible.end())\n              std::cout<<"[NATIVE-LIGHT-AUDIT] visible id="<<row.runtime.identity<<" definition="<<row.definition_identity\n                  <<" fade="<<row.runtime.overall_fade<<" color="<<row.runtime.state.diffuse.x<<\',\'<<row.runtime.state.diffuse.y<<\',\'<<row.runtime.state.diffuse.z<<\'\\n\';\n            for(const auto& gap:world_effects.diagnostics().unavailable)if(gap.starts_with("Native "))std::cout<<"[NATIVE-LIGHT-AUDIT] gap="<<gap<<\'\\n\';\n          }\n'
    extra='            for(const auto id:lights.visible) {\n              auto active=source.prepare_source_call();active.gp[0]=id;source.run_actual(0xc5170u,active);\n              auto local=source.prepare_source_call();local.gp[0]=id;source.run_actual(0x31500u,local);\n              const auto word=[&](auto a){return Bytes(source.memory().view(a,4)).u32(0);};\n              const auto feature=word(0x51dae8u);\n              std::cout<<"[NATIVE-LIGHT-AUDIT] admission id="<<id<<" active="<<(active.gp[0]&255u)\n                  <<" local="<<(local.gp[0]&255u)<<" renderer="<<std::to_integer<unsigned>(source.memory().view(feature+8u,1)[0])<<\'\\n\';\n            }\n'
    audit=audit.replace('            for(const auto& gap:',extra+'            for(const auto& gap:')
    text=text.replace(marker,audit+marker)
    chosen='''            for(const auto& selected:lights.selected) {
              const auto row=std::find_if(lights.records.begin(),lights.records.end(),[&](const auto& r){return r.runtime.identity==selected.identity;});
              if(row==lights.records.end() || !row->definition)continue;
              const auto& r=row->runtime;
              std::cout<<"[NATIVE-LIGHT-AUDIT] chosen id="<<selected.identity<<" tick="<<current.tick
                  <<" fade="<<selected.requested_fade<<" definition="<<row->definition_identity
                  <<" flags="<<row->definition->flags<<" radius="<<r.spatial_radius
                  <<" position="<<r.position.x<<','<<r.position.y<<','<<r.position.z
                  <<" forward="<<r.forward.x<<','<<r.forward.y<<','<<r.forward.z
                  <<" center="<<r.spatial_center.x<<','<<r.spatial_center.y<<','<<r.spatial_center.z<<'\\n';
            }
'''
    needle='            for(const auto& gap:world_effects.diagnostics().unavailable)'
    assert text.count(needle)==1;text=text.replace(needle,chosen+needle)
    omit_native_pool=bool(os.environ.get('HALO2_NATIVE_LIGHT_AUDIT_OMIT_POOL'))
    if omit_native_pool:
        marker='const bool native_light_pool=source_model_world &&'
        assert text.count(marker)==1
        text=text.replace(marker,'const bool native_light_pool=false && source_model_world &&')
    omit_fp_receiver=bool(os.environ.get('HALO2_NATIVE_LIGHT_AUDIT_OMIT_FP_RECEIVER'))
    if omit_fp_receiver:
        marker='          effect_light_views[i]=lights;effect_light_receivers[i]=receivers;'
        guard='''          std::erase_if(receivers,[&](const auto& receiver) {
            const auto row=std::find_if(lights.records.begin(),lights.records.end(),[&](const auto& r){return r.runtime.identity==receiver.identity;});
            return row!=lights.records.end() && row->definition && (row->definition->flags&0x20u) && !(row->definition->flags&0x40000u);
          });
'''
        assert text.count(marker)==1;text=text.replace(marker,guard+marker)
    paired_marker='    timing.submitted=renderer.draw(views.front().camera,'
    assert text.count(paired_marker)==1
    paired='    if(capture && std::getenv("HALO2_AUDIT_PAIRED_LIGHT_PIXELS")) {\n      const auto before=renderer.bsp_postprocess_state();\n      auto paired_views=views;\n      for(auto& v:paired_views) {\n        v.lens_queries={};v.completed_material_upload_observer={};v.completed_bsp_draw_observer={};\n      }\n      for(const auto label:{"baseline","repeat","without-light"}) {\n        if(before)renderer.restore_bsp_postprocess_state(*before);\n        if(std::string_view(label)=="without-light")for(auto& v:paired_views)v.raw_effect_lights={};\n        if(!renderer.draw(paired_views.front().camera,options.screenshot.string()+"."+label+".ppm",\n            {},false,{},{},hud_vertices,paired_views,{},pause_backdrops))\n          throw FormatError("Same-frame first-person light pixel diagnostic did not submit");\n      }\n      if(before)renderer.restore_bsp_postprocess_state(*before);\n    }\n'
    text=text.replace(paired_marker,paired+paired_marker)
    clone=source/'tools/combat_native_light_query_runtime.cpp'
    if not clone.exists() or clone.read_text()!=text:clone.write_text(text)
    cmake=source/'CMakeLists.txt';saved=cmake.read_bytes()
    maps=(ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11').resolve()
    players=int(os.environ.get('HALO2_NATIVE_LIGHT_AUDIT_PLAYERS','2'));assert players in (1,2)
    fixture=source/'tools/audit_combat_presentation_20261022.cpp'
    fixture_before=sha(fixture);fixture_data=(ROOT/'tools/audit_combat_presentation_20261022.cpp').read_bytes()
    if fixture.read_bytes()!=fixture_data:fixture.write_bytes(fixture_data)
    report=owner/'build/native-light-query.json'
    # A failed compile must never retain an earlier run's result as fresh evidence.
    report.unlink(missing_ok=True)
    # Dedicated one-off CTest stays inside this active scratch owner. The
    # existing audit supplies real SDL input and completed GPU observations.
    with cmake.open('a') as f:f.write('''
add_executable(combat_native_light_query_audit EXCLUDE_FROM_ALL
 tools/audit_combat_presentation_20261022.cpp tools/combat_native_light_query_runtime.cpp)
target_include_directories(combat_native_light_query_audit PRIVATE src/gameplay/multiplayer)
# Keep the read-only diagnostic dispatcher cheap to rebuild. Native owners,
# renderer libraries and all production targets retain their normal options.
target_compile_options(combat_native_light_query_audit PRIVATE -O0)
target_link_libraries(combat_native_light_query_audit PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
add_dependencies(combat_native_light_query_audit halo2_shaders)
add_test(NAME native_held_light_query_audit COMMAND combat_native_light_query_audit fire
 "'''+str(maps/'coagulation.map')+'" "'+str(report)+'''" plasma_pistol 0)
''')
    try:
        with tempfile.TemporaryDirectory(prefix='native-light-query-',dir=owner) as temporary:
            scratch=Path(temporary)
            with private_display(scratch) as display:
                env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',
                    XDG_CACHE_HOME=str(owner/'graphics-cache'),HALO2_COMBAT_AUDIT_CAPTURE=str(scratch/'frame.ppm'),HALO2_AUDIT_LOCAL_PLAYERS=str(players))
                cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
                    '--target','combat_native_light_query_audit','--filter','^native_held_light_query_audit$',
                    '--jobs','2','--test-jobs','1','--retain-report','native-light-query.json',
                    '--retain-report','Testing/Temporary/LastTest.log','--output',str(out/'validation')]
                with (out/'command.log').open('w') as log:
                    code=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT).returncode
    finally:cmake.write_bytes(saved)
    assert source_hashes(source)==pins
    log=out/'validation/evidence/Testing/Temporary/LastTest.log'
    raw=log.read_text(errors='replace') if log.exists() else ''
    observations=[line for line in raw.splitlines() if line.startswith('[NATIVE-LIGHT-AUDIT]')]
    summaries=[]
    for line in observations:
        if 'summary view=' in line:summaries.append({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',line)})
    actual_path=out/'validation/evidence/native-light-query.json'
    actual=json.loads(actual_path.read_text()) if actual_path.exists() else {}
    receipt=dict(scope=__doc__,local_players=players,native_pool_omitted_for_regression_control=omit_native_pool,fixture_sha256=sha(fixture),fixture_before_sha256=fixture_before,diagnostic_dispatcher_optimization="O0",exit_code=code,production_unchanged=source_hashes(source)==pins,
        production_runtime_sha256=sha(original),private_clone_sha256=sha(clone),
        native_snapshot_reader_sha256=sha(source/'src/pstencil/native_lights.cpp'),
        diagnostic_collection_sha256=sha(source/'src/pstencil/campaign_lights.cpp'),
        executable_sha256=sha(owner/'build/combat_native_light_query_audit') if code==0 else None,
        summaries=summaries,observations=observations,
        actual_raw_fp_packets=len(actual.get('actual_raw_first_person_light_packets',[])),
        raw_native_fp_records=len(actual.get('actual_source_first_person_light_census',[])),
        native_priority_or_admission_result_supplied=False,
        rendered_light_repair_verified=False,launcher_gameplay_verified=False)
    active_frames=sum(row.get('fp_active',0)>0 for row in summaries)
    packets=receipt['actual_raw_fp_packets']
    muzzle=bool(os.environ.get('HALO2_AUDIT_FP_MUZZLE_LIGHT'))
    expect_muzzle_guard=bool(os.environ.get('HALO2_NATIVE_LIGHT_AUDIT_EXPECT_MUZZLE_GUARD'))
    assert not expect_muzzle_guard or muzzle
    receipt.update(muzzle_light_case=muzzle,muzzle_tap_case=bool(os.environ.get('HALO2_AUDIT_FP_LIGHT_TAP')),
        expected_existing_muzzle_guard=expect_muzzle_guard,nonworld_fp_receiver_omitted=omit_fp_receiver)
    caches=[{k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',row)} for row in observations if 'first-person-cache view=' in row]
    receipt['first_person_cache_samples']=caches
    receipt['first_person_cache_qualified']=any(row.get('owner') not in (None,0,0xffffffff) and 0<row.get('models',0)<=4 for row in caches)
    # Whole original31570 rejects these unflagged first-person lights in
    # split screen before the model cache is requested. There must still be
    # real native pool observations, but no admitted FP frame/cache is due.
    eligible_frames=active_frames if players==1 else receipt['raw_native_fp_records']
    receipt['integration_signal_passed']=bool(code==0 and eligible_frames and
        (packets==0 if omit_native_pool or players==2 or expect_muzzle_guard or omit_fp_receiver else packets>0) and
        (players==2 or not muzzle or expect_muzzle_guard or receipt['first_person_cache_qualified']))
    receipt['active_native_fp_frames']=active_frames
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('scope','summaries','observations','first_person_cache_samples')}),flush=True)
    return code if code else (0 if receipt['integration_signal_passed'] else 1)

if __name__=='__main__':raise SystemExit(main())
