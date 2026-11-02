#!/usr/bin/env python3
"""Stage native constraint lookup/capacity services after the frozen parent delta."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
from prepare_native_activation_binding_refresh_tu15 import once
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',type=Path,required=True);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.base=a.base.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')];before=[(a.base/f).read_text()for f in paths];source,header=before
 header=once(header,'#include "halo2/native_tu15_body_constraint_update.hpp"','#include "halo2/native_tu15_body_constraint_update.hpp"\n#include "halo2/native_tu15_model_constraint_lookup.hpp"\n#include "halo2/native_tu15_model_constraint_capacity.hpp"')
 header=once(header,'    NativeTu15ModelConstraintUpdateStats body_constraints;','    NativeTu15ModelConstraintUpdateStats body_constraints;\n    NativeTu15ModelConstraintLookupStats constraint_lookup;\n    NativeTu15ModelConstraintCapacityStats constraint_capacity;')
 source=once(source,'                NativeTu15ModelConstraintServices constraints;','''                NativeTu15ModelConstraintServices constraints;
                constraints.find_existing=[&](auto component,auto request,auto flags,auto&fp){
                    return native_tu15_find_model_constraint(memory,component,request,flags,fp,
                        activity?&activity->constraint_lookup:nullptr);
                };
                constraints.allow_creation=[&](auto component,auto slot,auto extent,auto&fp){
                    NativeTu15ModelConstraintCapacityServices capacity;
                    return native_tu15_admit_model_constraint(memory,component,slot,extent,fp,
                        capacity,observer,activity?&activity->constraint_capacity:nullptr);
                };''')
 patch=[]
 for path,old,new in zip(paths,before,[source,header]):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new);patch.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 delta=a.output/'activation-constraint-services-binding.diff';delta.write_text(''.join(patch));files=[ROOT/f'src/physics/native_tu15_model_constraint_{name}.cpp'for name in ['lookup','capacity']]+[ROOT/f'include/halo2/native_tu15_model_constraint_{name}.hpp'for name in ['lookup','capacity']]+[Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-constraint-services-binding-delta-v1',qualified=False,working_tree_modified=False,base_overlay=str(a.base),base_hashes={str(p):sha(a.base/p)for p in paths},staged_hashes={str(p):sha(a.output/'staged'/p)for p in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},patch=dict(path=str(delta),sha256=sha(delta)),required_compile_units=['src/physics/native_tu15_model_constraint_lookup.cpp','src/physics/native_tu15_model_constraint_capacity.cpp'],whole_parent_fallback=False,unbound_owners=['0x22D0A0 capacity lifetime','0x22CEA0 creation'],owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]));print(delta)
if __name__=='__main__':main()
