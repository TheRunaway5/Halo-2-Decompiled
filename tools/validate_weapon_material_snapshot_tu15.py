#!/usr/bin/env python3
"""Validate weapon material changes against one disposable source snapshot.

Use tools/run_test.py. CMake validation still runs through tools/run_tests.py.
Large original assets/reference fixtures are borrowed; changed code and selected
fixtures are copied and hashed so concurrent header/library edits cannot mix ABIs.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from weapon_test_cache import mcc_runtime_inputs,sha

ROOT=Path(__file__).absolute().parents[1]
VEHICLE_HUD_CASES=('ghost','banshee','scorpion','wraith','warthog-mg','warthog-gauss','spectre')
VEHICLE_SECONDARY_CASES=('banshee','scorpion')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mcc-root',type=Path,required=True,help='Prepared MCC cache root; Xbox/Vista cache fallback is forbidden')
    parser.add_argument('--application',action='store_true')
    parser.add_argument('--live-gauge',action='store_true')
    parser.add_argument('--live-sword-material',action='store_true',help='Actual MCC sword pickup,alive source owner and ready material inputs; audio/full sequence remain separate')
    parser.add_argument('--live-sword-melee',action='store_true',help='Actual MCC sword attack path with strict advancing-pose and owned-audio requirements; declared stationary placement')
    parser.add_argument('--live-staged-reload',action='store_true',help='Actual MCC shotgun fire/partial reload for both species, retaining source ammunition and authored pose evidence')
    parser.add_argument('--live-charge',action='store_true',help='Actual MCC plasma-pistol full charge and early release for both species, retaining source pose and projectile evidence')
    parser.add_argument('--live-all-handheld',action='store_true',help='26 actual MCC firearm/Sword cases on four maps,plus both dual-SMG cases; all strict feature/geometry/audio assertions remain')
    parser.add_argument('--source-sound-startup',action='store_true',help='Observe actual224EC0 driver startup through the native MCC world without supplied readiness or results')
    parser.add_argument('--source-sound-host-output',action='store_true',help='Also observe actual224EC0 with the existing native speaker preference and a real PulseAudio callback resource; dummy/disk output is rejected')
    parser.add_argument('--particle-model-math',action='store_true',help='Compare actual MCC Turf PRTM matrices/spans with unchanged TU1.5 instructions; all raw inputs/outputs are disposable')
    parser.add_argument('--reference-python',type=Path,default=ROOT/'.tools/python/bin/python' if (ROOT/'.tools/python/bin/python').is_file() else Path(sys.executable),help='Python with Capstone/Unicorn for optional original-code arithmetic references')
    parser.add_argument('--source-world-weapon-publication',action='store_true',help='Compare actual loose MCC source generations,roots,ammunition and material fields with canonical-engine world publication for both species')
    parser.add_argument('--vehicle-weapon-hud',action='store_true',help='Seven canonical MCC Coagulation vehicle/armed-seat entry,source weapon HUD and exit cases;preserve actual map spawn inputs')
    parser.add_argument('--vehicle-weapon-fire',action='store_true',help='Seven canonical MCC vehicle primary-trigger cases;require original preparation and constructed authored projectile generations,then normal exit')
    parser.add_argument('--vehicle-weapon-secondary-fire',action='store_true',help='Actual Banshee multiplayer input2 suppression and Scorpion input1/barrel1 firing through original command delivery')
    parser.add_argument('--owned-build-worker',action='store_true',help='Use the surrounding run_test.py scratch/build while its owner continues an active investigation; outer cleanup remains mandatory')
    parser.add_argument('--prepare-only',action='store_true',help='Freeze/register the requested cohort for an active owned coordinator; status is prepared,never passed,and no validation runs')
    parser.add_argument('--copy-drift-retries',type=int,default=0,help='Re-copy concurrently changed inputs and verify the entire snapshot again, at most five times')
    parser.add_argument('--first-person-bindings',action='store_true',help='Focused MCC first-person/body and compiled-query-key checks without the native render application')
    parser.add_argument('--all-multiplayer-bindings',action='store_true',help='Check every prepared MCC multiplayer scenario within the owned validation build')
    parser.add_argument('--live-firing',action='store_true')
    parser.add_argument('--live-dual',action='store_true',help='Actual SDL dual-SMG per-hand source histories for both species')
    parser.add_argument('--trace-dual-selection',action='store_true',help='Read-only original offhand equip admission and transfer observations')
    parser.add_argument('--dual-drop-lifetime',action='store_true',help='Observe actual offhand detachment and neutral source-world continuation')
    parser.add_argument('--dual-world-poses',action='store_true',help='Original completed loose-weapon draw palettes and native LAN pose transport')
    parser.add_argument('--campaign-weapons',action='store_true',help='Actual original Cairo actor and independently submitted held-weapon main units')
    parser.add_argument('--campaign-weapon-application',action='store_true',help='Actual campaign application Vulkan weapon draw commands')
    parser.add_argument('--campaign-scenery-lighting',action='store_true',help='Original Cairo per-instance baked/dynamic scenery consumer at the captured application camera')
    parser.add_argument('--campaign-copy-dispatch',action='store_true',help='Shared live CRT overlapping-copy dispatch across original branch inputs')
    parser.add_argument('--campaign-native-hud',action='store_true',help='Cold native world selects its original authored HUD globals, then passes shared-copy cases')
    parser.add_argument('--verify-source-shot-tick',action='store_true')
    parser.add_argument('--trace-projectile-births',action='store_true')
    parser.add_argument('--trace-projectile-retirement',action='store_true')
    parser.add_argument('--trace-weapon-effect-requests',action='store_true',help='Read-only original108A70 entry/return requests in the live firing fixture')
    parser.add_argument('--trace-sword-selection',action='store_true',help='Read-only original Sword admission,candidate,availability and graph-query entry/return observations')
    parser.add_argument('--trace-reload-timing',action='store_true',help='Read-only original magazine transition,timing and graph-query entry/return observations')
    parser.add_argument('--clear-firing-lane',action='store_true')
    parser.add_argument('--compare-live-glow',action='store_true')
    parser.add_argument('--glow-view-cohorts',action='store_true')
    parser.add_argument('--glow-cohort',action='append',choices=('spartan2','spartan4','elite1','elite2','elite4'))
    parser.add_argument('--live-firing-placement',choices=('both','pinned','native_motion'),default='both')
    parser.add_argument('--jobs',type=int,default=2)
    parser.add_argument('--release-only',action='store_true')
    parser.add_argument('--cmake-arg',action='append',default=[])
    args=parser.parse_args()
    vehicle_firing=args.vehicle_weapon_fire or args.vehicle_weapon_secondary_fire
    if args.live_all_handheld:
        args.live_dual=True;args.source_world_weapon_publication=True
    if args.source_sound_host_output:args.source_sound_startup=True
    if args.owned_build_worker and not args.release_only:
        parser.error('--owned-build-worker requires --release-only so an active build keeps one configuration')
    if args.prepare_only and not args.owned_build_worker:
        parser.error('--prepare-only requires --owned-build-worker and its enclosing active scratch owner')
    if args.campaign_scenery_lighting:
        parser.error('--campaign-scenery-lighting replays an ISO reference camera; it is not MCC runtime acceptance')
    mcc_maps,cache_inputs=mcc_runtime_inputs(args.mcc_root)
    handheld_cases=[]
    if args.live_all_handheld:
        handheld_cases=[(weapon,'lockout.map')for weapon in
            ('magnum','needler','plasma_pistol','battle_rifle','plasma_rifle','shotgun','sniper_rifle','sword')]
        handheld_cases += [('beam_rifle','burial_mounds.map'),('covenant_carbine','burial_mounds.map'),
            ('rocket_launcher','burial_mounds.map'),('brute_shot','backwash.map'),('brute_plasma_rifle','turf.map')]
        from mcc_content import MccCache
        inputs=[]
        for name in sorted({name for _,name in handheld_cases}):
            path=mcc_maps/name
            assert path.is_file(),'Prepared MCC handheld map is absent: '+str(path)
            MccCache(path) # Its parser rejects non-MCC headers; no cache fallback.
            inputs.append(dict(path=str(path),bytes=path.stat().st_size,sha256=sha(path)))
        cache_inputs['all_handheld_source_inputs']=inputs
    if args.vehicle_weapon_hud or vehicle_firing:
        from mcc_content import MccCache
        path=mcc_maps/'coagulation.map';MccCache(path)
        cache_inputs['vehicle_weapon_hud_source_input']=dict(path=str(path),bytes=path.stat().st_size,sha256=sha(path))
    if args.particle_model_math:
        from mcc_content import MccCache
        path=mcc_maps/'turf.map';MccCache(path)
        cache_inputs['particle_model_source_input']=dict(path=str(path),bytes=path.stat().st_size,sha256=sha(path))
        args.reference_python=args.reference_python.expanduser().absolute()
        command=[str(args.reference_python),'-B','-c',
            'import json,capstone,unicorn;print(json.dumps(dict(capstone=capstone.__version__,unicorn=unicorn.__version__)))']
        versions=json.loads(subprocess.check_output(command,text=True))
        cache_inputs['particle_model_reference_python']=dict(path=str(args.reference_python),dependency_probe=command,versions=versions)
    binding_maps=[mcc_maps/'lockout.map']
    if args.all_multiplayer_bindings:
        assert args.first_person_bindings or args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld
        from mcc_content import MccCache
        binding_maps=[];binding_inputs=[]
        for path in sorted(mcc_maps.glob('*.map')):
            metadata=MccCache(path)
            scenario=metadata.tags.get(metadata.u32(metadata.index+12))
            if scenario and scenario['group']=='scnr' and scenario['name'].startswith('scenarios\\multi\\'):
                binding_maps.append(path)
                binding_inputs.append(dict(path=str(path),bytes=path.stat().st_size,
                    sha256=sha(path),scenario=scenario['name']))
        assert mcc_maps/'lockout.map' in binding_maps,'Prepared MCC multiplayer scenario set lost Lockout'
        cache_inputs['all_multiplayer_first_person_binding_inputs']=binding_inputs
    assert not args.trace_projectile_retirement or args.trace_projectile_births
    assert not args.trace_weapon_effect_requests or args.live_firing or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld
    assert not args.trace_sword_selection or args.live_sword_melee
    assert not args.trace_reload_timing or args.live_staged_reload
    assert not args.dual_drop_lifetime or args.live_dual
    assert not args.trace_dual_selection or args.live_dual
    assert not args.dual_world_poses or args.live_dual
    assert not args.campaign_weapon_application or (args.campaign_weapons and args.application)
    assert not args.campaign_scenery_lighting or args.campaign_weapons
    assert not args.campaign_copy_dispatch or args.campaign_weapons
    assert not args.campaign_native_hud or (args.campaign_weapons and args.campaign_copy_dispatch)
    assert not args.clear_firing_lane or args.live_firing
    assert not args.compare_live_glow or (args.live_firing and args.clear_firing_lane and args.live_firing_placement=='pinned')
    assert not args.glow_view_cohorts or args.compare_live_glow
    assert not args.glow_cohort or args.glow_view_cohorts
    cohorts={'spartan2':(2,2),'spartan4':(2,4),'elite1':(3,1),'elite2':(3,2),'elite4':(3,4)}
    live_cases=([(False,label,*cohorts[label])for label in (args.glow_cohort or ['spartan4','elite1','elite4'])]
        if args.glow_view_cohorts else [(motion,'native_motion'if motion else'pinned',2,1)
            for motion in (False,True)if args.live_firing_placement=='both' or args.live_firing_placement==('native_motion'if motion else'pinned')])
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute();output.mkdir(exist_ok=False)
    source=scratch/'source';source.mkdir()
    copied=[]
    def copy_file(path):
        relative=path.relative_to(ROOT);destination=source/relative
        destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(path,destination);copied.append(relative)
    def copy_tree(name):
        base=ROOT/name
        for path in sorted(base.rglob('*')):
            if any(part in ('__pycache__','.git','target') for part in path.relative_to(base).parts):continue
            if path.is_file():copy_file(path)
    copy_file(ROOT/'CMakeLists.txt')
    for name in ('src','include','cmake','shaders','third_party','external','rust'):
        copy_tree(name)
    for path in sorted((ROOT/'tools').iterdir()):
        if path.is_file() and path.suffix in ('.py','.cpp','.hpp','.h','.c','.S','.sh','.txt'):
            copy_file(path)
    for name in ('data','reference','pstencil'):
        if (ROOT/'tools'/name).exists():(source/'tools'/name).symlink_to(ROOT/'tools'/name,target_is_directory=True)
    for path in sorted((ROOT/'tests').iterdir()):
        if path.is_file():copy_file(path)
        elif path.is_dir() and path.name not in ('fixtures','__pycache__'):
            (source/'tests'/path.name).symlink_to(path,target_is_directory=True)
    fixtures=source/'tests/fixtures';fixtures.mkdir()
    selected={'weapon_material_builtins_tu15.json','weapon_gauge_animation_tu15.json','weapon_heat_shader_original.tsv',
        'weapon_primary_charge_tu15_original.json','retail_source_weapon_shader_input.tsv','tu15_ai_behavior_descriptors.json',
        'mcc_gun_hinge_node_map_tu15.json','native_tu15_vehicle_render_operands.original.inc',
        'weapon_ammunition_digits_tu15.json','object_material_random_constant_tu15.json',
        'network_configuration_defaults_tu15.json','game_language_tu15_original.json','weapon_alive_material_tu15.json',
        'weapon_firing_material_tu15.json','native_tu15_melee_candidate_table.original.inc',
        'native_tu15_game_engine_methods.original.inc'}
    for path in (ROOT/'tests/fixtures').iterdir():
        # Small expected-data files can change with another chat's source
        # work. Freeze them alongside that source; borrow only larger retained
        # reference inputs and fixture directories.
        if path.name in selected or (path.is_file() and path.stat().st_size<=65536):copy_file(path)
        else:(fixtures/path.name).symlink_to(path,target_is_directory=path.is_dir())
    for name in ('assets','analysis','resources','data','multiplayer','.tools'):
        if (ROOT/name).exists():(source/name).symlink_to(ROOT/name,target_is_directory=True)
    if not 0<=args.copy_drift_retries<=5:parser.error('--copy-drift-retries must be 0..5')
    copy_retries=[]
    for attempt in range(args.copy_drift_retries+1):
        pins={str(relative):sha(source/relative) for relative in copied}
        drift=[str(relative) for relative in copied if sha(ROOT/relative)!=pins[str(relative)]]
        if not drift or attempt==args.copy_drift_retries:break
        copy_retries.append(drift)
        for relative in drift:shutil.copy2(ROOT/relative,source/relative)
    manifest=dict(source=str(source),source_files_sha256=pins,source_drift_during_copy=drift,
        changed_code_and_selected_fixtures_are_private_copies=True,
        small_regular_fixture_copy_limit_bytes=65536,
        borrowed_large_original_inputs_are_not_copied=True,runner_scratch=str(scratch),
        runtime_cache_inputs=cache_inputs,concurrent_copy_retries=copy_retries)
    (output/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    assert not drift,drift[:10]
    if args.trace_projectile_births or vehicle_firing or args.dual_drop_lifetime or args.trace_weapon_effect_requests or args.trace_sword_selection or args.trace_reload_timing or args.trace_dual_selection:
        assert not args.trace_projectile_births or args.live_firing or vehicle_firing,'Projectile birth observation requires the live firing fixture'
        instrumentation=[]
        entries=tuple(([0xb9b70]if args.trace_projectile_births or vehicle_firing else [])+
            ([0xba490]if args.trace_projectile_retirement or args.dual_drop_lifetime or vehicle_firing else [])+
            ([0x108a70]if args.trace_weapon_effect_requests else [])+
            ([0xebfa0,0xebf20,0x16b31b,0x1d3d30,0xe13b0,0x169863,0x1d5320,0x1d46a0,0x1d3c80]
             if args.trace_sword_selection else [])+
            ([0x10ab50,0x16b44a,0x16b31b,0x1d3d30]if args.trace_reload_timing else [])+
            ([0xeaf00,0xe9530,0xeaa60,0xeac70,0xcfb80,0x113190,0x1123b0]if args.trace_dual_selection else []))
        entries=tuple(dict.fromkeys(entries))
        for path in sorted((source/'src').rglob('*generated.cpp')):
            text=path.read_text()
            observed_entries=[]
            for entry in entries:
                marker=f'static void run_{entry:x}(WorldState &s) {{'
                if marker not in text:continue
                assert text.count(marker)==1
                text=text.replace(marker,marker+f'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x{entry:x}u,[&]{{return s.save();}});',1)
                observed_entries.append(f'{entry:X}')
            if not observed_entries:continue
            text='#include "halo2/source_physics_entry_observation.hpp"\n'+text
            path.write_text(text)
            instrumentation.append(dict(path=str(path.relative_to(source)),
                original_sha256=pins[str(path.relative_to(source))],observed_sha256=sha(path),entries=observed_entries))
        assert instrumentation
        observed={entry for row in instrumentation for entry in row['entries']}
        assert {f'{entry:X}'for entry in entries}<=observed,'A requested original diagnostic entry has no compiled owner'
        observation_name=('vehicle-projectile-birth-retirement-observation.json'if vehicle_firing else
            'weapon-effect-request-observation.json'if args.trace_weapon_effect_requests else
            'sword-selection-observation.json'if args.trace_sword_selection else
            'reload-timing-observation.json'if args.trace_reload_timing else
            'dual-selection-observation.json'if args.trace_dual_selection else
            'projectile-birth-observation.json'if args.trace_projectile_births else'dual-drop-retirement-observation.json')
        (output/observation_name).write_text(json.dumps(dict(
            entries=[f'{entry:X}'for entry in entries],read_only_entry_return_observation=True,original_instruction_changes=[],
            supplied_results=[],private_generated_owner_instrumentation=instrumentation),indent=2)+'\n')
    # These two arithmetic comparisons consume retained original results only.
    # The legacy material/animation registrations load ISO caches, so they are
    # deliberately excluded from this MCC acceptance route. Actual material
    # coverage is exercised against the MCC campaign/runtime inputs below.
    tests=['original_weapon_heat_shader','original_tu15_weapon_primary_charge']
    targets=['weapon_heat_shader_tests','weapon_primary_charge_tu15_tests']
    component_tests=tests.copy();component_targets=targets.copy()
    if args.vehicle_weapon_hud or vehicle_firing:
        targets.append('vehicle_weapon_hud_world_tests')
        if vehicle_firing:
            tests.append('source_effect_compiled_symbols_tu15')
            targets.append('source_effect_compiled_symbols_tu15_tests')
        with (source/'CMakeLists.txt').open('a')as stream:
            for mode,enabled in (('hud',args.vehicle_weapon_hud),('fire',args.vehicle_weapon_fire),
                    ('secondary',args.vehicle_weapon_secondary_fire)):
                if not enabled:continue
                for case in (VEHICLE_SECONDARY_CASES if mode=='secondary'else VEHICLE_HUD_CASES):
                    name='weapon_mcc_vehicle_'+mode+'_'+case.replace('-','_');tests.append(name)
                    fire_environment=';HALO2_TEST_VEHICLE_WEAPON_FIRE=1'if mode!='hud'else''
                    if mode=='secondary':fire_environment+=';HALO2_TEST_VEHICLE_WEAPON_SECONDARY=1'
                    stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:vehicle_weapon_hud_world_tests>"
    "{mcc_maps}/coagulation.map" "{mcc_maps}" native-mcc)
set_tests_properties({name} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
    ENVIRONMENT "SDL_AUDIODRIVER=dummy;HALO2_TEST_VEHICLE_WEAPON_HUD_CASE={case};HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-vehicle-{mode}-{case}.json{fire_environment}")
''')
    if args.source_world_weapon_publication:
        targets.append('weapon_mcc_source_world_weapon_publication_tests')
        with (source/'CMakeLists.txt').open('a')as stream:
            stream.write('''\nadd_executable(weapon_mcc_source_world_weapon_publication_tests
    tests/mcc_source_world_weapon_publication_tests.cpp)
target_link_libraries(weapon_mcc_source_world_weapon_publication_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
''')
            for representation,species in ((2,'spartan'),(3,'elite')):
                name='weapon_mcc_source_world_weapon_publication_'+species;tests.append(name)
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:weapon_mcc_source_world_weapon_publication_tests>"
    "{mcc_maps}/lockout.map" "{mcc_maps}"
    "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-publication-{species}.json" {representation})
set_tests_properties({name} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE ENVIRONMENT "SDL_AUDIODRIVER=dummy")
''')
    if args.particle_model_math:
        tests.append('weapon_mcc_particle_model_tu15');targets.append('particle_model_tests')
        with (source/'CMakeLists.txt').open('a')as stream:
            stream.write(f'''\nadd_test(NAME weapon_mcc_particle_model_tu15 COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "{args.reference_python}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/oracle_mcc_particle_model_tu15.py"
    --native-binary "$<TARGET_FILE:particle_model_tests>" --mcc-map "{mcc_maps}/turf.map"
    --output "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-particle-model-tu15.json" --require-byte-exact)
set_tests_properties(weapon_mcc_particle_model_tu15 PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
''')
    if args.live_firing or args.live_gauge or args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld or args.campaign_weapons:
        tests.append('weapon_mcc_material_firing_inputs');targets.append('source_weapon_firing_material_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_mcc_material_firing_inputs COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:source_weapon_firing_material_tests>"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/weapon_firing_material_tu15.json"
    "{mcc_maps}/lockout.map" "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-material-firing-inputs.json")
set_tests_properties(weapon_mcc_material_firing_inputs PROPERTIES TIMEOUT 120
    ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
''')
        tests.append('weapon_mcc_material_alive_inputs');targets.append('source_weapon_alive_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_mcc_material_alive_inputs COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:source_weapon_alive_tests>"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/weapon_alive_material_tu15.json"
    "{mcc_maps}/lockout.map" "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-material-alive-inputs.json")
set_tests_properties(weapon_mcc_material_alive_inputs PROPERTIES FIXTURES_SETUP weapon_material_alive_ready
    TIMEOUT 120 ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
''')
    if args.live_firing or args.live_gauge or args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld:
        tests.append('native_tu15_network_configuration_defaults');targets.append('native_tu15_network_configuration_tests')
        tests.append('native_tu15_game_language_storage');targets.append('native_tu15_game_language_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write('set_tests_properties(native_tu15_network_configuration_defaults PROPERTIES FIXTURES_SETUP weapon_network_configuration_ready)\n')
            stream.write('set_tests_properties(native_tu15_game_language_storage PROPERTIES FIXTURES_SETUP weapon_game_language_ready)\n')
    if args.campaign_native_hud:
        tests.append('weapon_ai_descriptor_data');targets.append('ai_behavior_descriptor_data_tu15_tests')
        tests.append('weapon_mcc_model_node_map');targets.append('mcc_model_node_map_tests')
        tests.append('weapon_mcc_runtime_delivery');targets.append('mcc_runtime_delivery_tests')
        tests.append('native_tu15_world_memory_ownership');targets.append('native_tu15_world_memory_tests')
        tests.append('weapon_mcc_material_inputs');targets.append('source_weapon_material_namespace_tests')
        tests.append('weapon_mcc_material_random_inputs');targets.append('source_weapon_random_constant_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write('set_tests_properties(native_tu15_world_memory_ownership PROPERTIES FIXTURES_SETUP weapon_native_cold_ready)\n')
            stream.write('''\nadd_executable(ai_behavior_descriptor_data_tu15_tests tests/ai_behavior_descriptor_data_tu15_tests.cpp)
target_link_libraries(ai_behavior_descriptor_data_tu15_tests PRIVATE halo2_systems)
add_test(NAME weapon_ai_descriptor_data COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- "$<TARGET_FILE:ai_behavior_descriptor_data_tu15_tests>"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_ai_behavior_descriptors.json"
    "${CMAKE_CURRENT_BINARY_DIR}/weapon-ai-descriptor-data.json")
set_tests_properties(weapon_ai_descriptor_data PROPERTIES FIXTURES_SETUP weapon_ai_descriptor_ready TIMEOUT 60)
''')
            stream.write(f'''\nadd_test(NAME weapon_mcc_model_node_map COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:mcc_model_node_map_tests>"
    "{mcc_maps}/01b_spacestation.map"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/mcc_gun_hinge_node_map_tu15.json"
    "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-model-node-map.json")
set_tests_properties(weapon_mcc_model_node_map PROPERTIES FIXTURES_SETUP weapon_model_node_map_ready
    TIMEOUT 180 ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
add_test(NAME weapon_mcc_runtime_delivery COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:mcc_runtime_delivery_tests>"
    "{mcc_maps}/01b_spacestation.map" "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-runtime-delivery.json")
set_tests_properties(weapon_mcc_runtime_delivery PROPERTIES FIXTURES_SETUP weapon_model_delivery_ready
    TIMEOUT 180 ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
''')
            stream.write(f'''\nadd_test(NAME weapon_mcc_material_inputs COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:source_weapon_material_namespace_tests>"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/weapon_ammunition_digits_tu15.json"
    "{mcc_maps}/01b_spacestation.map" "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-material-inputs.json")
set_tests_properties(weapon_mcc_material_inputs PROPERTIES FIXTURES_SETUP weapon_material_inputs_ready
    TIMEOUT 120 ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
add_test(NAME weapon_mcc_material_random_inputs COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:source_weapon_random_constant_tests>"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tests/fixtures/object_material_random_constant_tu15.json"
    "{mcc_maps}/lockout.map" "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-material-random-inputs.json")
set_tests_properties(weapon_mcc_material_random_inputs PROPERTIES FIXTURES_SETUP weapon_material_random_ready
    TIMEOUT 120 ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
''')
    if args.dual_world_poses:
        tests.append('native_source_scene_physics');targets.append('source_scene_physics_tests')
    if args.campaign_weapons:
        tests.append('weapon_campaign_source_submission');targets.append('campaign_model_geometry_runtime_tu15_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_campaign_source_submission COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:campaign_model_geometry_runtime_tu15_tests>"
    "native-mcc" "{mcc_maps}/01b_spacestation.map" cairo-armed)
set_tests_properties(weapon_campaign_source_submission PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
    FIXTURES_SETUP weapon_campaign_source_ready
    ENVIRONMENT "HALO2_TEST_CAMPAIGN_WEAPON_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-source-submission.json")\n''')
            if args.campaign_native_hud:
                stream.write('set_tests_properties(weapon_campaign_source_submission PROPERTIES FIXTURES_REQUIRED "weapon_ai_descriptor_ready;weapon_model_node_map_ready;weapon_model_delivery_ready;weapon_native_cold_ready;weapon_material_inputs_ready;weapon_material_random_ready;weapon_material_alive_ready")\n')
    if args.campaign_scenery_lighting:
        tests.append('weapon_campaign_scenery_lighting')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_campaign_scenery_lighting COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:campaign_model_geometry_runtime_tu15_tests>"
    "native-mcc" "{mcc_maps}/01b_spacestation.map" cairo-scenery-lighting)
set_tests_properties(weapon_campaign_scenery_lighting PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
    FIXTURES_SETUP weapon_campaign_scenery_ready
    ENVIRONMENT "HALO2_TEST_CAMPAIGN_SCENERY_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-scenery-lighting.json")\n''')
    if args.campaign_copy_dispatch and not args.campaign_native_hud:
        tests.append('weapon_campaign_copy_dispatch')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_campaign_copy_dispatch COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:campaign_model_geometry_runtime_tu15_tests>"
    "native-mcc" "{mcc_maps}/01b_spacestation.map" cairo-copy-dispatch)
set_tests_properties(weapon_campaign_copy_dispatch PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
    FIXTURES_SETUP weapon_campaign_copy_ready
    ENVIRONMENT "HALO2_TEST_CAMPAIGN_COPY_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-copy-dispatch.json")\n''')
    if args.campaign_native_hud:
        tests.append('weapon_campaign_native_hud')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_campaign_native_hud COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:campaign_model_geometry_runtime_tu15_tests>"
    "native-mcc" "{mcc_maps}/01b_spacestation.map" cairo-native-hud)
set_tests_properties(weapon_campaign_native_hud PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
    FIXTURES_SETUP weapon_campaign_native_hud_ready
    ENVIRONMENT "HALO2_TEST_CAMPAIGN_COPY_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-native-hud.json")\n''')
    if args.campaign_weapon_application:
        tests.append('weapon_campaign_application')
        scene_case_command='--scene-case "${CMAKE_CURRENT_BINARY_DIR}/weapon-campaign-scenery-lighting.json"'if args.campaign_scenery_lighting else''
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_campaign_application COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/check_campaign_weapon_application_tu15.py"
    --native "$<TARGET_FILE:halo2_native>"
    --map "{mcc_maps}/01b_spacestation.map"
    --source-case "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-source-submission.json"
    {scene_case_command}
    --output "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-application.json")
set_tests_properties(weapon_campaign_application PROPERTIES TIMEOUT 330 RUN_SERIAL TRUE
    FIXTURES_REQUIRED weapon_campaign_source_ready)\n''')
            if args.campaign_scenery_lighting:
                stream.write('set_tests_properties(weapon_campaign_application PROPERTIES FIXTURES_REQUIRED weapon_campaign_scenery_ready)\n')
            if args.campaign_copy_dispatch and not args.campaign_native_hud:
                stream.write('set_property(TEST weapon_campaign_application APPEND PROPERTY FIXTURES_REQUIRED weapon_campaign_copy_ready)\n')
            if args.campaign_native_hud:
                stream.write('set_property(TEST weapon_campaign_application APPEND PROPERTY FIXTURES_REQUIRED weapon_campaign_native_hud_ready)\n')
    if args.live_dual:
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write('\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)\n')
            for species,representation in (('spartan',2),('elite',3)):
                name='weapon_source_dual_'+species;tests.append(name)
                dual_environment=';HALO2_TEST_DUAL_DROP_LIFETIME=1'if args.dual_drop_lifetime else''
                if args.dual_world_poses:dual_environment+=';HALO2_TEST_DUAL_WORLD_POSES=1'
                if args.trace_dual_selection:dual_environment+=';HALO2_TEST_TRACE_DUAL_SELECTION=1'
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map"
    "{mcc_maps}" "native-mcc")
set_tests_properties({name} PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION={representation};HALO2_TEST_DUAL_HISTORY_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-dual-{species}.json{dual_environment}")\n''')
    if args.live_firing:
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write('\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)\n')
            for motion,label,representation,players in live_cases:
                name='weapon_source_firing_'+label;tests.append(name)
                environment=';HALO2_TEST_ALLOW_NATIVE_MOTION=1' if motion else ''
                if args.verify_source_shot_tick:environment+=';HALO2_TEST_VERIFY_SOURCE_SHOT_TICK=1'
                if args.trace_projectile_births:environment+=';HALO2_TEST_TRACE_PROJECTILE_BIRTHS=1'
                if args.trace_projectile_retirement:environment+=';HALO2_TRACE_SOURCE_CONTACT_EFFECTS=1'
                if args.trace_weapon_effect_requests:environment+=';HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS=1'
                if args.clear_firing_lane:environment+=';HALO2_TEST_CLEAR_FIRING_LANE=1'
                live_command=''
                if args.compare_live_glow:
                    prefix='weapon-live-glow-'+label if args.glow_view_cohorts else 'weapon-live-glow'
                    live_command=f'''"${{Python3_EXECUTABLE}}" "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/compare_live_weapon_glow_tu15.py"
    --report "${{CMAKE_CURRENT_BINARY_DIR}}/{prefix}-comparison.json"
    --inputs-report "${{CMAKE_CURRENT_BINARY_DIR}}/{prefix}-inputs.json"
    --original-report "${{CMAKE_CURRENT_BINARY_DIR}}/{prefix}-original.json" -- '''
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- {live_command}"${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map"
    "{mcc_maps}" "native-mcc" plasma_rifle)
set_tests_properties({name} PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION={representation};HALO2_TEST_LOCAL_PLAYERS={players};HALO2_TEST_WEAPON_STATE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-firing-{label}.json{environment}")\n''')
    if args.live_gauge:
        tests.append('weapon_material_live_gauge')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)
add_test(NAME weapon_material_live_gauge COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map"
    "{mcc_maps}" "native-mcc" plasma_rifle)
set_tests_properties(weapon_material_live_gauge PROPERTIES TIMEOUT 600 FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION=2;HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_MATERIAL_ONLY=1;HALO2_GAUGE_ACCEPTANCE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/live-gauge-acceptance.json")\n''')
    if args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld or args.first_person_bindings:
        tests.append('source_weapon_animation_query_symbols_tu15')
        targets.append('source_weapon_animation_query_symbols_tu15_tests')
        tests.append('original_unit_animation_selection');targets.append('unit_animation_selection_tests')
        targets.append('mcc_first_person_binding_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            for path in binding_maps:
                suffix=''if path.stem=='lockout'else'_'+path.stem
                report_suffix=''if path.stem=='lockout'else'-'+path.stem
                name='weapon_mcc_first_person_bindings'+suffix;tests.append(name)
                fixture=' FIXTURES_SETUP weapon_first_person_ready'if path.stem=='lockout'else''
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:mcc_first_person_binding_tests>"
    "{path}" "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-mcc-first-person-bindings{report_suffix}.json")
set_tests_properties({name} PROPERTIES TIMEOUT 180{fixture}
    ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
''')
    if args.live_sword_material:
        tests.append('weapon_material_live_sword')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)
add_test(NAME weapon_material_live_sword COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map" "{mcc_maps}" "native-mcc" sword)
set_tests_properties(weapon_material_live_sword PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE
    FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready;weapon_first_person_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION=2;HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_MATERIAL_ONLY=1;HALO2_SWORD_MATERIAL_ACCEPTANCE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/live-sword-material-acceptance.json")
''')
    if args.live_sword_melee:
        tests.append('weapon_mcc_sword_melee')
        environment=';HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS=1' if args.trace_weapon_effect_requests else ''
        if args.trace_sword_selection:environment+=';HALO2_TEST_TRACE_SWORD_SELECTION=1'
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)
add_test(NAME weapon_mcc_sword_melee COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map" "{mcc_maps}" "native-mcc" sword)
set_tests_properties(weapon_mcc_sword_melee PROPERTIES TIMEOUT 600 RUN_SERIAL TRUE
    FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready;weapon_first_person_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION=2;HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_STATE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-sword-melee.json{environment}")
''')
    if args.live_staged_reload:
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write('\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)\n')
            for representation,species in ((2,'spartan'),(3,'elite')):
                name='weapon_mcc_staged_reload_'+species;tests.append(name)
                environment=';HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS=1' if args.trace_weapon_effect_requests else ''
                if args.trace_reload_timing:environment+=';HALO2_TEST_TRACE_RELOAD_TIMING=1'
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map" "{mcc_maps}" "native-mcc" shotgun "" reload)
set_tests_properties({name} PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
    FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready;weapon_first_person_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION={representation};HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_STATE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-staged-reload-{species}.json{environment}")
''')
    if args.live_charge:
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write('\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)\n')
            for representation,species in ((2,'spartan'),(3,'elite')):
                for phase in ('charge','tap'):
                    name='weapon_mcc_'+phase+'_'+species;tests.append(name)
                    environment=';HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS=1'if args.trace_weapon_effect_requests else''
                    stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "${{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps}/lockout.map" "{mcc_maps}" "native-mcc" plasma_pistol "" {phase})
set_tests_properties({name} PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
    FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready;weapon_first_person_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION={representation};HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_STATE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-{phase}-{species}.json{environment}")
''')
    handheld_reports=[]
    if args.live_all_handheld:
        dollar=chr(36)
        with (source/'CMakeLists.txt').open('a')as stream:
            stream.write('\nfind_program(HALO2_XVFB_RUN xvfb-run REQUIRED)\n')
            for weapon,mapname in handheld_cases:
                for representation,species in ((2,'spartan'),(3,'elite')):
                    name='weapon_mcc_handheld_'+weapon+'_'+species;tests.append(name)
                    report_name='weapon-source-handheld-'+weapon+'-'+species+'.json'
                    handheld_reports.append(report_name)
                    stream.write(f'''\nadd_test(NAME {name} COMMAND "{dollar}{{Python3_EXECUTABLE}}"
    "{dollar}{{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "{dollar}{{HALO2_XVFB_RUN}}" -a
    -s "-screen 0 800x600x24" "$<TARGET_FILE:native_covenant_weapon_runtime_tests>"
    "{mcc_maps/mapname}" "{mcc_maps}" native-mcc "{weapon}")
set_tests_properties({name} PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
    FIXTURES_REQUIRED "weapon_network_configuration_ready;weapon_game_language_ready;weapon_material_alive_ready;weapon_first_person_ready"
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION={representation};HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_STATE_REPORT={dollar}{{CMAKE_CURRENT_BINARY_DIR}}/{report_name}")
''')
    if args.source_sound_startup:
        tests.append('weapon_mcc_source_sound_startup');targets.append('weapon_mcc_source_sound_startup_tests')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_executable(weapon_mcc_source_sound_startup_tests tests/mcc_weapon_sound_startup_tests.cpp)
target_link_libraries(weapon_mcc_source_sound_startup_tests PRIVATE halo2_systems halo2_sound_platform nlohmann_json::nlohmann_json)
add_test(NAME weapon_mcc_source_sound_startup COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:weapon_mcc_source_sound_startup_tests>"
    "{mcc_maps}/lockout.map" "@HALO2_TEST_SCRATCH@/sound-startup-save"
    "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-sound-startup.json")
set_tests_properties(weapon_mcc_source_sound_startup PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
    ENVIRONMENT "SDL_AUDIODRIVER=dummy")
''')
            if args.source_sound_host_output:
                tests.append('weapon_mcc_source_sound_host_output')
                stream.write(f'''\nadd_test(NAME weapon_mcc_source_sound_host_output COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:weapon_mcc_source_sound_startup_tests>"
    "{mcc_maps}/lockout.map" "@HALO2_TEST_SCRATCH@/sound-startup-save"
    "${{CMAKE_CURRENT_BINARY_DIR}}/weapon-source-sound-host-output.json" native-output)
set_tests_properties(weapon_mcc_source_sound_host_output PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
    ENVIRONMENT "SDL_AUDIODRIVER=pulseaudio")
''')
    if args.application:
        # Only validation registrations are added to this private CMake copy.
        # Each finite CLI invocation has its own standard scratch wrapper.
        with (source/'CMakeLists.txt').open('a') as stream:
            for players in (1,4):
                name=f'weapon_material_application_{players}_players';tests.append(name)
                stream.write(f'''\nadd_test(NAME {name} COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:halo2_native>"
    --multiplayer --map "{mcc_maps}/lockout.map"
    --shared-maps "{mcc_maps}"
    --mp-starting-weapon 5 --local-players {players}
    --simulate 120 --mute)
set_tests_properties({name} PROPERTIES TIMEOUT 120)\n''')
    # Renderer/cache helpers may otherwise use the shared desktop cache. Each
    # nested test wrapper expands these tokens into its own disposable owner.
    with (source/'CMakeLists.txt').open('a') as stream:
        stream.write('\nset_property(TEST '+' '.join(tests)+' APPEND PROPERTY ENVIRONMENT\n'
            '    "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache"\n'
            '    "HALO2_PIPELINE_CACHE_DIRECTORY=@HALO2_TEST_SCRATCH@/vulkan-cache")\n')
    commands=[];steps=[]
    report=dict(passed=False,status='running',runner_scratch=str(scratch),private_source=str(source),
        source_manifest_sha256=sha(output/'source-manifest.json'),commands=commands,steps=steps,
        runtime_cache_inputs=cache_inputs,
        all_handheld_source_cases=[dict(weapon=w,map=n)for w,n in handheld_cases],
        registered_tests=tests,preparation_only=args.prepare_only,
        owned_build_worker=args.owned_build_worker,
        active_owned_build=str(scratch/'build')if args.owned_build_worker else None,
        source_cleanup_pending_runner_exit=True,large_artifacts_retained=[])
    def save():(output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    save()
    for sanitizer in ((False,) if args.release_only else (False,True)):
        label='sanitizers' if sanitizer else 'release'
        command=[str(Path(os.sys.executable).absolute()),str(ROOT/'tools/run_tests.py'),
            '--source',str(source),'--scratch-root',str(scratch/'cmake-scratch'),
            '--output',str(output/label),'--cmake-arg=-DPython3_EXECUTABLE='+str(Path(os.sys.executable).absolute()),
            '--jobs',str(args.jobs),
            '--filter','^('+'|'.join(component_tests if sanitizer else tests)+')$']
        if args.owned_build_worker:command+=['--build-worker']
        if args.live_gauge and not sanitizer:command+=['--retain-report','live-gauge-acceptance.json']
        if args.live_sword_material and not sanitizer:command+=['--retain-report','live-sword-material-acceptance.json']
        if (args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld or args.first_person_bindings) and not sanitizer:
            for path in binding_maps:
                suffix=''if path.stem=='lockout'else'-'+path.stem
                command+=['--retain-report','weapon-mcc-first-person-bindings'+suffix+'.json']
        if args.live_sword_melee and not sanitizer:command+=['--retain-report','weapon-source-sword-melee.json']
        if args.live_staged_reload and not sanitizer:
            for species in ('spartan','elite'):command+=['--retain-report','weapon-source-staged-reload-'+species+'.json']
        if args.live_charge and not sanitizer:
            for species in ('spartan','elite'):
                for phase in ('charge','tap'):command+=['--retain-report','weapon-source-'+phase+'-'+species+'.json']
        if args.live_all_handheld and not sanitizer:
            for name in handheld_reports:command+=['--retain-report',name]
        if args.vehicle_weapon_hud and not sanitizer:
            for case in VEHICLE_HUD_CASES:command+=['--retain-report','weapon-mcc-vehicle-hud-'+case+'.json']
        if args.vehicle_weapon_fire and not sanitizer:
            for case in VEHICLE_HUD_CASES:command+=['--retain-report','weapon-mcc-vehicle-fire-'+case+'.json']
        if args.vehicle_weapon_secondary_fire and not sanitizer:
            for case in VEHICLE_SECONDARY_CASES:command+=['--retain-report','weapon-mcc-vehicle-secondary-'+case+'.json']

        if args.source_sound_startup and not sanitizer:command+=['--retain-report','weapon-source-sound-startup.json']
        if args.source_sound_host_output and not sanitizer:command+=['--retain-report','weapon-source-sound-host-output.json']
        if args.particle_model_math and not sanitizer:command+=['--retain-report','weapon-mcc-particle-model-tu15.json']
        if args.source_world_weapon_publication and not sanitizer:
            for species in ('spartan','elite'):command+=['--retain-report','weapon-source-publication-'+species+'.json']
        if (args.live_firing or args.live_gauge or args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld or args.campaign_weapons) and not sanitizer:
            command+=['--retain-report','weapon-mcc-material-alive-inputs.json']
            command+=['--retain-report','weapon-mcc-material-firing-inputs.json']
        if args.compare_live_glow and not sanitizer:
            prefixes=['weapon-live-glow-'+label for _,label,_,_ in live_cases]if args.glow_view_cohorts else['weapon-live-glow']
            for prefix in prefixes:
                for name in ('comparison','inputs','original'):command+=['--retain-report',prefix+'-'+name+'.json']
        if args.live_firing and not sanitizer:
            for _,case_label,_,_ in live_cases:command+=['--retain-report','weapon-source-firing-'+case_label+'.json']
        if args.live_dual and not sanitizer:
            for species in ('spartan','elite'):command+=['--retain-report','weapon-source-dual-'+species+'.json']
        if args.campaign_weapons and not sanitizer:command+=['--retain-report','weapon-campaign-source-submission.json']
        if args.campaign_weapon_application and not sanitizer:command+=['--retain-report','weapon-campaign-application.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-mcc-material-inputs.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-mcc-material-random-inputs.json']
        if args.campaign_scenery_lighting and not sanitizer:command+=['--retain-report','weapon-campaign-scenery-lighting.json']
        if args.campaign_copy_dispatch and not args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-campaign-copy-dispatch.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-campaign-native-hud.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-ai-descriptor-data.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-mcc-model-node-map.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-mcc-runtime-delivery.json']
        for target in (component_targets if sanitizer else targets)+(['halo2_native'] if args.application and not sanitizer else [])+(['native_covenant_weapon_runtime_tests'] if (args.live_gauge or args.live_firing or args.live_sword_material or args.live_sword_melee or args.live_staged_reload or args.live_charge or args.live_all_handheld) and not sanitizer else [])+(['native_weapon_runtime_tests'] if args.live_dual and not sanitizer else []):
            command+=['--target',target]
        if sanitizer:command+=['--sanitizers']
        else:command+=['--cmake-arg=-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG']
        command+=['--cmake-arg='+value for value in args.cmake_arg]
        commands.append(command);save()
        if args.prepare_only:continue
        with (output/(label+'-runner.log')).open('w') as log:
            result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
        child=json.loads((output/label/'report.json').read_text())
        removed=not Path(child['build']).parent.exists()
        steps.append(dict(name=label,exit_code=result.returncode,private_build_removed=removed,
            private_build_owned_by_outer_runner=args.owned_build_worker,
            report_sha256=sha(output/label/'report.json')))
        save()
        assert removed or args.owned_build_worker
        if result.returncode:
            report['status']='failed';save();return result.returncode
    if args.prepare_only:
        report.update(status='prepared',validation_not_run=True,
            registered_snapshot_source_files_sha256={path:sha(source/path)for path in pins},
            private_build_cleanup_pending_outer_runner=True)
        save();print(json.dumps(dict(status='prepared',passed=False,validation_not_run=True,
            registered_tests=len(tests),runner_scratch=str(scratch))),flush=True)
        return 0
    report.update(passed=True,status='passed',snapshot_source_files_unchanged=all(sha(source/path)==pin
        for path,pin in pins.items() if path!='CMakeLists.txt'),private_test_builds_removed=not args.owned_build_worker,
        private_build_cleanup_pending_outer_runner=args.owned_build_worker,
        full_source_tree_immutable_during_root_work=False)
    save()
    print(json.dumps(dict(passed=True,checks=[step['name'] for step in steps],runner_scratch=str(scratch))),flush=True)
    return 0


if __name__=='__main__':
    raise SystemExit(main())
