#!/usr/bin/env python3
"""Stage native22D460 activation wiring without changing the frozen live binding."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
from prepare_native_activation_binding_refresh_tu15 import once
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')];before=[(ROOT/f).read_text()for f in paths];source,header=before
 header=once(header,'#include "halo2/native_tu15_body_policy.hpp"','#include "halo2/native_tu15_body_policy.hpp"\n#include "halo2/native_tu15_body_constraint_update.hpp"')
 header=once(header,'    NativeTu15BodyPolicyStats body_policy;','    NativeTu15BodyPolicyStats body_policy;\n    NativeTu15ModelConstraintUpdateStats body_constraints;')
 source=once(source,'                const std::array arguments{datum};call(0x22d460u,arguments,{},4,context);','''                // Reuse ordinary DATA storage only after policy/bounds work
                // has returned. Positive creation owners remain explicitly
                // unbound until their actual native closure is qualified.
                NativeTu15ModelConstraintServices constraints;
                native_tu15_update_body_constraints(memory,datum,{{scratch.shape_scratch_first}},
                    context,constraints,observer,activity?&activity->body_constraints:nullptr);''')
 patch=[]
 for path,old,new in zip(paths,before,[source,header]):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new);patch.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 delta=a.output/'activation-body-constraint-binding.diff';delta.write_text(''.join(patch));files=[ROOT/'include/halo2/native_tu15_body_constraint_update.hpp',ROOT/'src/physics/native_tu15_body_constraint_update.cpp',ROOT/'src/core/native_tu15_data_array_allocate.cpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp',ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'include/halo2/native_tu15_geometry.hpp',Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-body-constraint-binding-delta-v1',qualified=False,working_tree_modified=False,base_hashes={str(p):sha(ROOT/p)for p in paths},staged_hashes={str(p):sha(a.output/'staged'/p)for p in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},patch=dict(path=str(delta),sha256=sha(delta)),required_compile_units=['src/physics/native_tu15_body_constraint_update.cpp','src/core/native_tu15_data_array_allocate.cpp','src/physics/native_tu15_geometry.cpp'],whole_parent_fallback=False,positive_creation_owners_unbound=['0x1DD710','0x1DD7B0','0x22CEA0'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]));print(delta)
if __name__=='__main__':main()
