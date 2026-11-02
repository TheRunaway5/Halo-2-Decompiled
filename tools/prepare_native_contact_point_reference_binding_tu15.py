#!/usr/bin/env python3
"""Stage narrow native22E1B0 point composition against an explicit frozen root."""
import argparse,difflib,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_contact_point_constraint_reference_calls':'reference.calls',
 'native_contact_point_constraint_reference_decrements':'reference.decrements',
 'native_contact_point_constraint_reference_runtime_removals':'reference.runtime_removals',
 'native_contact_point_constraint_reference_runtime_refusals':'reference.unbound_runtime_removals'}
def sha(raw):return hashlib.sha256(raw).hexdigest()
def change(text,before,after,n=1):assert text.count(before)==n,(before,text.count(before));return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';oldpins={};newpins={};patch=[]
 paths=('include/halo2/native_tu15_contact_point_binding.hpp','src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp')
 for name in paths:
  source=a.source_root/name
  if not source.exists()and name=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=source.read_bytes();text=raw.decode();oldpins[name]=sha(raw)
  if name.endswith('native_tu15_contact_point_binding.hpp'):
   anchor='#include "halo2/native_tu15_gameplay_contact_removal.hpp"';text=change(text,anchor,anchor+'\n#include "halo2/native_tu15_constraint_reference_release.hpp"')
   anchor='    NativeTu15GameplayContactRemovalStats gameplay;';text=change(text,anchor,anchor+'\n    NativeTu15ConstraintReferenceReleaseStats reference;')
   before='gameplay.release_constraint_reference=[&](auto,auto,auto&){unqualified("gameplay constraint-reference profile22E1B0",0x22e1b0u);};'
   diagnostic='native_tu15_observe_constraint_reference_input(memory,{instance,datum,state,incoming_x87_depth});'if'native_tu15_constraint_reference_input_observer.hpp'in text else''
   if before not in text:
    before='gameplay.release_constraint_reference=[&](auto instance,auto datum,auto&context){native_tu15_observe_constraint_reference_input(memory,{instance,datum,context,incoming_x87_depth});unqualified("gameplay constraint-reference profile22E1B0",0x22e1b0u);};'
   after='''gameplay.release_constraint_reference=[&](auto instance,auto datum,auto&state){
            '''+diagnostic+'''
            // Empty runtime service preserves the source conditional child
            // boundary: the native core counts/refuses22FEA0 when selected.
            native_tu15_release_constraint_reference(memory,instance,datum,state,
                incoming_x87_depth,{},observe,&result.reference);
        };''';text=change(text,before,after)
   text=text.replace('Unknown/effect/part/zero-reference owners refuse; there is no CPU fallback.','Unknown/runtime-effect/part/zero-reference owners refuse; there is no CPU fallback.')
  elif name.endswith('world_integration_residuals.cpp'):
   anchor='          stats->native_contact_point_effect_requests+=activity.gameplay.constraint_releases;'
   text=change(text,anchor,anchor+'\n'+'\n'.join('          stats->'+f+'+=activity.'+m+';'for f,m in FIELDS.items()))
  elif name.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_contact_point_attempts{}';i=text.index(anchor)
   text=text[:i]+'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+text[i:]
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_contact_point_effect_requests=impl_->native_physics_integrations.native_contact_point_effect_requests;'
   text=change(text,anchor,anchor+'\n'+'\n'.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';'for f in FIELDS))
  else:
   anchor='        {"native_contact_point_effect_requests",native_frames.native_contact_point_effect_requests},'
   text=change(text,anchor,anchor+'\n'+'\n'.join('        {"'+f+'",native_frames.'+f+'},'for f in FIELDS),2)
   before='native_frames.native_contact_point_effect_requests==0&&native_frames.native_contact_point_part_requests==0&&'
   after='''native_frames.native_contact_point_effect_requests==native_frames.native_contact_point_constraint_reference_calls&&
      native_frames.native_contact_point_constraint_reference_decrements==native_frames.native_contact_point_constraint_reference_calls&&
      native_frames.native_contact_point_constraint_reference_runtime_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x22e1b0u)&&native_frames.native_contact_point_part_requests==0&&'''
   text=change(text,before,after)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_POINT_REFERENCE_RELEASE")) {
    const bool reference_passed=native_frames.native_contact_point_calls>0&&
      native_frames.native_contact_point_refusals==0&&native_frames.native_contact_point_unqualified_services==0&&
      native_frames.native_contact_point_constraint_reference_calls>0&&
      native_frames.native_contact_point_effect_requests==native_frames.native_contact_point_constraint_reference_calls&&
      native_frames.native_contact_point_constraint_reference_decrements==native_frames.native_contact_point_constraint_reference_calls&&
      native_frames.native_contact_point_constraint_reference_runtime_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x22e1b0u);
    report["native_contact_point_reference_release_gate"]=reference_passed;passed&=reference_passed;
  }
''';text=change(text,anchor,gate+anchor)
  destination=stage/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_text(text);newpins[name]=sha(text.encode());patch+=difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name)
 observer='include/halo2/native_tu15_constraint_reference_input_observer.hpp'
 if(a.source_root/observer).exists():
  raw=(a.source_root/observer).read_bytes();target=stage/observer;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw);newpins[observer]=sha(raw)
 (a.output/'native-contact-point-reference-binding.diff').write_text(''.join(patch))
 proof=ROOT/'analysis/physics/native-constraint-reference-actual1470-10-20261102/report.json';d=json.loads(proof.read_text())
 assert d['passed']
 (a.output/'source-pins.json').write_text(json.dumps(dict(base_source_root=str(a.source_root.resolve()),base_hashes=oldpins,staged_hashes=newpins,fields=FIELDS,actual_reference_proof=str(proof.relative_to(ROOT)),actual_reference_proof_sha256=sha(proof.read_bytes()),source_units_added=[],production_applied=False,live_qualified=False,scope='Native22E1B0 core only; actualrefs5/flag10zero authenticated. Kernel retains generic condition; selected runtime22FEA0 remains counted unbound refusal. Other point/merge/regroup/base-lock/profile guards unchanged. Preserve MP10 readonly observer before the native release when merging its diagnostic hunk.'),indent=2,sort_keys=True)+'\n')
 print(a.output)
if __name__=='__main__':main()
