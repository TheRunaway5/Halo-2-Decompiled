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
    ('bf0c0','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('bfb90','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('b8780','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('b87b0','native_object_node_rebuild_replacements','install_native_object_nodes'),
    ('1a5080','native_async_order_replacements','install_native_async_order'),
    ('122020','native_async_insert_replacements','install_native_async_insert'),
    ('1232c0','native_async_insert_replacements','install_native_async_insert'),
    ('1236a0','native_async_promote_replacements','install_native_async_promote'),
    ('24dec0','native_instanced_ray_child_replacements','replace_instanced_ray_child'),
    ('1f1d20','native_polygon_support_replacements','replace_polygon_support'),
    ('1f1da0','native_polygon_features_replacements','replace_polygon_features'),
    ('16b810','native_camera_instance_admission_replacements','replace_camera_instance_admission'),
    ('186e30','native_camera_instance_bitmap_replacements','replace_camera_instance_bitmap'),
    ('16b9a0','native_camera_collision_publication_replacements','replace_camera_collision_publication'),
    ('120fa0','native_segment_sphere_replacements','replace_segment_sphere'),
    ('16c000','native_camera_instance_query_replacements','replace_camera_instance_query'),
    ('24def0','native_instanced_ray_callback_replacements','replace_instanced_ray_callbacks'),
    ('24df20','native_instanced_ray_callback_replacements','replace_instanced_ray_callbacks'),
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
        entry_specific=name in('native_async_insert_replacements','native_instanced_ray_callback_replacements')
        prefix=module.prefix(module.PINS[int(entry,16)][0])if nested else module.PREFIXES[entry]if family else module.prefix(entry)if entry_specific else module.PREFIX
        if prefix not in text:continue
        tool=TOOL_ROOT/(name+'.py')
        evidence={'binder':'tools/'+name+'.py','binder_sha256':hashlib.sha256(tool.read_bytes()).hexdigest(),
            'source_sha256':hashlib.sha256(text.encode()).hexdigest(),'entry_symbol':entry,
            'validation_dependency_sha256':{p:hashlib.sha256((TOOL_ROOT/p).read_bytes()).hexdigest()for p in('native_source_prefixes.py','native_selected_operand_binding.py','native_halfword_bit_query_replacements.py','specialize_native_operands.py')},
            'runtime_include_source_sha256':runtime_include_hashes(Path(root),''.join(module.INCLUDES)if family else getattr(module,'INCLUDE','')),
            'runtime_include_scope':'Private source include closure; conditional preprocessing and execution are not proved.',
            'qualification':'complete source/body/resolver/dispatcher checks implemented by the owning exact binder',
            'runtime_admission_proved':False,'native_dependency_closure_proved':False}
        try:
            bind=getattr(module,function)
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
            original=module.canonical_body(entry,body)if family else module.canonical_body(body,entry)if entry_specific else module.canonical_body(body)
            if original==body:raise ValueError('Owning canonicalizer did not remove target prefix')
            unbound=text[:signature.start()]+original+text[signature.start()+len(body):]
            if bind(unbound)!=text:raise ValueError('Owning binder did not authenticate and reconstruct the target body')
        except ValueError as error:evidence.update(authenticated=False,error=str(error))
        else:evidence.update(authenticated=True,target_body_rebinding_proved=True,
            admission_placement='inside_complete_authenticated_tu15_guard'if nested else'function_entry')
        result[entry]=evidence
    return result
