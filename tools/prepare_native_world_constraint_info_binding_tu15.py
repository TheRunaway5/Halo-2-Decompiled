#!/usr/bin/env python3
"""Add two source-authentic nativeinfo services to immutable reverse50 stage."""
import argparse,difflib,json,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/physics/native-reversed-contact-process-binding-stage-20261102'
FILES=['cmake/PhysicsLiveWorldTu15.cmake','include/halo2/native_tu15_island_regroup_binding.hpp','src/physics/native_tu15_island_regroup_binding.cpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp']
FIELDS={'native_world_regroup_constant_info_queries':'constant_info_queries','native_world_regroup_extended_info_queries':'extended_info_queries'}
def once(s,before,after,n=1):assert s.count(before)==n,(before,s.count(before));return s.replace(before,after)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';shutil.copytree(BASE/'staged',stage);old=json.loads((BASE/'source-pins.json').read_text());assert old['qualified']and all(sha(stage/f)==h for f,h in old['staged_hashes'].items());before={f:(stage/f).read_text()for f in FILES};base_hashes={f:sha(stage/f)for f in FILES}
 for name in('constant_constraint_info','extended_constraint_info'):
  for f in('include/halo2/native_tu15_'+name+'.hpp','src/physics/native_tu15_'+name+'.cpp'):target=stage/f;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/f,target)
 f=FILES[0];s=once(before[f],'    src/physics/native_tu15_island_regroup_binding.cpp','    src/physics/native_tu15_island_regroup_binding.cpp\n    src/physics/native_tu15_constant_constraint_info.cpp\n    src/physics/native_tu15_extended_constraint_info.cpp');(stage/f).write_text(s)
 f=FILES[1];s=once(before[f],'  NativeTu15IslandRegroupStats regroup;','  std::uint64_t constant_info_queries{},extended_info_queries{};\n  NativeTu15IslandRegroupStats regroup;');(stage/f).write_text(s)
 f=FILES[2];s=once(before[f],'#include <stdexcept>','#include "halo2/native_tu15_constant_constraint_info.hpp"\n#include "halo2/native_tu15_extended_constraint_info.hpp"\n#include <stdexcept>');oldcallback='''    services.constraint_info=[&](auto,auto owner,auto,auto&){
      if(activity)++activity->unknown_constraint_info;unknown("constraint info",owner);
    };''';newcallback='''    services.constraint_info=[&](auto constraint,auto owner,auto info,auto&){
      if(owner.identity==0x31d630u){
        (void)native_tu15_get_constant_constraint_info(m,info,observe);
        if(activity){++activity->constant_info_queries;++activity->regroup.native_info_queries;}
        return;
      }
      if(owner.identity==0x31f840u){
        (void)native_tu15_get_extended_constraint_info(m,constraint,info,observe);
        if(activity){++activity->extended_info_queries;++activity->regroup.native_info_queries;}
        return;
      }
      if(activity)++activity->unknown_constraint_info;unknown("constraint info",owner);
    };''';s=once(s,oldcallback,newcallback);(stage/f).write_text(s)
 f=FILES[3];s=once(before[f],'  std::uint64_t native_world_regroup_attempts{}','  std::uint64_t '+','.join(n+'{}'for n in FIELDS)+';\n  std::uint64_t native_world_regroup_attempts{}');(stage/f).write_text(s)
 f=FILES[4];anchor='    result.native_world_regroup_attempts=world_regroup.attempts;';s=once(before[f],anchor,''.join('    result.'+field+'=world_regroup.'+path+';\n'for field,path in FIELDS.items())+anchor);(stage/f).write_text(s)
 f=FILES[5];anchor='        {"native_world_regroup_attempts",native_frames.native_world_regroup_attempts},';s=once(before[f],anchor,''.join('        {"'+field+'",native_frames.'+field+'},\n'for field in FIELDS)+anchor,2);anchor='  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_MERGE")) {';gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_WORLD_CONSTRAINT_INFO")) {
    const bool info=native_frames.native_world_regroup_constant_info_queries+
      native_frames.native_world_regroup_extended_info_queries>0&&
      native_frames.native_world_regroup_attempts==native_frames.native_world_regroup_calls&&
      native_frames.native_world_regroup_refusals==0&&native_frames.native_world_regroup_unbound_services==0&&
      !native_frames.residual_owner_calls.contains(0x31d630u)&&!native_frames.residual_owner_calls.contains(0x31f840u);
    report["native_world_constraint_info_gate"]=info;passed&=info;
  }
''';s=once(s,anchor,gate+anchor);(stage/f).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[f].splitlines(True),(stage/f).read_text().splitlines(True),fromfile='a/'+f,tofile='b/'+f))for f in FILES));manifest=dict(qualified=False,production_applied=False,live_qualified=False,base_stage=str(BASE),base_hashes=base_hashes,delta_files=FILES,fields=FIELDS,staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Native31D630 constantinfo and31F840 extensionofsole325E90 intypedregroupservice; otherunknowns countedthrow. Inheritsexactreverse50/pointRef4/baseLock/regroup. No Rootforeigndeferredcapacitychanges or MPprobeoverwrite.',remaining=['CoherentWorldmodel/countertransport','Actualtypedinfoinput/livePLAYER/framepositiveassociation','Otherremainingnativephysicsowners'])
 save(a.output/'source-pins.json',manifest);print(dict(prepared=True,files=len(manifest['staged_hashes']),modified_shared_files=len(FILES)));return 0
if __name__=='__main__':raise SystemExit(main())
