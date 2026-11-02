#!/usr/bin/env python3
"""Narrow native232D20 pair-storage route on a frozen World source."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
SHARED=['include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/native_tu15_world_integration_residuals.cpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake']
NEW=['include/halo2/native_tu15_main_heap_release.hpp','include/halo2/native_tu15_main_heap_release_binding.hpp','src/physics/native_tu15_main_heap_release.cpp']
INHERITED=['tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']
FIELDS={'attempts':'attempts','calls':'completed','refusals':'refusals','wrapper_calls':'kernel.wrapper_calls','ledger_calls':'kernel.ledger_calls','visited':'kernel.visited','matches':'kernel.matched','moved_rows':'kernel.moved_rows','kernel_completed':'kernel.completed','unbound_services':'kernel.unbound_services'}
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--base-overlay',type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';before={};bases={}
 for relative in SHARED:
  source=a.source_root/relative
  if a.base_overlay and(a.base_overlay/'staged'/relative).is_file():source=a.base_overlay/'staged'/relative
  if not source.exists()and relative=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  before[relative]=source.read_text();bases[relative]=sha(source);target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 for relative in NEW+INHERITED:
  target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile((ROOT if relative in NEW else a.source_root)/relative,target)
 r=SHARED[0];s=once(before[r],'#include <map>','#include "halo2/native_tu15_main_heap_release_binding.hpp"\n#include <map>');s=once(s,'  NativeTu15ContactEntityProcessorStats contact_processor;','  NativeTu15ContactEntityProcessorStats contact_processor;\n  NativeTu15MainHeapReleaseBindingActivity main_heap_release;');(stage/r).write_text(s)
 r=SHARED[1];s=once(before[r],'  std::uint64_t native_contact_entity_calls{}','  std::uint64_t '+','.join('native_main_heap_release_'+field+'{}'for field in FIELDS)+';\n  std::map<std::uint32_t,std::uint64_t> native_main_heap_release_refused_owners;\n  std::uint64_t native_contact_entity_calls{}');(stage/r).write_text(s)
 r=SHARED[2];s=before[r];anchor='''    if(request.owner!=0x233420u)
      unqualified_pair_service("Native TU1.5 pair storage requires qualified pool release owner="+std::to_string(request.owner));''';s=once(s,anchor,'''    if(request.owner==0x232d20u){
      // Authenticated main-heap ledger lifetime is distinct from233420 pool
      // release. The sole helper refuses unknown virtual+C before any CPU.
      native_tu15_release_bound_main_heap_allocation(m,request,context,state.x87_depth,observe,
        stats?&stats->main_heap_release:nullptr);
      return;
    }
'''+anchor);(stage/r).write_text(s)
 r=SHARED[3];s=before[r];anchor='    result.native_contact_entity_calls=contact_entities.calls;';lines=''.join('    result.native_main_heap_release_'+key+'=world_integration.main_heap_release.'+value+';\n'for key,value in FIELDS.items());lines+='    result.native_main_heap_release_refused_owners=world_integration.main_heap_release.kernel.unbound_owners;\n';s=once(s,anchor,lines+anchor);(stage/r).write_text(s)
 r=SHARED[4];s=before[r];anchor='        {"native_contact_entity_calls",native_frames.native_contact_entity_calls},';assert s.count(anchor)==2;lines=''.join('        {"native_main_heap_release_'+field+'",native_frames.native_main_heap_release_'+field+'},\n'for field in FIELDS);lines+='        {"native_main_heap_release_refused_owners",native_frames.native_main_heap_release_refused_owners},\n';s=s.replace(anchor,lines+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_RELEASE")) {
    const bool native_heap=native_frames.native_main_heap_release_calls>0&&
      native_frames.native_main_heap_release_attempts==native_frames.native_main_heap_release_calls&&
      native_frames.native_main_heap_release_wrapper_calls==native_frames.native_main_heap_release_calls&&
      native_frames.native_main_heap_release_ledger_calls==native_frames.native_main_heap_release_calls&&
      native_frames.native_main_heap_release_kernel_completed==native_frames.native_main_heap_release_calls&&
      native_frames.native_main_heap_release_refusals==0&&native_frames.native_main_heap_release_unbound_services==0&&
      native_frames.native_main_heap_release_refused_owners.empty()&&
      !native_frames.residual_owner_calls.contains(0x232d20u)&&
      !native_frames.residual_owner_calls.contains(0x232c80u);
    report["native_main_heap_release_gate"]=native_heap;passed&=native_heap;
  }
  if(std::getenv("HALO2_REQUIRE_NATIVE_MAIN_HEAP_LEDGER_REMOVAL")) {
    const bool removed=native_frames.native_main_heap_release_matches>0;
    report["native_main_heap_ledger_removal_gate"]=removed;passed&=removed;
  }
''';s=once(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE")) {');(stage/r).write_text(s)
 r=SHARED[5];s=before[r]+'\n# Sole source-authenticated232D20/232C80 main-heap release owner.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_main_heap_release.cpp)\n';(stage/r).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(stage/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in SHARED))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,base_source_root=str(a.source_root),base_overlay=None if not a.base_overlay else str(a.base_overlay.resolve()),base_hashes=bases,delta_files=SHARED,new_files=NEW,inherited_files=INHERITED,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual pair-storage232D20 native-only helper; authenticated232C80 ledger removal composed directly, unknown owner count+throw. Existing readonly capture and233420 pool branch retained. Separate native-attempt and positive-removal counters/gates; no heap flags/physics state injected.'))
if __name__=='__main__':main()
