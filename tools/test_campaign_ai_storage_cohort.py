#!/usr/bin/env python3
"""Check current campaign AI storage in one disposable immutable source cohort.

Keep the production CMake target, source files, exact include dependency closure
and original fixtures together while other work changes the checkout. Normal
and sanitizer checks use the same snapshot through tools/run_tests.py. The
outer tools/run_test.py removes all copied sources and generated files.
This is component evidence, not a complete client or live campaign/co-op test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCES = (
    'tests/campaign_ai_working_mask_tu15_tests.cpp',
    'src/campaign/ai/source_ai_frame_tu15_generated.cpp', 'src/campaign/ai/source_ai_actor_tu15_generated.cpp',
    'src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp',
    'src/platform/native_tu15_final_startup_storage_data.cpp', 'src/runtime/source_runtime_memory.cpp',
    'src/campaign/scripting/native_mcc_script_builtin.cpp', 'src/assets/cache_file_snapshot.cpp',
    'src/runtime/source_runtime_snapshot_bytes.cpp',
)
INPUTS = (
    'cmake/CampaignAiWorkingMask.cmake', 'cmake/SnapshotStorage.cmake',
    'cmake/TestStorage.cmake', 'tools/run_test.py',
    'tests/fixtures/campaign_ai_working_mask_tu15.json',
    'tests/fixtures/campaign_ai_decision_scratch_tu15.json',
    'tests/fixtures/campaign_ai_children_tu15.json',
)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--compiler', default=os.environ.get('CXX','c++'),
        help='Use this compiler for both dependency capture and disposable builds')
    ap.add_argument('--child-directory', action='store_true',
        help='Qualify the authored-property/whole-directory Actor target')
    ap.add_argument('--scalar-defaults', action='store_true',
        help='Qualify the complete original AI scalar _cinit callbacks')
    ap.add_argument('--descriptor-callbacks', action='store_true',
        help='Qualify original kind2 descriptor callback tail selectors')
    ap.add_argument('--object-decisions', action='store_true',
        help='Qualify original AI object-decision bodies and retained nested owners')
    args = ap.parse_args()
    compiler=shutil.which(args.compiler)
    if compiler is None:ap.error('Compiler is unavailable')
    if sum((args.child_directory,args.scalar_defaults,args.descriptor_callbacks,args.object_decisions))>1:
        ap.error('Select one production component target')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    project = scratch/'campaign-ai-storage'; project.mkdir()
    args.output = args.output.resolve(); args.output.mkdir(parents=True, exist_ok=True)
    sources=SOURCES
    inputs=INPUTS
    module='CampaignAiWorkingMask'
    target_name='campaign_ai_working_mask_tu15_tests'
    test='original_campaign_ai_working_mask_tu15'
    if args.child_directory:
        sources=tuple(n for n in SOURCES if n not in ('tests/campaign_ai_working_mask_tu15_tests.cpp',
            'src/campaign/ai/source_ai_frame_tu15_generated.cpp'))+(
            'tests/campaign_ai_child_directory_tu15_tests.cpp',)
        inputs=tuple(n for n in INPUTS if not n.startswith('tests/fixtures/') and n!='cmake/CampaignAiWorkingMask.cmake')+(
            'cmake/CampaignAiChildDirectory.cmake','tests/fixtures/campaign_ai_child_directory_tu15.json',
            'tests/fixtures/campaign_ai_dispatch_tu15.json')
        module='CampaignAiChildDirectory'
        target_name='campaign_ai_child_directory_tu15_tests'
        test='original_campaign_ai_child_directory_tu15'
    if args.scalar_defaults:
        sources=tuple(n for n in SOURCES if n not in ('tests/campaign_ai_working_mask_tu15_tests.cpp',
            'src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/campaign/ai/source_ai_actor_tu15_generated.cpp',
            'src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp'))+(
            'tests/campaign_ai_scalar_defaults_tu15_tests.cpp','src/campaign/ai/source_campaign_ai_scalar_defaults_tu15.cpp')
        inputs=tuple(n for n in INPUTS if not n.startswith('tests/fixtures/') and n!='cmake/CampaignAiWorkingMask.cmake')+(
            'cmake/CampaignAiScalarDefaults.cmake','tests/fixtures/campaign_ai_scalar_defaults_tu15.json')
        module='CampaignAiScalarDefaults'
        target_name='campaign_ai_scalar_defaults_tu15_tests'
        test='original_campaign_ai_scalar_defaults_tu15'
    if args.descriptor_callbacks:
        sources=tuple(n for n in SOURCES if n not in ('tests/campaign_ai_working_mask_tu15_tests.cpp',
            'src/campaign/ai/source_ai_actor_tu15_generated.cpp'))+('tests/campaign_ai_descriptor_callbacks_tu15_tests.cpp',)
        inputs=tuple(n for n in INPUTS if not n.startswith('tests/fixtures/') and n!='cmake/CampaignAiWorkingMask.cmake')+(
            'cmake/CampaignAiDescriptorCallbacks.cmake','tests/fixtures/campaign_ai_descriptor_callbacks_tu15.json',
            'tests/fixtures/campaign_ai_object_tracking_tu15.json','tests/fixtures/campaign_ai_tracking_dispatch_tu15.json')
        module='CampaignAiDescriptorCallbacks'
        target_name='campaign_ai_descriptor_callbacks_tu15_tests'
        test='original_campaign_ai_descriptor_callbacks_tu15'
    if args.object_decisions:
        sources=tuple(n for n in SOURCES if n not in ('tests/campaign_ai_working_mask_tu15_tests.cpp',
            'src/campaign/ai/source_ai_actor_tu15_generated.cpp'))+('tests/campaign_ai_object_decisions_tu15_tests.cpp',
            'src/campaign/ai/source_campaign_ai_object_decisions_tu15_generated.cpp','src/campaign/ai/source_campaign_ai_object_callbacks_tu15_generated.cpp',
            'src/campaign/source_campaign_birth_query_variants_tu15_generated.cpp',
            'src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp')
        sources+=('src/physics/native_tu15_physics_branch_data.cpp',)
        sources+=('src/gameplay/units/source_damage_delivery_runtime_generated.cpp',)
        inputs=tuple(n for n in INPUTS if not n.startswith('tests/fixtures/') and n!='cmake/CampaignAiWorkingMask.cmake')+(
            'cmake/CampaignAiObjectDecisions.cmake','tests/fixtures/campaign_ai_object_decisions_tu15.json',
            'tests/fixtures/campaign_ai_workspace_leases_tu15.json','tests/fixtures/campaign_ai_object_callbacks_tu15.json',
            'tests/fixtures/campaign_ai_object_callback_bodies_tu15.json','tests/fixtures/campaign_ai_callback_numeric_data_tu15.json',
            'tests/fixtures/campaign_ai_query_identity_tu15.json','tests/fixtures/campaign_ai_prop_selector_tu15.json',
            'tests/fixtures/campaign_ai_object_tracking_tu15.json','tests/fixtures/campaign_ai_actor_state_tu15.json',
            'tests/fixtures/campaign_ai_query_bucket_tu15.json','tests/fixtures/campaign_birth_query_variants_tu15.json',
            'tests/fixtures/campaign_update_query_variants_tu15.json','tests/fixtures/campaign_remaining_query_variants_tu15.json',
            'tests/fixtures/campaign_ai_actor_behavior_inputs_tu15.json','tests/fixtures/campaign_ai_script_context_tu15.json',
            'tests/fixtures/campaign_marine_commands_tu15.json','tests/fixtures/campaign_hs_function_registry_tu15.json',
            'tests/fixtures/campaign_ai_shared_prop_helper_tu15.json','tests/fixtures/campaign_actor_scalars_tu15.json',
            'tests/fixtures/campaign_ai_bsp_audibility_cache_tu15.json',
            'tests/fixtures/campaign_ai_difficulty_tu15.json',
            'tests/fixtures/campaign_camera_effect_request_tu15.json',
            'tests/fixtures/campaign_camera_effect_request_hardware_tu15.json',
            'tests/fixtures/campaign_damage_readonly_tu15.json',
            'tests/fixtures/campaign_damage_reaction_tu15.json',
            'tests/fixtures/campaign_ai_prop_owner_lookup_tu15.json',
            'tests/fixtures/campaign_ai_prop_creation_tu15.json',
            'tests/fixtures/campaign_ai_prop_creation_admissible_tu15.json',
            'tests/fixtures/campaign_ai_decision_workspace_tu15.json',
            'tests/fixtures/campaign_geometry_projection_tu15.json',
            'tests/fixtures/campaign_ai_child_links_tu15.json',
            'tests/fixtures/campaign_ai_path_distance_tu15.json',
            'tests/fixtures/campaign_ai_behavior_27_tu15.json',
            'tools/extract_campaign_ai_bsp_cache_test_slice_tu15.py',
            'src/physics/physics_live_support_tu15_generated.cpp','src/audio/source_sound_listener_selection_tu15_generated.cpp',
            'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp')
        inputs+=('src/platform/native_tu15_world_memory.cpp',)
        module='CampaignAiObjectDecisions'
        target_name='campaign_ai_object_decisions_tu15_tests'
        test='original_campaign_ai_object_decisions_tu15'
    paths = set(inputs) | set(sources)
    # Ask the compiler for the actual include closure instead of importing
    # unrelated generated resources or borrowing mutable root headers.
    for ordinal, name in enumerate(sources):
        dep = scratch/f'input-{ordinal}.d'
        with dep.open('w') as stream:
            subprocess.run([compiler, '-std=c++20', '-Iinclude', '-Isrc', '-MM', name],
                cwd=ROOT, stdout=stream, check=True)
        for path in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
            absolute = (ROOT/path).resolve()
            assert absolute.is_relative_to(ROOT), absolute
            paths.add(str(absolute.relative_to(ROOT)))
    hashes = {}
    for name in sorted(paths):
        raw = (ROOT/name).read_bytes(); target = project/name
        target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(raw)
        hashes[name] = hashlib.sha256(raw).hexdigest()
    assert all(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest
        for name, digest in hashes.items()), 'Checkout changed while capturing input closure'
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(CampaignAiStorage LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
include(CTest)
include(cmake/TestStorage.cmake)
include(cmake/SnapshotStorage.cmake)
include(cmake/SELECTED_MODULE.cmake)
'''.replace('SELECTED_MODULE',module))
    report = dict(scope=__doc__, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inputs_sha256=hashes, commands=[], passed=False,
        complete_client_validated=False, live_campaign_or_coop_validated=False,
        retained_large_artifacts=[])
    output = args.output/'source-cohort.json'
    def save(): output.write_text(json.dumps(report, indent=2)+'\n')
    save()
    for name, sanitized in (('normal', False), ('sanitizers', True)):
        command = [sys.executable, str(ROOT/'tools/run_tests.py'), '--source', str(project),
            '--scratch-root', str(scratch/'builds'), '--output', str(args.output/name),
            '--target', target_name,
            '--filter', '^'+test+'$', '--jobs', '2','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
        if sanitized: command.append('--sanitizers')
        row = dict(name=name, command=command); report['commands'].append(row); save()
        row['returncode'] = subprocess.run(command, cwd=ROOT).returncode; save()
        if row['returncode']: return row['returncode']
    report['snapshot_changed'] = [name for name, digest in hashes.items()
        if hashlib.sha256((project/name).read_bytes()).hexdigest() != digest]
    report['checkout_drift'] = [name for name, digest in hashes.items()
        if not (ROOT/name).is_file() or hashlib.sha256((ROOT/name).read_bytes()).hexdigest() != digest]
    report['passed'] = not report['snapshot_changed']; save()
    return 0 if report['passed'] else 1


if __name__ == '__main__': raise SystemExit(main())
