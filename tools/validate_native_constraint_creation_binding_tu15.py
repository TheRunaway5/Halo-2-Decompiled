#!/usr/bin/env python3
"""Compose the isolated native creation delta with the actual F2091 transport."""
from pathlib import Path
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Creation transport seam changed: '+needle[:100])
 return text.replace(needle,replacement,1)
def main():
 base=Path(__file__).with_name('validate_native_object_motion_activation_tu15.py');text=base.read_text()
 text=once(text,"units=['object_motion','island_activation'", "units=['model_constraint_removal','constraint_reference_release','constraint_body_slots','constraint_scalar_cache','model_constraint_creation','constraint_local_points','vector_normalize','constraint_motion_compare','object_motion','island_activation'")
 needle="if new_constraint_binding:files.extend([ROOT/'src/core/native_tu15_data_array_allocate.cpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp'])"
 text=once(text,needle,needle+"\n if new_constraint_binding:files.extend([ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'include/halo2/native_tu15_data_array_delete.hpp',ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',ROOT/'include/halo2/native_tu15_camera_projection.hpp',ROOT/'tools/validate_native_object_motion_activation_tu15.py'])")
 needle="if new_constraint_binding:shutil.copyfile(ROOT/'src/core/native_tu15_data_array_allocate.cpp',project/'data_array_allocate.cpp')"
 text=once(text,needle,needle+"\n  if new_constraint_binding:shutil.copyfile(ROOT/'src/core/native_tu15_data_array_delete.cpp',project/'data_array_delete.cpp');shutil.copyfile(ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',project/'inverse.cpp')")
 text=once(text,"units+(['data_array_allocate']if new_constraint_binding else[])","units+(['data_array_allocate','data_array_delete','inverse']if new_constraint_binding else[])")
 needle='  driver.write_text(';assert text.count(needle)==1
 injection='''  owner='transport.binding'if a.transport else'activity'
  marker='\\n std::size_t different{},compared{};';assert body.count(marker)==1
  body=body.replace(marker,'\\n report.update(J{{"binding_native_constraint_removals",'+owner+'.constraint_removal.calls},{"binding_unbound_constraint_handle_releases",'+owner+'.constraint_removal.unbound_handle_releases},{"binding_unbound_constraint_runtime_removals",'+owner+'.constraint_removal.unbound_runtime_removals},{"binding_unbound_constraint_reference_runtime_removals",'+owner+'.constraint_removal.reference.unbound_runtime_removals},{"binding_native_constraint_creation_calls",'+owner+'.constraint_creation.calls},{"binding_native_constraint_created",'+owner+'.constraint_creation.created},{"binding_unavailable_constraint_creation_storage",'+owner+'.unavailable_constraint_creation_storage},{"binding_unsupported_constraint_creation_numerics",'+owner+'.constraint_creation.unsupported_numerical_profiles},{"binding_unbound_creation_reference_runtime_removals",'+owner+'.constraint_creation.reference.unbound_runtime_removals}});'+marker,1)
'''
 text=once(text,needle,injection+needle)
 text=once(text,'report.update(passed=all(','report.update(new_native_creation_binding=True,genuine_authored_positive_caller_qualified=False,capacity_removal_positive_input_qualified=False,passed=all(')
 namespace={'__file__':__file__,'__name__':'native_constraint_creation_binding_base'}
 exec(compile(text,str(base),'exec'),namespace);return namespace['main']()
if __name__=='__main__':raise SystemExit(main())
