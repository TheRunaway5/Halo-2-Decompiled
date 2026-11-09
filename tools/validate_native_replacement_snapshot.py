#!/usr/bin/env python3
"""Build and compare one disposable, frozen native-migration source cohort.

Run inside tools/run_test.py. The official CMake worker owns configuration,
build and component tests; the surrounding owner keeps that build alive only
for deterministic frame/performance checks. No launcher selection is changed.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

from build_launcher_engine import freeze_source, source_hashes
from collect_native_production_closure import prepare_peer_source
from native_cohort_test_filter import expand_test_names, flatten_test_filter

ROOT = Path(__file__).resolve().parents[1]
CONTROLS = {
    'HALO2_REFERENCE_MATRIX_LEAVES': '1',
    'HALO2_REFERENCE_BSP_CLUSTER_FLOOD': '1',
    'HALO2_REFERENCE_MODEL_PRT_HELPERS': '1',
    'HALO2_REFERENCE_ARRAY_OWNERS': '1',
    'HALO2_REFERENCE_PARTITION_TEMPORARIES': '1',
    'HALO2_REFERENCE_BROADPHASE_REMOVE': '1',
    'HALO2_REFERENCE_BROADPHASE_ADD': '1',
    'HALO2_REFERENCE_GEOMETRY_RESOURCE': '1',
    'HALO2_REFERENCE_POOL_ALLOCATOR': '1',
    'HALO2_REFERENCE_ARRAY_ALLOCATOR': '1',
    'HALO2_REFERENCE_STACK_ARENA': '1',
    'HALO2_REFERENCE_MAIN_HEAP': '1',
    'HALO2_REFERENCE_RAW_ALLOCATOR': '1',
    'HALO2_REFERENCE_ASYNC_QUEUE': '1',
    'HALO2_REFERENCE_INSTANCED_RAY_CHILD': '1',
    'HALO2_REFERENCE_POLYGON_SUPPORT': '1',
    'HALO2_REFERENCE_POLYGON_FEATURES': '1',
    'HALO2_REFERENCE_CAMERA_INSTANCE_ADMISSION': '1',
    'HALO2_REFERENCE_CAMERA_INSTANCE_BITMAP': '1',
    'HALO2_REFERENCE_TICK_CONVERSION': '1',
    'HALO2_REFERENCE_MILLISECOND_CLOCK': '1',
    'HALO2_REFERENCE_KERNEL_YIELD': '1',
    'HALO2_REFERENCE_RECORD_SEARCH': '1',
    'HALO2_REFERENCE_RECORD_LOOKUP': '1',
    'HALO2_REFERENCE_RESOURCE_RECORD_SEARCH': '1',
    'HALO2_REFERENCE_RESOURCE_RECORD_COUNT': '1',
    'HALO2_REFERENCE_RESOURCE_TOUCH': '1',
    'HALO2_REFERENCE_RESOURCE_MAINTENANCE': '1',
    'HALO2_REFERENCE_RESOURCE_PAGE': '1',
    'HALO2_REFERENCE_ANIMATION_RESOURCE_READ': '1',
    'HALO2_REFERENCE_ANIMATION_RESOURCE_WAIT': '1',
    'HALO2_REFERENCE_RESOURCE_BUSY': '1',
    'HALO2_REFERENCE_HALFWORD_BIT_QUERY': '1',
    'HALO2_REFERENCE_MOPP_AABB': '1',
    'HALO2_REFERENCE_NODE_RESOURCE': '1',
    'HALO2_REFERENCE_NODE_DESCRIPTOR_LOOKUP': '1',
    'HALO2_REFERENCE_VEHICLE_CHOICE': '1',
    'HALO2_REFERENCE_SPHERE_FRUSTUM': '1',
    'HALO2_REFERENCE_OBJECT_NODE_REBUILD': '1',
    'HALO2_REFERENCE_OBJECT_ANIMATION': '1',
    'HALO2_REFERENCE_OBJECT_PREDICTION_COPY': '1',
    'HALO2_REFERENCE_NODE_RESOURCE_LIFECYCLE': '1',
    'HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE': '1',
    'HALO2_REFERENCE_EFFECT_TRANSFORM_PRODUCT': '1',
    'HALO2_REFERENCE_MODEL_DEFAULT_POSE': '1',
    'HALO2_REFERENCE_BSP_SURFACE_2D': '1',
    'HALO2_REFERENCE_BSP_SURFACE_POLYGON': '1',
    'HALO2_REFERENCE_BSP_LEAF_SURFACE': '1',
    'HALO2_REFERENCE_BSP_SEGMENT': '1',
    'HALO2_REFERENCE_BSP_SEGMENT_SETUP': '1',
    'HALO2_REFERENCE_PHYSICAL_EPOCHS': '1',
    'HALO2_REFERENCE_DATUM_DELETE': '1',
    'HALO2_REFERENCE_DATUM_SPECIFIC': '1',
    'HALO2_REFERENCE_DATUM_INITIALIZE': '1',
    'HALO2_REFERENCE_PAGE_ALLOCATOR': '1',
    'HALO2_REFERENCE_PAGE_ALLOCATE': '1',
    'HALO2_REFERENCE_OBJECT_SCAN': '1',
}
TEST_FILTER_PATTERN = ('^(native_replacement_cohort_filter|native_bsp_leaves|native_matrix_leaf_faults|native_bsp_cluster_flood|'
               'native_translation_partition|native_translation_partition_linkage|'
               'native_operand_specialization|native_instruction_cursor|native_instruction_census|'
               'native_migration_inventory|original_tu15_native_collision_matrix_leaves|'
               'original_tu15_native_broadphase_leaves|original_tu15_native_broadphase_bounds_leaves|'
               'original_tu15_native_portal_predicate_leaves|original_tu15_native_partition_temporary_abi|'
               'original_tu15_native_matrix_alignment|original_tu15_native_prt_helpers|original_tu15_native_prt_sample|'
               'original_tu15_native_array_abi|original_tu15_native_broadphase_remove|'
               'original_tu15_native_pool_allocator_abi(_release)?|native_execution_accounting|'
               'original_tu15_native_broadphase_add_leaves|original_tu15_native_broadphase_add_object|'
               'native_geometry_request_bindings|original_tu15_native_geometry_request_abi|original_tu15_native_geometry_publish_abi|'
               'native_async_order_bindings|native_async_insert_bindings|original_tu15_native_async_order|original_tu15_native_async_insert|'
               'original_tu15_native_async_promotion|'
               'original_tu15_native_record_search|'
               'original_tu15_native_record_lookup|'
               'original_tu15_native_resource_record_search|'
               'original_tu15_native_resource_record_count|'
               'original_tu15_native_resource_touch|'
               'original_tu15_native_resource_maintenance|'
               'original_tu15_native_resource_page|'
               'original_tu15_animation_cache_io_policy|'
               'original_tu15_native_animation_resource_read|'
               'original_tu15_native_animation_resource_wait(_release)?|'
               'original_tu15_native_resource_busy|'
               'original_tu15_native_halfword_bit_query(_release)?|'
               'original_tu15_native_datum_(initialize|specific)(_release)?|'
               'original_tu15_native_mopp_aabb_abi(_release)?|'
               'original_mappack_native_mopp_aabb_abi(_release)?|'
               'original_tu15_native_mopp_ray_data|'
               'original_tu15_native_mopp_ray_integer_frame(_release)?|'
               'original_tu15_native_mopp_ray_scale_frame(_release)?|'
               'original_tu15_native_mopp_ray_primitive_key_frame(_release)?|'
               'original_tu15_native_mopp_ray_(plane|branch|crossing|recursion|resume|leaf|slab|diagnostic|vector_store)_frame(_release)?|'
               'original_tu15_native_mopp_ray_split_controller(_release)?|'
               'original_tu15_native_instanced_ray_callback_frame(_release)?|'
               'native_instanced_ray_child_bindings|original_tu15_native_instanced_ray_child(_release)?|native_instanced_ray_child_public_values|'
               'native_tu15_instruction_cursor|'
               'original_tu15_native_camera_instance_admission(_release|_generation)?|'
               'original_tu15_native_camera_instance_bitmap(_release|_generation)?|native_object_node_live_admission|'
               'original_tu15_native_polygon_support(_release|_generation)?|original_tu15_native_polygon_features(_generation)?|'
               'original_mappack_native_polygon_support(_release)?|'
               'original_tu15_native_raw_x87_scope(_release)?|'
               'native_tu15_retained_observer_fp(_release)?|'
               'native_world_observation_generator|'
               'original_tu15_native_node_resource|'
               'original_tu15_native_node_descriptor_lookup(_release)?|'
               'original_tu15_native_vehicle_choice_data(_release)?|'
               'original_tu15_native_object_node_hierarchy|'
               'original_tu15_native_object_node_rebuild|'
               'original_tu15_native_object_animation_delivery|'
               'original_tu15_native_object_prediction_copy|'
               'original_tu15_native_resource_composition(_release)?|'
               'original_tu15_native_sphere_frustum(_release|_generation)?|'
               'original_tu15_native_simplex_direction|original_tu15_native_simplex_candidate|original_tu15_native_simplex_finalize|original_tu15_native_simplex_initialize|original_tu15_native_simplex_projection|original_tu15_native_simplex_iteration|'
               'native_tu15_async_queue_data_bindings|native_tu15_async_queue_source_routes|'
               'original_tu15_native_async_queue_data(_release)?|'
               'native_tu15_async_queue_public_values|'
               'native_tu15_async_promotion_data_bindings|original_tu15_native_async_promotion_data(_release)?|native_tu15_async_promotion_public_values|'
               'original_tu15_native_node_resource_lifecycle(_release)?|'
               'original_tu15_native_effect_datum_allocate|'
               'original_tu15_native_effect_transform_product(_release)?|'
               'original_tu15_native_model_default_pose|'
               'original_tu15_native_bsp_surface_2d(_release)?|'
               'original_tu15_native_bsp_surface_polygon|'
               'original_tu15_native_bsp_leaf_surface(_release)?|'
               'original_tu15_native_bsp_segment(_release)?|'
               'original_tu15_native_bsp_segment_setup(_release)?|'
               'original_tu15_native_physical_epoch|'
               'original_tu15_native_datum_delete|'
               'original_tu15_native_page_window|'
               'original_tu15_native_page_retire|'
               'original_tu15_native_page_allocate_data|'
               'original_tu15_native_page_allocate_abi|'
               'original_tu15_native_stack_probe|'
               'original_tu15_native_tick_conversion|original_tu15_native_tick_conversion_release|'
               'original_tu15_native_millisecond_clock|'
               'original_tu15_native_kernel_yield(_release)?|'
               'original_tu15_native_object_scan|'
               'native_tu15_lazy_heap_controller_data|'
               'original_tu15_native_physical_allocation|original_tu15_native_effect_slot_scan|'
               'original_tu15_native_physical_callbacks|'
               'native_instruction_shaped_audit_parser|native_instruction_shaped_audit|'
               'original_tu15_native_prt_cache|original_tu15_native_prt_cache_build|'
               'original_tu15_native_prt_update|original_tu15_native_model_lighting_leaves|original_tu15_native_model_vector_length|original_tu15_native_model_light_cache|native_tu15_prt_bindings)$')

# CTest's regex engine has a bounded capture table. Keep the human-readable
# finite selector above, but execute its exact flat language with one group.
TEST_NAMES=expand_test_names(TEST_FILTER_PATTERN)
TEST_FILTER=flatten_test_filter(TEST_FILTER_PATTERN)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def test_code_hashes(root):
    return {str(path.relative_to(root)): sha(path) for path in sorted((root / 'tests').rglob('*'))
            if path.is_file() and '__pycache__' not in path.parts and
            path.suffix in ('.cpp', '.hpp', '.h', '.inc', '.py', '.cmake')}


def auxiliary_source_hashes(root):
    """Authenticate the vendored inputs copied by the source freezer too."""
    result={}
    for name in ('third_party','external'):
        directory=root/name
        if not directory.is_dir():continue
        for parent,directories,files in os.walk(directory,followlinks=True):
            directories[:]=[part for part in directories if part not in ('.git','__pycache__','target')]
            for name in files:
                path=Path(parent)/name
                if path.is_file():result[str(path.relative_to(root))]=sha(path)
    return result


def freeze_test_code(root, source):
    """Copy test code; borrow required existing non-code reference inputs."""
    tests = source / 'tests'
    for path in list(tests.iterdir()):
        if path.is_symlink() and path.is_dir():
            path.unlink()
    for original in sorted((root / 'tests').rglob('*')):
        relative = original.relative_to(root / 'tests')
        if '__pycache__' in relative.parts:
            continue
        target = tests / relative
        if original.is_dir():
            target.mkdir(parents=True, exist_ok=True)
        elif original.is_file() and not target.exists():
            target.parent.mkdir(parents=True, exist_ok=True)
            if original.suffix in ('.cpp', '.hpp', '.h', '.inc', '.py', '.cmake'):
                shutil.copy2(original, target)
            else:
                target.symlink_to(original)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--map', action='append', dest='maps')
    parser.add_argument('--case', action='append', dest='cases')
    parser.add_argument('--frames', type=int, default=300)
    parser.add_argument('--build-and-components-only', action='store_true')
    parser.add_argument('--execution-accounting',action='store_true',help='Build diagnostic route counters and collect bounded static/moving workloads; omit performance measurements')
    parser.add_argument('--tu15-xbe',type=Path,required=True,help='Authenticated owned original for physical component oracles')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py for complete source/build/capture cleanup')
    if args.jobs < 1 or args.frames <= 30:
        parser.error('Positive jobs and more than thirty profiling frames are required')
    root, output = args.root.resolve(), args.output.resolve()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    source, build = scratch / 'source', scratch / 'build'
    output.mkdir(parents=True, exist_ok=True)
    report = {'schema': 'native-replacement-frozen-cohort-v1', 'status': 'freezing',
        'passed': False, 'goal_complete': False, 'steps': [],
        'owned_scratch': str(scratch), 'source': str(source), 'build': str(build),
        'reference_controls': CONTROLS, 'retained_large_artifacts': [],
        'execution_accounting_enabled':args.execution_accounting,
        'limits': ['Component and selected-map checks do not establish complete native-only closure.',
                   'FP/MMX exceptional partial ABI domains remain separately qualified.',
                   'Launcher delivery and all-map/campaign/online qualification remain separate.']}

    def save():
        (output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')

    def retain_accounting_metadata(original,destination):
        if original.stat().st_size>1_048_576:raise RuntimeError('Accounting metadata exceeds1MiB')
        existing=[path for path in output.rglob('*.json') if path!=destination]
        retained=sum(path.stat().st_size for path in existing)+original.stat().st_size
        if retained+128*1024>8*1024*1024:
            report['metadata_retention_rejected_bytes']=retained;save()
            raise RuntimeError('Accounting/component metadata exceeds8MiB retention budget')
        shutil.copyfile(original,destination)
        report['retained_metadata_bytes']=retained

    def run(label, command, env=None):
        step = {'name': label, 'command': command, 'started': time.time(), 'exit_code': None}
        report['steps'].append(step);report['status'] = label;save()
        print(json.dumps({'step': label, 'status': 'running'}), flush=True)
        log = output / (label + '.log')
        with log.open('wb') as stream:
            result = subprocess.run(command, cwd=root, env=env, stdout=stream, stderr=subprocess.STDOUT)
        step.update(exit_code=result.returncode, elapsed_seconds=time.time()-step['started'], log=str(log))
        if log.stat().st_size > 8 * 1024 * 1024:
            with log.open('rb') as stream:
                stream.seek(-8 * 1024 * 1024, os.SEEK_END);tail = stream.read()
            log.write_bytes(tail)
        save();print(json.dumps({'step': label, 'exit_code': result.returncode}), flush=True)
        if result.returncode:
            raise RuntimeError(label + ' failed; see ' + str(log))

    save()
    try:
        inherited={key:value for key,value in os.environ.items() if value and
                   key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
        report['cleared_inherited_admission_controls']=inherited
        for key in list(os.environ):
            if key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_')):os.environ.pop(key)
        if sha(args.tu15_xbe)!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
            raise RuntimeError('Wrong original TU1.5 component reference')
        report['component_reference_image']={'path':str(args.tu15_xbe.resolve()),'sha256':sha(args.tu15_xbe)}
        for attempt in range(3):
            before_tests = test_code_hashes(root)
            before_auxiliary=auxiliary_source_hashes(root)
            try:
                pins = freeze_source(root, source)
                freeze_test_code(root, source)
                if test_code_hashes(source) != before_tests or test_code_hashes(root) != before_tests:
                    raise RuntimeError('Test code changed while freezing')
                if auxiliary_source_hashes(source)!=before_auxiliary or auxiliary_source_hashes(root)!=before_auxiliary:
                    raise RuntimeError('Auxiliary source changed while freezing')
                break
            except RuntimeError as error:
                if attempt == 2 or ('changed while freezing' not in str(error)):
                    raise
                shutil.rmtree(source)
                print(json.dumps({'freeze_retry': attempt+1, 'reason': str(error)}), flush=True)
        (output / 'source-hashes.json').write_text(json.dumps({**pins, **before_tests,**before_auxiliary}, indent=2, sort_keys=True) + '\n')
        report['source_inventory_sha256'] = sha(output / 'source-hashes.json')
        selection = json.loads((root / 'gameplay-runtime-selection.json').read_text())
        maps = root / selection['cache'] / 'halo2/h2_maps_win64_dx11'
        report['selected_runtime_at_freeze'] = selection['runtime']
        report['mcc_maps'] = str(maps)
        report['map_sha256'] = {name: sha(maps / (name + '.map')) for name in (args.maps or ['coagulation', 'zanzibar'])}
        dependency, inputs = prepare_peer_source(root / 'build-perf-4k60/_seed/halo2_datachannel-src', scratch)
        report['dependency_committed_inputs'] = inputs;save()
        command = [sys.executable, '-B', str(root / 'tools/run_tests.py'), '--build-worker',
            '--source', str(source), '--output', str(output / 'cmake'), '--jobs', str(args.jobs),
            '--target', 'native_tu15_prt_helpers_tests', '--target', 'native_bsp_leaf_tests',
            '--target', 'native_tu15_prt_sample_tests', '--filter', TEST_FILTER,
            '--target', 'native_tu15_prt_cache_tests', '--target', 'native_tu15_prt_cache_build_tests',
            '--target', 'native_tu15_prt_update_tests', '--target', 'native_tu15_model_lighting_leaves_tests',
            '--target', 'native_tu15_model_light_cache_tests',
            '--target', 'native_tu15_model_vector_length_tests',
            '--target', 'native_tu15_async_promote_tests',
            '--target', 'native_record_search_tests',
            '--target', 'native_record_lookup_tests',
            '--target', 'native_resource_record_search_tests',
            '--target', 'native_resource_record_count_tests',
            '--target', 'native_resource_touch_tests',
            '--target', 'native_resource_maintenance_tests',
            '--target', 'native_resource_page_tests',
            '--target', 'native_animation_cache_io_tests',
            '--target', 'native_animation_resource_read_tests',
            '--target', 'native_animation_resource_wait_tests',
            '--target', 'native_resource_busy_tests',
            '--target', 'native_halfword_bit_query_tests',
            '--target', 'native_mopp_aabb_abi_tests',
            '--target', 'native_mopp_ray_data_tests',
            '--target', 'native_mopp_ray_integer_frame_tests',
            '--target', 'native_mopp_ray_scale_frame_tests',
            '--target', 'native_mopp_ray_primitive_key_frame_tests',
            '--target', 'native_mopp_ray_plane_frame_tests',
            '--target', 'native_mopp_ray_branch_frame_tests',
            '--target', 'native_mopp_ray_crossing_frame_tests',
            '--target', 'native_mopp_ray_recursion_frame_tests',
            '--target', 'native_mopp_ray_resume_frame_tests',
            '--target', 'native_mopp_ray_leaf_frame_tests',
            '--target', 'native_mopp_ray_slab_frame_tests',
            '--target', 'native_instanced_ray_callback_frame_tests',
            '--target', 'native_instanced_ray_child_tests',
            '--target', 'native_instruction_cursor_tests',
            '--target', 'native_cohort_test_filter_tests',
            '--target', 'native_polygon_support_tests',
            '--target', 'native_polygon_features_tests',
            '--target', 'native_camera_instance_admission_tests',
            '--target', 'native_camera_instance_bitmap_tests',
            '--target', 'native_mopp_ray_diagnostic_frame_tests',
            '--target', 'native_mopp_ray_split_controller_tests',
            '--target', 'native_mopp_ray_vector_store_frame_tests',
            '--target', 'native_raw_x87_scope_tests',
            '--target', 'native_retained_observer_fp_tests',
            '--target', 'native_node_resource_tests',
            '--target', 'native_node_descriptor_lookup_tests',
            '--target', 'native_vehicle_choice_data_tests',
            '--target', 'native_object_node_hierarchy_tests',
            '--target', 'native_object_node_rebuild_tests',
            '--target', 'native_object_animation_delivery_tests',
            '--target', 'native_object_prediction_copy_tests',
            '--target', 'native_resource_composition_tests',
            '--target', 'native_sphere_frustum_tests',
            '--target', 'native_simplex_direction_tests',
            '--target', 'native_simplex_candidate_tests',
            '--target', 'native_simplex_finalize_tests',
            '--target', 'native_simplex_initialize_tests',
            '--target', 'native_simplex_projection_tests',
            '--target', 'native_simplex_iteration_tests',
            '--target', 'native_tu15_async_queue_data_tests',
            '--target', 'native_tu15_async_promotion_data_tests',
            '--target', 'native_node_resource_lifecycle_tests',
            '--target', 'native_effect_datum_allocate_tests',
            '--target', 'native_effect_transform_product_tests',
            '--target', 'native_model_default_pose_tests',
            '--target', 'native_bsp_surface_2d_tests',
            '--target', 'native_bsp_surface_polygon_tests',
            '--target', 'native_bsp_leaf_surface_tests',
            '--target', 'native_bsp_segment_tests',
            '--target', 'native_bsp_segment_setup_tests',
            '--target', 'native_physical_epoch_tests',
            '--target', 'native_datum_delete_tests',
            '--target', 'native_datum_specific_tests',
            '--target', 'native_page_window_tests',
            '--target', 'native_page_retire_tests',
            '--target', 'native_page_allocate_data_tests',
            '--target', 'native_page_allocate_abi_tests',
            '--target', 'native_stack_probe_tests',
            '--target', 'native_tu15_tick_conversion_tests',
            '--target', 'native_millisecond_clock_tests',
            '--target', 'native_kernel_yield_tests',
            '--target', 'native_object_scan_tests',
            '--target', 'native_lazy_heap_data_tests',
            '--target', 'native_physical_allocation_tests', '--target', 'native_effect_slot_scan_tests',
            '--target', 'native_physical_callbacks_tests',
            '--retain-report', 'native-tu15-prt-helpers.json', '--retain-report', 'native-tu15-prt-sample.json',
            '--retain-report', 'native-matrix-alignment.json',
            '--retain-report', 'native-array-abi.json', '--retain-report', 'native-broadphase-remove.json',
            '--retain-report', 'native-broadphase-add-leaves.json', '--retain-report', 'native-broadphase-add-object.json',
            '--retain-report', 'native-geometry-request-abi.json', '--retain-report', 'native-tu15-prt-cache.json',
            '--retain-report', 'native-geometry-publish-abi.json',
            '--retain-report', 'native-tu15-prt-cache-build.json', '--retain-report', 'native-tu15-prt-update.json',
            '--retain-report', 'native-tu15-model-lighting-leaves.json',
            '--retain-report', 'native-tu15-model-light-cache.json',
            '--retain-report', 'native-tu15-model-vector-length.json',
            '--retain-report', 'native-tu15-async-promotion.json',
            '--retain-report', 'native-tu15-record-search.json',
            '--retain-report', 'native-tu15-record-lookup.json',
            '--retain-report', 'native-tu15-resource-record-search.json',
            '--retain-report', 'native-tu15-resource-record-count.json',
            '--retain-report', 'native-tu15-resource-touch.json',
            '--retain-report', 'native-tu15-resource-maintenance.json',
            '--retain-report', 'native-tu15-resource-page.json',
            '--retain-report', 'source-animation-cache-io-tu15.json',
            '--retain-report', 'native-tu15-animation-resource-read.json',
            '--retain-report', 'native-tu15-animation-resource-wait.json',
            '--retain-report', 'native-resource-busy.json',
            '--retain-report', 'native-tu15-halfword-bit-query.json',
            '--retain-report', 'native-tu15-mopp-aabb-abi.json',
            '--retain-report', 'native-mopp-aabb-mappack.json',
            '--retain-report', 'native-mopp-ray-data/report.json',
            '--retain-report', 'native-mopp-ray-integer-frame/report.json',
            '--retain-report', 'native-mopp-ray-scale-frame/report.json',
            '--retain-report', 'native-mopp-ray-primitive-key-frame/report.json',
            '--retain-report', 'native-mopp-ray-plane-frame/report.json',
            '--retain-report', 'native-mopp-ray-branch-frame/report.json',
            '--retain-report', 'native-mopp-ray-crossing-frame/report.json',
            '--retain-report', 'native-mopp-ray-recursion-frame/report.json',
            '--retain-report', 'native-mopp-ray-resume-frame/report.json',
            '--retain-report', 'native-mopp-ray-leaf-frame/report.json',
            '--retain-report', 'native-mopp-ray-slab-frame/report.json',
            '--retain-report', 'native-instanced-ray-callback-frame/report.json',
            '--retain-report', 'native-instanced-ray-child/report.json',
            '--retain-report', 'native-instanced-ray-child-release/report.json',
            '--retain-report', 'native-instanced-ray-child-values.json',
            '--retain-report', 'native-instruction-cursor/report.json',
            '--retain-report', 'native-cohort-test-filter.json',
            '--retain-report', 'native-polygon-support/report.json',
            '--retain-report', 'native-polygon-support-release/report.json',
            '--retain-report', 'native-polygon-support-generation.json',
            '--retain-report', 'native-polygon-support-mappack/report.json',
            '--retain-report', 'native-polygon-support-mappack-release/report.json',
            '--retain-report', 'native-polygon-features.json',
            '--retain-report', 'native-polygon-features-generation.json',
            '--retain-report', 'native-tu15-camera-instance-admission/report.json',
            '--retain-report', 'native-tu15-camera-instance-admission-release/report.json',
            '--retain-report', 'native-tu15-camera-instance-admission-generation.json',
            '--retain-report', 'native-tu15-camera-instance-bitmap/report.json',
            '--retain-report', 'native-tu15-camera-instance-bitmap-release/report.json',
            '--retain-report', 'native-tu15-camera-instance-bitmap-generation.json',
            '--retain-report', 'native-object-node-live-admission.json',
            '--retain-report', 'native-mopp-ray-diagnostic-frame/report.json',
            '--retain-report', 'native-mopp-ray-split-controller/report.json',
            '--retain-report', 'native-mopp-ray-vector-store-frame/report.json',
            '--retain-report', 'native-tu15-raw-x87-scope.json',
            '--retain-report', 'native-tu15-retained-observer-fp.json',
            '--retain-report', 'native-tu15-node-resource.json',
            '--retain-report', 'native-tu15-node-descriptor-lookup.json',
            '--retain-report', 'native-tu15-vehicle-choice-data.json',
            '--retain-report', 'native-object-node-hierarchy.json',
            '--retain-report', 'native-object-node-rebuild.json',
            '--retain-report', 'native-object-animation-delivery.json',
            '--retain-report', 'native-object-prediction-copy.json',
            '--retain-report', 'native-tu15-resource-composition.json',
            '--retain-report', 'native-tu15-sphere-frustum.json',
            '--retain-report', 'native-tu15-sphere-frustum-generation.json',
            '--retain-report', 'native-simplex-direction.json',
            '--retain-report', 'native-simplex-candidate.json',
            '--retain-report', 'native-simplex-finalize.json',
            '--retain-report', 'native-simplex-initialize.json',
            '--retain-report', 'native-simplex-projection.json',
            '--retain-report', 'native-simplex-iteration.json',
            '--retain-report', 'native-tu15-async-queue-data.json',
            '--retain-report', 'native-tu15-async-queue-source-routes.json',
            '--retain-report', 'native-tu15-async-queue-values.json',
            '--retain-report', 'native-tu15-async-promotion-data.json',
            '--retain-report', 'native-tu15-async-promotion-values.json',
            '--retain-report', 'native-tu15-node-resource-lifecycle.json',
            '--retain-report', 'native-tu15-effect-datum-allocate.json',
            '--retain-report', 'native-tu15-effect-transform-product.json',
            '--retain-report', 'native-tu15-model-default-pose.json',
            '--retain-report', 'native-tu15-bsp-surface-2d.json',
            '--retain-report', 'native-tu15-bsp-surface-polygon.json',
            '--retain-report', 'native-tu15-bsp-leaf-surface.json',
            '--retain-report', 'native-tu15-bsp-segment.json',
            '--retain-report', 'native-tu15-bsp-segment-setup.json',
            '--retain-report', 'native-physical-epoch-abi.json',
            '--retain-report', 'native-datum-delete-abi.json',
            '--retain-report', 'native-tu15-datum-initialize.json',
            '--retain-report', 'native-tu15-datum-specific.json',
            '--retain-report', 'native-page-window-abi.json',
            '--retain-report', 'native-page-retire-abi.json',
            '--retain-report', 'native-page-allocation-data.json',
            '--retain-report', 'native-page-allocation-abi.json',
            '--retain-report', 'native-stack-probe-abi.json',
            '--retain-report', 'native-tu15-tick-conversion.json',
            '--retain-report', 'native-tu15-millisecond-clock.json',
            '--retain-report', 'native-tu15-kernel-yield.json',
            '--retain-report', 'native-tu15-object-scan.json',
            '--retain-report', 'native-lazy-heap-data.json',
            '--retain-report', 'native-physical-allocation-abi.json', '--retain-report', 'native-tu15-effect-slot-scan.json',
            '--retain-report', 'native-physical-callbacks-abi.json',
            '--retain-report', 'native-pool-allocator-abi.json',
            '--retain-report', 'native-async-order.json', '--retain-report', 'native-async-insert.json',
            '--retain-report', 'native-instruction-shapes.json',
            '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + str(dependency),
            '--cmake-arg=-DFETCHCONTENT_FULLY_DISCONNECTED=ON',
            '--cmake-arg=-DHALO2_TEST_ASSET_SOURCE_ROOT=' + str(root),
            '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE='+str(args.tu15_xbe.resolve()),
            '--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=OFF']
        if args.execution_accounting:
            command+=['--target','native_execution_accounting_aliases','--target','native_execution_accounting_tests',
                      '--retain-report','native-execution-coverage.json','--cmake-arg=-DHALO2_NATIVE_EXECUTION_ACCOUNTING=ON']
        environment = dict(os.environ, CARGO_NET_OFFLINE='true')
        # Qualify the frozen component/oracle cohort before compiling the full
        # engine. A rejected boundary fixture must not consume an engine build.
        report['validation_sequence']=['components','engine-build'];save()
        run('components', command, environment)
        if '***Skipped' in (output/'cmake/tests.log').read_text():
            raise RuntimeError('A required component oracle was skipped; see CMake test log')
        engine_command=[sys.executable,'-B',str(root/'tools/run_tests.py'),'--build-worker',
            '--source',str(source),'--output',str(output/'engine-build'),'--jobs',str(args.jobs),
            '--target','halo2_native','--build-only',
            *(value for value in command if value.startswith('--cmake-arg='))]
        run('engine-build',engine_command,environment)
        runtime = build / 'halo2_native'
        report['runtime_sha256'] = sha(runtime);report['runtime_bytes'] = runtime.stat().st_size
        if args.execution_accounting and not args.build_and_components_only:
            # Use the frozen capture helper without modifying runtime service
            # callbacks or native admission. Each run owns a distinct report;
            # captures and raw counters remain in the enclosing scratch owner.
            spec=importlib.util.spec_from_file_location('frozen_migration_parity',source/'tools/perf_4k60/parity.py')
            capture_helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture_helper)
            coverage=json.loads((output/'cmake/evidence/native-execution-coverage.json').read_text())
            if coverage['canonical_source_drift']:raise RuntimeError('Accounting aliases have source drift')
            allowed={(row['source'],row['canonical_sha256'],row['input_sha256']) for row in coverage['aliases']}
            report['accounting_runs']=[]
            for name in (args.maps or ['coagulation','zanzibar']):
                for players in (1,4):
                    for moving in (False,True):
                        label=name+'-'+str(players)+'p-'+('moving' if moving else 'static')
                        raw=scratch/(label+'-execution.json')
                        raw.unlink(missing_ok=True)
                        capture=capture_helper.capture(runtime,'accounting-'+label,players,90,10,80,
                            {'HALO2_NATIVE_EXECUTION_REPORT':str(raw)},scratch=scratch,maps=maps,
                            map_name=name,motion=moving)
                        capture.pop('_payload',None)
                        item={'label':label,'capture':capture,'native_closure_proved':False}
                        script=scratch/('accounting-'+label+'.tsv')
                        item.update(input_script_sha256=sha(script),input_script_bytes=script.stat().st_size)
                        report['accounting_runs'].append(item);save()
                        if raw.is_file():
                            if raw.stat().st_size>1_048_576:raise RuntimeError('Accounting report exceeds retention bound')
                            data=json.loads(raw.read_text())
                            for unit in data['units']:
                                if (unit[0],unit[1],unit[2]) not in allowed:
                                    raise RuntimeError('Runtime counter identity has no authenticated alias receipt')
                                if pins.get(unit[0])!=unit[1]:raise RuntimeError('Runtime canonical counter identity differs from frozen source')
                            retained=output/(label+'-execution.json');retain_accounting_metadata(raw,retained)
                            item.update(runtime_report=str(retained),runtime_report_sha256=sha(retained))
                            summary=scratch/(label+'-execution-summary.json')
                            run('accounting-analysis-'+label,[sys.executable,'-B',str(source/'tools/native_execution_report.py'),
                                '--runtime',str(raw),'--output',str(summary)],environment)
                            analyzed=json.loads(summary.read_text())
                            if analyzed['totals'].get('active',0):
                                raise RuntimeError('Completed process has active generated scopes: '+label)
                            retained_summary=output/summary.name;retain_accounting_metadata(summary,retained_summary)
                            item.update(analysis=str(retained_summary),analysis_sha256=sha(retained_summary));save()
                        else:raise RuntimeError('Runtime did not produce an execution report for '+label)
                        if not capture['valid']:raise RuntimeError('Accounting workload failed: '+label)
            report['bounded_runtime_accounting_recorded']=True
        elif not args.build_and_components_only:
            controls = ','.join(name+'='+value for name,value in CONTROLS.items())
            parity = source / 'tools/perf_4k60/parity.py'
            cases = args.cases or ['4:90:0:0', '4:150:10:110', '2:150:10:110', '1:150:10:110']
            for name in (args.maps or ['coagulation', 'zanzibar']):
                env = dict(environment, H2_MAP=name, H2_REPEAT_CANDIDATE='1')
                run('parity-'+name, [sys.executable, '-B', str(parity), str(output / ('parity-'+name+'.json')),
                    controls+'@'+str(runtime), str(runtime), *cases, '--maps', str(maps)], env)
                for players in (1,4):
                    for mode in ('native', 'reference'):
                        label = name+'-'+str(players)+'p-'+mode
                        env = dict(environment, H2_MAP=name, H2_MAPS=str(maps), H2_CPUMON='1',
                                   H2_GPU_TIMINGS='1', SDL_VIDEODRIVER='x11')
                        if mode == 'reference':
                            env.update(CONTROLS)
                        run('profile-'+label, ['xvfb-run', '--auto-servernum', '--server-args',
                            '-screen 0 1280x720x24', str(source / 'tools/perf_4k60/profile_run.sh'),
                            str(runtime), str(output / 'timings'), label, str(players), str(args.frames), ''], env)
        if (source_hashes(source) != pins or test_code_hashes(source) != before_tests or
            auxiliary_source_hashes(source)!=before_auxiliary):
            raise RuntimeError('Frozen source changed during validation')
        report.update(passed=True, status='passed',
            selected_map_frame_parity_recorded=not args.build_and_components_only and not args.execution_accounting,
            selected_map_timing_recorded=not args.build_and_components_only and not args.execution_accounting)
        save();return 0
    except Exception as error:
        report.update(status='failed', error=str(error));save()
        print(json.dumps({'status': 'failed', 'error': str(error)}), flush=True)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
