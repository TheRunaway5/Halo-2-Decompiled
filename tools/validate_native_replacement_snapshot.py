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
import signal
import subprocess
import sys
import time

from build_launcher_engine import freeze_source, source_hashes
from collect_native_production_closure import prepare_peer_source
from native_cohort_test_filter import expand_test_names, flatten_test_filter
from native_metadata import (retain_bytes as retain_metadata_bytes,retain_file as retain_metadata_file,
    MetadataBudgetError,check_budget as check_metadata_budget,load as load_metadata)
from native_component_evidence_storage import compact_component_evidence

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
    'HALO2_REFERENCE_INSTANCED_RAY_CALLBACKS': '1',
    'HALO2_REFERENCE_AFFINE_INVERSE': '1',
    'HALO2_REFERENCE_MODEL_MARKER_LOOKUP': '1',
    'HALO2_REFERENCE_OBJECT_PARENT_MODE': '1',
    'HALO2_REFERENCE_MARKER_OWNER': '1',
    'HALO2_REFERENCE_SOLVER_HULL_APPEND': '1',
    'HALO2_REFERENCE_ZERO_SHAPE_SUPPORT': '1',
    'HALO2_REFERENCE_BOX_SHAPE_SUPPORT': '1',
    'HALO2_REFERENCE_PACKED_ROTATION': '1',
    'HALO2_REFERENCE_CODEC_TRANSLATION': '1',
    'HALO2_REFERENCE_CODEC_SCALE': '1',
    'HALO2_REFERENCE_CODEC_ROTATION_INTERPOLATION': '1',
    'HALO2_REFERENCE_CODEC_TRANSLATION_INTERPOLATION': '1',
    'HALO2_REFERENCE_CODEC_SCALE_INTERPOLATION': '1',
    'HALO2_REFERENCE_CODEC_COPY_ROWS': '1',
    'HALO2_REFERENCE_OBJECT_BODY': '1',
    'HALO2_REFERENCE_OBJECT_TYPE_QUERY': '1',
    'HALO2_REFERENCE_PROJECTILE_FUNCTION_QUERY': '1',
    'HALO2_REFERENCE_OBJECT_DEFINITION_MODE_QUERY': '1',
    'HALO2_REFERENCE_OBJECT_RECORD_RATIO': '1',
    'HALO2_REFERENCE_OBJECT_TABLE_RATIO': '1',
    'HALO2_REFERENCE_OBJECT_TABLE_RATIO_MODE': '1',
    'HALO2_REFERENCE_MAP_WORLD_PROPERTY_CALLER': '1',
    'HALO2_REFERENCE_MAP_WORLD_PROPERTY_QUERY': '1',
    'HALO2_REFERENCE_DEVICE_PROPERTY_QUERY': '1',
    'HALO2_REFERENCE_CONTACT_PREDICATE': '1',
    'HALO2_REFERENCE_VEHICLE_OBJECT_LOOKUP': '1',
    'HALO2_REFERENCE_VEHICLE_RENDER_ROOT': '1',
    'HALO2_REFERENCE_VEHICLE_EFFECT_RATIO': '1',
    'HALO2_REFERENCE_FLIGHT_PROPERTY_QUERY': '1',
    'HALO2_REFERENCE_DEATH_CYCLE_FLIGHT_PROPERTY_QUERY': '1',
    'HALO2_REFERENCE_SOLVER_HULL_SUPPORT_PAIR': '1',
    'HALO2_REFERENCE_SOLVER_HULL_EDGE': '1',
    'HALO2_REFERENCE_SOLVER_HULL_INITIALIZE': '1',
    'HALO2_REFERENCE_SOLVER_HULL_LINE': '1',
    'HALO2_REFERENCE_SOLVER_HULL_SAMPLE': '1',
    'HALO2_REFERENCE_SOLVER_HULL_MINIMUM': '1',
    'HALO2_REFERENCE_SOLVER_HULL_CLEAR': '1',
    'HALO2_REFERENCE_SOLVER_HULL_CLIP': '1',
    'HALO2_REFERENCE_ARENA_INITIALIZE': '1',
    'HALO2_REFERENCE_SPHERE_RAY_CAST': '1',
    'HALO2_REFERENCE_DIAGNOSTIC_STREAM': '1',
    'HALO2_REFERENCE_EFFECT_REFERENCE_ELEMENT': '1',
    'HALO2_REFERENCE_DIAGNOSTIC_LOGGER': '1',
    'HALO2_REFERENCE_POLYGON_SUPPORT': '1',
    'HALO2_REFERENCE_POLYGON_FEATURES': '1',
    'HALO2_REFERENCE_CAMERA_INSTANCE_ADMISSION': '1',
    'HALO2_REFERENCE_CAMERA_INSTANCE_BITMAP': '1',
    'HALO2_REFERENCE_CAMERA_INSTANCE_QUERY': '1',
    'HALO2_REFERENCE_CAMERA_COLLISION_PUBLICATION': '1',
    'HALO2_REFERENCE_SEGMENT_SPHERE': '1',
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
    'HALO2_REFERENCE_NODE_PROGRESS_QUERY': '1',
    'HALO2_REFERENCE_NODE_FUNCTION_PROGRESS': '1',
    'HALO2_REFERENCE_OBJECT_FUNCTION_QUERY': '1',
    'HALO2_REFERENCE_OBJECT_FUNCTION_SCALAR': '1',
    'HALO2_REFERENCE_VEHICLE_CHOICE': '1',
    'HALO2_REFERENCE_SPHERE_FRUSTUM': '1',
    'HALO2_REFERENCE_CLUSTER_FRUSTUM': '1',
    'HALO2_REFERENCE_BSP_QUEUE_FIND': '1',
    'HALO2_REFERENCE_FOG_MEMBERSHIP': '1',
    'HALO2_REFERENCE_VISIBILITY_ASSOCIATION': '1',
    'HALO2_REFERENCE_GEOMETRY_VISIBILITY': '1',
    'HALO2_REFERENCE_BSP_ADMISSION': '1',
    'HALO2_REFERENCE_SIMPLEX_TRIANGLE': '1',
    'HALO2_REFERENCE_SIMPLEX_FACE_REFINE': '1',
    'HALO2_REFERENCE_SIMPLEX_FACE_SELECT': '1',
    'HALO2_REFERENCE_CLOSEST_SEGMENTS': '1',
    'HALO2_REFERENCE_SIMPLEX_PAIRED': '1',
    'HALO2_REFERENCE_SIMPLEX_BOUNDARY': '1',
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
TEST_FILTER_PATTERN = ('^(native_replacement_cohort_filter|native_metadata_storage|native_metadata_retention_replay|native_component_evidence_storage(_replay)?|native_bsp_leaves|native_matrix_leaf_faults|native_bsp_cluster_flood|'
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
               'original_tu15_native_mopp_ray_setup_frame(_release)?|'
               'original_tu15_native_sphere_ray_cast(_release)?|'
               'original_tu15_native_sphere_ray_cast_routes(_release)?|'
               'original_tu15_native_sphere_ray_cast_generation|'
               'native_diagnostic_stream_binding|'
               'original_tu15_native_diagnostic_stream(_release|_generation)?|'
               'original_tu15_native_effect_reference_element(_release|_generation)?|'
               'native_diagnostic_logger_world_binding|original_tu15_native_diagnostic_logger_owner(_release)?|original_tu15_native_diagnostic_logger_world|'
               'original_tu15_native_mopp_ray_sphere_query(_release)?|'
               'original_tu15_native_mopp_diagnostic_return|'
               'original_tu15_native_mopp_ray_primitive_key_frame(_release)?|'
               'original_tu15_native_mopp_ray_(plane|branch|crossing|recursion|resume|leaf|slab|diagnostic|vector_store)_frame(_release)?|'
               'original_tu15_native_mopp_ray_split_controller(_release)?|'
               'original_tu15_native_instanced_ray_callback_frame(_release)?|'
               'native_instanced_ray_child_bindings|original_tu15_native_instanced_ray_child(_release)?|native_instanced_ray_child_public_values|'
               'original_tu15_native_instanced_ray_callbacks(_generation|_release)?|'
               'original_tu15_native_affine_inverse(_release|_mappack|_mappack_release|_generation|_motion_c1|_animation|_animation_release)?|'
               'original_tu15_native_model_marker_lookup(_release|_generation|_sandbox)?|'
               'original_tu15_native_object_parent_mode(_release|_generation)?|'
               'original_tu15_native_marker_owner(_release|_generation)?|'
               'native_tu15_instruction_cursor|'
               'native_tu15_required_owner_policy(_release)?|'
               'original_tu15_native_required_abi_fp(_release)?|'
               'original_tu15_native_camera_instance_admission(_release|_generation)?|'
               'original_tu15_native_camera_instance_bitmap(_release|_generation)?|native_object_node_live_admission|'
               'original_tu15_native_polygon_support(_release|_generation)?|original_tu15_native_polygon_features(_generation)?|'
               'original_mappack_native_polygon_support(_release)?|'
               'original_tu15_native_raw_x87_scope(_release)?|'
               'native_tu15_retained_observer_fp(_release)?|'
               'native_tu15_host_fp_io(_release)?|'
               'native_world_observation_generator|'
               'native_polygon_prepared_owners|original_tu15_native_camera_collision_publication(_release|_generation)?|original_tu15_native_segment_sphere(_release|_generation)?|original_tu15_native_camera_instance_query(_release|_generation|_values)?|'
               'original_tu15_native_node_resource|'
               'original_tu15_native_node_descriptor_lookup(_release)?|'
               'original_tu15_native_node_progress_query_(frame|source)|native_node_progress_query_(binding|local_caller)|'
               'native_d3950_world_admission|'
               'native_object_function_query_(binding|public)|'
               'original_tu15_native_object_function_scalar_frame|native_object_function_scalar_producer|'
               'original_tu15_native_indexed_datum_gate_frame|original_mappack11_native_indexed_datum_gate_frame|native_indexed_datum_gate_source|native_indexed_datum_gate_linked|'
               'original_tu15_native_particle_type_filter_frame|native_particle_type_filter_source|'
               'original_tu15_native_particle_envelope_frame|native_particle_envelope_source|'
               'original_tu15_native_particle_record_frame|native_particle_record_source|'
               'original_tu15_native_object_ratio_caller_frame|native_object_ratio_caller_(source|world|inherited)|'
               'original_tu15_native_vehicle_choice_data(_release)?|'
               'original_tu15_native_object_node_hierarchy|'
               'original_tu15_native_object_node_rebuild|'
               'original_tu15_native_object_animation_delivery|'
               'original_tu15_native_object_prediction_copy|'
               'original_tu15_native_resource_composition(_release)?|'
               'original_tu15_native_sphere_frustum(_release|_generation)?|'
               'native_cluster_frustum_binding|original_tu15_native_cluster_frustum(_values|_generation|_release)?|'
               'native_bsp_queue_find_binding|original_tu15_native_bsp_queue_find(_values|_generation|_release)?|original_tu15_native_simplex_triangle_generation|'
               'native_fog_membership_binding|original_tu15_native_fog_membership(_values|_generation|_release)?|'
               'native_visibility_association_binding|original_tu15_native_visibility_association(_values|_generation|_release)?|'
               'native_geometry_visibility_binding|original_tu15_native_geometry_visibility(_values|_generation|_release)?|'
               'native_bsp_admission_binding|original_tu15_native_bsp_admission(_values|_generation|_release)?|'
               'original_tu15_native_simplex_face_refine(_routes|_value|_generation)?|'
               'original_tu15_native_simplex_face_select(_routes|_generation)?|'
               'original_tu15_native_closest_segments_frame|'
               'original_tu15_native_closest_segments_(routes|generation)|'
               'original_tu15_native_simplex_paired(_routes|_generation)?|'
               'original_tu15_native_simplex_boundary(_routes|_generation)?|'
               'original_tu15_native_solver_hull_context|'
               'original_tu15_native_solver_hull_context_value_(declared|genuine)|'
               'original_tu15_native_solver_hull_append_(frame|value)|'
               'original_tu15_native_solver_hull_append_(routes|generation)|'
               'original_tu15_native_solver_hull_support_pair|'
               'original_tu15_native_solver_hull_support_pair_(routes|value|generation)|'
               'original_tu15_native_solver_hull_edge_frame|'
               'original_tu15_native_solver_hull_edge_value|'
               'original_tu15_native_solver_hull_edge_legacy|'
               'original_tu15_native_solver_hull_edge_routes|'
               'original_tu15_native_solver_hull_edge_generation|'
               'original_tu15_native_solver_hull_initialize_frame|'
               'original_tu15_native_solver_hull_initialize_value|'
               'original_tu15_native_solver_hull_initialize_routes|'
               'original_tu15_native_solver_hull_initialize_generation|'
               'original_tu15_native_solver_hull_line_frame|'
               'original_tu15_native_solver_hull_line_value|'
               'original_tu15_native_solver_hull_sample_frame|'
               'original_tu15_native_solver_hull_sample_value|'
               'original_tu15_native_solver_hull_minimum_frame|'
               'original_tu15_native_solver_hull_minimum_value|'
               'original_tu15_native_solver_hull_minimum_routes|'
               'original_tu15_native_solver_hull_minimum_generation|'
               'original_tu15_native_solver_hull_basis_frame|'
               'original_tu15_native_solver_hull_cleanup_frame|'
               'original_tu15_native_solver_hull_cleanup_source|'
               'original_tu15_native_solver_hull_clear_frame|'
               'original_tu15_native_solver_hull_clear_value|'
               'original_tu15_native_solver_hull_clear_routes|'
               'original_tu15_native_solver_hull_clip_frame|'
               'original_tu15_native_solver_hull_clip_value|'
               'original_tu15_native_solver_hull_clip_routes|'
               'original_tu15_native_simplex_hull_owner_frame|'
               'original_tu15_native_simplex_owner_frame|'
               'original_tu15_native_solver_hull_line_routes|'
               'original_tu15_native_solver_hull_line_generation|'
               'original_tu15_native_solver_hull_sample_routes|'
               'original_tu15_native_solver_hull_sample_generation|'
               'original_tu15_native_codec_translation(_source|_release)?|'
               'original_tu15_native_codec_scale(_source|_release)?|'
               'original_tu15_native_codec_rotation_interpolation(_source|_release)?|'
               'original_tu15_native_codec_translation_interpolation(_source|_release)?|'
               'original_tu15_native_codec_scale_interpolation(_source|_release)?|'
               'original_tu15_native_codec_row_copy(_release)?|'
               'original_tu15_native_codec_scale_row(_release)?|'
               'original_tu15_native_codec_copy_rows_source|'
               'original_tu15_native_zero_shape_support(_routes|_value|_generation)?|'
               'original_tu15_native_box_shape_support(_routes|_value|_generation)?|'
               'original_tu15_native_object_body_lookup_frame(_release)?|'
               'original_tu15_native_object_type_query_frame(_release)?|'
               'native_tu15_object_type_query_source_frame(_release)?|'
               'native_tu15_object_type_query_owner(_release)?|'
               'native_tu15_object_type_query_candidates|'
               'native_tu15_object_type_query_candidate_routes(_release)?|'
               'native_tu15_vehicle_object_lookup_(raw(_release)?|generation|public(_controls|_release)?)|'
               'original_tu15_native_render_ownership|'
               'original_tu15_native_projectile_function_query_frame(_release)?|'
               'original_tu15_native_projectile_function_query_source|'
               'native_projectile_function_query_(binding|public)|'
               'original_tu15_native_object_definition_mode_query_(frame|source)|native_object_definition_mode_query_(binding|public)|'
               'original_tu15_native_object_record_ratio_(frame|source)|native_object_record_ratio_(binding|public)|'
               'original_tu15_native_object_table_ratio_(frame|source)|native_object_table_ratio_(binding|public)|'
               'original_tu15_native_map_world_property_(query|caller)_(frame|source)|'
               'native_map_world_property_queries_(binding|public)|'
               'original_tu15_native_device_property_query_(frame|source)|native_device_property_query_(binding|public)|'
               'original_tu15_native_contact_predicate_(raw|source|generation)|'
               'original_tu15_native_vehicle_effect_ratio_(raw|source)|'
               'original_tu15_native_flight_property_query_(frame|source)|'
               'original_tu15_native_death_cycle_flight_query_source|'
               'native_flight_property_query_(binding|public)|'
               'native_death_cycle_flight_query_(binding|public)|'
               'native_tu15_object_body_lookup_value(_release)?|'
               'native_tu15_object_body_lookup_source_frame(_release)?|'
               'native_tu15_object_body_lookup_routes(_release)?|'
               'native_tu15_object_body_lookup_generation|'
               'native_tu15_object_material_lookup_routes(_release)?|'
               'native_tu15_object_material_lookup_generation|'
               'native_tu15_object_lookup_copies_(routes(_release)?|generation)|'
               'native_tu15_object_lookup_canonical_(routes(_release)?|generation)|'
               'native_tu15_object_lookup_format(_release)?|'
               'native_tu15_object_body_observer_branch(_release)?|'
               'native_tu15_object_body_foreign_continuation(_release)?|'
               'native_arena_initialize_binding|'
               'original_tu15_native_arena_initialize(_release|_source|_source_release|_generation|_schedule|_transport|_bootstrap)?|'
               'native_arena_world_initialize_binding|'
               'original_tu15_native_arena_world_initialize_source|'
               'original_tu15_native_arena_world_initialize_source_release|'
               'original_tu15_native_arena_world_initialize_publication|'
               'original_tu15_native_packed_rotation(_policy|_value|_routes|_generation|_release)?|'
               'original_tu15_native_simplex_direction|original_tu15_native_simplex_candidate|original_tu15_native_simplex_finalize|original_tu15_native_simplex_initialize|original_tu15_native_simplex_projection|original_tu15_native_simplex_iteration|original_tu15_native_simplex_support|original_tu15_native_simplex_triangle_frame|'
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


TEST_CODE_SUFFIXES = {'.cpp', '.hpp', '.h', '.inc', '.py', '.cmake'}


def test_tree(root):
    """Include borrowed test directories when copying a frozen donor."""
    def visit(directory, ancestors):
        resolved=directory.resolve(strict=True)
        if resolved in ancestors:
            raise ValueError('Test source directory cycle: '+str(directory))
        for child in sorted(directory.iterdir()):
            if child.name=='__pycache__':continue
            if child.is_dir():
                yield child
                yield from visit(child,ancestors|{resolved})
            elif child.is_file():yield child
            elif child.is_symlink() and child.suffix in TEST_CODE_SUFFIXES:
                raise ValueError('Borrowed test code is unavailable: '+str(child))
    yield from visit(root/'tests',set())


def test_code_hashes(root):
    return {str(path.relative_to(root)):sha(path)for path in test_tree(root)
            if path.is_file() and path.suffix in TEST_CODE_SUFFIXES}


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
    for original in test_tree(root):
        relative = original.relative_to(root / 'tests')
        target = tests / relative
        if original.is_dir():
            if target.is_symlink():target.unlink()
            target.mkdir(parents=True, exist_ok=True)
        elif original.suffix in TEST_CODE_SUFFIXES:
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.is_symlink():target.unlink()
            if not target.exists():shutil.copy2(original,target)
        elif not target.exists() and not target.is_symlink():
            target.parent.mkdir(parents=True, exist_ok=True)
            target.symlink_to(original.resolve(strict=True))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--test-jobs', type=int, help='Concurrent component tests (defaults to --jobs)')
    parser.add_argument('--map', action='append', dest='maps')
    parser.add_argument('--case', action='append', dest='cases')
    parser.add_argument('--frames', type=int, default=300)
    parser.add_argument('--build-and-components-only', action='store_true')
    parser.add_argument('--engine-first',action='store_true',help='Check the frozen engine build before the long component suite')
    parser.add_argument('--execution-accounting',action='store_true',help='Build diagnostic route counters and collect bounded static/moving workloads; omit performance measurements')
    parser.add_argument('--tu15-xbe',type=Path,required=True,help='Authenticated owned original for physical component oracles')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py for complete source/build/capture cleanup')
    if args.jobs < 1 or (args.test_jobs is not None and args.test_jobs < 1) or args.frames <= 30:
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
        data=(json.dumps(report, indent=2, sort_keys=True) + '\n').encode()
        if len(data)>1_048_576:raise RuntimeError('Cohort report exceeds1MiB retention bound')
        (output / 'report.json').write_bytes(data)

    def retain_accounting_metadata(original,destination):
        if original.stat().st_size>1_048_576:raise RuntimeError('Accounting metadata exceeds1MiB')
        try:receipt=retain_metadata_file(original,destination,output,max_bytes=1_048_576)
        except MetadataBudgetError as error:
            report['metadata_retention_rejected_bytes']=error.retained_bytes;save();raise
        report['retained_metadata_bytes']=receipt['retained_metadata_bytes']
        return receipt

    def run(label, command, env=None):
        step = {'name': label, 'command': command, 'started': time.time(), 'exit_code': None}
        report['steps'].append(step);report['status'] = label;save()
        print(json.dumps({'step': label, 'status': 'running'}), flush=True)
        log = output / (label + '.log')
        with log.open('wb') as stream:
            process=subprocess.Popen(command,cwd=root,env=env,stdout=stream,stderr=subprocess.STDOUT)
            try:
                while True:
                    try:
                        code=process.wait(timeout=30);break
                    except subprocess.TimeoutExpired:
                        print(json.dumps({'step':label,'status':'running',
                            'elapsed_seconds':int(time.time()-step['started']),'log_bytes':log.stat().st_size}),flush=True)
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:process.wait(timeout=2)
                    except subprocess.TimeoutExpired:process.kill();process.wait()
        step.update(exit_code=code, elapsed_seconds=time.time()-step['started'], log=str(log))
        if log.stat().st_size > 8 * 1024 * 1024:
            with log.open('rb') as stream:
                stream.seek(-8 * 1024 * 1024, os.SEEK_END);tail = stream.read()
            log.write_bytes(tail)
        save();print(json.dumps({'step': label, 'exit_code': code}), flush=True)
        if code:
            raise RuntimeError(label + ' failed; see ' + str(log))

    def interrupted(signum, _frame):
        raise SystemExit(128 + signum)

    for signum in (signal.SIGINT, signal.SIGTERM):
        signal.signal(signum, interrupted)
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
        inventory_bytes=(json.dumps({**pins, **before_tests,**before_auxiliary}, indent=2, sort_keys=True) + '\n').encode()
        inventory_storage=retain_metadata_bytes(inventory_bytes,output/'source-hashes.json',output)
        report['source_inventory_sha256']=inventory_storage['raw_sha256']
        report['source_inventory_storage']=inventory_storage
        report['metadata_retention_encoding']='lossless gzip; hashes identify decoded original bytes'
        save()
        print(json.dumps({'status':'snapshot-ready','source_inventory_sha256':inventory_storage['raw_sha256']}),flush=True)
        selection = json.loads((root / 'gameplay-runtime-selection.json').read_text())
        maps = root / selection['cache'] / 'halo2/h2_maps_win64_dx11'
        report['selected_runtime_at_freeze'] = selection['runtime']
        report['mcc_maps'] = str(maps)
        report['map_sha256'] = {name: sha(maps / (name + '.map')) for name in (args.maps or ['coagulation', 'zanzibar'])}
        dependency, inputs = prepare_peer_source(root / 'build-perf-4k60/_seed/halo2_datachannel-src', scratch)
        report['dependency_committed_inputs'] = inputs;save()
        command = [sys.executable, '-B', str(source / 'tools/run_tests.py'), '--build-worker',
            '--source', str(source), '--output', str(output / 'cmake'), '--jobs', str(args.jobs),
            '--test-jobs', str(args.test_jobs or args.jobs),
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
            '--target', 'native_mopp_ray_setup_frame_tests',
            '--target', 'native_sphere_ray_cast_tests',
            '--target', 'native_sphere_ray_cast_routes_tests',
            '--target', 'native_sphere_ray_cast_generation_tests',
            '--target', 'native_diagnostic_stream_tests',
            '--target', 'native_effect_reference_element_tests',
            '--target', 'native_diagnostic_logger_owner_tests',
            '--target', 'native_mopp_ray_sphere_query_tests',
            '--target', 'native_mopp_diagnostic_return_tests',
            '--target', 'native_model_marker_lookup_tests',
            '--target', 'native_object_parent_mode_tests',
            '--target', 'native_required_owner_policy_tests',
            '--target', 'native_marker_owner_tests',
            '--target', 'native_required_abi_fp_tests',
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
            '--target', 'native_instanced_ray_callbacks_tests',
            '--target', 'native_affine_inverse_tests',
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
            '--target', 'native_node_progress_query_tests',
            '--target', 'native_d3950_world_admission_tests',
            '--target', 'native_object_function_query_tests',
            '--target', 'native_object_function_scalar_tests',
            '--target', 'native_indexed_datum_gate_linked_tests',
            '--target', 'native_object_ratio_caller_tests',
            '--target', 'native_vehicle_choice_data_tests',
            '--target', 'native_object_node_hierarchy_tests',
            '--target', 'native_object_node_rebuild_tests',
            '--target', 'native_object_animation_delivery_tests',
            '--target', 'native_object_prediction_copy_tests',
            '--target', 'native_resource_composition_tests',
            '--target', 'native_sphere_frustum_tests',
            '--target', 'native_cluster_frustum_tests',
            '--target', 'native_bsp_queue_find_tests',
            '--target', 'native_fog_membership_tests',
            '--target', 'native_visibility_association_tests',
            '--target', 'native_geometry_visibility_tests',
            '--target', 'native_bsp_admission_tests',
            '--target', 'native_simplex_direction_tests',
            '--target', 'native_simplex_candidate_tests',
            '--target', 'native_simplex_finalize_tests',
            '--target', 'native_simplex_initialize_tests',
            '--target', 'native_simplex_projection_tests',
            '--target', 'native_simplex_iteration_tests',
            '--target', 'native_simplex_support_tests',
            '--target', 'native_simplex_owner_frame_tests',
            '--target', 'native_simplex_paired_tests',
            '--target', 'native_simplex_paired_routes_tests',
            '--target', 'native_simplex_paired_generation_tests',
            '--target', 'native_simplex_boundary_tests',
            '--target', 'native_simplex_boundary_routes_tests',
            '--target', 'native_simplex_boundary_generation_tests',
            '--target', 'native_solver_hull_context_tests',
            '--target', 'native_solver_hull_context_value_tests',
            '--target', 'native_solver_hull_append_frame_tests',
            '--target', 'native_solver_hull_append_value_tests',
            '--target', 'native_solver_hull_append_routes_tests',
            '--target', 'native_solver_hull_append_generation_tests',
            '--target', 'native_solver_hull_support_pair_tests',
            '--target', 'native_solver_hull_support_pair_routes_tests',
            '--target', 'native_solver_hull_support_pair_value_tests',
            '--target', 'native_solver_hull_support_pair_generation_tests',
            '--target', 'native_solver_hull_edge_frame_tests',
            '--target', 'native_solver_hull_edge_value_tests',
            '--target', 'native_solver_hull_edge_routes_tests',
            '--target', 'native_solver_hull_edge_generation_tests',
            '--target', 'native_solver_hull_initialize_frame_tests',
            '--target', 'native_solver_hull_initialize_value_tests',
            '--target', 'native_solver_hull_initialize_routes_tests',
            '--target', 'native_solver_hull_initialize_generation_tests',
            '--target', 'native_solver_hull_line_frame_tests',
            '--target', 'native_solver_hull_line_value_tests',
            '--target', 'native_solver_hull_sample_frame_tests',
            '--target', 'native_solver_hull_sample_value_tests',
            '--target', 'native_solver_hull_minimum_frame_tests',
            '--target', 'native_solver_hull_minimum_value_tests',
            '--target', 'native_solver_hull_minimum_routes_tests',
            '--target', 'native_solver_hull_minimum_generation_tests',
            '--target', 'native_solver_hull_basis_frame_tests',
            '--target', 'native_solver_hull_cleanup_frame_tests',
            '--target', 'native_solver_hull_cleanup_source_tests',
            '--target', 'native_solver_hull_clear_frame_tests',
            '--target', 'native_solver_hull_clear_value_tests',
            '--target', 'native_solver_hull_clear_routes_tests',
            '--target', 'native_solver_hull_clip_frame_tests',
            '--target', 'native_solver_hull_clip_value_tests',
            '--target', 'native_solver_hull_clip_routes_tests',
            '--target', 'native_solver_hull_line_routes_tests',
            '--target', 'native_solver_hull_line_generation_tests',
            '--target', 'native_solver_hull_sample_routes_tests',
            '--target', 'native_solver_hull_sample_generation_tests',
            '--target', 'native_codec_translation_tests',
            '--target', 'native_codec_scale_tests',
            '--target', 'native_codec_rotation_interpolation_tests',
            '--target', 'native_codec_translation_interpolation_tests',
            '--target', 'native_codec_scale_interpolation_tests',
            '--target', 'native_codec_copy_rows_tests',
            '--target', 'native_zero_shape_support_tests',
            '--target', 'native_zero_shape_support_routes_tests',
            '--target', 'native_zero_shape_support_value_tests',
            '--target', 'native_zero_shape_support_generation_tests',
            '--target', 'native_box_shape_support_tests',
            '--target', 'native_box_shape_support_routes_tests',
            '--target', 'native_box_shape_support_value_tests',
            '--target', 'native_box_shape_support_generation_tests',
            '--target', 'native_object_body_lookup_frame_tests',
            '--target', 'native_object_type_query_frame_tests',
            '--target', 'native_object_type_query_source_frame_tests',
            '--target', 'native_object_type_query_owner_tests',
            '--target', 'native_vehicle_object_lookup_tests',
            '--target', 'native_projectile_function_query_frame_tests',
            '--target', 'native_object_definition_mode_query_tests',
            '--target', 'native_object_record_ratio_tests',
            '--target', 'native_object_table_ratio_tests',
            '--target', 'native_map_world_property_query_frame_tests',
            '--target', 'native_device_property_query_tests',
            '--target', 'native_contact_predicate_tests',
            '--target', 'native_vehicle_effect_ratio_tests',
            '--target', 'native_flight_property_query_tests',
            '--target', 'native_tu15_render_ownership_tests',
            '--target', 'native_object_body_lookup_value_tests',
            '--target', 'native_object_body_lookup_source_frame_tests',
            '--target', 'native_object_body_lookup_binding_tests',
            '--target', 'native_object_material_lookup_tests',
            '--target', 'native_object_lookup_copies_tests',
            '--target', 'native_object_lookup_canonical_tests',
            '--target', 'native_object_lookup_format_tests',
            '--target', 'native_arena_initialize_tests',
            '--target', 'native_arena_world_initialize_tests',
            '--target', 'native_packed_rotation_tests',
            '--target', 'native_closest_segments_frame_tests',
            '--target', 'native_closest_segments_routes_tests',
            '--target', 'native_closest_segments_generation_tests',
            '--target', 'native_metadata_retention_tests',
            '--target', 'native_component_evidence_storage_tests',
            '--target', 'native_host_fp_io_tests',
            '--target', 'native_simplex_triangle_frame_tests',
            '--target', 'native_simplex_triangle_generation_tests',
            '--target', 'native_simplex_face_refine_tests',
            '--target', 'native_simplex_face_refine_routes_tests',
            '--target', 'native_simplex_face_refine_value_tests',
            '--target', 'native_simplex_face_refine_generation_tests',
            '--target', 'native_simplex_face_select_tests',
            '--target', 'native_simplex_face_select_routes_tests',
            '--target', 'native_simplex_face_select_generation_tests',
            '--target', 'native_camera_collision_publication_tests',
            '--target', 'native_camera_instance_query_tests',
            '--target', 'native_segment_sphere_tests',
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
            '--retain-report', 'native-instanced-ray-callbacks/report.json',
            '--retain-report', 'native-instanced-ray-callbacks-release/report.json',
            '--retain-report', 'native-instanced-ray-callbacks-generation.json',
            '--retain-report', 'native-tu15-affine-inverse/report.json',
            '--retain-report', 'native-tu15-affine-inverse-release/report.json',
            '--retain-report', 'native-tu15-affine-inverse-mappack/report.json',
            '--retain-report', 'native-tu15-affine-inverse-mappack-release/report.json',
            '--retain-report', 'native-tu15-affine-inverse-generation.json',
            '--retain-report', 'native-tu15-affine-inverse-motion-c1/report.json',
            '--retain-report', 'native-tu15-affine-inverse-animation/report.json',
            '--retain-report', 'native-tu15-affine-inverse-animation-release/report.json',
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
            '--retain-report', 'native-tu15-host-fp-io.json',
            '--retain-report', 'native-tu15-host-fp-io-release.json',
            '--retain-report', 'native-tu15-geometry-visibility.json',
            '--retain-report', 'native-tu15-geometry-visibility-release.json',
            '--retain-report', 'native-tu15-geometry-visibility-values.json',
            '--retain-report', 'native-tu15-geometry-visibility-generation.json',
            '--retain-report', 'native-tu15-bsp-admission.json',
            '--retain-report', 'native-tu15-bsp-admission-release.json',
            '--retain-report', 'native-tu15-bsp-admission-values.json',
            '--retain-report', 'native-tu15-bsp-admission-generation.json',
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
            '--retain-report', 'native-tu15-cluster-frustum.json',
            '--retain-report', 'native-tu15-cluster-frustum-values.json',
            '--retain-report', 'native-tu15-cluster-frustum-generation.json',
            '--retain-report', 'native-tu15-cluster-frustum-release.json',
            '--retain-report', 'native-tu15-bsp-queue-find.json',
            '--retain-report', 'native-tu15-bsp-queue-find-values.json',
            '--retain-report', 'native-tu15-bsp-queue-find-generation.json',
            '--retain-report', 'native-tu15-bsp-queue-find-release.json',
            '--retain-report', 'native-tu15-fog-membership.json',
            '--retain-report', 'native-tu15-fog-membership-values.json',
            '--retain-report', 'native-tu15-fog-membership-generation.json',
            '--retain-report', 'native-tu15-fog-membership-release.json',
            '--retain-report', 'native-tu15-visibility-association.json',
            '--retain-report', 'native-tu15-visibility-association-values.json',
            '--retain-report', 'native-tu15-visibility-association-generation.json',
            '--retain-report', 'native-tu15-visibility-association-release.json',
            '--retain-report', 'native-simplex-direction.json',
            '--retain-report', 'native-simplex-candidate.json',
            '--retain-report', 'native-simplex-finalize.json',
            '--retain-report', 'native-simplex-initialize.json',
            '--retain-report', 'native-simplex-projection.json',
            '--retain-report', 'native-simplex-iteration.json',
            '--retain-report', 'native-simplex-support.json',
            '--retain-report', 'native-simplex-hull-owner-frame.json',
            '--retain-report', 'native-simplex-owner-frame.json',
            '--retain-report', 'native-closest-segments-frame.json',
            '--retain-report', 'native-simplex-paired.json',
            '--retain-report', 'native-simplex-paired-routes.json',
            '--retain-report', 'native-simplex-paired-generation.json',
            '--retain-report', 'native-simplex-boundary.json',
            '--retain-report', 'native-simplex-boundary-routes.json',
            '--retain-report', 'native-simplex-boundary-generation.json',
            '--retain-report', 'native-solver-hull-context.json',
            '--retain-report', 'native-solver-hull-context-value-declared.json',
            '--retain-report', 'native-solver-hull-context-value-genuine.json',
            '--retain-report', 'native-solver-hull-append-frame.json',
            '--retain-report', 'native-solver-hull-append-value.json',
            '--retain-report', 'native-solver-hull-append-routes.json',
            '--retain-report', 'native-solver-hull-append-generation.json',
            '--retain-report', 'native-solver-hull-support-pair.json',
            '--retain-report', 'native-solver-hull-support-pair-routes.json',
            '--retain-report', 'native-solver-hull-support-pair-value.json',
            '--retain-report', 'native-solver-hull-support-pair-generation.json',
            '--retain-report', 'native-solver-hull-edge-frame.json',
            '--retain-report', 'native-solver-hull-edge-value.json',
            '--retain-report', 'native-solver-hull-edge-legacy.json',
            '--retain-report', 'native-solver-hull-edge-routes.json',
            '--retain-report', 'native-solver-hull-edge-generation.json',
            '--retain-report', 'native-solver-hull-initialize-frame.json',
            '--retain-report', 'native-solver-hull-initialize-value.json',
            '--retain-report', 'native-solver-hull-initialize-routes.json',
            '--retain-report', 'native-solver-hull-initialize-generation.json',
            '--retain-report', 'native-solver-hull-line-frame.json',
            '--retain-report', 'native-solver-hull-line-value.json',
            '--retain-report', 'native-solver-hull-sample-frame.json',
            '--retain-report', 'native-solver-hull-sample-value.json',
            '--retain-report', 'native-solver-hull-minimum-frame.json',
            '--retain-report', 'native-solver-hull-minimum-value.json',
            '--retain-report', 'native-solver-hull-minimum-routes.json',
            '--retain-report', 'native-solver-hull-minimum-generation.json',
            '--retain-report', 'native-solver-hull-basis-frame.json',
            '--retain-report', 'native-solver-hull-cleanup-frame.json',
            '--retain-report', 'native-solver-hull-cleanup-source.json',
            '--retain-report', 'native-solver-hull-clear-frame.json',
            '--retain-report', 'native-solver-hull-clear-value.json',
            '--retain-report', 'native-solver-hull-clear-routes.json',
            '--retain-report', 'native-solver-hull-clip-frame.json',
            '--retain-report', 'native-solver-hull-clip-value.json',
            '--retain-report', 'native-solver-hull-clip-routes.json',
            '--retain-report', 'native-solver-hull-line-routes.json',
            '--retain-report', 'native-solver-hull-line-generation.json',
            '--retain-report', 'native-solver-hull-sample-routes.json',
            '--retain-report', 'native-solver-hull-sample-generation.json',
            '--retain-report', 'native-tu15-codec-translation/report.json',
            '--retain-report', 'native-tu15-codec-translation-source/report.json',
            '--retain-report', 'native-tu15-codec-translation-release/report.json',
            '--retain-report', 'native-tu15-codec-scale/report.json',
            '--retain-report', 'native-tu15-codec-scale-source/report.json',
            '--retain-report', 'native-tu15-codec-scale-release/report.json',
            '--retain-report', 'native-tu15-codec-rotation-interpolation/report.json',
            '--retain-report', 'native-tu15-codec-rotation-interpolation-source/report.json',
            '--retain-report', 'native-tu15-codec-rotation-interpolation-source/identity/report.json',
            '--retain-report', 'native-tu15-codec-rotation-interpolation-release/report.json',
            '--retain-report', 'native-tu15-codec-translation-interpolation/report.json',
            '--retain-report', 'native-tu15-codec-translation-interpolation-source/report.json',
            '--retain-report', 'native-tu15-codec-translation-interpolation-source/identity/report.json',
            '--retain-report', 'native-tu15-codec-translation-interpolation-release/report.json',
            '--retain-report', 'native-tu15-codec-scale-interpolation/report.json',
            '--retain-report', 'native-tu15-codec-scale-interpolation-source/report.json',
            '--retain-report', 'native-tu15-codec-scale-interpolation-source/identity/report.json',
            '--retain-report', 'native-tu15-codec-scale-interpolation-release/report.json',
            '--retain-report', 'native-tu15-codec-row-copy/report.json',
            '--retain-report', 'native-tu15-codec-row-copy-release/report.json',
            '--retain-report', 'native-tu15-codec-scale-row/report.json',
            '--retain-report', 'native-tu15-codec-scale-row-release/report.json',
            '--retain-report', 'native-tu15-codec-copy-rows-source/report.json',
            '--retain-report', 'native-tu15-codec-copy-rows-source/identity/report.json',
            '--retain-report', 'native-zero-shape-support.json',
            '--retain-report', 'native-zero-shape-support-routes.json',
            '--retain-report', 'native-zero-shape-support-value.json',
            '--retain-report', 'native-zero-shape-support-generation.json',
            '--retain-report', 'native-box-shape-support.json',
            '--retain-report', 'native-box-shape-support-routes.json',
            '--retain-report', 'native-box-shape-support-value.json',
            '--retain-report', 'native-box-shape-support-generation.json',
            '--retain-report', 'native-object-body-lookup-frame/report.json',
            '--retain-report', 'native-object-body-lookup-frame-release/report.json',
            '--retain-report', 'native-object-body-lookup-value/report.json',
            '--retain-report', 'native-object-body-lookup-value-release/report.json',
            '--retain-report', 'native-object-body-lookup-source-frame/report.json',
            '--retain-report', 'native-object-body-lookup-source-frame-release/report.json',
            '--retain-report', 'native-object-body-lookup-routes/report.json',
            '--retain-report', 'native-object-body-lookup-routes-release/report.json',
            '--retain-report', 'native-object-body-lookup-generation/report.json',
            '--retain-report', 'native-object-material-lookup-routes/report.json',
            '--retain-report', 'native-object-material-lookup-routes-release/report.json',
            '--retain-report', 'native-object-material-lookup-generation/report.json',
            '--retain-report', 'native-object-lookup-copies-routes/report.json',
            '--retain-report', 'native-object-lookup-copies-routes-release/report.json',
            '--retain-report', 'native-object-lookup-copies-generation/report.json',
            '--retain-report', 'native-object-lookup-canonical-routes/report.json',
            '--retain-report', 'native-object-lookup-canonical-routes-release/report.json',
            '--retain-report', 'native-object-lookup-canonical-generation/report.json',
            '--retain-report', 'native-object-lookup-format/report.json',
            '--retain-report', 'native-object-lookup-format-release/report.json',
            '--retain-report', 'native-object-type-query-frame/report.json',
            '--retain-report', 'native-object-type-query-frame-release/report.json',
            '--retain-report', 'native-object-type-query-source-frame/report.json',
            '--retain-report', 'native-object-type-query-source-frame-release/report.json',
            '--retain-report', 'native-object-type-query-owner/report.json',
            '--retain-report', 'native-object-type-query-owner-release/report.json',
            '--retain-report', 'native-object-type-query-candidates/report.json',
            '--retain-report', 'native-object-type-query-candidate-routes/report.json',
            '--retain-report', 'native-object-type-query-candidate-routes-release/report.json',
            '--retain-report', 'native-vehicle-object-lookup-raw/report.json',
            '--retain-report', 'native-vehicle-object-lookup-raw-release/report.json',
            '--retain-report', 'native-vehicle-object-lookup-generation/report.json',
            '--retain-report', 'native-vehicle-object-lookup-public/report.json',
            '--retain-report', 'native-vehicle-object-lookup-public-controls/report.json',
            '--retain-report', 'native-vehicle-object-lookup-public-release/report.json',
            '--retain-report', 'native-projectile-function-query-frame/report.json',
            '--retain-report', 'native-projectile-function-query-frame-release/report.json',
            '--retain-report', 'native-projectile-function-query-source/report.json',
            '--retain-report', 'native-projectile-function-query-binding/report.json',
            '--retain-report', 'native-projectile-function-query-public/report.json',
            '--retain-report', 'native-tu15-render-ownership.json',
            '--retain-report', 'native-map-world-property-query-frame/report.json',
            '--retain-report', 'native-map-world-property-query-source/report.json',
            '--retain-report', 'native-map-world-property-caller-frame/report.json',
            '--retain-report', 'native-map-world-property-caller-source/report.json',
            '--retain-report', 'native-map-world-property-binding/report.json',
            '--retain-report', 'native-map-world-property-public/report.json',
            '--retain-report', 'native-device-property-query-frame/report.json',
            '--retain-report', 'native-device-property-query-source/report.json',
            '--retain-report', 'native-device-property-query-binding/report.json',
            '--retain-report', 'native-device-property-query-public/report.json',
            '--retain-report', 'native-object-definition-mode-query-frame/report.json',
            '--retain-report', 'native-object-definition-mode-query-source/report.json',
            '--retain-report', 'native-object-definition-mode-query-binding/report.json',
            '--retain-report', 'native-object-definition-mode-query-public/report.json',
            '--retain-report', 'native-object-record-ratio-frame/report.json',
            '--retain-report', 'native-object-record-ratio-source/report.json',
            '--retain-report', 'native-object-record-ratio-binding/report.json',
            '--retain-report', 'native-object-record-ratio-public/report.json',
            '--retain-report', 'native-object-table-ratio-frame/report.json',
            '--retain-report', 'native-object-table-ratio-source/report.json',
            '--retain-report', 'native-object-table-ratio-binding/report.json',
            '--retain-report', 'native-object-table-ratio-public/report.json',
            '--retain-report', 'native-contact-predicate-raw.json',
            '--retain-report', 'native-contact-predicate-source.json',
            '--retain-report', 'native-contact-predicate-generation.json',
            '--retain-report', 'native-vehicle-effect-ratio-raw.json',
            '--retain-report', 'native-vehicle-effect-ratio-source.json',
            '--retain-report', 'native-node-progress-query-frame/report.json',
            '--retain-report', 'native-node-progress-query-source/report.json',
            '--retain-report', 'native-node-progress-query-local/report.json',
            '--retain-report', 'native-d3950-world-admission.json',
            '--retain-report', 'native-object-function-query-binding.json',
            '--retain-report', 'native-object-function-query-public.json',
            '--retain-report', 'native-object-function-scalar-frame.json',
            '--retain-report', 'native-object-function-scalar-producer.json',
            '--retain-report', 'native-indexed-datum-gate-tu15/report.json',
            '--retain-report', 'native-indexed-datum-gate-mappack11/report.json',
            '--retain-report', 'native-indexed-datum-gate-source/report.json',
            '--retain-report', 'native-indexed-datum-gate-linked.json',
            '--retain-report', 'native-object-ratio-caller-frame/report.json',
            '--retain-report', 'native-object-ratio-caller-source/report.json',
            '--retain-report', 'native-object-ratio-caller-world.json',
            '--retain-report', 'native-object-ratio-caller-inherited.json',
            '--retain-report', 'native-node-progress-query-binding/report.json',
            '--retain-report', 'native-flight-property-query-frame/report.json',
            '--retain-report', 'native-flight-property-query-source/report.json',
            '--retain-report', 'native-flight-property-query-binding/report.json',
            '--retain-report', 'native-flight-property-query-public/report.json',
            '--retain-report', 'native-death-cycle-flight-query-source/report.json',
            '--retain-report', 'native-death-cycle-flight-query-binding/report.json',
            '--retain-report', 'native-death-cycle-flight-query-public/report.json',
            '--retain-report', 'native-object-body-observer-branch/report.json',
            '--retain-report', 'native-object-body-observer-branch-release/report.json',
            '--retain-report', 'native-object-body-foreign-continuation/report.json',
            '--retain-report', 'native-object-body-foreign-continuation-release/report.json',
            '--retain-report', 'native-arena-initialize.json',
            '--retain-report', 'native-arena-initialize-release.json',
            '--retain-report', 'native-arena-initialize-source.json',
            '--retain-report', 'native-arena-initialize-source-release.json',
            '--retain-report', 'native-arena-initialize-generation.json',
            '--retain-report', 'native-arena-initialize-schedule.json',
            '--retain-report', 'native-arena-initialize-transport.json',
            '--retain-report', 'native-arena-initialize-bootstrap.json',
            '--retain-report', 'native-arena-world-initialize-source.json',
            '--retain-report', 'native-arena-world-initialize-source-release.json',
            '--retain-report', 'native-arena-world-initialize-publication.json',
            '--retain-report', 'native-tu15-packed-rotation/report.json',
            '--retain-report', 'native-tu15-packed-rotation-policy/report.json',
            '--retain-report', 'native-tu15-packed-rotation-value/report.json',
            '--retain-report', 'native-tu15-packed-rotation-routes/report.json',
            '--retain-report', 'native-tu15-packed-rotation-generation.json',
            '--retain-report', 'native-tu15-packed-rotation-release/report.json',
            '--retain-report', 'native-closest-segments-routes.json',
            '--retain-report', 'native-closest-segments-generation.json',
            '--retain-report', 'native-mopp-ray-setup-frame/report.json',
            '--retain-report', 'native-mopp-ray-setup-frame-release/report.json',
            '--retain-report', 'native-sphere-ray-cast/report.json',
            '--retain-report', 'native-sphere-ray-cast-release/report.json',
            '--retain-report', 'native-sphere-ray-cast-routes/report.json',
            '--retain-report', 'native-sphere-ray-cast-routes-release/report.json',
            '--retain-report', 'native-sphere-ray-cast-generation.json',
            '--retain-report', 'native-tu15-diagnostic-stream.json',
            '--retain-report', 'native-tu15-diagnostic-stream-release.json',
            '--retain-report', 'native-tu15-diagnostic-stream-generation.json',
            '--retain-report', 'native-effect-reference-element/report.json',
            '--retain-report', 'native-effect-reference-element-release/report.json',
            '--retain-report', 'native-effect-reference-element-generation.json',
            '--retain-report', 'native-tu15-diagnostic-logger-owner.json',
            '--retain-report', 'native-tu15-diagnostic-logger-owner-release.json',
            '--retain-report', 'native-tu15-diagnostic-logger-world.json',
            '--retain-report', 'native-mopp-ray-sphere-query/report.json',
            '--retain-report', 'native-mopp-ray-sphere-query-release/report.json',
            '--retain-report', 'native-tu15-mopp-diagnostic-return.json',
            '--retain-report', 'native-tu15-model-marker-lookup/report.json',
            '--retain-report', 'native-tu15-model-marker-lookup-release/report.json',
            '--retain-report', 'native-tu15-model-marker-lookup-generation.json',
            '--retain-report', 'native-tu15-model-marker-lookup-sandbox/report.json',
            '--retain-report', 'native-tu15-object-parent-mode/report.json',
            '--retain-report', 'native-tu15-object-parent-mode-release/report.json',
            '--retain-report', 'native-tu15-object-parent-mode-generation.json',
            '--retain-report', 'native-required-owner-policy.json',
            '--retain-report', 'native-required-owner-policy-release.json',
            '--retain-report', 'native-tu15-required-abi-fp/report.json',
            '--retain-report', 'native-tu15-required-abi-fp-release/report.json',
            '--retain-report', 'native-tu15-marker-owner/report.json',
            '--retain-report', 'native-tu15-marker-owner-release/report.json',
            '--retain-report', 'native-tu15-marker-owner-generation.json',
            '--retain-report', 'native-metadata-retention.json',
            '--retain-report', 'native-component-evidence-storage.json',
            '--retain-report', 'native-simplex-triangle-frame.json',
            '--retain-report', 'native-simplex-triangle-generation.json',
            '--retain-report', 'native-simplex-face-refine.json',
            '--retain-report', 'native-simplex-face-refine-routes.json',
            '--retain-report', 'native-simplex-face-refine-value.json',
            '--retain-report', 'native-simplex-face-refine-generation.json',
            '--retain-report', 'native-simplex-face-select.json',
            '--retain-report', 'native-simplex-face-select-routes.json',
            '--retain-report', 'native-simplex-face-select-generation.json',
            '--retain-report', 'native-tu15-camera-collision-publication/report.json',
            '--retain-report', 'native-tu15-camera-collision-publication-release/report.json',
            '--retain-report', 'native-tu15-camera-collision-publication-generation.json',
            '--retain-report', 'native-tu15-camera-instance-query/report.json',
            '--retain-report', 'native-tu15-camera-instance-query-release/report.json',
            '--retain-report', 'native-tu15-camera-instance-query-values/report.json',
            '--retain-report', 'native-tu15-camera-instance-query-generation.json',
            '--retain-report', 'native-tu15-segment-sphere/report.json',
            '--retain-report', 'native-tu15-segment-sphere-release/report.json',
            '--retain-report', 'native-tu15-segment-sphere-generation.json',
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
            '--cmake-arg=-DHALO2_OBJECT_FUNCTION_MCC_MAP='+str(
                root/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11/coagulation.map'),
            '--cmake-arg=-DHALO2_TU15_GAMEPLAY_XBE='+str(args.tu15_xbe.resolve()),
            '--cmake-arg=-DHALO2_STAGE_CUSTOM_ASSETS=OFF']
        if args.execution_accounting:
            command+=['--target','native_execution_accounting_aliases','--target','native_execution_accounting_tests',
                      '--retain-report','native-execution-coverage.json','--cmake-arg=-DHALO2_NATIVE_EXECUTION_ACCOUNTING=ON']
        environment = dict(os.environ, CARGO_NET_OFFLINE='true')
        engine_command=[sys.executable,'-B',str(source/'tools/run_tests.py'),'--build-worker',
            '--source',str(source),'--output',str(output/'engine-build'),'--jobs',str(args.jobs),
            '--target','halo2_native','--build-only',
            *(value for value in command if value.startswith('--cmake-arg='))]
        # Keep the component-first default. An engine-first retry can expose
        # compile errors outside the registered component targets early.
        report['validation_sequence']=(['engine-build','components'] if args.engine_first
                                       else ['components','engine-build']);save()
        if args.engine_first:
            run('engine-build',engine_command,environment)
        run('components', command, environment)
        report['component_evidence_storage']=compact_component_evidence(output)
        report['retained_metadata_bytes']=check_metadata_budget(output);save()
        if '***Skipped' in (output/'cmake/tests.log').read_text():
            raise RuntimeError('A required component oracle was skipped; see CMake test log')
        if not args.engine_first:
            run('engine-build',engine_command,environment)
        report['retained_metadata_bytes']=check_metadata_budget(output);save()
        runtime = build / 'halo2_native'
        report['runtime_sha256'] = sha(runtime);report['runtime_bytes'] = runtime.stat().st_size
        if args.execution_accounting and not args.build_and_components_only:
            # Use the frozen capture helper without modifying runtime service
            # callbacks or native admission. Each run owns a distinct report;
            # captures and raw counters remain in the enclosing scratch owner.
            spec=importlib.util.spec_from_file_location('frozen_migration_parity',source/'tools/perf_4k60/parity.py')
            capture_helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture_helper)
            coverage=load_metadata(output/'cmake/evidence/native-execution-coverage.json')
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
                            retained=retain_accounting_metadata(raw,output/(label+'-execution.json'))
                            item.update(runtime_report=retained['path'],runtime_report_sha256=retained['raw_sha256'],runtime_report_storage=retained)
                            summary=scratch/(label+'-execution-summary.json')
                            run('accounting-analysis-'+label,[sys.executable,'-B',str(source/'tools/native_execution_report.py'),
                                '--runtime',str(raw),'--output',str(summary)],environment)
                            analyzed=json.loads(summary.read_text())
                            if analyzed['totals'].get('active',0):
                                raise RuntimeError('Completed process has active generated scopes: '+label)
                            retained_summary=retain_accounting_metadata(summary,output/summary.name)
                            item.update(analysis=retained_summary['path'],analysis_sha256=retained_summary['raw_sha256'],analysis_storage=retained_summary);save()
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
        save();report['retained_metadata_bytes']=check_metadata_budget(output);save();return 0
    except SystemExit as error:
        report.update(passed=False, status='interrupted', exit_code=error.code)
        save()
        raise
    except Exception as error:
        report.update(passed=False, status='failed', error=str(error));save()
        print(json.dumps({'status': 'failed', 'error': str(error)}), flush=True)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
