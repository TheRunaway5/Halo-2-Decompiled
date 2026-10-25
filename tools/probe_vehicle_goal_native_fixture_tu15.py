#!/usr/bin/env python3
"""Exercise scoped native fixture changes and observe actual SDL motion.

Only a private copy of the qualified GPU fixture is augmented. No vehicle pose,
velocity, controller return or admission result is supplied. Current compiled
native gameplay/render libraries are borrowed read-only. Build/raw outputs use
the enclosing run_test.py owner and CMake validation uses run_tests.py.
"""
import argparse
import difflib
import json
import math
import os
from pathlib import Path
import shlex
import subprocess
import sys
import time

from weapon_test_cache import sha
from validate_vehicle_goal_rendered_tu15 import CASES
from build_launcher_engine import source_hashes

ROOT=Path(__file__).resolve().parents[1]
INSTRUMENTATION='''          // Private diagnostic: ordinary SDL keys and mouse motion only.
          if(tick==10 || tick==120) {
            for(const auto key:std::array{SDL_SCANCODE_W,SDL_SCANCODE_SPACE}) {
              SDL_Event event{};event.type=tick==10?SDL_KEYDOWN:SDL_KEYUP;
              event.key.windowID=SDL_GetWindowID(window);event.key.keysym.scancode=key;
              event.key.keysym.sym=key==SDL_SCANCODE_W?SDLK_w:SDLK_SPACE;
              event.key.state=tick==10?SDL_PRESSED:SDL_RELEASED;
              check(SDL_PushEvent(&event)==1,"Queue actual Banshee motion input");
            }
          }
          // The ordinary primary press at30 captures the mouse. Submit look
          // after that real event has been processed by the game window.
          if(tick==32) {
            SDL_Event event{};event.type=SDL_MOUSEMOTION;event.motion.windowID=SDL_GetWindowID(window);
            event.motion.yrel=-240;
            check(SDL_PushEvent(&event)==1,"Queue actual Banshee upward look input");
          }
          const auto actual_state=world.match.state();
          const auto parent=actual_state.players[player].source_parent;
          check(parent!=null_datum,"Native Banshee motion observation lost its occupied parent");
          const auto pose=world.source_objects->object_transform(parent);
          row["native_flight_observation"]["samples"].push_back({{"tick",tick},{"parent",parent},
            {"forward_input",input[player].forward},{"jump_input",input[player].jump},
            {"yaw_input",input[player].yaw},{"pitch_input",input[player].pitch},
            {"position",{pose.position.x,pose.position.y,pose.position.z}},
            {"forward",{pose.basis[0].x,pose.basis[0].y,pose.basis[0].z}}});
'''


def save(path,data):path.write_text(json.dumps(data,indent=2)+'\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','validation','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--case',choices=CASES,default='banshee')
    parser.add_argument('--canonical-manifest',type=Path,
        help='Authenticate a newer canonical production tree and the unchanged frozen GPU fixture/helpers')
    parser.add_argument('--also-case',action='append',choices=CASES,default=[])
    parser.add_argument('--banshee-motion',action='store_true')
    parser.add_argument('--live-projectile-observation',action='store_true',
        help='Use completed native source state and full live datum generations for projectile observation')
    parser.add_argument('--refresh-contrail',action='store_true')
    parser.add_argument('--observe-audio',action='store_true',
        help='Retain bounded rejected submission identities without changing any assertion')
    parser.add_argument('--native-audio-output',action='store_true')
    parser.add_argument('--typed-loop-audio',action='store_true',
        help='Use only the current fixture audio-observer repair; retain every other frozen assertion and input')
    parser.add_argument('--after-rendered-report',type=Path,
        help='Wait at most30 minutes for this rendered sweep to finish before using the GPU')
    parser.add_argument('--observe-progress',action='store_true',
        help='Record passive real input and completed-frame timing boundaries in the private fixture')
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.banshee_motion and args.case!='banshee':parser.error('The declared motion inputs are specific to Banshee')
    chosen=[args.case,*args.also_case]
    if len(set(chosen))!=len(chosen) or (args.banshee_motion and len(chosen)!=1):
        parser.error('Use distinct cases and keep Banshee motion as a single-case observation')
    if not (args.banshee_motion or args.refresh_contrail or args.observe_progress or args.observe_audio or args.typed_loop_audio):parser.error('Declare a scoped fixture change')
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    refresh=json.loads(args.refresh.read_text());validation=json.loads(args.validation.read_text())
    borrowed=Path(cohort['owner']);frozen=borrowed/'source';build=borrowed/'build'
    if owner==borrowed or Path(prepared['runner_scratch'])!=borrowed or Path(refresh['owner'])!=borrowed or Path(validation['build'])!=build:
        raise ValueError('Separate authenticated current native build owner required')
    if not all(any(s['name']==name and s['exit_code']==0 for s in validation['steps']) for name in ('configure','build')):
        raise ValueError('Native build has not finished')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for row in cohort['changes']:
        if expected[row['path']]!=row['before']:raise ValueError('Cohort preimage differs')
        expected[row['path']]=row['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if args.canonical_manifest:
        production=json.loads(args.canonical_manifest.read_text())
        if source_hashes(frozen)!=production:
            raise ValueError('Canonical production manifest differs from the current source')
        fixture_inputs={name:pin for name,pin in expected.items() if
            name=='tests/vehicle_weapon_hud_world_tests.cpp' or
            (name.startswith('tests/helpers/') and name.endswith('.hpp'))}
        if 'tests/vehicle_weapon_hud_world_tests.cpp' not in fixture_inputs:
            raise ValueError('Canonical fixture has no authenticated original preimage')
        expected={**production,**fixture_inputs}
    if any(sha(frozen/name)!=pin for name,pin in expected.items()):raise ValueError('Frozen source drifted')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    source=owner/'source';source.mkdir();fixture=frozen/'tests/vehicle_weapon_hud_world_tests.cpp'
    before=fixture.read_text();needle='          if(tick==start+fire_begin || tick==start+fire_end) {'
    if before.count(needle)!=1:raise ValueError('Requalify the private SDL motion observation seam')
    base_fixture=before
    if args.live_projectile_observation:
        from vehicle_hud_live_projectile_observation_tu15 import install_live_projectile_observation
        base_fixture=install_live_projectile_observation(base_fixture)
    after=base_fixture.replace(needle,INSTRUMENTATION+needle) if args.banshee_motion else base_fixture
    if args.typed_loop_audio:
        current=(ROOT/'tests/vehicle_weapon_hud_world_tests.cpp').read_text()
        old_start='        options.audio_observer=[&](const MultiplayerAudioSubmission &submission) {'
        new_start='        options.audio_observer=[&,word](const MultiplayerAudioSubmission &submission) {'
        end='      }\n      if(ghost_fire) {'
        if base_fixture.count(old_start)!=1 or current.count(new_start)!=1:
            raise ValueError('Requalify the audio observer repair boundary')
        first=base_fixture.index(old_start);last=base_fixture.index(end,first)
        new_first=current.index(new_start);new_last=current.index(end,new_first)
        if current[:new_first]+base_fixture[first:last]+current[new_last:]!=base_fixture:
            raise ValueError('The current fixture changed outside its audio observer')
        block=current[new_first:new_last]
        if '// Classify attached loop packets before applying the firing-cue invariant.' not in block:
            raise ValueError('Required typed-loop observation repair is absent')
        after=after.replace(base_fixture[first:last],block,1)
    if args.observe_audio:
        catch='}catch(const std::exception &error){result["submission_violations"].push_back(error.what());}'
        if after.count(catch)!=1:raise ValueError('Requalify the private audio rejection observation seam')
        after=after.replace(catch,
            '}catch(const std::exception &error){\n'
            '  result["submission_violations"].push_back(error.what());\n'
            '  auto &rejected=result["rejected_submissions"];if(rejected.is_null())rejected=nlohmann::json::array();\n'
            '  if(rejected.size()<128){const auto *tag=cache.query_tag({.handle=TagHandle{submission.sound}});\n'
            '    rejected.push_back({{"sound",submission.sound},{"name",tag?tag->name:std::string{}},\n'
            '      {"kind",unsigned(submission.kind)},{"owner",submission.owner},{"view",submission.view},\n'
            '      {"voice",submission.voice},{"frames",submission.frames},{"error",error.what()}});}\n'
            '}',1)
    if args.observe_progress:
        after=after.replace(needle,
            '          if(tick<200)std::cerr<<"[VEHICLE-INPUT] tick="<<tick<<"\\n";\n'+needle,1)
        entry='      check(run_multiplayer(options)==0 && submitted>=4,"Actual vehicle HUD Vulkan submission stopped early");'
        if after.count(entry)!=1:raise ValueError('Requalify the private completed-frame observation seam')
        after=after.replace(entry,
            '      options.frame_observer=[&](const MultiplayerFrameTiming &t){\n'
            '        std::cerr<<"[VEHICLE-FRAME] frame="<<t.frame<<" submitted="<<t.submitted\n'
            '          <<" input_ms="<<t.input_ms<<" renderer_ms="<<t.renderer_ms<<" wall_ms="<<t.wall_ms<<"\\n";\n'
            '      };\n      std::cerr<<"[VEHICLE-RUN-ENTER]\\n";\n'+entry,1)
    (source/'fixture.cpp').write_text(after)
    (args.output/'fixture.diff').write_text(''.join(difflib.unified_diff(before.splitlines(True),
        after.splitlines(True),fromfile='qualified GPU fixture',tofile='private SDL motion fixture')))
    overrides={}
    if args.refresh_contrail:
        name='src/rendering/effects/source_contrail_snapshot.cpp'
        original=(frozen/name).read_text();current=(ROOT/name).read_text()
        addition=('    // Original CRT math32EA98/32EB36 and32CD20/32CD84 read these writable\n'
            '    // dispatch policies in reached contrail geometry helpers. Retain each\n'
            '    // live authority word; loader defaults and later backing reads cannot\n'
            '    // represent this completed view/replica input.\n'
            '    add(0x46ecd0u,4);\n    add(0x48d4f4u,4);\n')
        if current.count(addition)!=1 or current.replace(addition,'',1)!=original:
            raise ValueError('Contrail override differs from the qualified two-word input repair')
        path=source/'source_contrail_snapshot.cpp';path.write_text(current)
        overrides[name]=dict(before=sha(frozen/name),after=sha(path))
        if sha(ROOT/name)!=sha(path):raise ValueError('Contrail input changed while copying')
    link_file=build/'CMakeFiles/vehicle_weapon_hud_runtime_tests.dir/link.txt'
    native_link_file=build/'CMakeFiles/halo2_native.dir/link.txt'
    link=shlex.split(link_file.read_text());at=link.index('-o')
    objects=[(build/name).resolve(strict=True) for name in link[:at] if name.endswith('multiplayer_runtime.cpp.o')]
    native=shlex.split(native_link_file.read_text())
    caches=[(build/name).resolve(strict=True) for name in native if name.endswith('native_vulkan_pipeline_cache.cpp.o')]
    wrappers=[name for name in native if name.startswith('-Wl,--wrap=vk')]
    if len(objects)!=1 or len(caches)!=1 or len(wrappers)!=4:raise ValueError('Native entry/cache link owners differ')
    pins={str(path):sha(path) for path in (*objects,*caches)};resolved=[*map(str,objects),*map(str,caches),*wrappers]
    for name in link[at+2:]:
        if name.startswith('-'):resolved.append(name);continue
        path=(build/name).resolve(strict=True);pins[str(path)]=sha(path);resolved.append(str(path))
    q=lambda value:'"'+str(value).replace('\\','/').replace('"','\\"')+'"'
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    cmake='''cmake_minimum_required(VERSION 3.20)
project(VehicleNativeFixtureObservation LANGUAGES CXX)
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
add_executable(vehicle_flight_gpu fixture.cpp)
target_compile_features(vehicle_flight_gpu PRIVATE cxx_std_20)
target_compile_definitions(vehicle_flight_gpu PRIVATE HALO2_VEHICLE_HUD_GPU=1)
target_compile_options(vehicle_flight_gpu PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
    cmake+=('target_include_directories(vehicle_flight_gpu PRIVATE ${SDL2_INCLUDE_DIRS} '+
        ' '.join(q(frozen/name) for name in ('include','src','tests'))+')\n')
    if args.refresh_contrail:cmake+='target_sources(vehicle_flight_gpu PRIVATE source_contrail_snapshot.cpp)\n'
    cmake+=('target_link_libraries(vehicle_flight_gpu PRIVATE "-Wl,--start-group" '+
        ' '.join(map(q,resolved))+' "-Wl,--end-group")\n')
    def gameplay_directory(case):return args.output/'gameplay'/case if len(chosen)>1 else args.output/'gameplay'
    for case in chosen:
        test='vehicle_native_fixture_'+case if len(chosen)>1 else 'vehicle_native_fixture'
        cmake+=('add_test(NAME '+test+' COMMAND "${Python3_EXECUTABLE}" '+q(ROOT/'tools/run_test.py')+
            ' -- "${Python3_EXECUTABLE}" -B '+q(ROOT/'tools/validate_vehicle_goal_rendered_tu15.py')+
            ' --case-worker --case '+case+' --runtime $<TARGET_FILE:vehicle_flight_gpu> --maps '+q(maps)+
            ' --output '+q(gameplay_directory(case))+(' --native-audio-output' if args.native_audio_output else '')+')\n')
        cmake+='set_tests_properties('+test+' PROPERTIES TIMEOUT 330 RUN_SERIAL TRUE)\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--output',str(args.output/'validation'),'--target','vehicle_flight_gpu','--filter','^vehicle_native_fixture',
        '--jobs','2','--test-jobs','1']
    report=dict(passed=False,status='prepared',owner=str(owner),borrowed_owner=str(borrowed),
        fixture_source_sha256=sha(fixture),private_fixture_sha256=sha(source/'fixture.cpp'),
        case=args.case,banshee_motion_inputs=args.banshee_motion,native_source_overrides=overrides,
        selected_cases=chosen,
        passive_progress_observation=args.observe_progress,
        passive_audio_rejection_observation=args.observe_audio,
        typed_loop_audio_observer_repair=args.typed_loop_audio,
        live_source_projectile_observation=args.live_projectile_observation,
        canonical_manifest_sha256=sha(args.canonical_manifest) if args.canonical_manifest else None,
        borrowed_native_inputs_sha256=pins,command=command,
        original_physics_or_controller_instructions_changed=False,pose_or_velocity_supplied=False,
        original_flight_compared=False,full_vehicle_parity=False,
        tool_sha256=sha(Path(__file__)))
    save(args.output/'report.json',report)
    if args.after_rendered_report:
        deadline=time.monotonic()+1800
        while True:
            other=json.loads(args.after_rendered_report.read_text())
            if other.get('status')=='completed':break
            if time.monotonic()>=deadline:raise ValueError('Prior rendered sweep did not finish within the finite wait')
            time.sleep(1)
        if not (all(sha(Path(name))==pin for name,pin in pins.items()) and
                all(sha(frozen/name)==pin for name,pin in expected.items())):
            raise ValueError('Borrowed native inputs changed while awaiting the GPU')
    code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    p=gameplay_directory(args.case)/'gameplay.json';data=json.loads(p.read_text()) if p.exists() else {}
    samples=data.get('cases',[{}])[0].get('native_flight_observation',{}).get('samples',[])
    motion={}
    if samples:
        initial=samples[0]['position'];final=samples[-1]['position']
        delta=[final[i]-initial[i] for i in range(3)]
        motion=dict(samples=len(samples),initial=initial,final=final,delta=delta,
            displacement=math.sqrt(sum(v*v for v in delta)),
            maximum_height_gain=max(row['position'][2] for row in samples)-initial[2],
            actual_forward_input_ticks=sum(row['forward_input']>0 for row in samples),
            actual_jump_input_ticks=sum(row['jump_input'] for row in samples),
            actual_pitch_range=[min(row['pitch_input'] for row in samples),max(row['pitch_input'] for row in samples)],
            parent_generations=sorted(set(row['parent'] for row in samples)))
    motion_passed=bool(samples and motion['actual_forward_input_ticks']>0 and
        motion['actual_jump_input_ticks']>0 and motion['displacement']>0 and
        motion['maximum_height_gain']>0 and len(motion['parent_generations'])==1 and
        motion['actual_pitch_range'][1]>motion['actual_pitch_range'][0] and
        motion['parent_generations'][0]!=0xffffffff and all(
            math.isfinite(value) for sample in samples for value in sample['position']))
    unchanged=all(sha(Path(name))==pin for name,pin in pins.items()) and all(sha(frozen/name)==pin for name,pin in expected.items())
    cases=[]
    for case in chosen:
        path=gameplay_directory(case)/'report.json';value=json.loads(path.read_text()) if path.exists() else {}
        cases.append(dict(case=case,passed=value.get('passed') is True,report=str(path),
            report_sha256=sha(path) if path.exists() else None,
            actual_host_output_verified=value.get('actual_host_output_verified')))
    report.update(passed=code==0 and unchanged and (motion_passed if args.banshee_motion else data.get('passed') is True),status='completed',validation_returncode=code,
        native_inputs_unchanged=unchanged,motion_observation=motion,
        native_motion_observation_passed=motion_passed if args.banshee_motion else None,
        full_flight_acceptance=False,cases=cases,required_frame=str(gameplay_directory(args.case)/'reference.png'))
    if not args.banshee_motion:report['passed'] &= all(row['passed'] for row in cases)
    save(args.output/'report.json',report)
    print(json.dumps(dict(passed=report['passed'],motion=motion,report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed'] else code or 1


if __name__=='__main__':raise SystemExit(main())
