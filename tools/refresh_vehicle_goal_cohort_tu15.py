#!/usr/bin/env python3
"""Stage named native vehicle owners and variant tests in the active owned build.

Use the same run_test.py owner as prepare_vehicle_goal_cohort_tu15.py. The
official run_tests.py worker compiles and executes the registered cases.
"""
import argparse
import difflib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]
FILES=(
    'include/halo2/source_vehicle_native_rates.hpp',
    'src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp',
    'src/physics/runtime/physics_instruction_remap_tu15.hpp',
    'src/physics/physics_phantom_world_tu15_generated.cpp',
    'include/halo2/source_vehicle_bv_agent_tu15.hpp',
    'src/physics/collision/source_vehicle_bv_agent_tu15_generated.cpp',
    'src/physics/physics_live_world_tu15.cpp',
    'src/physics/vehicles/physics_vehicle_runtime_generated.cpp',
    'src/gameplay/units/source_direct_damage_runtime_generated.cpp',
    'src/gameplay/units/source_vitality_runtime_generated.cpp',
    'src/physics/physics_live_support_tu15_generated.cpp',
    'tools/native_damage_model_kernel_tu15.py',
    'tools/generate_source_direct_damage_runtime.py',
    'tools/generate_source_vitality_runtime.py',
    'tools/generate_physics_live_support_tu15.py',
    'tools/generate_vehicle_runtime_20261011.py',
    'tools/generate_phantom_world_tu15_20261011.py',
    'tools/generate_vehicle_bv_agent_tu15.py',
    'tests/physics_vehicle_runtime_tests.cpp',
    'tests/vehicle_variants_world_tu15_tests.cpp',
    'tests/vehicle_weapon_hud_world_tests.cpp',
    'tests/vehicle_bv_agent_tu15_tests.cpp',
    'tests/destruction_runtime_tu15_tests.cpp',
    'tests/native_tu15_physics_branch_ownership_tests.cpp',
    'tests/native_tu15_world_memory_tests.cpp',
    'tests/source_contrail_snapshot_tests.cpp',
    'tests/light_fade_curves_tu15_tests.cpp',
    'src/rendering/effects/source_contrail_snapshot.cpp',
    'src/rendering/renderer/renderer.cpp',
    'src/platform/native_tu15_world_memory.cpp',
    'src/physics/native_tu15_physics_branch_data.cpp',
    'src/physics/native_tu15_light_portal_literal_data.inc',
    'src/physics/native_tu15_fade_curve_data.inc',
    'tools/export_native_tu15_fade_curve_data.py',
    'tools/export_native_tu15_light_portal_literal_data.py',
    'tools/audit_native_tu15_portal_bounds_binding.py',
    'tests/fixtures/vehicle_bv_agent_original_tu15.json',
    'tests/fixtures/vehicle_bv_agent_original_tu15.provenance.json',
    'cmake/PhysicsVehicleRuntime.cmake',
    'cmake/VehicleBvAgentTu15.cmake',
)


def save(path,value):
    path.write_text(json.dumps(value,indent=2)+'\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--cohort',type=Path,required=True)
    parser.add_argument('--cases',type=Path,required=True)
    parser.add_argument('--previous-refresh',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--prepare-only',action='store_true')
    parser.add_argument('--variants',action='store_true')
    parser.add_argument('--case-id',action='append')
    parser.add_argument('--file',action='append',choices=FILES,
                        help='Refresh only these named files; preserve every other authenticated frozen input')
    parser.add_argument('--parallel-destruction',action='store_true',
                        help='Allow at most two independent headless destruction processes with four CTest jobs')
    parser.add_argument('--install-native-model-support',action='store_true',
                        help='Install only the authenticated shared lookup route into frozen live support; preserve every other body')
    parser.add_argument('--jobs',type=int,default=5)
    parser.add_argument('--test-jobs',type=int,default=4)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the active tools/run_test.py owner')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=owner/'source'
    prepared=json.loads(args.prepared.read_text());cohort=json.loads(args.cohort.read_text())
    if Path(prepared['runner_scratch']).resolve()!=owner or Path(cohort['owner']).resolve()!=owner:
        raise ValueError('Vehicle cohort belongs to another owner')
    # Never modify a source/build pair while its existing build or tests run.
    session=ROOT/'analysis/vehicles-tu15-goal-20261023/session-v1/session.json'
    if session.exists():
        for command in json.loads(session.read_text())['commands']:
            if 'finished' not in command and not any(
                    Path(arg).name==Path(__file__).name for arg in command['argv'] if '\n' not in arg):
                raise ValueError('Another source/build command is still running')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for change in cohort['changes']:
        if expected[change['path']]!=change['before']:raise ValueError('Unrelated partition preimage')
        expected[change['path']]=change['after']
    expected.update({'data/'+name:pin for name,pin in cohort.get('private_data_sha256',{}).items()})
    refreshed={}
    if args.previous_refresh:
        prior=json.loads(args.previous_refresh.read_text())
        if prior['owner']!=str(owner):raise ValueError('Previous refresh belongs to another owner')
        refreshed.update(prior['refreshed_source_sha256']);expected.update(refreshed)
    drift=[name for name,pin in expected.items() if sha(source/name)!=pin]
    if drift:raise ValueError('Undeclared frozen source drift: '+', '.join(drift[:10]))
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    changes=[]
    for name in (args.file or FILES):
        path=source/name;before=sha(path) if path.exists() else None
        if before is not None and name not in expected:raise ValueError('Unregistered preexisting source: '+name)
        old=path.read_text() if path.exists() else ''
        derived=args.install_native_model_support and name in (
            'src/physics/physics_live_support_tu15_generated.cpp','tools/generate_physics_live_support_tu15.py')
        if derived:
            import hashlib
            if name.endswith('.cpp'):
                from native_damage_model_kernel_tu15 import install_actual_model_lookup,strip_actual_model_lookup
                new=install_actual_model_lookup(old)
                if strip_actual_model_lookup(new)!=strip_actual_model_lookup(old):
                    raise ValueError('Native lookup installation changed another frozen body')
            else:
                anchor="    temporary=a.output.with_name('.'+a.output.name+f'.{os.getpid()}.tmp')\n"
                write='temporary.write_text(preamble+body_source+ending)'
                hook=('    generated=preamble+body_source+ending\n'
                    '    if 0xd80a0 in ENTRIES:\n'
                    '        from native_damage_model_kernel_tu15 import install_actual_model_lookup\n'
                    '        generated=install_actual_model_lookup(generated)\n')
                if old.count(anchor)!=1 or old.count(write)!=1 or 'install_actual_model_lookup' in old:
                    raise ValueError('Frozen live-support generator layout changed')
                new=old.replace(anchor,hook+anchor).replace(write,'temporary.write_text(generated)')
            pin=hashlib.sha256(new.encode()).hexdigest()
        else:
            pin=sha(ROOT/name);new=(ROOT/name).read_text()
        path.parent.mkdir(parents=True,exist_ok=True)
        # Changed inputs receive fresh timestamps. Authenticated identical
        # bodies keep theirs, so a fixture-only refresh need not rebuild the
        # large unchanged native translation units.
        if before!=pin:
            if derived:path.write_text(new)
            else:shutil.copyfile(ROOT/name,path)
        if sha(path)!=pin or (not derived and sha(ROOT/name)!=pin):raise ValueError('Source changed during scoped copy: '+name)
        refreshed[name]=pin
        diff=''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile=name+' (frozen)',tofile=name+' (refreshed)'))
        if len(diff.encode())>512*1024:raise ValueError('Vehicle refresh exceeds its declared narrow scope')
        changes.append(dict(path=name,before=before,after=pin,copied=before!=pin,diff=diff,
            native_route_only=derived,all_existing_literal_bodies_preserved=derived))
    cases=json.loads(args.cases.read_text())
    if cases['schema']!='H2_VEHICLE_VARIANT_CONSTRUCTOR_CASES_V1' or len(cases['cases'])!=327:
        raise ValueError('Complete pinned variant constructor cohort required')
    chosen=[case for case in cases['cases'] if not args.case_id or case['case_id'] in args.case_id]
    if args.case_id and len(chosen)!=len(set(args.case_id)):raise ValueError('Requested case identity missing')
    maps=Path(prepared['runtime_cache_inputs']['maps_directory'])
    inputs=source/'tests/fixtures/vehicle-variants-tu15';inputs.mkdir(parents=True,exist_ok=True)
    cmake=source/'CMakeLists.txt';before=sha(cmake)
    marker='\n# VEHICLE_NATIVE_VARIANTS_TU15\n'
    text=cmake.read_text().split(marker)[0]+marker
    if args.parallel_destruction:
        # Each case owns its native world, clock, output and scratch, with no
        # renderer or UDP endpoint. PROCESSORS2 limits the four-job cohort to
        # two simultaneous destruction processes; no input/result changes.
        for case in cohort['destruction_cases']:
            before_properties=f'set_tests_properties(vehicle_goal_destruction_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
            after_properties=f'set_tests_properties(vehicle_goal_destruction_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL FALSE PROCESSORS 2 '
            if text.count(before_properties)==1:text=text.replace(before_properties,after_properties)
            elif text.count(after_properties)!=1:raise ValueError('Destruction registration changed: '+case)
    if 'include(cmake/VehicleBvAgentTu15.cmake)' not in text:
        text+='include(cmake/VehicleBvAgentTu15.cmake)\n'
    for case in chosen:
        original=args.cases.parent/case['path'];target=inputs/case['path']
        if sha(original)!=case['sha256']:raise ValueError('Variant case manifest changed')
        shutil.copyfile(original,target)
        if sha(target)!=case['sha256'] or sha(original)!=case['sha256']:
            raise ValueError('Variant input changed while freezing')
        refreshed[str(target.relative_to(source))]=sha(target)
        map_path=maps/case['map']
        if not map_path.is_file():raise ValueError('Prepared runtime map absent: '+case['map'])
        identifier=case['case_id']
        text+=(f'add_test(NAME vehicle_goal_variant_{identifier} COMMAND "${{Python3_EXECUTABLE}}" '
               '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:vehicle_variants_world_tu15_tests> '
               f'"{map_path}" "{maps}" "{target}" '
               f'"${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-variant-{identifier}.json")\n'
               f'set_tests_properties(vehicle_goal_variant_{identifier} PROPERTIES TIMEOUT 300 PROCESSORS 1)\n')
    for case in ('banshee','scorpion'):
        text+=(f'add_test(NAME vehicle_goal_secondary_{case} COMMAND "${{CMAKE_COMMAND}}" -E env '
               '--unset=HALO2_TEST_TURRET_TARGET_DAMAGE "${Python3_EXECUTABLE}" '
               '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:vehicle_weapon_hud_world_tests> '
               f'"{maps / "coagulation.map"}" "{maps}" native-mcc)\n'
               f'set_tests_properties(vehicle_goal_secondary_{case} PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
               f'ENVIRONMENT "HALO2_TEST_VEHICLE_WEAPON_HUD_CASE={case};HALO2_TEST_VEHICLE_WEAPON_FIRE=1;'
               'HALO2_TEST_VEHICLE_WEAPON_SECONDARY=1;HALO2_TEST_TURRET_SOUND_TRACE=1;'
               f'HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${{CMAKE_CURRENT_BINARY_DIR}}/vehicle-secondary-{case}.json")\n')
    text+=('add_test(NAME vehicle_goal_boost_ghost COMMAND "${Python3_EXECUTABLE}" '
           '"${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" -- $<TARGET_FILE:vehicle_weapon_hud_world_tests> '
           f'"{maps / "coagulation.map"}" "{maps}" native-mcc)\n'
           'set_tests_properties(vehicle_goal_boost_ghost PROPERTIES TIMEOUT 300 RUN_SERIAL TRUE '
           'ENVIRONMENT "HALO2_TEST_VEHICLE_WEAPON_HUD_CASE=ghost;HALO2_TEST_GHOST_BOOST=1;'
           'HALO2_TEST_VEHICLE_WEAPON_HUD_REPORT=${CMAKE_CURRENT_BINARY_DIR}/vehicle-boost-ghost.json")\n')
    cmake.write_text(text);refreshed['CMakeLists.txt']=sha(cmake)
    changes.append(dict(path='CMakeLists.txt',before=before,after=sha(cmake),
                        diff='Owned vehicle test registrations only; optional two-process headless destruction scheduling'))
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
             '--output',str(args.output/'validation'),'--jobs',str(args.jobs),'--test-jobs',str(args.test_jobs),
             '--target','vehicle_variants_world_tu15_tests' if args.variants else 'vehicle_weapon_hud_world_tests',
             '--filter','^vehicle_goal_variant_' if args.variants else '^vehicle_goal_weapon_']
    for value in cohort['cmake_args']:command+=['--cmake-arg='+value]
    if args.variants:
        for case in chosen:command+=['--retain-report','vehicle-variant-'+case['case_id']+'.json']
    else:
        for case in ('ghost','banshee','scorpion','wraith','warthog-mg','warthog-gauss','spectre','h_turret_ap','c_turret_ap'):
            command+=['--retain-report','vehicle-weapon-'+case+'.json']
    report=dict(passed=False,status='prepared',validation_not_run=True,owner=str(owner),
                changes=changes,refreshed_source_sha256=refreshed,command=command,
                cases_sha256=sha(args.cases),registered_cases=[case['case_id'] for case in chosen],
                additional_cases=['secondary_banshee','secondary_scorpion','boost_ghost'],
                parallel_destruction=args.parallel_destruction,
                tool_sha256=sha(Path(__file__)),full_tu15_parity=False,large_artifacts_retained=[])
    receipt=args.output/'refresh.json';save(receipt,report)
    if args.prepare_only:
        print(json.dumps(dict(status='prepared',report=str(receipt))));return 0
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT)
    report.update(passed=result.returncode==0,status='validated' if result.returncode==0 else 'failed',
                  validation_not_run=False,returncode=result.returncode,
                  frozen_source_drift=[name for name,pin in (expected|refreshed).items() if sha(source/name)!=pin])
    report['passed'] &= not report['frozen_source_drift'];save(receipt,report)
    print(json.dumps(dict(passed=report['passed'],report=str(receipt))))
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
