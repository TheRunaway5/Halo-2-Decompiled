#!/usr/bin/env python3
"""Stage native17A0F0 in removal, optionally following an earlier staged delta."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Handle binding seam changed: '+needle)
 return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--base-overlay',type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 base=a.base_overlay.resolve()if a.base_overlay else ROOT;assert base.is_relative_to(ROOT)
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')]
 before=[(base/f).read_text()for f in paths];source,header=before
 header=once(header,'#include "halo2/native_tu15_model_constraint_removal.hpp"',
  '#include "halo2/native_tu15_model_constraint_removal.hpp"\n#include "halo2/native_tu15_constraint_runtime_handles.hpp"')
 header=once(header,'    NativeTu15ModelConstraintRemovalStats constraint_removal;',
  '    NativeTu15ModelConstraintRemovalStats constraint_removal;\n    NativeTu15ConstraintRuntimeHandleStats constraint_runtime_handles;')
 needle='                        NativeTu15ModelConstraintRemovalServices removal;\n'
 source=once(source,needle,needle+'''                        removal.release_runtime_handle=[&](auto handle,auto deleting,auto&fp){
                            NativeTu15ConstraintRuntimeHandleServices lifetime;
                            native_tu15_release_constraint_runtime_handle(memory,handle,deleting,fp,
                                lifetime,observer,activity?&activity->constraint_runtime_handles:nullptr);
                        };
''')
 delta=[]
 for path,old,new in zip(paths,before,(source,header)):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  delta.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 patch=a.output/'native-constraint-handle-binding.diff';patch.write_text(''.join(delta))
 files=[ROOT/'src/physics/native_tu15_constraint_runtime_handles.cpp',ROOT/'include/halo2/native_tu15_constraint_runtime_handles.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-constraint-handle-binding-delta-v1',working_tree_modified=False,
  base_overlay=str(base),component_qualified=True,binding_qualified=False,base_hashes={str(f):sha(base/f)for f in paths},
  staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},
  patch=dict(path=str(patch),sha256=sha(patch)),component_report='build/test-results/native-constraint-runtime-handles-tu15/report.json',
  required_new_compile_units=['src/physics/native_tu15_constraint_runtime_handles.cpp'],
  remaining_owner_policy='Unbound actualC6120/17A4A0 children count and throw; no original parent replay',
  required_refusal_counters=['constraint_runtime_handles.unbound_tag_value_calls','constraint_runtime_handles.unbound_destroy_calls'],
  parent_removal_positive_handle_caller_qualified=False,owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
