#!/usr/bin/env python3
"""Stage native22CEA0 construction in the existing typed activation chain."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Creation binding seam changed: '+needle)
 return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')]
 before=[(ROOT/f).read_text()for f in paths];source,header=before
 header=once(header,'#include "halo2/native_tu15_model_constraint_removal.hpp"',
  '#include "halo2/native_tu15_model_constraint_removal.hpp"\n#include "halo2/native_tu15_model_constraint_creation.hpp"')
 header=once(header,'    NativeTu15ModelConstraintRemovalStats constraint_removal;',
  '    NativeTu15ModelConstraintRemovalStats constraint_removal;\n    NativeTu15ModelConstraintCreationStats constraint_creation;\n    std::uint64_t unavailable_constraint_creation_storage{};')
 needle='                native_tu15_update_body_constraints(memory,datum,{{scratch.shape_scratch_first}},'
 source=once(source,needle,'''                constraints.create=[&](auto request,auto flags,auto&fp){
                    if(scratch.shape_scratch_bytes<192u){
                        if(activity)++activity->unavailable_constraint_creation_storage;
                        throw std::runtime_error("Native TU1.5 constraint creation requires192 caller-owned request/frame DATA bytes");
                    }
                    NativeTu15ModelConstraintCreationServices creation;
                    return native_tu15_create_model_constraint(memory,request,flags,
                        {scratch.shape_scratch_first+64u,scratch.shape_scratch_first+128u},fp,depth,
                        creation,observer,activity?&activity->constraint_creation:nullptr);
                };
'''+needle)
 delta=[]
 for path,old,new in zip(paths,before,(source,header)):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  delta.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 patch=a.output/'native-constraint-creation-binding.diff';patch.write_text(''.join(delta))
 units=['model_constraint_creation','constraint_local_points','vector_normalize','constraint_motion_compare']
 files=[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units]+[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units]
 files+=[ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',ROOT/'include/halo2/native_tu15_camera_projection.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-constraint-creation-binding-delta-v1',working_tree_modified=False,
  component_qualified=True,binding_qualified=False,base_hashes={str(f):sha(ROOT/f)for f in paths},
  staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},
  patch=dict(path=str(patch),sha256=sha(patch)),component_report='build/test-results/native-model-constraint-creation-declared-tu15/report.json',
  required_new_compile_units=[str(f.relative_to(ROOT))for f in files if f.suffix=='.cpp'and f.parent==ROOT/'src/physics'],
  existing_native_inverse_owner='halo2_physics_world_runtime:src/rendering/world/native_tu15_camera_projection.cpp; reuse symbol144090, do not duplicate source',
  required_caller_owned_data_bytes=192,preserved_x87_depth_profile=[0,1,2,3],
  remaining_owner_policy='Unbound actual22FEA0 reference child counts and throws; no original parent replay',
  required_refusal_counters=['unavailable_constraint_creation_storage','constraint_creation.unsupported_numerical_profiles','constraint_creation.reference.unbound_runtime_removals'],
  genuine_authored_positive_caller_qualified=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
