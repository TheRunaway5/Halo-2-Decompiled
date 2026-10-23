#!/usr/bin/env python3
"""Build combat diagnostics in an owned snapshot; production files stay intact.

Run via tools/run_test.py. CMake validation uses tools/run_tests.py.
Negative parity results are retained as evidence, never converted to passes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CASES = [('sniper-unzoomed', 'sniper_rifle', 0), ('sniper-1', 'sniper_rifle', 1),
         ('sniper-2', 'sniper_rifle', 2), ('beam-1', 'beam_rifle', 1),
         ('beam-2', 'beam_rifle', 2), ('br-1', 'battle_rifle', 1),
         ('carbine-1', 'covenant_carbine', 1), ('rocket-1', 'rocket_launcher', 1),
         ('smg-binoculars', 'smg', 1)]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write_changed(path, value):
    data = value.encode() if isinstance(value, str) else value
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--jobs', type=int, default=8)
    ap.add_argument('--refresh-existing', action='store_true')
    ap.add_argument('--prepare-only', action='store_true')
    ap.add_argument('--partition-ai', action='store_true',
                    help='Partition large generated runtime units with the existing unchanged-body tool')
    ap.add_argument('--case', action='append')
    ap.add_argument('--retain-small-reference', action='store_true',
                    help='Retain bounded PNG controls for sniper-1 and pilot-view Banshee inspection')
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source = scratch / 'source'
    args.output = args.output.resolve();args.output.mkdir(parents=True, exist_ok=True)
    if not args.refresh_existing:
        subprocess.run([sys.executable, str(ROOT / 'tools/validate_weapon_material_snapshot_tu15.py'),
                        '--output', str(args.output / 'frozen'), '--mcc-root', str(args.mcc_root),
                        '--owned-build-worker', '--release-only', '--prepare-only',
                        '--vehicle-weapon-fire', '--copy-drift-retries', '5'], check=True)
    if not source.is_dir() or source.is_symlink():
        raise RuntimeError('The enclosing owner has no private frozen source tree')
    from mcc_content import directory
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    map_ = maps / 'coagulation.map'
    files = {}
    for name in ('audit_combat_presentation_20261022.cpp', 'run_combat_presentation_audit_20261022.py'):
        relative = Path('tools') / name
        write_changed(source / relative, (ROOT / relative).read_bytes())
        files[str(relative)] = {'after': sha(source / relative)}
    backup = scratch / 'audit-originals';backup.mkdir(exist_ok=True)
    if args.partition_ai:
        relative = Path('cmake/PhysicsLiveWorldTu15.cmake')
        old = backup / relative.name
        if not old.exists():
            shutil.copyfile(source / relative, old)
        needle = 'target_sources(halo2_physics_world_runtime PRIVATE src/campaign/ai/source_ai_frame_tu15_generated.cpp)'
        text = old.read_text();assert text.count(needle) == 1
        text = text.replace(needle, 'halo2_partition_native_translation(halo2_physics_world_runtime src/campaign/ai/source_ai_frame_tu15_generated.cpp 12)')
        write_changed(source / relative, text)
        files[str(relative)] = {'before': sha(old), 'after': sha(source / relative),
                                'unchanged_generated_bodies_partitioned': 12}
        relative = Path('cmake/DeathCycle.cmake')
        old = backup / relative.name
        if not old.exists():
            shutil.copyfile(source / relative, old)
        needle = 'target_sources(halo2_systems PRIVATE src/gameplay/units/death_cycle_runtime_tu15_generated.cpp)'
        text = old.read_text();assert text.count(needle) == 1
        text = text.replace(needle, 'include(cmake/NativeTranslationUnits.cmake)\n'
            'halo2_partition_native_translation(halo2_systems src/gameplay/units/death_cycle_runtime_tu15_generated.cpp 12)')
        write_changed(source / relative, text)
        files[str(relative)] = {'before': sha(old), 'after': sha(source / relative),
                                'unchanged_generated_bodies_partitioned': 12}
    runtime = Path('src/gameplay/multiplayer/multiplayer_runtime.cpp')
    vehicle = Path('tests/vehicle_weapon_hud_world_tests.cpp')
    for relative in (runtime, vehicle):
        old = backup / relative.name
        if not old.exists():
            shutil.copyfile(source / relative, old)
        text = old.read_text()
        if relative == runtime:
            needle = 'hands.shaders->append_selected(foregrounds[i],pose.arms,owner,*object_view.shader,hands.material);'
            assert text.count(needle) == 1
            text = text.replace(needle, needle + '\n          '
                'if(std::getenv("HALO2_COMBAT_AUDIT_HIDE_ARMS"))foregrounds[i].resize(first);')
            needle = 'view.foreground = foregrounds[i];'
            assert text.count(needle) == 1
            text = text.replace(needle,
                'if(std::getenv("HALO2_COMBAT_AUDIT"))std::cout<<"[COMBAT-AUDIT] view="<<i'
                '<<" tick="<<current.tick<<" zoom="<<int(p.source_zoom.current)'
                '<<" foreground_projection="<<foreground_projection_angle'
                '<<" foreground_vertices="<<foregrounds[i].size()'
                '<<" viewport="<<physical(rect).left<<\',\'<<physical(rect).top<<\',\''
                '<<physical(rect).width<<\',\'<<physical(rect).height<<\'\\n\';\n      ' + needle)
            text = text.replace(needle, needle + '\n      if(std::getenv("HALO2_COMBAT_AUDIT_HIDE_FOREGROUND"))view.foreground={};')
            needle = next((line.strip() for line in text.splitlines()
                if line.strip().startswith('view.light_volumes=weapon_projectile_scene.light_volume_view(')), None)
            assert needle
            assert text.count(needle) == 1
            text = text.replace(needle, needle + '\n      if(std::getenv("HALO2_COMBAT_AUDIT_HIDE_GLOW"))view.light_volumes={};')
            needle = ('const float foreground_projection_angle=user_fov?\n'
                      '          gameplay_first_person_camera_angle(biped.camera_field_of_view,layout):source_angles.projection_angle;')
            assert text.count(needle) == 1
            text = text.replace(needle, ('const float foreground_projection_angle=std::getenv("HALO2_COMBAT_AUDIT_SCOPE_LENS") && '
                'p.source_zoom.current>=0?source_angles.projection_angle:user_fov?\n'
                '          gameplay_first_person_camera_angle(biped.camera_field_of_view,layout):source_angles.projection_angle;'))
            needle = '    timing.submitted=renderer.draw(views.front().camera,'
            assert text.count(needle) == 1
            text = text.replace(needle, '''    if(capture)if(const auto *directory=std::getenv("HALO2_COMBAT_AUDIT_PAIR_DIRECTORY")) {
      const auto before=renderer.bsp_postprocess_state();
      for(const auto label:{"repeat","control"}) {
        auto paired=views;
        for(auto &view:paired) {
          // Only the actual submission below issues the original query IDs.
          // Lens colors/visibility and every source pose are the SAME inputs.
          view.lens_queries={};view.light_volume_observer={};
          view.completed_material_upload_observer={};view.completed_bsp_draw_observer={};
          view.completed_object_draw_observer={};
          if(std::string_view(label)=="control") {
            if(std::getenv("HALO2_COMBAT_AUDIT_PAIR_GLOW"))view.light_volumes={};
            else view.foreground={};
          }
        }
        if(!renderer.draw(paired.front().camera,std::filesystem::path(directory)/(std::string(label)+".ppm"),
            {},false,{},{},hud_vertices,paired,{},pause_backdrops))
          throw FormatError("Combat paired GPU control was not submitted");
        if(before)renderer.restore_bsp_postprocess_state(*before);
      }
    }
''' + needle)
        else:
            needle = 'check(mcc && name=="ghost","Ghost projectile rendering requires its prepared MCC driver");'
            assert text.count(needle) == 1
            text = text.replace(needle, 'check(mcc,"Vehicle glow audit requires prepared MCC content");')
            needle = 'const auto start=state.tick,projectile=content.weapons.at(actual.definition).projectile;'
            assert text.count(needle) == 1
            text = text.replace(needle, 'const auto start=0u,projectile=content.weapons.at(actual.definition).projectile;')
            needle = 'std::set<std::uint32_t> projectile_owners;'
            assert text.count(needle) == 1
            text = text.replace(needle, needle + '\n      std::set<std::uint32_t> observed_live_projectiles;')
            needle = 'if(view)return;\n        check(rocket_explosion'
            assert text.count(needle) == 1
            text = text.replace(needle, 'if(view)return;\n        if(ghost_fire)for(const auto &p:frame.weapon_projectiles)'
                'if(p.definition==content.weapons.at(actual.definition).projectile)observed_live_projectiles.insert(p.datum);\n        check(rocket_explosion')
            needle = '{"actual_projectile_owners",projectile_owners}};'
            assert text.count(needle) == 1
            text = text.replace(needle, '{"actual_projectile_owners",projectile_owners},'
                '{"source_live_projectiles",observed_live_projectiles}};')
            needle = 'options.desktop=std::make_shared<DesktopSession>();'
            assert text.count(needle) == 1
            text = text.replace(needle, needle + '\n        options.desktop->auxiliary_random()=0x2468ace0u;'
                '\n        options.desktop->display_settings.field_of_view=78;'
                '\n        options.desktop->keyboard_settings=PcKeyboardSettings{};')
            needle = 'check(fire_inputs>0 && projectile_owners.size()>1 && glow_draws>0,'
            assert text.count(needle) == 1
            text = text.replace(needle, 'check(fire_inputs>0 && observed_live_projectiles.size()>1 && '
                '(std::getenv("HALO2_COMBAT_AUDIT_HIDE_GLOW") || (projectile_owners.size()>1 && glow_draws>0)),')
            text = text.replace('options.frames=ghost_fire?90u:', 'options.frames=ghost_fire?180u:')
            text = text.replace('options.frames=ghost_fire?180u:',
                'options.frames=ghost_fire?(std::getenv("HALO2_COMBAT_AUDIT_PIXEL_CAPTURE")?140u:180u):')
            needle = 'options.frames=ghost_fire?(std::getenv("HALO2_COMBAT_AUDIT_PIXEL_CAPTURE")?140u:180u):rocket_explosion?24u:4u;options.mute=true;options.screenshot=argv[4];'
            assert text.count(needle) == 1
            text = text.replace(needle, needle + '\n      if(const auto *frame=std::getenv("HALO2_COMBAT_AUDIT_PIXEL_FRAME"))options.frames=unsigned(std::stoul(frame));')
            text = text.replace(needle, '''if(ghost_fire && std::getenv("HALO2_COMBAT_AUDIT_FLY")) {
        const auto point=cache.spawns().front().position+Vec3{0,0,12};
        const auto below=world.match.simulation().collision().raycast(point,{0,0,-1},1000.f);
        check(below && below->distance>5.f,"Airborne setup lacks an actual clear BSP column");
        world.source_objects->tu15_world().set_object_position(p.source_parent,point);
        row["declared_airborne_setup"]={{"original_entry","B95D0"},{"vehicle",p.source_parent},
          {"position",{point.x,point.y,point.z}},{"initial_BSP_clearance",below->distance},
          {"supplied_fire_physics_or_render_results",false}};
      }
      ''' + needle)
            text = text.replace('tick==start+2 || tick==start+35', 'tick==start+30 || tick==start+150')
            text = text.replace('tick==start+2?SDL_MOUSEBUTTONDOWN', 'tick==start+30?SDL_MOUSEBUTTONDOWN')
            # Observe the same original constructor/retirement seam already
            # qualified by the world-only test, without supplying any state.
            begin = old.read_text().index('      const auto &bank=world.source_objects->memory();',
                old.read_text().index('if(std::getenv("HALO2_TEST_VEHICLE_WEAPON_FIRE"))'))
            end = old.read_text().index('      std::uint64_t contact_frontier{};', begin)
            observer = old.read_text()[begin:end]
            observer = observer.replace('row["type"]=5;row["actual_definition"]=definition;',
                '''row["type"]=5;row["actual_definition"]=definition;
            const auto body=word(header+8u);
            row["parent"]=word(body+0x14u);
            row["position"]={std::bit_cast<float>(word(body+0x64u)),std::bit_cast<float>(word(body+0x68u)),std::bit_cast<float>(word(body+0x6cu))};
            row["velocity"]={std::bit_cast<float>(word(body+0x88u)),std::bit_cast<float>(word(body+0x8cu)),std::bit_cast<float>(word(body+0x90u))};''')
            needle = '      unsigned submitted{};'
            assert text.count(needle) == 1
            text = text.replace(needle, '''      auto &firing=row["source_projectile_trace"];
      firing={{"constructor_calls",nlohmann::json::array()},
          {"retirement_calls",nlohmann::json::array()}};
      std::set<std::uint32_t> constructed;
      std::set<std::uint32_t> expected_projectiles;
      if(ghost_fire)expected_projectiles.insert(content.weapons.at(actual.definition).projectile);
''' + observer + needle)
            needle = '        options.input_observer=[&,start,window](unsigned tick,std::span<const Controls> input,const auto &) {'
            assert text.count(needle) == 1
            text = text.replace(needle, needle + '''
          if(std::getenv("HALO2_COMBAT_AUDIT_FLY") && (tick==0 || tick==90)) {
            if(tick==90) {
              SDL_Event motion{};motion.type=SDL_MOUSEMOTION;
              motion.motion.windowID=SDL_GetWindowID(window);motion.motion.yrel=-220;
              check(SDL_PushEvent(&motion)==1,"Queue actual Banshee flight look");
            }
            for(const auto key:std::array{tick==0?SDL_SCANCODE_W:SDL_SCANCODE_SPACE}) {
              SDL_Event event{};event.type=SDL_KEYDOWN;event.key.windowID=SDL_GetWindowID(window);
              event.key.keysym.scancode=key;event.key.keysym.sym=SDL_GetKeyFromScancode(key);
              check(SDL_PushEvent(&event)==1,"Queue actual Banshee flight control");
            }
          }
          row["flight_input_samples"].push_back({{"step",tick},{"forward",input[player].forward},
            {"jump",input[player].jump},{"pitch",input[player].pitch}});
''')
            needle = '        if(ghost_fire)for(const auto &p:frame.weapon_projectiles)'
            assert text.count(needle) == 1
            text = text.replace(needle, '''        if(ghost_fire) {
          nlohmann::json live=nlohmann::json::array();
          for(const auto &p:frame.weapon_projectiles)
            if(p.definition==content.weapons.at(actual.definition).projectile)live.push_back(p.datum);
          row["projectile_frame_samples"].push_back({{"tick",frame.tick},{"owners",live}});
          const auto parent=frame.players[player].source_parent;
          for(const auto &vehicle:frame.world_vehicles)if(vehicle.handle==parent) {
            const auto point=vehicle.transform.position;
            const auto below=world.match.simulation().collision().raycast(point,{0,0,-1},1000.f);
            row["flight_samples"].push_back({{"tick",frame.tick},{"position",{point.x,point.y,point.z}},
              {"ground_clearance",below?below->distance:-1.f}});
          }
          for(const auto &event:frame.grenade_detonations)
            if(event.definition==content.weapons.at(actual.definition).projectile &&
               (row["source_contact_samples"].empty() || event.serial>row["source_contact_samples"].back()["serial"].get<std::uint64_t>()))
              row["source_contact_samples"].push_back({{"serial",event.serial},{"tick",event.tick},
                {"projectile",event.projectile},{"unit",event.unit},{"source_effect_instance",event.source_effect_instance},
                {"position",{event.position.x,event.position.y,event.position.z}}});
        }
''' + needle)
        write_changed(source / relative, text)
        files[str(relative)] = {'before': sha(old), 'after': sha(source / relative)}
    cmake = source / 'CMakeLists.txt'
    text = cmake.read_text().split('\nadd_executable(combat_presentation_audit EXCLUDE_FROM_ALL')[0]
    if args.partition_ai:
        needle = 'target_sources(halo2_systems PRIVATE src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp)'
        replacement = ('include(cmake/NativeTranslationUnits.cmake)\n'
                       'halo2_partition_native_translation(halo2_systems src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp 12)')
        if needle in text:
            text = text.replace(needle, replacement)
        elif replacement not in text:
            raise RuntimeError('Scene partition source admission changed')
    text += '''
add_executable(combat_presentation_audit EXCLUDE_FROM_ALL tools/audit_combat_presentation_20261022.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(combat_presentation_audit PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(combat_presentation_audit PRIVATE HALO2_SHADER_DIR=\\"${CMAKE_CURRENT_BINARY_DIR}/shaders\\")
add_dependencies(combat_presentation_audit halo2_shaders)
'''
    prefix = ('${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" '
              f'--scratch-root "{scratch / "cases"}" -- ${{Python3_EXECUTABLE}} '
              '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_combat_presentation_audit_20261022.py"')
    tests = [('projectiles', '$<TARGET_FILE:combat_presentation_audit>', 'projectiles', ''),
             ('metadata', '$<TARGET_FILE:combat_presentation_audit>', 'metadata', '')]
    tests += [(name, '$<TARGET_FILE:combat_presentation_audit>', 'zoom-visual', f'{weapon} {presses}')
              for name, weapon, presses in CASES]
    tests += [('full-' + name, '$<TARGET_FILE:combat_presentation_audit>', 'zoom-full-visual', f'{weapon} {presses}')
              for name, weapon, presses in CASES]
    # Spectre is contrail-only; requiring an unauthored MGS2 draw would be a
    # false failure. This fixture isolates the two model-less MGS2/lens bolts.
    tests += [('vehicle-' + name, '$<TARGET_FILE:vehicle_weapon_hud_runtime_tests>', 'vehicle', name)
              for name in ('ghost', 'banshee')]
    tests += [('vehicle-pixels-' + name, '$<TARGET_FILE:vehicle_weapon_hud_runtime_tests>', 'vehicle-visual', name)
              for name in ('ghost', 'banshee')]
    for name, executable, mode, extra in tests:
        if args.case and name not in args.case:
            continue
        text += (f'add_test(NAME combat_audit_{name} COMMAND {prefix} {executable} {mode} "{map_}" '
                 f'"${{CMAKE_CURRENT_BINARY_DIR}}/combat-audit-{name}.json" {extra})\n'
                 f'set_tests_properties(combat_audit_{name} PROPERTIES TIMEOUT 1200)\n')
    write_changed(cmake, text);files['CMakeLists.txt'] = {'after': sha(cmake)}
    (args.output / 'diagnostic-source-refresh.json').write_text(json.dumps({
        'scope': 'Only the owned frozen snapshot is instrumented; suppress-arms is a pixel isolation control, never a parity fix',
        'source': str(source), 'files': files}, indent=2) + '\n')
    if args.prepare_only:
        return 0
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker', '--source', str(source),
               '--target', 'combat_presentation_audit', '--target', 'vehicle_weapon_hud_runtime_tests',
               '--filter', '^combat_audit_', '--jobs', str(args.jobs), '--test-jobs', '1',
               '--output', str(args.output / 'validation')]
    for name, _, mode, _ in tests:
        if args.case and name not in args.case:
            continue
        command += ['--retain-report', f'combat-audit-{name}.json']
        if mode in ('zoom-visual', 'zoom-full-visual', 'vehicle-visual'):
            for suffix in ('baseline', 'repeat', 'without-glow' if mode == 'vehicle-visual' else 'without-foreground' if mode == 'zoom-full-visual' else 'without-arms'):
                command += ['--retain-report', f'combat-audit-{name}-{suffix}.json']
                command += ['--retain-report', f'combat-audit-{name}-{suffix}.log']
        else:
            command += ['--retain-report', f'combat-audit-{name}.log']
    if args.partition_ai:
        command += ['--retain-report', 'native-translations/source_ai_frame_tu15_generated/manifest.json']
        command += ['--retain-report', 'native-translations/source_scene_device_constructor_tu15_generated/manifest.json']
        command += ['--retain-report', 'native-translations/death_cycle_runtime_tu15_generated/manifest.json']
    environment = dict(os.environ, HALO2_COMBAT_AUDIT_CACHE=str(scratch / 'graphics-cache'))
    if args.retain_small_reference:
        environment['HALO2_COMBAT_AUDIT_REFERENCE_DIR'] = str(args.output / 'reference')
    return subprocess.run(command, env=environment, check=False).returncode


if __name__ == '__main__':
    raise SystemExit(main())
