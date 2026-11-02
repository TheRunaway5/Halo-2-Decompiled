#!/usr/bin/env python3
"""Stage native282700+B1870 with a typed unqualified BA570 refusal."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={
 'native_phantom_added_listener_attempts':'attempts',
 'native_phantom_added_listener_calls':'completed',
 'native_phantom_added_listener_refusals':'refusals',
 'native_phantom_added_components_marked':'listener.marked_components',
 'native_phantom_added_protection_queries':'listener.protection.queries',
 'native_phantom_added_objects_protected':'listener.protected_objects',
 'native_phantom_added_delete_requests':'listener.delete_requests',
 'native_phantom_added_unqualified_deletions':'unqualified_deletions'}
NEW=('include/halo2/native_tu15_phantom_added_listener.hpp','src/physics/native_tu15_phantom_added_listener.cpp','include/halo2/native_tu15_phantom_added_binding.hpp','include/halo2/native_tu15_phantom_added_input_observer.hpp')
def sha(raw):return hashlib.sha256(raw).hexdigest()
def change(text,before,after,n=1):assert text.count(before)==n,(before,text.count(before));return text.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--base-stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(a.base_stage/'staged',stage);oldpins={};newpins={};patch=[]
 for name in('src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake'):
  source=stage/name
  if not source.exists():
   source=a.source_root/name
   if not source.exists()and name=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  raw=source.read_bytes();text=raw.decode();oldpins[name]=sha(raw)
  if name.endswith('world_integration_residuals.cpp'):
   anchor='#include "halo2/native_tu15_phantom_overlap_add.hpp"';text=change(text,anchor,anchor+'\n#include "halo2/native_tu15_phantom_added_binding.hpp"')
   before='''overlap.added=[&](auto listener,auto target,auto event,auto&){
          if(target.identity==0x2e4d60u){native_tu15_ignore_removed_phantom_overlap();return;}'''
   after='''overlap.added=[&](auto listener,auto target,auto event,auto&current_fp){
          if(target.identity==0x2e4d60u){native_tu15_ignore_removed_phantom_overlap();return;}
          if(target.identity==0x282700u){
            NativeTu15PhantomAddedBindingActivity activity;
            const auto collect_added=[&]{if(stats){
'''+''.join('              stats->'+f+'+=activity.'+m+';\n'for f,m in FIELDS.items())+'''            }};
            try{native_tu15_notify_bound_gameplay_phantom_added(m,listener,event,
              current_fp,state.x87_depth,{},observe,&activity);collect_added();}
            catch(...){collect_added();throw;}
            return;
          }''';text=change(text,before,after)
  elif name.endswith(('native_tu15_world_integration.hpp','native_tu15_physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_phantom_pair_create_notifications{}';i=text.index(anchor);text=text[:i]+'  std::uint64_t '+','.join(f+'{}'for f in FIELDS)+';\n'+text[i:]
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_phantom_pair_create_notifications=impl_->native_physics_integrations.native_phantom_pair_create_notifications;';text=change(text,anchor,anchor+'\n'+'\n'.join('    result.'+f+'=impl_->native_physics_integrations.'+f+';'for f in FIELDS))
  elif name.endswith('PhysicsLiveWorldTu15.cmake'):
   anchor='target_include_directories(halo2_native_pair_consumer PUBLIC include)';text=change(text,anchor,'target_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_phantom_added_listener.cpp)\n'+anchor)
  else:
   anchor='        {"native_phantom_pair_create_notifications",native_frames.native_phantom_pair_create_notifications},';text=change(text,anchor,anchor+'\n'+'\n'.join('        {"'+f+'",native_frames.'+f+'},'for f in FIELDS),2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_GAMEPLAY_PHANTOM_ADDED")) {
    const bool added=native_frames.native_phantom_added_listener_calls>0&&
      native_frames.native_phantom_added_listener_attempts==native_frames.native_phantom_added_listener_calls&&
      native_frames.native_phantom_added_listener_refusals==0&&native_frames.native_phantom_added_unqualified_deletions==0&&
      native_frames.native_phantom_pair_create_unqualified_owners==0&&native_frames.native_pair_creation_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x282700u)&&!native_frames.residual_owner_calls.contains(0xb1870u)&&
      (native_frames.native_phantom_added_delete_requests==0||!native_frames.residual_owner_calls.contains(0xba570u));
    report["native_gameplay_phantom_added_gate"]=added;passed&=added;
  }
''';text=change(text,anchor,gate+anchor)
  destination=stage/name;destination.parent.mkdir(parents=True,exist_ok=True);destination.write_text(text);patch+=difflib.unified_diff(raw.decode().splitlines(True),text.splitlines(True),fromfile='a/'+name,tofile='b/'+name)
 for name in NEW:
  target=stage/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/name,target)
 for path in stage.rglob('*'):
  if path.is_file():newpins[str(path.relative_to(stage))]=sha(path.read_bytes())
 (a.output/'native-phantom-added-binding.diff').write_text(''.join(patch))
 receipt=ROOT/'analysis/physics/native-phantom-added-production-binding-tu15-20261102-second/report.json';r=json.loads(receipt.read_text());assert r['passed']
 for name in NEW:assert r['source_hashes'][name]==newpins[name],name
 (a.output/'source-pins.json').write_text(json.dumps(dict(base_source_root=str(a.source_root.resolve()),base_stage=str(a.base_stage.resolve()),base_stage_source_pins_sha256=sha((a.base_stage/'source-pins.json').read_bytes()),base_hashes=oldpins,staged_hashes=newpins,fields=FIELDS,production_binding_proof=str(receipt.relative_to(ROOT)),production_binding_proof_sha256=sha(receipt.read_bytes()),BA570_qualified_or_wired=False,production_applied=False,scope='Source43-site282700 +17-siteB1870 selected natively. MissingBA570 observes genuine event/object DATA+FP then counts/refuses; no oldexecutor fallback. Complete base PART21file stage preserved, no acceptance overwrite. NativeParent proof80profiles explicit declaredcomponent scope; actualPLAYERinput not yet supplied.'),indent=2,sort_keys=True)+'\n')
 print(a.output)
if __name__=='__main__':raise SystemExit(main())
