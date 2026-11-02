#!/usr/bin/env python3
"""Stage the qualified native phantom deletion/spatial composition privately."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
from prepare_native_phantom_deletion_world_binding_tu15 import change
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_phantom_object_spatial_flag_queries':'spatial.flag_queries',
 'native_phantom_object_spatial_packets':'spatial.packets',
 'native_phantom_object_spatial_chain_updates':'spatial.chain_updates',
 'native_phantom_object_spatial_chain_rows':'spatial.chain_rows',
 'native_phantom_object_spatial_record_searches':'spatial.record_searches',
 'native_phantom_object_spatial_searched_rows':'spatial.searched_rows',
 'native_phantom_object_spatial_matched_rows':'spatial.matched_rows',
 'native_phantom_object_spatial_missing_rows':'spatial.missing_rows',
 'native_phantom_object_spatial_word_copies':'spatial.word_copies',
 'native_phantom_object_spatial_byte_copies':'spatial.byte_copies',
 'native_phantom_object_spatial_zero_copies':'spatial.zero_copies'}
NEW=('include/halo2/native_tu15_object_spatial_packet.hpp','include/halo2/native_tu15_object_deletion_spatial_binding.hpp','src/physics/native_tu15_object_spatial_packet.cpp')
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base-stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(a.base_stage/'staged',stage);oldpins={};patch=[]
 for name in('src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake'):
  path=stage/name;raw=path.read_bytes();text=raw.decode();oldpins[name]=sha(raw)
  if name.endswith('world_integration_residuals.cpp'):
   text=change(text,'#include "halo2/native_tu15_object_deletion_detaching_binding.hpp"','#include "halo2/native_tu15_object_deletion_spatial_binding.hpp"')
   text=change(text,'NativeTu15ObjectDeletionDetachingBindingActivity deletion_activity;','NativeTu15ObjectDeletionSpatialBindingActivity deletion_activity;')
   text=change(text,'native_tu15_delete_detaching_bound_object(m,datum,deletion_fp,observe,&deletion_activity);','native_tu15_delete_spatially_bound_object(m,datum,deletion_fp,observe,&deletion_activity);')
   anchor='              stats->native_phantom_object_reference_clears+=deletion_activity.deletion.reference_clears;';text=change(text,anchor,anchor+'\n'+'\n'.join('              stats->'+f+'+=deletion_activity.'+v+';'for f,v in FIELDS.items()))
  elif name.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_phantom_object_deletion_attempts{}';i=text.index(anchor);text=text[:i]+'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+text[i:]
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_phantom_object_reference_clears=impl_->native_physics_integrations.native_phantom_object_reference_clears;';text=change(text,anchor,anchor+'\n'+'\n'.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';'for f in FIELDS))
  elif name.endswith('PhysicsLiveWorldTu15.cmake'):
   anchor='target_include_directories(halo2_native_pair_consumer PUBLIC include)';text=change(text,anchor,'target_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_object_spatial_packet.cpp)\n'+anchor)
  else:
   anchor='        {"native_phantom_object_reference_clears",native_frames.native_phantom_object_reference_clears},';text=change(text,anchor,anchor+'\n'+'\n'.join('        {"'+f+'",native_frames.'+f+'},'for f in FIELDS),2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PHANTOM_OBJECT_SPATIAL")) {
    const bool spatial=native_frames.native_phantom_object_deletion_calls>0&&
      native_frames.native_phantom_object_deletion_calls==native_frames.native_phantom_object_deletion_attempts&&
      native_frames.native_phantom_object_deletion_calls==native_frames.native_phantom_added_delete_requests&&
      native_frames.native_phantom_object_deletion_refusals==0&&native_frames.native_phantom_object_deletion_unbound_services==0&&
      native_frames.native_phantom_object_spatial_queries>0&&
      native_frames.native_phantom_object_spatial_queries==native_frames.native_phantom_object_spatial_packets&&
      native_frames.native_phantom_object_spatial_packets==native_frames.native_phantom_object_spatial_chain_updates&&
      native_frames.native_phantom_added_listener_refusals==0&&native_frames.native_phantom_object_detachment_refusals==0&&
      !native_frames.residual_owner_calls.contains(0xba570u)&&!native_frames.residual_owner_calls.contains(0xc0f60u)&&
      !native_frames.residual_owner_calls.contains(0xba910u)&&!native_frames.residual_owner_calls.contains(0x16e290u)&&
      !native_frames.residual_owner_calls.contains(0x1d30a0u)&&!native_frames.residual_owner_calls.contains(0x29fff0u);
    report["native_phantom_object_spatial_gate"]=spatial;passed&=spatial;
  }
''';text=change(text,anchor,gate+anchor)
  path.write_text(text);patch+=difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name)
 for name in NEW:
  target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 hashes={str(p.relative_to(stage)):sha(p.read_bytes())for p in stage.rglob('*')if p.is_file()};(a.output/'native-phantom-spatial-binding.diff').write_text(''.join(patch))
 proof=ROOT/'analysis/physics/native-phantom-added-spatial-actual1472-composition-20261102-first/report.json';r=json.loads(proof.read_text());assert r['passed']and r['full_return']and r['actual_first_boundary']is None
 for name in NEW:assert r['source_hashes'][name]==hashes[name],name
 (a.output/'source-pins.json').write_text(json.dumps(dict(base_stage=str(a.base_stage.resolve()),base_stage_pins_sha256=sha((a.base_stage/'source-pins.json').read_bytes()),base_hashes=oldpins,staged_hashes=hashes,fields=FIELDS,actual_fullreturn_proof=str(proof.relative_to(ROOT)),actual_fullreturn_proof_sha256=sha(proof.read_bytes()),production_applied=False,live_qualified=False,full_deletion_return_qualified=True,scope='Exact production native282700/B1870/BA570/C0F60/spatial helper composed on actual1472 typed BEFORE; all114953457B,116reads14stores/FP. Generic12 counters preserved;11 spatial activity fields added. Other actual deletion children still explicitly refuse. Host20B computed DATA, no guest scratch/control bytes or fallback.'),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':raise SystemExit(main())
