#!/usr/bin/env python3
"""Stage native effect selection atop the qualified bounded runtime seam."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Effect selector binding seam changed: '+needle[:120])
 return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 import json
 inherited=ROOT/'analysis/physics/native-activation-constraint-runtime-kind1-binding-20261101';receipt=json.loads((inherited/'source-pins.json').read_text());assert receipt['binding_qualified']
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')]
 before=[(ROOT/path).read_text()for path in paths]
 assert all(sha(ROOT/path)==receipt['base_hashes'][str(path)]for path in paths)or all(sha(ROOT/path)==receipt['staged_hashes'][str(path)]for path in paths)
 for path in paths:assert sha(inherited/'staged'/path)==receipt['staged_hashes'][str(path)]
 source,header=[(inherited/'staged'/path).read_text()for path in paths]
 header=once(header,'#include "halo2/native_tu15_constraint_runtime_update.hpp"',
  '#include "halo2/native_tu15_constraint_runtime_update.hpp"\n#include "halo2/native_tu15_constraint_effect_selector.hpp"')
 header=once(header,'    NativeTu15ConstraintRuntimeUpdateStats constraint_runtime_update;',
  '    NativeTu15ConstraintRuntimeUpdateStats constraint_runtime_update;\n    NativeTu15ConstraintEffectSelectorStats constraint_effect_selector;')
 source=once(source,'                    runtime.handles=lifetime;','''                    runtime.handles=lifetime;
                    runtime.select_effects=[&](auto request,auto&state){
                        native_tu15_select_constraint_effects(memory,request,
                            {scratch.shape_scratch_first+416u,scratch.shape_scratch_first+432u,scratch.shape_scratch_first+440u},
                            state,observer,activity?&activity->constraint_effect_selector:nullptr);
                    };''')
 after=[source,header];delta=[]
 for path,old,new in zip(paths,before,after):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  delta.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 patch=a.output/'native-constraint-effect-selector-binding.diff';patch.write_text(''.join(delta))
 units=['constraint_runtime_update','constraint_runtime_parameters','constraint_geometry_frame','constraint_point_basis','constraint_auxiliary_math','constraint_effect_selector','constraint_effect_location']
 files=[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units]+[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units]+[Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-bounded-constraint-effect-selector-binding-v1',working_tree_modified=False,
  component_qualified=True,binding_qualified=False,base_hashes={str(f):sha(ROOT/f)for f in paths},
  staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},
  inherited_runtime_receipt=dict(path=str(inherited/'source-pins.json'),sha256=sha(inherited/'source-pins.json')),
  patch=dict(path=str(patch),sha256=sha(patch)),required_new_compile_units=[f'src/physics/native_tu15_{n}.cpp'for n in units],
  required_refusal_counters=receipt['required_refusal_counters']+['constraint_effect_selector.unknown_constant_targets'],
  production_positive_parent_profiles_admitted=False,live_reference_owner_exercised=False,
  remaining=['Camera-qualified whole selector positive input','Native typed Audio/Effects service owners remain separate required boundaries',
   'Positive runtime state/effect/frame parent qualification','Genuine authored+80 caller and actual live reference-release flow'],
  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
