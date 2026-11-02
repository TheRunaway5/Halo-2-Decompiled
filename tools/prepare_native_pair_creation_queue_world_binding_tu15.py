#!/usr/bin/env python3
"""Prepare narrow native317200/320DA0 World queue bindings on a frozen cohort."""
import argparse,difflib,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FIELDS={'native_pair_merge_queue_attempts':'merge_attempts','native_pair_merge_queue_calls':'merge_calls','native_pair_merge_queue_refusals':'merge_refusals','native_pair_defer_queue_attempts':'defer_attempts','native_pair_defer_queue_calls':'defer_calls','native_pair_defer_queue_refusals':'defer_refusals'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def change(s,a,b,count=1):assert s.count(a)==count,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);dst=a.output/'staged';base={};pins={};patch=[];base_text={}
 files=['src/physics/native_tu15_world_integration_residuals.cpp','include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']
 for n in files:
  f=a.source_root/n
  if not f.is_file()and n=='src/physics/physics_live_world_tu15.cpp':f=a.source_root/'src/physics_live_world_tu15.cpp'
  old=f.read_text();s=old;base[n]=sha(f);base_text[n]=old
  if n.endswith('world_integration_residuals.cpp'):
   s=change(s,'    create.defer_pair=[&](auto island,auto input,auto&){const std::array args{input};call(0x320da0u,args,island.identity,4);};','    create.defer_pair=[&](auto island,auto input,auto&current){native_tu15_defer_bound_collision_pair(m,island.identity,input,current,pair_storage,observe,stats?&stats->pair_creation_queues:nullptr);};')
   s=change(s,'    create.merge_bodies=[&](auto w,auto first,auto second,auto&){const std::array args{first.identity,second.identity};call(0x317200u,args,w.identity,8);};','    create.merge_bodies=[&](auto w,auto first,auto second,auto&current){native_tu15_queue_bound_body_pair_merge(m,w.identity,first.identity,second.identity,current,pair_storage,observe,stats?&stats->pair_creation_queues:nullptr);};')
  elif n.endswith('world_integration.hpp'):
   anchor='#include "halo2/native_tu15_island_regroup_binding.hpp"';s=change(s,anchor,anchor+'\n#include "halo2/native_tu15_pair_creation_queue_binding.hpp"')
   anchor='  NativeTu15IslandRegroupBindingActivity island_regroup;';s=change(s,anchor,'  NativeTu15PairCreationQueueBindingActivity pair_creation_queues;\n'+anchor)
  elif n.endswith('physics_frame_residuals.hpp'):
   anchor='  std::uint64_t native_contact_manager_base_lock_releases{};';s=change(s,anchor,'  std::uint64_t '+','.join(n+'{}'for n in FIELDS)+';\n'+anchor)
  elif n.endswith('physics_live_world_tu15.cpp'):
   anchor='    result.native_contact_manager_base_lock_releases=impl_->native_physics_integrations.native_contact_manager_base_lock_releases;';s=change(s,anchor,''.join('    result.'+field+'=impl_->native_physics_integrations.pair_creation_queues.'+member+';\n'for field,member in FIELDS.items())+anchor)
  elif n.endswith('physics_multiplayer_acceptance.cpp'):
   anchor='        {"native_contact_manager_base_lock_releases",native_frames.native_contact_manager_base_lock_releases},';s=change(s,anchor,''.join('        {"'+field+'",native_frames.'+field+'},\n'for field in FIELDS)+anchor,2)
   anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_MANAGER_CLEANUP")) {'
   gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES")) {
    const bool queues=native_frames.native_pair_merge_queue_calls>0&&native_frames.native_pair_defer_queue_calls>0&&
      native_frames.native_pair_merge_queue_attempts==native_frames.native_pair_merge_queue_calls&&
      native_frames.native_pair_defer_queue_attempts==native_frames.native_pair_defer_queue_calls&&
      native_frames.native_pair_merge_queue_refusals==0&&native_frames.native_pair_defer_queue_refusals==0&&
      !native_frames.residual_owner_calls.contains(0x317200u)&&!native_frames.residual_owner_calls.contains(0x320da0u);
    report["native_pair_creation_queues_gate"]=queues;passed&=queues;
  }
''';s=change(s,anchor,gate+anchor)
  else:
   unit='src/physics/native_tu15_pair_creation_queues.cpp';assert unit not in s;s+='\n# Qualified native body-merge and deferred-pair queue owners.\ntarget_sources(halo2_native_pair_consumer PRIVATE '+unit+')\nset_source_files_properties('+unit+' PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")\n'
  t=dst/n;t.parent.mkdir(parents=True,exist_ok=True);t.write_text(s);pins[n]=sha(t);patch.extend(difflib.unified_diff(old.splitlines(True),s.splitlines(True),fromfile='a/'+n,tofile='b/'+n))
 for n in ['include/halo2/native_tu15_pair_creation_queue_binding.hpp','include/halo2/native_tu15_pair_creation_queues.hpp','src/physics/native_tu15_pair_creation_queues.cpp']:
  t=dst/n;t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/n,t);pins[n]=sha(t);f=a.source_root/n;base[n]=sha(f)if f.exists()else None
 (a.output/'delta.patch').write_text(''.join(patch));(a.output/'source-pins.json').write_text(json.dumps(dict(qualified=False,root_reviewed=False,live_qualified=False,production_applied=False,base_source_root=str(a.source_root.resolve()),base_hashes=base,staged_hashes=pins,fields=FIELDS,gate='HALO2_REQUIRE_NATIVE_PAIR_CREATION_QUEUES',scope='Native317200/320DA0 with existing native reserve/growth/router/pool; source queues33/25sites, typed by-value production inputs and lazy retained pair DATA. No original executor or supplied capacity/pointer/results. Other pair/wake/manager owners retain separate scope.'),indent=2,sort_keys=True)+'\n');print(a.output)
if __name__=='__main__':main()
