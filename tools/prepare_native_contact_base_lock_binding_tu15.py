#!/usr/bin/env python3
"""Freeze the absent-registry native base-lock seam on an explicit cohort.

The resulting candidate still requires the actual-input qualification and
World/live checks recorded separately. No supplied registry result is used.
"""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELD='native_contact_manager_base_lock_releases'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--base',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);dst=a.output/'staged';shutil.copytree(a.stage/'staged',dst);patch=[]
 paths=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp']
 for name in paths:
  f=dst/name;old=f.read_text();s=old
  if name.endswith('world_integration_residuals.cpp'):
   anchor='#include "halo2/native_tu15_contact_point_binding.hpp"';s=change(s,anchor,anchor+'\n#include "halo2/native_tu15_base_lock.hpp"')
   before='''  manager_cleanup.collector.release_base_lock=[&](auto object,auto flags,auto&){
    if(stats)++stats->native_contact_manager_service_residuals;
    const std::array args{flags};call(0x2e12a0u,args,object,4);
  };'''
   after='''  manager_cleanup.collector.release_base_lock=[&](auto object,auto flags,auto&){
    check(flags==0,"Native TU1.5 contact cleanup requires qualified base-lock release flags");
    native_tu15_release_unregistered_base_lock(m,object,observe);
    if(stats)++stats->native_contact_manager_base_lock_releases;
  };''';s=change(s,before,after)
   s=change(s,'native_owned_header(0x28u,1u)&&!(word(m,collector+4u,2)&0x8000u);','native_owned_header(0x28u,1u)&&\n        (!(word(m,collector+4u,2)&0x8000u)||!native_tu15_base_lock_registered(m,collector));')
  elif name.endswith(('world_integration.hpp','physics_frame_residuals.hpp')):
   anchor='  std::uint64_t native_contact_manager_cleanups{}';s=change(s,anchor,'  std::uint64_t '+FIELD+'{};\n'+anchor)
  elif name.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_contact_manager_cleanups=impl_->native_physics_integrations.native_contact_manager_cleanups;';s=change(s,anchor,anchor+'\n    result.'+FIELD+'=impl_->native_physics_integrations.'+FIELD+';')
  else:
   anchor='        {"native_contact_manager_cleanups",native_frames.native_contact_manager_cleanups},';s=change(s,anchor,anchor+'\n        {"'+FIELD+'",native_frames.'+FIELD+'},',2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_BASE_LOCK")) {
    const bool lock_passed=native_frames.native_contact_manager_base_lock_releases>0&&
      native_frames.native_contact_manager_service_residuals==0&&native_frames.native_contact_manager_cleanup_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x2e12a0u)&&!native_frames.residual_owner_calls.contains(0x2e11e0u)&&
      !native_frames.residual_owner_calls.contains(0x2e33f0u)&&!native_frames.residual_owner_calls.contains(0x2e3350u);
    report["native_contact_manager_base_lock_gate"]=lock_passed;passed&=lock_passed;
  }
''';s=change(s,anchor,gate+anchor)
  f.write_text(s);patch.extend(difflib.unified_diff(old.splitlines(True),s.splitlines(True),fromfile='a/'+name,tofile='b/'+name))
 helper='include/halo2/native_tu15_base_lock.hpp';shutil.copyfile(ROOT/helper,dst/helper)
 base_hashes={};pins={}
 for f in dst.rglob('*'):
  if not f.is_file():continue
  name=str(f.relative_to(dst));pins[name]=sha(f);base=a.base/name
  if not base.exists()and name=='src/physics/physics_live_world_tu15.cpp':base=a.base/'src/physics_live_world_tu15.cpp'
  base_hashes[name]=sha(base)if base.is_file()else None
 manifest=dict(qualified=False,production_applied=False,live_qualified=False,base_source_root=str(a.base.resolve()),base_hashes=base_hashes,staged_hashes=pins,parent_stage=str(a.stage.resolve()),parent_stage_manifest_sha256=sha(a.stage/'source-pins.json'),fields=[FIELD],gate='HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_BASE_LOCK',scope='Native absent-registry2E12A0 with real source lookup and source-phase recheck. Registered keys and other unqualified domains refuse. Includes qualified inherited point/regroup/factory/process cohort; no production original base-lock fallback.',remaining=['Actual new manager BEFORE registry/native full-manager qualification','Coherent World model','Visible multiplayer strict frame/host-peer/actual vehicle camera'])
 (a.output/'source-pins.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n');(a.output/'base-lock-delta.patch').write_text(''.join(patch));print(a.output)
if __name__=='__main__':main()
