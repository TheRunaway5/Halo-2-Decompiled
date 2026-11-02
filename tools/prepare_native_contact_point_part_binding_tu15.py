#!/usr/bin/env python3
"""Prepare a narrow PART/conveyor point binding from an explicit frozen root."""
import argparse,difflib,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_contact_point_part_calls':'part.attempts',
 'native_contact_point_part_completions':'part.completed',
 'native_contact_point_part_refusals':'part.refusals',
 'native_contact_point_part_inactive':'part.response.inactive',
 'native_contact_point_part_material_queries':'part.response.conveyor.material_queries',
 'native_contact_point_part_response_vectors':'part.response.response_vectors',
 'native_contact_point_part_zero_properties':'part.response.zero_properties',
 'native_contact_point_part_vector_properties':'part.response.vector_properties',
 'native_contact_point_part_conveyor_bindings':'part.response.conveyor.bindings',
 'native_contact_point_part_function_calls':'part.function.calls',
 'native_contact_point_part_function_refusals':'part.function.unsupported_kinds',
 'native_contact_point_part_function_fp_refusals':'part.function.unsupported_fp_profiles'}
UNITS=('src/physics/native_tu15_body_properties.cpp','src/physics/native_tu15_empty_part_contact.cpp','src/physics/native_tu15_part_contact_response.cpp','src/physics/native_tu15_surface_conveyor.cpp','src/core/native_tu15_function_evaluation.cpp')
HEADERS=tuple('include/halo2/'+n for n in ('native_tu15_body_properties.hpp','native_tu15_empty_part_contact.hpp','native_tu15_part_contact_response.hpp','native_tu15_surface_conveyor.hpp','native_tu15_function_evaluation.hpp','native_tu15_part_contact_binding.hpp'))
def sha(raw):return hashlib.sha256(raw).hexdigest()
def change(text,before,after,n=1):assert text.count(before)==n,(before,text.count(before));return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';oldpins={};newpins={};patch=[]
 paths=('include/halo2/native_tu15_contact_point_binding.hpp','src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake')
 for name in paths:
  source=a.source_root/name
  if not source.exists()and name=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=source.read_bytes();text=raw.decode();oldpins[name]=sha(raw)
  if name.endswith('native_tu15_contact_point_binding.hpp'):
   anchor='#include "halo2/native_tu15_constraint_reference_release.hpp"';text=change(text,anchor,anchor+'\n#include "halo2/native_tu15_part_contact_binding.hpp"\n#include <bit>')
   anchor='    NativeTu15ConstraintReferenceReleaseStats reference;';text=change(text,anchor,anchor+'\n    NativeTu15PartContactBindingActivity part;')
   before='gameplay.release_part_contact=[&](auto,auto,auto,auto&){unqualified("part-contact release1E0A60",0x1e0a60u);};'
   after='''gameplay.release_part_contact=[&](auto slot,auto signed_part,auto component,auto& state){
            // Source invokes the gameplay listener before325D90 initializes
            // removal_info, before the world result, and before detach_info.
            // These owned DATA aliases therefore have disjoint live phases.
            const NativeTu15PartContactResponseScratch part_scratch{
                scratch.removal_info,scratch.removal_info+12u,
                {scratch.removal_info+16u,scratch.result_byte}};
            if(!valid(scratch.removal_info,20)||!valid(scratch.result_byte,4)||
               overlap(scratch.removal_info,20,scratch.event,20)||
               overlap(scratch.result_byte,4,scratch.event,20)||
               overlap(scratch.removal_info,20,scratch.result_byte,4))
                bad_profile("Native TU1.5 PART binding requires owned20B info/gap and4B result DATA");
            native_tu15_update_bound_part_contact_response(memory,slot,
                std::bit_cast<std::int32_t>(std::uint32_t(signed_part)),component,
                part_scratch,state,incoming_x87_depth,allocation,observe,&result.part);
        };''';text=change(text,before,after)
   text=text.replace('Unknown/runtime-effect/part/zero-reference owners refuse;', 'Unknown/runtime-effect/zero-reference owners and unsupported authored-function kinds refuse;')
   text=text.replace('// point metadata consumer has returned before325E90 fully rewrites that16B.', '// point metadata consumer has returned before325E90 fully rewrites that16B.\n// Selected PART callbacks require removal_info to own20B (including its4B gap)\n// and result_byte to own4B. Their temporary values die before the later consumers.')
  elif name.endswith('world_integration_residuals.cpp'):
   anchor='          stats->native_contact_point_part_requests+=activity.gameplay.part_releases;';text=change(text,anchor,anchor+'\n'+'\n'.join('          stats->'+f+'+=activity.'+m+';'for f,m in FIELDS.items()))
  elif name.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_contact_point_attempts{}';i=text.index(anchor);text=text[:i]+'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+text[i:]
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_contact_point_part_requests=impl_->native_physics_integrations.native_contact_point_part_requests;';text=change(text,anchor,anchor+'\n'+'\n'.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';'for f in FIELDS))
  elif name.endswith('PhysicsLiveWorldTu15.cmake'):
   anchor='target_include_directories(halo2_native_pair_consumer PUBLIC include)';text=change(text,anchor,'target_sources(halo2_native_pair_consumer PRIVATE\n    '+'\n    '.join(UNITS)+')\n'+anchor)
  else:
   anchor='        {"native_contact_point_part_requests",native_frames.native_contact_point_part_requests},';text=change(text,anchor,anchor+'\n'+'\n'.join('        {"'+f+'",native_frames.'+f+'},'for f in FIELDS),2)
   before='native_frames.native_contact_point_part_requests==0&&'
   after='''native_frames.native_contact_point_part_requests==native_frames.native_contact_point_part_calls&&
      native_frames.native_contact_point_part_completions==native_frames.native_contact_point_part_calls&&
      native_frames.native_contact_point_part_refusals==0&&
      native_frames.native_contact_point_part_function_refusals==0&&native_frames.native_contact_point_part_function_fp_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x1e0a60u)&&!native_frames.residual_owner_calls.contains(0x184c50u)&&
      !native_frames.residual_owner_calls.contains(0x355d0u)&&!native_frames.residual_owner_calls.contains(0x13de70u)&&''';text=change(text,before,after)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_PART_RESPONSE")||std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_CONVEYOR")) {
    const bool part=native_frames.native_contact_point_part_calls>0&&native_frames.native_contact_point_refusals==0&&
      native_frames.native_contact_point_part_requests==native_frames.native_contact_point_part_calls&&
      native_frames.native_contact_point_part_completions==native_frames.native_contact_point_part_calls&&
      native_frames.native_contact_point_part_refusals==0&&native_frames.native_contact_point_part_function_refusals==0&&
      native_frames.native_contact_point_part_function_fp_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x1e0a60u)&&!native_frames.residual_owner_calls.contains(0x184c50u)&&
      !native_frames.residual_owner_calls.contains(0x355d0u)&&!native_frames.residual_owner_calls.contains(0x13de70u);
    report["native_contact_point_part_response_gate"]=part;passed&=part;
    if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_CONVEYOR")) {
      const bool conveyor=part&&native_frames.native_contact_point_part_material_queries>0&&
        native_frames.native_contact_point_part_response_vectors>0&&native_frames.native_contact_point_part_vector_properties>0&&
        native_frames.native_contact_point_part_conveyor_bindings>0&&native_frames.native_contact_point_part_function_calls>0;
      report["native_contact_point_conveyor_gate"]=conveyor;passed&=conveyor;
    }
  }
''';text=change(text,anchor,gate+anchor)
  destination=stage/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_text(text);newpins[name]=sha(text.encode());patch+=difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name)
 for name in(*UNITS,*HEADERS):
  raw=(ROOT/name).read_bytes();destination=stage/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(raw);newpins[name]=sha(raw)
 (a.output/'native-contact-point-part-binding.diff').write_text(''.join(patch))
 proof=ROOT/'analysis/physics/native-part-contact-production-binding-tu15-20261102-first/report.json';receipt=json.loads(proof.read_text());assert receipt['passed']
 for name in(*UNITS,*HEADERS):assert receipt['source_sha256'][name]==newpins[name],name
 (a.output/'source-pins.json').write_text(json.dumps(dict(base_source_root=str(a.source_root.resolve()),base_hashes=oldpins,staged_hashes=newpins,fields=FIELDS,production_builder_proof=str(proof.relative_to(ROOT)),production_builder_proof_sha256=sha(proof.read_bytes()),source_units_added=UNITS,production_applied=False,live_qualified=False,scope='Bounded native PART/conveyor13DE70kind0/1; genericsource-kind admission remains inside sole native evaluator. Unsupported positive function kinds count/refuse. World event/info/result owned scratch phases explicit; preserved sourceprivate rawRED, manager/reverse/regroup/phantom gates unchanged.'),indent=2,sort_keys=True)+'\n')
 print(a.output)
if __name__=='__main__':main()
