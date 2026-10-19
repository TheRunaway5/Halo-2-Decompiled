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
from weapon_test_cache import mcc_runtime_inputs,sha

ROOT=Path(__file__).absolute().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--mcc-root',type=Path,required=True,help='Prepared MCC cache root; Xbox/Vista cache fallback is forbidden')
    parser.add_argument('--application',action='store_true')
    parser.add_argument('--live-gauge',action='store_true')
    parser.add_argument('--live-firing',action='store_true')
    parser.add_argument('--live-dual',action='store_true',help='Actual SDL dual-SMG per-hand source histories for both species')
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
    parser.add_argument('--clear-firing-lane',action='store_true')
    parser.add_argument('--compare-live-glow',action='store_true')
    parser.add_argument('--glow-view-cohorts',action='store_true')
    parser.add_argument('--glow-cohort',action='append',choices=('spartan2','spartan4','elite1','elite2','elite4'))
    parser.add_argument('--live-firing-placement',choices=('both','pinned','native_motion'),default='both')
    parser.add_argument('--jobs',type=int,default=2)
    parser.add_argument('--release-only',action='store_true')
    parser.add_argument('--cmake-arg',action='append',default=[])
    args=parser.parse_args()
    if args.campaign_scenery_lighting:
        parser.error('--campaign-scenery-lighting replays an ISO reference camera; it is not MCC runtime acceptance')
    mcc_maps,cache_inputs=mcc_runtime_inputs(args.mcc_root)
    assert not args.trace_projectile_retirement or args.trace_projectile_births
    assert not args.dual_drop_lifetime or args.live_dual
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
            if any(part in ('__pycache__','.git') for part in path.relative_to(base).parts):continue
            if path.is_file():copy_file(path)
    copy_file(ROOT/'CMakeLists.txt')
    for name in ('src','include','cmake','shaders','third_party','external'):
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
        'weapon_primary_charge_tu15_original.json','retail_source_weapon_shader_input.tsv'}
    for path in (ROOT/'tests/fixtures').iterdir():
        if path.name in selected:copy_file(path)
        else:(fixtures/path.name).symlink_to(path,target_is_directory=path.is_dir())
    for name in ('assets','analysis','resources','data','multiplayer','.tools'):
        if (ROOT/name).exists():(source/name).symlink_to(ROOT/name,target_is_directory=True)
    pins={str(relative):sha(source/relative) for relative in copied}
    drift=[str(relative) for relative in copied if sha(ROOT/relative)!=pins[str(relative)]]
    manifest=dict(source=str(source),source_files_sha256=pins,source_drift_during_copy=drift,
        changed_code_and_selected_fixtures_are_private_copies=True,
        borrowed_large_original_inputs_are_not_copied=True,runner_scratch=str(scratch),
        runtime_cache_inputs=cache_inputs)
    (output/'source-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    assert not drift,drift[:10]
    if args.trace_projectile_births or args.dual_drop_lifetime:
        assert not args.trace_projectile_births or args.live_firing,'Projectile birth observation requires the live firing fixture'
        instrumentation=[]
        entries=tuple(([0xb9b70]if args.trace_projectile_births else [])+
            ([0xba490]if args.trace_projectile_retirement or args.dual_drop_lifetime else []))
        for path in sorted((source/'src').glob('*generated.cpp')):
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
        observation_name='projectile-birth-observation.json'if args.trace_projectile_births else'dual-drop-retirement-observation.json'
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
    if args.campaign_scenery_lighting:
        tests.append('weapon_campaign_scenery_lighting')
        with (source/'CMakeLists.txt').open('a') as stream:
            stream.write(f'''\nadd_test(NAME weapon_campaign_scenery_lighting COMMAND "${{Python3_EXECUTABLE}}"
    "${{CMAKE_CURRENT_SOURCE_DIR}}/tools/run_test.py" -- "$<TARGET_FILE:campaign_model_geometry_runtime_tu15_tests>"
    "native-mcc" "{mcc_maps}/01b_spacestation.map" cairo-scenery-lighting)
set_tests_properties(weapon_campaign_scenery_lighting PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE
    FIXTURES_SETUP weapon_campaign_scenery_ready
    ENVIRONMENT "HALO2_TEST_CAMPAIGN_SCENERY_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/weapon-campaign-scenery-lighting.json")\n''')
    if args.campaign_copy_dispatch:
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
            if args.campaign_copy_dispatch:
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
set_tests_properties({name} PROPERTIES TIMEOUT 900 RUN_SERIAL TRUE
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
set_tests_properties(weapon_material_live_gauge PROPERTIES TIMEOUT 600
    ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy;HALO2_TEST_REPRESENTATION=2;HALO2_TEST_LOCAL_PLAYERS=1;HALO2_TEST_WEAPON_MATERIAL_ONLY=1;HALO2_GAUGE_ACCEPTANCE_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/live-gauge-acceptance.json")\n''')
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
    commands=[];steps=[]
    report=dict(passed=False,status='running',runner_scratch=str(scratch),private_source=str(source),
        source_manifest_sha256=sha(output/'source-manifest.json'),commands=commands,steps=steps,
        runtime_cache_inputs=cache_inputs,
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
        if args.live_gauge and not sanitizer:command+=['--retain-report','live-gauge-acceptance.json']
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
        if args.campaign_scenery_lighting and not sanitizer:command+=['--retain-report','weapon-campaign-scenery-lighting.json']
        if args.campaign_copy_dispatch and not sanitizer:command+=['--retain-report','weapon-campaign-copy-dispatch.json']
        if args.campaign_native_hud and not sanitizer:command+=['--retain-report','weapon-campaign-native-hud.json']
        for target in (component_targets if sanitizer else targets)+(['halo2_native'] if args.application and not sanitizer else [])+(['native_covenant_weapon_runtime_tests'] if (args.live_gauge or args.live_firing) and not sanitizer else [])+(['native_weapon_runtime_tests'] if args.live_dual and not sanitizer else []):
            command+=['--target',target]
        if sanitizer:command+=['--sanitizers']
        else:command+=['--cmake-arg=-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG']
        command+=['--cmake-arg='+value for value in args.cmake_arg]
        commands.append(command);save()
        with (output/(label+'-runner.log')).open('w') as log:
            result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
        child=json.loads((output/label/'report.json').read_text())
        removed=not Path(child['build']).parent.exists()
        steps.append(dict(name=label,exit_code=result.returncode,private_build_removed=removed,
            report_sha256=sha(output/label/'report.json')))
        save()
        assert removed
        if result.returncode:
            report['status']='failed';save();return result.returncode
    report.update(passed=True,status='passed',snapshot_source_files_unchanged=all(sha(source/path)==pin
        for path,pin in pins.items() if path!='CMakeLists.txt'),private_test_builds_removed=True,
        full_source_tree_immutable_during_root_work=False)
    save()
    print(json.dumps(dict(passed=True,checks=[step['name'] for step in steps],runner_scratch=str(scratch))),flush=True)
    return 0


if __name__=='__main__':
    raise SystemExit(main())
