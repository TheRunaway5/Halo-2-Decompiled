#!/usr/bin/env python3
"""Stage the native22D0A0 capacity callback without editing production files."""
import argparse,difflib,json,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Binding seam changed: '+needle)
 return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')]
 before=[(ROOT/f).read_text()for f in paths];source,header=before
 header=once(header,'#include "halo2/native_tu15_model_constraint_capacity.hpp"',
  '#include "halo2/native_tu15_model_constraint_capacity.hpp"\n#include "halo2/native_tu15_model_constraint_removal.hpp"')
 header=once(header,'    NativeTu15ModelConstraintCapacityStats constraint_capacity;',
  '    NativeTu15ModelConstraintCapacityStats constraint_capacity;\n    NativeTu15ModelConstraintRemovalStats constraint_removal;')
 source=once(source,'                    NativeTu15ModelConstraintCapacityServices capacity;\n',
  '''                    NativeTu15ModelConstraintCapacityServices capacity;
                    capacity.remove_constraint=[&](auto constraint,auto&state){
                        // Unknown17A0F0/22FEA0 lifetime children count and
                        // throw inside this native parent. No old owner runs.
                        NativeTu15ModelConstraintRemovalServices removal;
                        native_tu15_remove_model_constraint(memory,constraint,state,depth,
                            removal,observer,activity?&activity->constraint_removal:nullptr);
                    };
''')
 delta=[]
 for path,old,new in zip(paths,before,(source,header)):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  delta.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 patch=a.output/'native-constraint-removal-binding.diff';patch.write_text(''.join(delta))
 units=['model_constraint_removal','constraint_reference_release','constraint_body_slots','constraint_scalar_cache']
 files=[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units]+[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units]
 files+=[ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'include/halo2/native_tu15_data_array_delete.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-constraint-removal-binding-delta-v1',working_tree_modified=False,
  component_qualified=True,binding_qualified=False,base_hashes={str(f):sha(ROOT/f)for f in paths},
  staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},
  patch=dict(path=str(patch),sha256=sha(patch)),component_report='build/test-results/native-model-constraint-removal-tu15/report.json',
  required_new_compile_units=[str(f.relative_to(ROOT))for f in files if f.suffix=='.cpp'],
  remaining_owner_policy='Unbound actual17A0F0/22FEA0 children count and throw; no whole-parent replay',
  required_refusal_counters=['constraint_removal.unbound_handle_releases','constraint_removal.unbound_runtime_removals','constraint_removal.reference.unbound_runtime_removals'],
  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
