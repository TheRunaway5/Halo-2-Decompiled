#!/usr/bin/env python3
"""Stage the native31D930 constraint-step callback on an immutable source."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
SHARED=['include/halo2/native_tu15_constraint_step_residuals.hpp','src/physics/native_tu15_constraint_step_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']
UNITS=['native_tu15_joint_constraint_build','native_tu15_joint_constraint_schema','native_tu15_joint_constraint_angular_row','native_tu15_joint_constraint_motor','native_tu15_joint_constraint_limits','native_tu15_constraint_angle','native_tu15_linear_jacobian_emitter']
NEW=[*['src/physics/'+n+'.cpp'for n in UNITS],*['include/halo2/'+n+'.hpp'for n in UNITS],'include/halo2/native_tu15_joint_constraint_build_binding.hpp','include/halo2/native_tu15_linear_jacobian_host.hpp']
INHERITED=['src/physics/native_tu15_joint_constraint_numeric_internal.hpp','tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','unsupported_fp':'unsupported_fp','kernel_entries':'parent.calls','kernel_completed':'parent.completed','headers':'parent.headers','angular_rows':'parent.angular_rows','motors':'parent.motors','linear_requests':'parent.linear_requests','linear_calls':'linear.calls','linear_completed':'linear.completed','linear_rows':'linear.rows','linear_refusals':'linear.refusals','limits':'parent.limits','positive_unwraps':'parent.positive_unwraps','negative_unwraps':'parent.negative_unwraps','unbound_linear':'parent.unbound_linear','linear_unaligned':'linear.unaligned_vectors','linear_unsupported_fp':'linear.unsupported_fp_profiles'}
PREFIX='native_joint_builder_'
def once(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';before={};bases={}
 for relative in SHARED:
  source=a.source_root/relative
  if not source.exists()and relative=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  before[relative]=source.read_text();bases[relative]=sha(source);target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 for relative in NEW+INHERITED:
  source=(ROOT if relative in NEW else a.source_root)/relative
  if relative=='src/physics/native_tu15_linear_jacobian_emitter.cpp':source=ROOT/'analysis/physics/native-linear-jacobian-host-stage-20261102/staged'/relative
  target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 r=SHARED[0];s=once(before[r],'#include <optional>','#include "halo2/native_tu15_joint_constraint_build_binding.hpp"\n#include <optional>');s=once(s,'  NativeTu15ConstraintStepStats step;','  NativeTu15ConstraintStepStats step;\n  NativeTu15BoundJointConstraintBuildActivity joint_builder;');(stage/r).write_text(s)
 r=SHARED[1];s=before[r];old='''      }else{
        if(owner.identity==0x31d930u)
          native_tu15_observe_constraint_frame_build_input(m,{constraint.identity,jacobian_input,packet,
            {word(m,packet),word(m,packet+4u),word(m,packet+8u),word(m,packet+12u)},context,depth});
        const std::array args{jacobian_input,packet};
        call(owner.identity,args,constraint.identity,8,context);
      }''';new='''      }else if(owner.identity==0x31d930u){
        native_tu15_observe_constraint_frame_build_input(m,{constraint.identity,jacobian_input,packet,
          {word(m,packet),word(m,packet+4u),word(m,packet+8u),word(m,packet+12u)},context,depth});
        native_tu15_build_bound_joint_constraint(m,constraint.identity,{jacobian_input},{packet},
          context,depth,observe,activity?&activity->joint_builder:nullptr);
      }else{
        const std::array args{jacobian_input,packet};
        call(owner.identity,args,constraint.identity,8,context);
      }''';s=once(s,old,new);(stage/r).write_text(s)
 r=SHARED[2];anchor='  std::uint64_t native_constraint_arena_allocations{}';s=once(before[r],anchor,'  std::uint64_t '+','.join(PREFIX+k+'{}'for k in FIELDS)+';\n'+anchor);(stage/r).write_text(s)
 r=SHARED[3];anchor='      stats->native_constraint_arena_allocations+=activity.native_arena_allocations;';s=once(before[r],anchor,''.join('      stats->'+PREFIX+k+'+=activity.joint_builder.'+v+';\n'for k,v in FIELDS.items())+anchor);(stage/r).write_text(s)
 r=SHARED[4];anchor='  std::uint64_t native_constraint_arena_allocations{}';s=once(before[r],anchor,'  std::uint64_t '+','.join(PREFIX+k+'{}'for k in FIELDS)+';\n'+anchor);(stage/r).write_text(s)
 r=SHARED[5];anchor='    result.native_constraint_arena_allocations=impl_->native_physics_integrations.native_constraint_arena_allocations;';s=once(before[r],anchor,''.join('    result.'+PREFIX+k+'=impl_->native_physics_integrations.'+PREFIX+k+';\n'for k in FIELDS)+anchor);(stage/r).write_text(s)
 r=SHARED[6];s=before[r];anchor='        {"native_constraint_arena_allocations",native_frames.native_constraint_arena_allocations},';lines=''.join('        {"'+PREFIX+k+'",native_frames.'+PREFIX+k+'},\n'for k in FIELDS);s=once(s,anchor,lines+anchor,2)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_JOINT_CONSTRAINT_BUILDER")) {
    const auto calls=native_frames.native_joint_builder_calls;
    const bool native_joint=calls>0&&native_frames.native_joint_builder_attempts==calls&&
      native_frames.native_joint_builder_kernel_entries==calls&&native_frames.native_joint_builder_kernel_completed==calls&&
      native_frames.native_joint_builder_headers==calls&&native_frames.native_joint_builder_angular_rows==2u*calls&&
      native_frames.native_joint_builder_motors==calls&&native_frames.native_joint_builder_linear_requests==calls&&
      native_frames.native_joint_builder_linear_calls==calls&&native_frames.native_joint_builder_linear_completed==calls&&
      native_frames.native_joint_builder_linear_rows==3u*calls&&native_frames.native_joint_builder_refusals==0&&
      native_frames.native_joint_builder_unsupported_fp==0&&native_frames.native_joint_builder_linear_refusals==0&&
      native_frames.native_joint_builder_unbound_linear==0&&native_frames.native_joint_builder_linear_unaligned==0&&
      native_frames.native_joint_builder_linear_unsupported_fp==0&&
      !native_frames.residual_owner_calls.contains(0x31d930u);
    report["native_joint_constraint_builder_gate"]=native_joint;passed&=native_joint;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_JOINT_CONSTRAINT_LIMITS")) {
    const bool native_limits=native_frames.native_joint_builder_limits>0&&
      native_frames.native_joint_builder_attempts==native_frames.native_joint_builder_calls&&
      native_frames.native_joint_builder_refusals==0;
    report["native_joint_constraint_limits_gate"]=native_limits;passed&=native_limits;
  }
''';anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_RELEASE")) {';s=once(s,anchor,gate+anchor);(stage/r).write_text(s)
 r=SHARED[7];sources=' '.join('src/physics/'+n+'.cpp'for n in UNITS);s=before[r]+'\n# Sole native31D930 parent and all selected schema/Jacobian arithmetic.\ntarget_sources(halo2_native_pair_consumer PRIVATE '+sources+')\nif(MSVC)\n    set_source_files_properties('+sources+' PROPERTIES COMPILE_OPTIONS /fp:strict)\nelse()\n    set_source_files_properties('+sources+' PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")\nendif()\n';(stage/r).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(stage/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in SHARED))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,live_qualified=False,base_source_root=str(a.source_root),base_hashes=bases,delta_files=SHARED,new_files=NEW,inherited_files=INHERITED,fields=FIELDS,prefix=PREFIX,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Only actual31D930 constraint-step owner becomes native; readonly actual-input observer and four-cursor recovery on normal/throw retained. No fallback or executor accepted by the sole native helper. Whole source+same helper component and genuine1478 actual banks are separate from World model and live acceptance. Other constraint owners retain their prior explicitly measured routes; no universal native-only claim.'))
 print(dict(prepared=True,shared=len(SHARED),new=len(NEW)))
if __name__=='__main__':main()
