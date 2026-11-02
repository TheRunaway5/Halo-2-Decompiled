#!/usr/bin/env python3
"""Stage native BA570+C0F60 inside the qualified282700 World callback."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_phantom_object_deletion_attempts':'attempts',
 'native_phantom_object_deletion_calls':'completed',
 'native_phantom_object_deletion_refusals':'refusals',
 'native_phantom_object_deletion_unbound_services':'deletion.unbound_services+detachment.unbound_services',
 'native_phantom_object_detachment_calls':'detachment.calls',
 'native_phantom_object_detachment_objects':'detachment.objects',
 'native_phantom_object_detachment_refusals':'detachment.unbound_services',
 'native_phantom_object_spatial_queries':'deletion.spatial_queries',
 'native_phantom_object_tracking_clears':'deletion.tracking_clears',
 'native_phantom_object_player_clears':'deletion.player_clears',
 'native_phantom_object_schedule_updates':'deletion.schedule_updates',
 'native_phantom_object_reference_clears':'deletion.reference_clears'}
NEW=('include/halo2/native_tu15_object_deletion.hpp','include/halo2/native_tu15_object_deletion_binding.hpp','include/halo2/native_tu15_object_detachment.hpp','include/halo2/native_tu15_object_deletion_detaching_binding.hpp','src/physics/native_tu15_object_deletion.cpp','src/physics/native_tu15_object_detachment.cpp')
def sha(raw):return hashlib.sha256(raw).hexdigest()
def change(text,before,after,n=1):assert text.count(before)==n,(before,text.count(before));return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base-stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(a.base_stage/'staged',stage);oldpins={};patch=[]
 for name in('src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake'):
  path=stage/name;raw=path.read_bytes();text=raw.decode();oldpins[name]=sha(raw)
  if name.endswith('world_integration_residuals.cpp'):
   anchor='#include "halo2/native_tu15_phantom_added_binding.hpp"';text=change(text,anchor,anchor+'\n#include "halo2/native_tu15_object_deletion_detaching_binding.hpp"')
   anchor='            NativeTu15PhantomAddedBindingActivity activity;';text=change(text,anchor,anchor+'''
            NativeTu15ObjectDeletionDetachingBindingActivity deletion_activity;
            NativeTu15PhantomAddedListenerServices deletion;
            deletion.delete_object=[&](auto datum,auto&deletion_fp){
              native_tu15_delete_detaching_bound_object(m,datum,deletion_fp,observe,&deletion_activity);
            };''')
   anchor='              stats->native_phantom_added_unqualified_deletions+=activity.unqualified_deletions;';text=change(text,anchor,anchor+'\n'+'\n'.join('              stats->'+f+'+='+'+'.join('deletion_activity.'+term for term in value.split('+'))+';'for f,value in FIELDS.items()))
   text=change(text,'current_fp,state.x87_depth,{},observe,&activity);collect_added();','current_fp,state.x87_depth,deletion,observe,&activity);collect_added();')
  elif name.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_phantom_added_listener_attempts{}';i=text.index(anchor);text=text[:i]+'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+text[i:]
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_phantom_added_unqualified_deletions=impl_->native_physics_integrations.native_phantom_added_unqualified_deletions;';text=change(text,anchor,anchor+'\n'+'\n'.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';'for f in FIELDS))
  elif name.endswith('PhysicsLiveWorldTu15.cmake'):
   anchor='target_include_directories(halo2_native_pair_consumer PUBLIC include)';text=change(text,anchor,'target_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_object_deletion.cpp src/physics/native_tu15_object_detachment.cpp)\n'+anchor)
  else:
   anchor='        {"native_phantom_added_unqualified_deletions",native_frames.native_phantom_added_unqualified_deletions},';text=change(text,anchor,anchor+'\n'+'\n'.join('        {"'+f+'",native_frames.'+f+'},'for f in FIELDS),2)
   anchor='native_frames.native_phantom_added_listener_refusals==0&&native_frames.native_phantom_added_unqualified_deletions==0&&'
   text=change(text,anchor,anchor+'''
      native_frames.native_phantom_added_delete_requests==native_frames.native_phantom_object_deletion_attempts&&
      native_frames.native_phantom_object_deletion_calls==native_frames.native_phantom_object_deletion_attempts&&
      native_frames.native_phantom_object_deletion_refusals==0&&native_frames.native_phantom_object_deletion_unbound_services==0&&''')
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PHANTOM_OBJECT_DELETION")) {
    const bool deletion=native_frames.native_phantom_object_deletion_calls>0&&
      native_frames.native_phantom_object_deletion_attempts==native_frames.native_phantom_object_deletion_calls&&
      native_frames.native_phantom_object_deletion_attempts==native_frames.native_phantom_added_delete_requests&&
      native_frames.native_phantom_object_deletion_refusals==0&&native_frames.native_phantom_object_deletion_unbound_services==0&&
      native_frames.native_phantom_object_detachment_calls>0&&native_frames.native_phantom_object_detachment_refusals==0&&
      native_frames.native_phantom_added_listener_refusals==0&&
      !native_frames.residual_owner_calls.contains(0xba570u)&&!native_frames.residual_owner_calls.contains(0xc0f60u)&&
      !native_frames.residual_owner_calls.contains(0xbbd10u)&&!native_frames.residual_owner_calls.contains(0xbbd50u);
    report["native_phantom_object_deletion_gate"]=deletion;passed&=deletion;
  }
''';text=change(text,anchor,gate+anchor)
  path.write_text(text);patch+=difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name)
 for name in NEW:
  target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 hashes={str(p.relative_to(stage)):sha(p.read_bytes())for p in stage.rglob('*')if p.is_file()};(a.output/'native-phantom-deletion-binding.diff').write_text(''.join(patch))
 proof=ROOT/'analysis/physics/native-phantom-added-detaching-actual1472-composition-20261102-first/report.json';r=json.loads(proof.read_text());assert r['passed']and not r['full_return']and r['actual_first_boundary']['owner']==0xba910
 for name in NEW:assert r['source_hashes'][name]==hashes[name],name
 (a.output/'source-pins.json').write_text(json.dumps(dict(base_stage=str(a.base_stage.resolve()),base_stage_pins_sha256=sha((a.base_stage/'source-pins.json').read_bytes()),base_hashes=oldpins,staged_hashes=hashes,fields=FIELDS,actual_prefix_proof=str(proof.relative_to(ROOT)),actual_prefix_proof_sha256=sha(proof.read_bytes()),production_applied=False,live_qualified=False,full_deletion_return_qualified=False,scope='NativeBA570+C0F60 sameproductionadapter boundONLYthrough282700 service. Actual47read/2store/full114953457B prefix toBA910. MissingBA910andremainingchildren stillthrow; no fakepacket/privatecontrolbytes. ExistingPART/phantomobserver/capture-independent source preserved; genericdeletion+detachmentfieldnames suitableforlaterspatialextension.'),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':raise SystemExit(main())
