"""Exact source-prefix admission evidence; never a native dependency-closure claim.

This bounded catalog supplements build-local specializer receipts for parents
whose authenticated prefixes already exist in canonical generated source.
Uncatalogued hooks remain ordinary audit hints in the migration inventory.
"""
import hashlib
import importlib
from pathlib import Path
import re
from generated_routine_ownership import function_bodies

TOOL_ROOT=Path(__file__).resolve().parent
# entry, whole-source authentication function, and exact exported prefix.
FAMILIES=(
    ('3eb10','native_indexed_datum_gate_replacements','replace_indexed_datum_gate'),
    ('17de60','native_particle_type_filter_replacements','replace_particle_type_filter'),
    ('252ec0','native_particle_envelope_replacements','replace_particle_envelope'),
    ('1e4c0','native_particle_record_replacements','replace_particle_record'),
    ('178070','native_effect_reference_element_replacements','replace_effect_reference_element'),
    ('13a6b0','native_effect_reference_element_replacements','replace_effect_reference_element'),
    ('22b790','native_effect_reference_element_replacements','replace_effect_reference_element'),
    ('22aca0','native_effect_reference_element_replacements','replace_effect_reference_element'),
    ('22b710','native_effect_reference_element_replacements','replace_effect_reference_element'),
    ('126200','native_resource_touch_replacements','replace_resource_touch'),
    ('1264e0','native_resource_maintenance_replacements','replace_resource_maintenance'),
    ('126060','native_resource_page_replacements','replace_resource_page'),
    ('1260c0','native_animation_resource_read_replacements','replace_animation_resource_read'),
    ('1e5c80','native_node_resource_lifecycle_replacements','replace_node_resource_lifecycle'),
    ('1e5af0','native_node_resource_replacements','replace_node_resource'),
    ('1e6cb0','native_bsp_leaf_surface_replacements','replace_bsp_leaf_surface'),
    ('126190','native_resource_record_search_replacements','replace_resource_record_search'),
    ('126480','native_resource_record_count_replacements','replace_resource_record_count'),
    ('33b590','native_millisecond_clock_replacements','replace_millisecond_clock'),
    ('1e6980','native_bsp_segment_replacements','replace_bsp_segment'),
    ('305e40','native_mopp_aabb_replacements','replace_mopp_aabb'),
    ('305e40','native_mopp_aabb_canonical_replacements','replace_mopp_aabb_canonical'),
    ('2dbf74','native_kernel_yield_replacements','replace_kernel_yield'),
    ('16e770','native_datum_specific_replacements','replace_datum_specific'),
    ('16e860','native_datum_initialize_replacements','replace_datum_initialize'),
    ('13fe50','native_page_allocate_replacements','install_native_page_allocate'),
    ('1e68e0','native_bsp_segment_setup_replacements','replace_bsp_segment_setup'),
    ('1262b0','native_animation_resource_wait_replacements','replace_animation_resource_wait'),
    ('1e3150','native_node_descriptor_lookup_replacements','replace_node_descriptor_lookup'),
    ('1cf0e0','native_node_progress_query_replacements','replace_node_progress_query'),
    ('d3950','native_node_function_progress_replacements','replace_node_function_progress'),
    ('c9070','native_object_function_query_replacements','replace_object_function_query'),
    ('bcb70','native_object_function_scalar_replacements','replace_object_function_scalar'),
    ('15cb60','native_vehicle_choice_data_replacements','replace_vehicle_choice_data'),
    ('15cc70','native_vehicle_choice_data_replacements','replace_vehicle_choice_data'),
    ('15cdf0','native_vehicle_choice_data_replacements','replace_vehicle_choice_data'),
    ('15cd30','native_vehicle_choice_data_replacements','replace_vehicle_choice_data'),
    ('184690','native_sphere_frustum_replacements','replace_sphere_frustum'),
    ('167ef0','native_cluster_frustum_replacements','replace_cluster_frustum'),
    ('166c00','native_bsp_queue_find_replacements','replace_bsp_queue_find'),
    ('171070','native_fog_membership_replacements','replace_fog_membership'),
    ('136e10','native_visibility_association_replacements','replace_visibility_association'),
    ('135970','native_geometry_visibility_replacements','replace_geometry_visibility'),
    ('136ea0','native_bsp_admission_replacements','replace_bsp_admission'),
    ('135b60','native_object_admission_replacements','replace_object_admission'),
    ('136a80','native_model_draw_submission_replacements','replace_model_draw_submission'),
    ('134d30','native_cluster_contents_replacements','replace_cluster_contents'),
    ('136d50','native_prt_partition_replacements','replace_prt_partition'),
    ('4bae0','native_object_fade_replacements','replace_object_fade'),
    ('135e00','native_object_draw_flags_replacements','replace_object_draw_flags'),
    ('136230','native_object_pass_replacements','replace_object_pass'),
    ('1365c0','native_model_hierarchy_replacements','replace_model_hierarchy'),
    ('1362a0','native_object_submit_replacements','replace_object_submit'),
    ('c3ab0','native_light_shape_replacements','replace_light_shape'),
    ('1373f0','native_view_reset_replacements','replace_view_reset'),
    ('bbd10','native_object_hierarchy_root_replacements','replace_object_hierarchy_root'),
    ('135690','native_view_setup_replacements','replace_view_setup'),
    ('bf0c0','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('bfb90','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('b8780','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('b87b0','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('1a5080','native_async_order_replacements','install_native_async_order'),
    ('122020','native_async_insert_replacements','install_native_async_insert'),
    ('1232c0','native_async_insert_replacements','install_native_async_insert'),
    ('1236a0','native_async_promote_replacements','install_native_async_promote'),
    ('24dec0','native_instanced_ray_child_replacements','replace_instanced_ray_child'),
    ('2e8dd0','native_sphere_ray_cast_replacements','replace_sphere_ray_cast'),
    ('1f1d20','native_polygon_support_replacements','replace_polygon_support'),
    ('1f1da0','native_polygon_features_replacements','replace_polygon_features'),
    ('16b810','native_camera_instance_admission_replacements','replace_camera_instance_admission'),
    ('186e30','native_camera_instance_bitmap_replacements','replace_camera_instance_bitmap'),
    ('16b9a0','native_camera_collision_publication_replacements','replace_camera_collision_publication'),
    ('120fa0','native_segment_sphere_replacements','replace_segment_sphere'),
    ('16c000','native_camera_instance_query_replacements','replace_camera_instance_query'),
    ('24def0','native_instanced_ray_callback_replacements','replace_instanced_ray_callbacks'),
    ('24df20','native_instanced_ray_callback_replacements','replace_instanced_ray_callbacks'),
    ('30eb30','native_closest_segments_replacements','replace_closest_segments'),
    ('312620','native_closest_segments_replacements','replace_closest_segments'),
    ('309d40','native_simplex_paired_replacements','replace_simplex_paired'),
    ('30dc10','native_simplex_boundary_replacements','replace_simplex_boundary'),
    ('313290','native_solver_hull_append_replacements','replace_solver_hull_append'),
    ('313390','native_solver_hull_support_pair_replacements','replace_solver_hull_support_pair'),
    ('313470','native_solver_hull_edge_replacements','replace_solver_hull_edge'),
    ('313560','native_solver_hull_initialize_replacements','replace_solver_hull_initialize'),
    ('3136a0','native_solver_hull_line_replacements','replace_solver_hull_line'),
    ('313300','native_solver_hull_sample_replacements','replace_solver_hull_sample'),
    ('3131b0','native_solver_hull_minimum_replacements','replace_solver_hull_minimum'),
    ('314870','native_solver_hull_clear_replacements','replace_solver_hull_clear'),
    ('3147d0','native_solver_hull_clip_replacements','replace_solver_hull_clip'),
    ('2e8d20','native_zero_shape_support_replacements','replace_zero_shape_support'),
    ('306790','native_box_shape_support_replacements','replace_box_shape_support'),
    ('296180','native_packed_rotation_replacements','replace_packed_rotation'),
    ('2961f0','native_codec_translation_replacements','replace_codec_translation'),
    ('296220','native_codec_scale_replacements','replace_codec_scale'),
    ('296240','native_codec_rotation_interpolation_replacements','replace_codec_rotation_interpolation'),
    ('2962f0','native_codec_translation_interpolation_replacements','replace_codec_translation_interpolation'),
    ('296620','native_codec_scale_interpolation_replacements','replace_codec_scale_interpolation'),
    ('2963d0','native_codec_copy_replacements','replace_codec_copy'),
    ('296410','native_codec_copy_replacements','replace_codec_copy'),
    ('2cec60','native_codec_copy_replacements','replace_codec_copy'),
    ('10de40','native_object_type_query_replacements','replace_object_type_query'),
    ('e0900','native_flight_property_query_replacements','replace_flight_property_query'),
    ('e0900','native_death_cycle_flight_query_replacements','replace_death_cycle_flight_query'),
    ('100e70','native_projectile_function_query_replacements','replace_projectile_function_query'),
    ('165590','native_object_definition_mode_query_replacements','replace_object_definition_mode_query'),
    ('107140','native_object_record_ratio_replacements','replace_object_record_ratio'),
    ('1046c0','native_object_ratio_caller_replacements','replace_object_ratio_caller'),
    ('1655f0','native_object_table_ratio_replacements','replace_object_table_ratio'),
    ('24b430','native_object_table_ratio_replacements','replace_object_table_ratio'),
    ('11e490','native_map_world_property_query_replacements','replace_map_world_property_queries'),
    ('1db800','native_map_world_property_query_replacements','replace_map_world_property_queries'),
    ('10be20','native_device_property_query_replacements','replace_device_property_query'),
    ('bbe70','native_vehicle_render_root_replacements','replace_vehicle_render_root'),
    ('d26c0','native_vehicle_effect_ratio_replacements','install'),
    ('303800','native_contact_predicate_replacements','install'),
    ('bcdf0','native_vehicle_object_lookup_replacements','replace_vehicle_object_lookup'),
    ('bcdf0','native_object_body_replacements','replace_object_body'),
    ('bcdf0','native_object_material_lookup_replacements','replace_object_material_lookup'),
    ('bcdf0','native_object_lookup_copies_replacements','replace_object_lookup_copies'),
    ('bbcb0','native_object_lookup_canonical_replacements','replace_canonical_object_lookup'),
    ('125280','native_arena_world_initialize_replacements','replace_arena_world_initialize'),
    ('1e11b0','native_model_marker_lookup_replacements','replace_model_marker_lookup'),
    ('1dc8f0','native_model_marker_lookup_replacements','replace_model_marker_lookup'),
    ('marker_lookup','native_model_marker_lookup_replacements','replace_effect'),
    ('fc510','native_object_parent_mode_replacements','replace_object_parent_mode'),
    ('marker_parent_mode','native_object_parent_mode_replacements','replace_effect'),
    ('bacd0','native_marker_owner_replacements','replace_marker_owner'),
    ('b9b90','native_marker_owner_replacements','replace_marker_owner'),
    ('marker_parent_unit','native_marker_owner_replacements','replace_effect'),
    ('2e2d10','native_diagnostic_stream_replacements','replace_diagnostic_stream'),
    ('2e2e00','native_diagnostic_stream_replacements','replace_diagnostic_stream'),
    ('2e3080','native_diagnostic_stream_replacements','replace_diagnostic_stream'),
    ('2e30e0','native_diagnostic_stream_replacements','replace_diagnostic_stream'),
)

def runtime_include_hashes(root,include):
    """Hash private include inputs without asserting preprocessor reachability."""
    pending=[(root/'src',name)for name in re.findall(r'#include "([^"]+)"',include)]
    hashes={}
    while pending:
        directory,name=pending.pop()
        path=next((p.resolve()for p in(directory/name,root/'src'/name,root/'include'/name)if p.is_file()),None)
        if path is None:continue
        if not path.is_relative_to(root.resolve()):continue
        relative=str(path.relative_to(root.resolve()))
        if relative in hashes:continue
        data=path.read_bytes();hashes[relative]=hashlib.sha256(data).hexdigest()
        pending.extend((path.parent,name)for name in re.findall(r'^\s*#include "([^"]+)"',data.decode(),re.M))
    return dict(sorted(hashes.items()))

def authenticated_prefixes(root,path,text):
    """Return per-entry proof or explicit rejection using each owning binder.

    A known prefix only qualifies when the owning complete-body/resolver/
    dispatcher checks accept the entire source, an idempotent application
    preserves its bytes, and removing the target body's exact prefix makes
    that binder reconstruct the original source. The reconstruction prevents
    a nonmatching function signature from turning a binder no-op into proof.
    Strict single-owner overlays may also remove/reinstall their exact include
    at its original position; every remaining byte must reconstruct unchanged.
    Admission at runtime and child closure remain open.
    """
    result={};targets=None
    for entry,name,function in FAMILIES:
        module=importlib.import_module(name)
        # Vehicle DATA admission sits inside the complete authenticated TU1.5
        # guard of its retained route. The owning binder pins and reconstructs
        # that whole guard/body; this is not an arbitrary substring hook.
        nested=name=='native_vehicle_choice_data_replacements'
        family=name=='native_object_node_rebuild_replacements'
        arena_world=name=='native_arena_world_initialize_replacements'
        closest=name=='native_closest_segments_replacements'
        marker=name=='native_model_marker_lookup_replacements'
        marker_effect=marker and entry=='marker_lookup'
        parent_mode=name=='native_object_parent_mode_replacements'
        parent_effect=parent_mode and entry=='marker_parent_mode'
        inherited=name=='native_marker_owner_replacements'
        inherited_effect=inherited and entry=='marker_parent_unit'
        entry_specific=name in('native_async_insert_replacements','native_instanced_ray_callback_replacements','native_diagnostic_stream_replacements','native_effect_reference_element_replacements','native_object_table_ratio_replacements')
        if marker_effect or parent_effect or inherited_effect:
            helper='native_weapon_marker_lookup_tu15_internal.hpp'if marker_effect else'native_weapon_marker_matrix_tu15_internal.hpp'
            if Path(path).resolve()!=(Path(root)/'src/gameplay/weapons'/helper).resolve():continue
            prefix=module.EFFECT_PREFIX
        elif marker:
            owner=module.BODY.search(text)
            if not owner or owner[1]!=entry:continue
            prefix=module.prefix('actual'if module.prefix('actual')in owner[0]else'canonical')
        elif name in('native_object_body_replacements','native_object_material_lookup_replacements','native_object_lookup_copies_replacements','native_object_lookup_canonical_replacements'):
            choices=[value for value in(module.prefix(False),module.prefix(True),module.PREFIX)if value in text]
            if not choices:continue
            prefix=choices[0]
        elif name in('native_solver_hull_clip_replacements','native_solver_hull_clear_replacements','native_solver_hull_initialize_replacements','native_solver_hull_line_replacements','native_solver_hull_sample_replacements','native_solver_hull_minimum_replacements','native_flight_property_query_replacements','native_death_cycle_flight_query_replacements','native_device_property_query_replacements'):
            choices=[value for value in module.PREFIXES.values() if value in text]
            if not choices:continue
            prefix=choices[0]
        elif name in('native_codec_rotation_interpolation_replacements','native_codec_translation_interpolation_replacements','native_codec_scale_interpolation_replacements'):
            choices=[value for value in(module.RAW_PREFIX,module.PREPARED_PREFIX)if value in text]
            if not choices:continue
            prefix=choices[0]
        elif name=='native_indexed_datum_gate_replacements':
            choices=[module.prefix(literal,image) for _,_,_,image in module.CONTEXTS
                     for literal in (False,True) if module.prefix(literal,image) in text]
            if not choices:continue
            prefix=choices[0]
        elif name=='native_codec_copy_replacements':
            choices=[value for value in module.PREFIXES[int(entry,16)].values()if value in text]
            if not choices:continue
            prefix=choices[0]
        elif name in ('native_object_type_query_replacements','native_projectile_function_query_replacements','native_object_record_ratio_replacements','native_object_ratio_caller_replacements','native_vehicle_render_root_replacements','native_vehicle_effect_ratio_replacements','native_contact_predicate_replacements','native_node_progress_query_replacements','native_node_function_progress_replacements','native_object_function_query_replacements','native_object_function_scalar_replacements','native_object_definition_mode_query_replacements','native_particle_type_filter_replacements','native_particle_envelope_replacements','native_particle_record_replacements'):
            choices=[value for value in(module.prefix(False),module.prefix(True))if value in text]
            if not choices:continue
            prefix=choices[0]
        elif name in ('native_map_world_property_query_replacements','native_object_table_ratio_replacements'):
            choices=[value for value in(module.prefix(entry,False),module.prefix(entry,True))if value in text]
            if not choices:continue
            prefix=choices[0]
        elif arena_world:
            owner=module.BODY.search(text)
            choices=[value for value in(module.prefix(False),module.prefix(True))if owner and value in owner[0]]
            if not choices:continue
            prefix=choices[0]
        elif inherited:
            owner=module.BODY.search(text)
            if not owner or owner[1]!=entry:continue
            prefix=module.CANONICAL_PREFIX if entry=='b9b90'else module.PREFIX
        else:prefix=module.ACTUAL_PREFIX if closest and entry=='312620'else module.PREFIX if closest else module.prefix(module.PINS[int(entry,16)][0])if nested else module.PREFIXES[entry]if family else module.prefix(entry)if entry_specific else module.PREFIX
        if prefix not in text:continue
        tool=TOOL_ROOT/(name+'.py')
        evidence={'binder':'tools/'+name+'.py','binder_sha256':hashlib.sha256(tool.read_bytes()).hexdigest(),
            'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'entry_symbol':entry,
            'validation_dependency_sha256':{p:hashlib.sha256((TOOL_ROOT/p).read_bytes()).hexdigest()for p in('native_source_prefixes.py','native_selected_operand_binding.py','native_halfword_bit_query_replacements.py','specialize_native_operands.py')},
            'runtime_include_source_sha256':runtime_include_hashes(Path(root),''.join(module.INCLUDES)if family or arena_world else getattr(module,'INCLUDE','')),
            'runtime_include_scope':'Private source include closure; conditional preprocessing and execution are not proved.',
            'qualification':'complete source/body/resolver/dispatcher checks implemented by the owning exact binder',
            'runtime_admission_proved':False,'native_dependency_closure_proved':False}
        try:
            bind=getattr(module,function)
            if name=='native_solver_hull_clip_replacements' and prefix in (module.PREFIXES[True,False],module.PREFIXES[True,True]):
                bind=module.replace_solver_hull_clip_prepared
            if name=='native_solver_hull_clear_replacements' and prefix==module.PREFIXES[True]:
                bind=module.replace_solver_hull_clear_prepared
            if marker_effect or parent_effect or inherited_effect:
                producer=Path(root)/'src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
                producer_text=producer.read_text()
                if producer_text.count('#include "'+helper+'"')!=1:
                    raise ValueError('Actual Effect helper include ownership drift')
                evidence['validation_source_sha256']={str(producer.relative_to(root)):hashlib.sha256(producer.read_bytes()).hexdigest()}
                bind=lambda source:module.replace_effect(source,producer_text)
            if marker:
                evidence['validation_dependency_sha256']['native_sandbox_dispatch_preservation.py']=hashlib.sha256((TOOL_ROOT/'native_sandbox_dispatch_preservation.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact old instruction-shaped body remains for reference/unknown admission; child closure and engine-wide native execution remain unproved.'
            if parent_mode:
                evidence['validation_dependency_sha256']['native_model_marker_lookup_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_model_marker_lookup_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact instruction-shaped continuation remains for unsupported/reference admission; private MapPack evidence does not qualify a generated default or engine-wide native execution.'
            if inherited:
                for dependency in('native_model_marker_lookup_replacements.py','native_object_parent_mode_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact instruction-shaped continuation remains for unsupported/reference admission. The actual required-owner provider and referenced Effect ABI/observer transport are distinct from direct native child execution; dense-only defaults do not qualify returned opaque MMX or MapPack routes.'
            if name=='native_diagnostic_stream_replacements':
                evidence['validation_dependency_sha256']['native_model_marker_lookup_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_model_marker_lookup_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact controller continuations remain for unsupported/reference admission. Formatter, buffered writer/copy and platform output dependencies remain retained or unqualified; this prefix does not establish all-native diagnostics.'
            if name=='native_effect_reference_element_replacements':
                evidence['validation_dependency_sha256']['native_model_marker_lookup_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_model_marker_lookup_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact complete current/fresh bodies remain for unsupported/reference admission. Actual recursive/local dispatch and external accessor providers are preserved; exact owned required-provider policy is separately admitted. Dense TU15 source prefixes do not qualify MapPack, opaque MMX, all descendants or runtime delivery.'
            if name=='native_sphere_ray_cast_replacements':
                evidence['validation_dependency_sha256']['native_instanced_ray_child_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_instanced_ray_child_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Two exact current/fresh continuations remain; the fresh C1/spill delta and helper are fully pinned. Runtime admission and complete native dependency closure remain separate.'
            if name=='native_packed_rotation_replacements':
                evidence['validation_dependency_sha256']['native_packed_rotation_source_inputs.py']=hashlib.sha256((TOOL_ROOT/'native_packed_rotation_source_inputs.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Three TU15 generated owners retain exact complete reference bodies. The native decoder owns authored EMMS and opaque incoming MMX payloads; intermediate MMX callbacks, earlier mutable IO policy, partial vector-store atomicity and other images remain outside this prefix evidence.'
            if name=='native_codec_translation_replacements':
                for dependency in('native_packed_rotation_replacements.py','native_packed_rotation_source_inputs.py','native_model_marker_lookup_replacements.py','native_halfword_bit_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Three exact TU15 codec translation owners preserve their full fallback bodies and authenticated suffixes after retired provider instructions. Full GP/observer feedback is qualified within the readonly source FP-metadata and host FP-control/status domain. Other images, deliberate FP metadata mutation, complete parent/runtime closure and installed delivery remain separate.'
            if name=='native_codec_copy_replacements':
                for dependency in('native_model_marker_lookup_replacements.py','native_selected_operand_binding.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Two exact Primary/ModelRender groups preserve five complete TU15 row-copy bodies and thirty-three authenticated retired-I/O suffixes. Prepared and callback-free raw admission use explicit format tags; GP and observer feedback preserve source instruction retirement. Source FP metadata and host controls/status are readonly. Other images, physical signals/MMIO/partial writes and complete parent/runtime closure remain separate.'
            if name=='native_codec_scale_replacements':
                for dependency in('native_model_marker_lookup_replacements.py','native_sandbox_dispatch_preservation.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Two exact ModelRender/Sandbox TU15 codec scalar-copy owners retain their whole reference bodies and retired-I/O suffixes. Source FP metadata/host control feedback, raw instruction-callback format, other images, complete parent/runtime closure and delivery remain separate.'
            if name=='native_codec_rotation_interpolation_replacements':
                for dependency in('native_packed_rotation_replacements.py','native_packed_rotation_source_inputs.py','native_model_marker_lookup_replacements.py','native_halfword_bit_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Two exact Primary/ModelRender TU15 quaternion interpolation owners retain complete ordered-SSE fallbacks and nineteen authenticated retired-I/O suffixes. Raw callback-bearing entry remains retained; prepared and callback-free raw admission use explicit tags. Source FP metadata and host controls/status are readonly; physical unaligned faults, retained unaligned handoffs, partial vector atomicity, other images and full parent/runtime closure remain separate.'
            if name=='native_codec_translation_interpolation_replacements':
                for dependency in('native_packed_rotation_replacements.py','native_packed_rotation_source_inputs.py','native_model_marker_lookup_replacements.py','native_halfword_bit_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Two exact Primary/ModelRender TU15 translation interpolation owners preserve complete reference bodies and twenty-nine authenticated retired-I/O suffixes. Raw callback-bearing entry remains retained; prepared and callback-free raw admission use explicit tags. Source FP metadata and host controls/status are readonly. Physical signals, MMIO, partial writes, other images and complete parent/runtime closure remain separate.'
            if name=='native_codec_scale_interpolation_replacements':
                for dependency in('native_packed_rotation_replacements.py','native_packed_rotation_source_inputs.py','native_model_marker_lookup_replacements.py','native_halfword_bit_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='One exact ModelRender TU15 scalar interpolation owner retains the complete reference body and fifteen authenticated retired-I/O suffixes. The late memory multiply retires only after SIMD publication. Raw callback-bearing entry remains retained; prepared and callback-free raw admission use explicit tags. Source FP metadata and host controls/status are readonly; other images, physical signals, MMIO, partial writes and complete runtime closure remain separate.'
            if name=='native_object_body_replacements':
                evidence['retained_fallback_scope']='One exact UnitView TU15 owner retains its complete reference body and authenticated suffixes after retired provider instructions. Other owners/images, opaque MMX, hostile host-FP mutation and complete native dependency closure remain separate.'
            if name=='native_object_type_query_replacements':
                evidence['retained_fallback_scope']='Exact Primary TU15 TypeQuery parent retains its whole reference body and24 authenticated retired-provider/child suffixes. Five local and three external inherited callback dispatch edges are unchanged; only finite genuine E0900 child paths are independently executed. Raw callback-bearing entry, other images/MMX, complete child domains, public DATA and full runtime/native-only closure remain separate.'
            if name=='native_projectile_function_query_replacements':
                symbol_sources=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbol_sources}
                evidence['retained_fallback_scope']='Exact CombatAction TU15 childless query keeps all91 raw/prepared callbacks and the five live bank symbol lookups. The retained control has the original ordered-SSE multiply at 100F61. Other images, parent callback owners and coherent runtime remain separate.'
            if name=='native_object_definition_mode_query_replacements':
                dependency='native_projectile_function_query_replacements.py'
                evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact TU15 CombatAction 165590 preserves all 23 callbacks, signed datum sentinel, live definition word and AL publication. The genuine Projectile sibling and public owner remain unchanged. Linked and inherited runtime routes are separate.'
            if name=='native_object_record_ratio_replacements':
                for dependency in ('native_flight_property_query_replacements.py','native_object_type_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact Primary TU15 107140 preserves 36 callbacks and both genuine 1046C0 edges. The x86 native and retained routes preserve the observed flag behavior of both original shifts. Other images/backends and inherited runtime qualification are separate.'
            if name=='native_object_ratio_caller_replacements':
                symbols=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbols}
                evidence['retained_fallback_scope']='Exact TU1.5 1046C0 retains all 406 original callbacks, independent FC510, 107140 and CombatAction child routes, and scoped observed x86 arithmetic flags. MapPack11 and unsupported x86 backends stay retained.'
            if name=='native_object_table_ratio_replacements':
                for dependency in ('native_object_definition_mode_query_replacements.py','native_projectile_function_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact TU15 CombatAction 1655F0 and 24B430 preserve 42 callbacks, the actual local call and independent reference controls. The exact 165590 and Projectile owners are preserved. Other images and inherited Primary 1046C0 remain separate.'
            if name=='native_map_world_property_query_replacements':
                symbol_sources=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbol_sources}
                evidence['retained_fallback_scope']='Exact paired TU15 MapWorld owners preserve all64 raw/prepared callbacks, the genuine 11E4DA to 1DB800 child edge and actual bank symbol resolution. Both retained scalar multiply sites use original destination-first SSE order. Same-address MapPack11 bodies are different and remain outside this binding.'
            if name=='native_device_property_query_replacements':
                dependency='native_map_world_property_query_replacements.py'
                evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                symbols=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbols}
                evidence['retained_fallback_scope']='Exact TU15 MapWorld 10BE20 preserves all 171 callbacks, five named symbol joins, distinct current/fresh C1 and spill policies, and the paired MapWorld sibling entries. Other images, linked dispatch and whole-runtime admission are separate.'
            if name=='native_node_function_progress_replacements':
                evidence['retained_fallback_scope']='Exact TU15 Primary D3950 preserves all 37 callbacks and both genuine required calls to the independently controlled AIFrame 1CF0E0 owner. The complete original source remains available through the D3950 reference control.'
            if name=='native_object_function_query_replacements':
                dependency='native_node_function_progress_replacements.py'
                evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                symbols=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbols}
                evidence['retained_fallback_scope']='Exact Primary TU1.5 C9070 preserves 144 callbacks, 16 bank symbol operations, three genuine D3750/D3950 calls and the independent 1046C0 inherited route. MapPack11 is excluded. The original source remains available through the independent C9070 reference control.'
            if name=='native_object_function_scalar_replacements':
                for dependency in ('native_object_function_query_replacements.py','native_object_type_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                symbols=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbols}
                evidence['retained_fallback_scope']='Exact Primary TU1.5 BCB70 preserves all 138 callbacks, two bank-key joins, four ordered shifts and the genuine 10FB70, 10DE40, BC8F0 and recursive BCB70 calls. The two retained child owners remain independently counted. MapPack11 keeps the authored path.'
            if name=='native_node_progress_query_replacements':
                evidence['retained_fallback_scope']='Exact AIFrame TU15 1CF0E0 keeps all 34 callbacks, actual local 1D63C0 ownership and the genuine 1CF108 to 1E3150 descriptor edge. The direct public case and the existing live ownership predicate are preserved. External D3950, other images and runtime closure remain separate.'
            if name=='native_vehicle_render_root_replacements':
                evidence['retained_fallback_scope']='Exact VehicleUpdate canonical BBE70/actual BCFB0 source owner preserves each live direct or literal callback and the genuine Phantom remap. Empty-callback admission keeps the existing native renderer bridge. The independent retained reference body and MapPack11 are unchanged; sibling World/ModelRender copies and full runtime remain separate.'
            if name=='native_vehicle_effect_ratio_replacements':
                dependency='native_vehicle_render_root_replacements.py'
                evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['actual_tu15_entry']='d3750'
                evidence['retained_fallback_scope']='Exact TU15 VehicleUpdate canonical D26C0 / actual D3750 keeps all 34 direct or prepared source callback sites and the genuine Phantom mapping. Unsupported FP/MMX and MapPack11 retain their authored route. The existing BBE70 sibling, exported chain and C9070 parent remain separate.'
            if name=='native_contact_predicate_replacements':
                evidence['retained_fallback_scope']='Exact TU1.5 LiveSupport 303800 preserves its complete ordered reference body and 25 authenticated retired-I/O suffixes. Raw callback-bearing entries stay retained; prepared quiet cursor and callback-free raw admissions remain distinct. The native path preserves the original logical x87 and real scratch store/reread transport. Mapped callers and coherent runtime closure remain separate.'
            if name=='native_flight_property_query_replacements':
                dependency='native_object_type_query_replacements.py'
                evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                symbol_sources=('src/rendering/effects/source_effect_compiled_symbols_tu15.cpp','src/rendering/effects/source_object_function_symbols_tu15.inc')
                evidence['validation_source_sha256']={p:hashlib.sha256((Path(root)/p).read_bytes()).hexdigest()for p in symbol_sources}
                evidence['retained_fallback_scope']='Exact TU15 Primary E0900 preserves all 76 raw/prepared boundaries and current/fresh C1 and spill policies. The independent DeathCycle copy, original hardware versus modeled observer retirement and other images are separate.'
            if name=='native_death_cycle_flight_query_replacements':
                for dependency in ('native_flight_property_query_replacements.py','native_object_type_query_replacements.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact TU15 DeathCycle E0900 preserves all 76 boundaries, current fixed selector keys, fresh named symbol joins and both status spill policies. Primary is a distinct owner; other images and full runtime remain separate.'
            if name=='native_vehicle_object_lookup_replacements':
                evidence['retained_fallback_scope']='Exact complete26-function VehicleEnvironment TU15 owner preserves all25 neighboring functions and public services-copy/load/save/FP/catch context. The childless raw view delivers all35 actual callbacks and preserves live source registers/flags/providers; finite actual public/opaqueMMX proofs remain scoped. Other owners/images, parent dependencies and full runtime/native-only/Original-frame closure remain separate.'
            if name=='native_object_material_lookup_replacements':
                evidence['validation_dependency_sha256']['native_object_body_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_object_body_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact object-material TU15 helper shares the childless lookup and retired-I/O continuations. Public18B4C0 parent, other lookup owners/images, opaque MMX and full runtime/descendant closure remain separate.'
            if name=='native_object_lookup_copies_replacements':
                evidence['validation_dependency_sha256']['native_object_body_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_object_body_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Four exact SceneDevice/SandboxSpecial/ParticleDraw/AI TU15 helpers share the childless lookup with independently authenticated complete owners and retired-I/O continuations. Parent routines, other owners/images, opaque MMX and complete runtime/descendant closure remain separate.'
            if name=='native_object_lookup_canonical_replacements':
                evidence['validation_dependency_sha256']['native_object_body_replacements.py']=hashlib.sha256((TOOL_ROOT/'native_object_body_replacements.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Two exact DirectDamage/Vitality TU15 helpers preserve canonical BBCB0 observation PCs while reading the actual TU15 object root. Raw callback-bearing entry remains retained; prepared and callback-free raw admission share authenticated retired-I/O suffixes. Parent routines, other images/MMX and complete runtime closure remain separate.'
            if name=='native_solver_hull_initialize_replacements':
                evidence['retained_fallback_scope']='Exact TU15 Primary initialization uses the shared89-site policy and43 authenticated retained source suffixes. Current/fresh spill profiles remain distinct; raw callback-bearing and unsupported provider/FP contexts retain the exact body. Genuine Pair/Append dispatch remains unchanged. Physical atomicity, child-internal policy domains and full313BB0/runtime closure remain separate.'
            if name=='native_solver_hull_line_replacements':
                evidence['retained_fallback_scope']='Exact TU15 Primary line reduction uses35 authenticated retained suffixes and exact current/fresh C1 profiles. Raw callbacks and unsupported provider/FP retain the source. Genuine Pair and Append dispatch are preserved. Child-internal policies, physical partial-vector faults and full313BB0 remain separate.'
            if name=='native_solver_hull_sample_replacements':
                evidence['retained_fallback_scope']='Exact TU15 Primary hull sampler uses19 authenticated retained suffixes and the exact current/fresh spill and C1 profiles. Raw callbacks and unsupported provider/FP retain the original source. The separate raw CPU frame, physical fault domains and full313BB0 remain distinct.'
            if name=='native_solver_hull_clip_replacements':
                evidence['retained_fallback_scope']='Exact TU15 Primary clipping uses 23 authenticated retired I/O continuations and distinct current/fresh C1 and spill profiles. Callback-bearing raw entry or unsupported FP/provider policies remain retained. The public value and original physical contracts are independently qualified; other images, physical fault domains and full313BB0 remain open.'
            if name=='native_solver_hull_clear_replacements':
                evidence['retained_fallback_scope']='Exact TU15 Primary table clearing preserves all 32 source sites and 12 authenticated retired-I/O continuations. Raw callback-bearing entry remains retained. Prepared or callback-free raw admission is bounded; physical page faults, other images and full313BB0/runtime closure remain separate.'
            if name=='native_solver_hull_minimum_replacements':
                evidence['retained_fallback_scope']='Exact actual TU15 Primary facet minimum retains 24 complete-instruction continuations and distinct current/fresh single-spill C1 profiles. Raw callbacks or unsupported FP remain on retained source. Physical partial-write, other images and full hull closure are separate.'
            if name=='native_solver_hull_edge_replacements':
                evidence['retained_fallback_scope']='Exact current/fresh TU15 Primary owner has43 authenticated retired-I/O continuations. Raw callback-bearing entry remains retained; prepared and callback-free raw modes use explicit tags. Genuine Pair/shape dispatch is preserved; child-internal policy/FP changes, other images/MMX, physical vector atomicity and full313BB0/runtime closure remain separate.'
            if arena_world:
                for dependency in('native_arena_initialize_replacements.py','native_arena_owner_policy_prototype.py','native_arena_initialize_continuation_replacements.py','native_arena_world_policy_prototype.py','native_arena_world_producer_inputs.py'):
                    evidence['validation_dependency_sha256'][dependency]=hashlib.sha256((TOOL_ROOT/dependency).read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact Arena-only125280/actual126640 preserves the independent callback-free Borrow fallback. Prepared callback-bearing World uses the private exact construction/const-table producer and authenticated retired-I/O continuations; raw callbacks, unknown producers/schedules, fixed1.1 owners and original physical/file/CRC descendant closure remain explicit retained scopes.'
            if name=='native_arena_initialize_replacements':
                evidence['validation_dependency_sha256']['native_arena_owner_policy_prototype.py']=hashlib.sha256((TOOL_ROOT/'native_arena_owner_policy_prototype.py').read_bytes()).hexdigest()
                evidence['retained_fallback_scope']='Exact Arena-only canonical125280/actual126640 default requires the callback-free private const-table Borrow and dense TU15 state. Callback-bearing World, fixed1.1 owners, original physical/file/CRC child closure and complete native dependency closure remain separate.'
            if bind(text)!=text:raise ValueError('Owning binder would change source; prefix is not fully installed')
            if targets is None:
                targets={}
                for signature,body in function_bodies(text):targets.setdefault(signature[1],[]).append((signature,body))
            matches=targets.get(entry,[])
            if len(matches)!=1:raise ValueError('Expected exactly one source-qualified target body')
            signature,body=matches[0];start=body.index('{')+1
            if nested:
                if body.count(prefix)!=1:raise ValueError('Expected one nested authenticated target admission')
            elif not(body[start:].startswith(prefix)or body[start:].startswith('\n'+prefix)):
                raise ValueError('Exact native prefix is not installed at target entry')
            original=module.canonical_body(int(entry,16),body)if name=='native_codec_copy_replacements' else module.canonical_body(entry,body)if family else module.canonical_body(body,entry)if entry_specific else module.canonical_body(body)
            if original==body:raise ValueError('Owning canonicalizer did not remove target prefix')
            unbound=text[:signature.start()]+original+text[signature.start()+len(body):]
            if arena_world:
                rebound=bind(module.remove_arena_world_initialize(text))
            elif name=='native_codec_translation_interpolation_replacements':
                rebound=bind(module.remove_codec_translation_interpolation(text))
            elif name=='native_solver_hull_line_replacements':
                rebound=bind(module.remove_solver_hull_line(text))
            elif name=='native_solver_hull_sample_replacements':
                rebound=bind(module.remove_solver_hull_sample(text))
            elif name=='native_solver_hull_minimum_replacements':
                rebound=bind(module.remove_solver_hull_minimum(text))
            elif name=='native_solver_hull_clear_replacements':
                rebound=bind(module.remove_solver_hull_clear(text))
            elif name=='native_solver_hull_clip_replacements':
                remove=module.remove_solver_hull_clip_prepared if bind==module.replace_solver_hull_clip_prepared else module.remove_solver_hull_clip
                rebound=bind(remove(text))
            elif name=='native_projectile_function_query_replacements':
                rebound=bind(module.remove_projectile_function_query(text))
            elif name=='native_object_definition_mode_query_replacements':
                rebound=bind(module.remove_object_definition_mode_query(text))
            elif name=='native_object_record_ratio_replacements':
                rebound=bind(module.remove_object_record_ratio(text))
            elif name=='native_indexed_datum_gate_replacements':
                rebound=bind(module.remove_indexed_datum_gate(text))
            elif name=='native_particle_type_filter_replacements':
                rebound=bind(module.remove_particle_type_filter(text))
            elif name=='native_particle_envelope_replacements':
                rebound=bind(module.remove_particle_envelope(text))
            elif name=='native_particle_record_replacements':
                rebound=bind(module.remove_particle_record(text))
            elif name=='native_object_ratio_caller_replacements':
                rebound=bind(module.remove_object_ratio_caller(text))
            elif name=='native_object_table_ratio_replacements':
                rebound=bind(module.remove_object_table_ratio(text))
            elif name=='native_map_world_property_query_replacements':
                rebound=bind(module.remove_map_world_property_queries(text))
            elif name=='native_device_property_query_replacements':
                rebound=bind(module.remove_device_property_query(text))
            elif name=='native_node_function_progress_replacements':
                rebound=bind(module.remove_node_function_progress(text))
            elif name=='native_object_function_query_replacements':
                rebound=bind(module.remove_object_function_query(text))
            elif name=='native_object_function_scalar_replacements':
                rebound=bind(module.remove_object_function_scalar(text))
            elif name=='native_node_progress_query_replacements':
                rebound=bind(module.remove_node_progress_query(text))
            elif name=='native_vehicle_render_root_replacements':
                rebound=bind(module.remove_vehicle_render_root(text))
            elif name=='native_vehicle_effect_ratio_replacements':
                rebound=bind(module.remove(text))
            elif name=='native_contact_predicate_replacements':
                rebound=bind(module.remove(text))
            elif name=='native_flight_property_query_replacements':
                rebound=bind(module.remove_flight_property_query(text))
            elif name=='native_death_cycle_flight_query_replacements':
                rebound=bind(module.remove_death_cycle_flight_query(text))
            elif name=='native_codec_scale_interpolation_replacements':
                rebound=bind(module.remove_codec_scale_interpolation(text))
            elif name=='native_codec_copy_replacements':
                rebound=bind(module.remove_codec_copy(text))
            elif closest or name in('native_simplex_paired_replacements','native_simplex_boundary_replacements','native_solver_hull_append_replacements','native_solver_hull_support_pair_replacements','native_solver_hull_edge_replacements','native_solver_hull_initialize_replacements','native_zero_shape_support_replacements','native_box_shape_support_replacements','native_sphere_ray_cast_replacements','native_packed_rotation_replacements','native_codec_translation_replacements','native_codec_scale_replacements','native_codec_rotation_interpolation_replacements','native_object_type_query_replacements','native_vehicle_object_lookup_replacements','native_object_body_replacements','native_object_material_lookup_replacements','native_object_lookup_copies_replacements','native_object_lookup_canonical_replacements','native_arena_initialize_replacements'):
                if text.count(module.INCLUDE)!=1:raise ValueError('Expected one exact native source-owner include')
                include_at=text.index(module.INCLUDE);unbound=unbound.replace(module.INCLUDE,'',1);rebound=bind(unbound)
                if rebound.count(module.INCLUDE)!=1:raise ValueError('Owning native installer did not restore its exact include')
                without_include=rebound.replace(module.INCLUDE,'',1)
                rebound=without_include[:include_at]+module.INCLUDE+without_include[include_at:]
            else:rebound=bind(unbound)
            if rebound!=text:raise ValueError('Owning binder did not authenticate and reconstruct the target body')
        except ValueError as error:evidence.update(authenticated=False,error=str(error))
        else:evidence.update(authenticated=True,target_body_rebinding_proved=True,
            admission_placement='inside_complete_authenticated_tu15_guard'if nested else'function_entry')
        result[entry]=evidence
    return result
