#!/usr/bin/env python3
"""Stage bounded native22FEA0 binding without changing live source files."""
import argparse,difflib,os
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_contact_agent_dispatch_tu15 import sha,save
def once(text,needle,replacement):
 if text.count(needle)!=1:raise RuntimeError('Runtime binding seam changed: '+needle[:120])
 return text.replace(needle,replacement,1)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False)
 paths=[Path('src/physics/native_tu15_object_activation_residuals.cpp'),Path('include/halo2/native_tu15_object_activation_residuals.hpp')]
 before=[(ROOT/f).read_text()for f in paths];source,header=before
 header=once(header,'#include "halo2/native_tu15_model_constraint_creation.hpp"',
  '#include "halo2/native_tu15_model_constraint_creation.hpp"\n#include "halo2/native_tu15_constraint_runtime_update.hpp"')
 header=once(header,'    NativeTu15ModelConstraintCreationStats constraint_creation;',
  '    NativeTu15ModelConstraintCreationStats constraint_creation;\n    NativeTu15ConstraintRuntimeUpdateStats constraint_runtime_update;\n    std::uint64_t unavailable_constraint_runtime_storage{},unqualified_constraint_runtime_profiles{};')
 first=source.index('                            NativeTu15ConstraintRuntimeHandleServices lifetime;')
 last=source.index('                            native_tu15_release_constraint_runtime_handle(',first)
 block=source[first:last]
 source=source[:first]+source[last:]
 block='\n'.join(line[12:]if line.startswith('            ')else line for line in block.split('\n'))
 insertion=block+'''                NativeTu15ConstraintReferenceReleaseServices reference;
                reference.remove_runtime_constraint=[&](auto instance,auto datum,auto slots,auto&fp){
                    const auto word=[&](std::uint32_t p,unsigned width=4){
                        std::array<std::byte,4>bytes{};memory.read(p,std::span(bytes).first(width));
                        std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
                        return value;
                    };
                    if(scratch.shape_scratch_bytes<512u){
                        if(activity)++activity->unavailable_constraint_runtime_storage;
                        throw std::runtime_error("Native TU1.5 constraint runtime update requires512 caller-owned DATA bytes");
                    }
                    // Only the positively inventoried counter-only profile
                    // is currently admitted by this production binding.
                    // Other native parent branches remain explicit refusals
                    // until source-authenticated positive inputs qualify them.
                    const auto kind=word(instance+0xcu,1);
                    if(word(instance+0x11u,1)||kind!=1u||word(instance+0x30u)!=0xffffffffu||
                       word(instance+0x34u)!=0xffffffffu||word(instance+0x38u)!=0xffffffffu||word(instance+0x3cu)!=0xffffffffu){
                        if(activity)++activity->unqualified_constraint_runtime_profiles;
                        throw std::runtime_error("Native TU1.5 constraint runtime update requires its positive state/effect/frame qualification");
                    }
                    NativeTu15ConstraintRuntimeUpdateServices runtime;
                    runtime.handles=lifetime;
                    native_tu15_update_constraint_runtime(memory,instance,datum,slots,
                        {scratch.shape_scratch_first+192u,{scratch.shape_scratch_first+416u,scratch.shape_scratch_first+464u}},
                        fp,depth,runtime,observer,activity?&activity->constraint_runtime_update:nullptr);
                };
'''
 source=once(source,'                NativeTu15ModelConstraintServices constraints;',insertion+'                NativeTu15ModelConstraintServices constraints;')
 source=once(source,'                        NativeTu15ModelConstraintRemovalServices removal;',
  '                        NativeTu15ModelConstraintRemovalServices removal;\n                        removal.reference=reference;')
 source=once(source,'                    NativeTu15ModelConstraintCreationServices creation;',
  '                    NativeTu15ModelConstraintCreationServices creation;\n                    creation.reference=reference;')
 after=[source,header];delta=[]
 for path,old,new in zip(paths,before,after):
  target=a.output/'staged'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(new)
  delta.extend(difflib.unified_diff(old.splitlines(keepends=True),new.splitlines(keepends=True),fromfile='a/'+str(path),tofile='b/'+str(path)))
 patch=a.output/'native-constraint-runtime-binding.diff';patch.write_text(''.join(delta))
 units=['constraint_runtime_update','constraint_runtime_parameters','constraint_geometry_frame','constraint_point_basis','constraint_auxiliary_math']
 files=[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units]+[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units]+[Path(__file__)]
 save(a.output/'source-pins.json',dict(schema='native-tu15-bounded-constraint-runtime-binding-v1',working_tree_modified=False,
  component_qualified=True,positive_runtime_state_qualified=False,binding_qualified=False,
  base_hashes={str(f):sha(ROOT/f)for f in paths},staged_hashes={str(f):sha(a.output/'staged'/f)for f in paths},
  source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},patch=dict(path=str(patch),sha256=sha(patch)),
  required_new_compile_units=[f'src/physics/native_tu15_{n}.cpp'for n in units],
  required_refusal_counters=['unavailable_constraint_runtime_storage','unqualified_constraint_runtime_profiles',
   'constraint_runtime_update.unsupported_profiles','constraint_runtime_update.unbound_effect_selections',
   'constraint_runtime_update.unbound_effect_publications','constraint_runtime_update.unbound_handle_creations',
   'constraint_runtime_update.handles.unbound_tag_value_calls','constraint_runtime_update.handles.unbound_destroy_calls'],
  admission='actual flag11==0,kind1,links30/34 and handles38/3C None;512 scratch bytes;native numerical profile',
  remaining=['Positive native runtime state/effect/frame parent branches','Native22EEC0/18C440/178E30 dependency closure','Actual reference-release caller and visible multiplayer'],
  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[]))
 print(patch);return 0
if __name__=='__main__':raise SystemExit(main())
