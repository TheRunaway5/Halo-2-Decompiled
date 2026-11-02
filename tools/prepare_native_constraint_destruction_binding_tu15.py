#!/usr/bin/env python3
"""Stage native17A4A0/C6120 and the shared17A030 wrapper as one source cohort."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Destruction binding seam changed: '+needle)
 return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp'),Path('src/physics/native_tu15_constraint_runtime_handles.cpp')]
 before=[(ROOT/f).read_text()for f in paths];source,header,_=before
 header=once(header,'#include "halo2/native_tu15_constraint_runtime_handles.hpp"',
  '#include "halo2/native_tu15_constraint_runtime_handles.hpp"\n#include "halo2/native_tu15_constraint_handle_destruction.hpp"\n#include "halo2/native_tu15_constraint_tag_values.hpp"')
 header=once(header,'    NativeTu15ConstraintRuntimeHandleStats constraint_runtime_handles;',
  '    NativeTu15ConstraintRuntimeHandleStats constraint_runtime_handles;\n    NativeTu15ConstraintHandleDestructionStats constraint_handle_destruction;\n    NativeTu15ConstraintTagValueStats constraint_tag_values;')
 needle='                            NativeTu15ConstraintRuntimeHandleServices lifetime;\n'
 source=once(source,needle,needle+'''                            lifetime.publish_zero_tagged_value=[&](auto reference,auto context,auto scalar,auto&state){
                                native_tu15_publish_constraint_tag_value(memory,reference,context,scalar,state,
                                    observer,activity?&activity->constraint_tag_values:nullptr);
                            };
                            lifetime.destroy_handle=[&](auto datum,auto&state){
                                NativeTu15ConstraintHandleDestructionServices destruction;
                                destruction.tagged_values.publish_zero_tagged_value=lifetime.publish_zero_tagged_value;
                                native_tu15_destroy_constraint_runtime_handle(memory,datum,state,destruction,
                                    observer,activity?&activity->constraint_handle_destruction:nullptr);
                            };
''')
 source=once(source,'// Unknown17A0F0/22FEA0 lifetime children count and\n                        // throw inside this native parent. No old owner runs.',
  '// Remaining child-link/22FEA0 lifetime owners count\n                        // and throw. No admitted native parent replays an old owner.')
 api=ROOT/'analysis/physics/native-constraint-handle-cleanup-api-20261101/staged'/paths[2]
 after=[source,header,api.read_text()];delta=[]
 for path,old,new in zip(paths,before,after):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  delta.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 patch=a.output/'native-constraint-destruction-binding.diff';patch.write_text(''.join(delta))
 units=['constraint_handle_destruction','constraint_tag_values'];files=[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units]+[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units]
 files+=[ROOT/'include/halo2/native_tu15_constraint_tag_cleanup.hpp',ROOT/'include/halo2/native_tu15_constraint_runtime_handles.hpp',ROOT/'src/core/native_tu15_data_array_iteration.cpp',ROOT/'include/halo2/native_tu15_data_array_iteration.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-constraint-destruction-binding-delta-v1',working_tree_modified=False,
  component_qualified=True,binding_qualified=False,base_hashes={str(f):sha(ROOT/f)for f in paths},
  staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},
  patch=dict(path=str(patch),sha256=sha(patch)),component_reports=['build/test-results/native-constraint-handle-destruction-tu15/report.json','build/test-results/native-constraint-handle-cleanup-api-tu15/report.json','build/test-results/native-constraint-tag-values-tu15/report.json'],
  required_new_compile_units=[f'src/physics/native_tu15_{n}.cpp'for n in units],
  existing_native_iteration_owner='halo2_native_tu15_data_array_delete already owns data_array_iteration.cpp',
  required_refusal_counters=['constraint_handle_destruction.unbound_child_unlinks','constraint_handle_destruction.unbound_child_deletions','constraint_handle_destruction.tagged_values.unbound_tag_value_calls'],
  positive_tagged_value_store_qualified=False,positive_deferred_destruction_qualified=False,
  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
