#!/usr/bin/env python3
"""Narrow native1D77D0 World route on an explicitly frozen live source."""
import argparse,difflib,shutil
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
SHARED=['include/halo2/native_tu15_world_integration.hpp','include/halo2/native_tu15_physics_frame_residuals.hpp','src/physics/native_tu15_world_integration_residuals.cpp','src/physics/physics_live_world_tu15.cpp','tests/physics_multiplayer_acceptance.cpp','cmake/PhysicsLiveWorldTu15.cmake','tests/native_tu15_world_residuals_tests.cpp','include/halo2/native_tu15_object_activation_residuals.hpp','src/physics/native_tu15_object_activation_residuals.cpp']
NEW=['include/halo2/native_tu15_contact_body_update.hpp','include/halo2/native_tu15_contact_body_update_binding.hpp','src/physics/native_tu15_contact_body_update.cpp']
FIELDS={'attempts':'contact_body.attempts','calls':'contact_body.completed','refusals':'contact_body.refusals','point_queries':'contact_body.update.point_queries','property_queries':'contact_body.update.property_queries','motion_type_queries':'contact_body.update.motion_type_queries','point_velocity_queries':'contact_body.update.point_velocity_queries','impulse_requests':'contact_body.update.impulse_requests','wake_attempts':'contact_body_wake_attempts','wake_calls':'contact_body_wake_calls','wake_refusals':'contact_body_wake_refusals','unqualified_owners':'contact_body.update.unqualified_services+world_integration.contact_body.native_only.refused+world_integration.contact_body_wake_unqualified_owners','unsupported_fp_profiles':'contact_body.update.unsupported_fp_profiles'}
def once(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.source_root=a.source_root.resolve();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.output/'staged';before={};base={}
 for relative in SHARED:
  source=a.source_root/relative
  if not source.exists()and relative=='src/physics/physics_live_world_tu15.cpp':source=a.source_root/'src/physics_live_world_tu15.cpp'
  before[relative]=source.read_text();base[relative]=sha(source);target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 for relative in NEW+['tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp']:
  source=ROOT/relative if relative in NEW else a.source_root/relative;target=stage/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,target)
 r=SHARED[7];s=before[r];s=once(s,'} // namespace halo2','''// Actual315CB0 with the SAME native activation/listener/lifetime builder.
void native_tu15_activate_body_with_native_services(SourcePhysicsContactMemory&,
    NativeTu15PhysicsBody,NativeTu15PhysicsFrameFp&,unsigned,
    NativeTu15ObjectActivationScratch,const NativeTu15ObjectActivationNativeServices&,
    const NativeTu15ObjectActivationResidualCall&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectActivationBindingActivity* = nullptr);
} // namespace halo2''');(stage/r).write_text(s)
 r=SHARED[8];s=before[r];s=once(s,'} // namespace halo2','''void native_tu15_activate_body_with_native_services(SourcePhysicsContactMemory&memory,
 NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,unsigned depth,
 NativeTu15ObjectActivationScratch scratch,const NativeTu15ObjectActivationNativeServices&native,
 const NativeTu15ObjectActivationResidualCall&invoke,const SourcePhysicsContactRecordObserver&observer,
 NativeTu15ObjectActivationBindingActivity*activity){
    with_activation_services(memory,depth,scratch,invoke,observer,activity,&native,[&](const auto&activation){
        native_tu15_activate_physics_body(memory,body,fp,activation,observer,
            activity?&activity->activation:nullptr);
    });
}
} // namespace halo2''');(stage/r).write_text(s)
 r=SHARED[0];s=once(before[r],'#include <map>','#include "halo2/native_tu15_contact_body_update_binding.hpp"\n#include <map>');s=once(s,'  NativeTu15ContactEntityProcessorStats contact_processor;','  NativeTu15ContactEntityProcessorStats contact_processor;\n  NativeTu15ContactBodyUpdateBindingActivity contact_body;\n  std::uint64_t contact_body_wake_attempts{},contact_body_wake_calls{},contact_body_wake_refusals{},contact_body_wake_unqualified_owners{};\n  std::map<std::uint32_t,std::uint64_t> contact_body_wake_refused_owners;');(stage/r).write_text(s)
 r=SHARED[1];s=once(before[r],'  std::uint64_t native_contact_entity_calls{}','  '+','.join('std::uint64_t native_contact_body_'+key+'{}' if i==0 else 'native_contact_body_'+key+'{}' for i,key in enumerate(FIELDS))+';\n  std::map<std::uint32_t,std::uint64_t> native_contact_body_refused_owners;\n  std::uint64_t native_contact_entity_calls{}');(stage/r).write_text(s)
 r=SHARED[2];s=before[r];start=s.index('    processor.update_contact=[&]');end=s.index('\n    native_tu15_process_entity_contacts',start);old=s[start:end];observer=[line for line in old.splitlines()if 'native_tu15_observe_contact_body_update_input('in line];assert len(observer)==1
 new='''    processor.update_contact=[&](auto manager,auto row,bool reversed,auto datum,auto slot,auto&current){
'''+observer[0].replace('current_fp','current')+'''
      // Distinct retained DATA: body wake must not overwrite the point-velocity
      // or impulse packets that remain live across its native listener graph.
      const auto body_data=(scratch.first+0x3000u+15u)&~15u;
      check(scratch.bytes>=0x6200u&&std::uint64_t(body_data)+0x920u<=end-0x2800u,
        "Native TU1.5 body contact requires distinct caller-owned retained DATA");
      NativeTu15ContactBodyUpdateNativeServices native;
      native.activate_body=[&](auto body,auto&current_fp,auto depth){
        if(stats)++stats->contact_body_wake_attempts;
        const NativeTu15ObjectActivationNativeServices factories;
        try{
          native_tu15_activate_body_with_native_services(m,body,current_fp,depth,
            {body_data+0x20u,body_data+0x24u,body_data+0x40u,body_data+0x80u,0x800u},factories,
            [&](auto owner,auto,auto,unsigned,auto&)->std::uint32_t{
              if(stats){++stats->contact_body_wake_unqualified_owners;++stats->contact_body_wake_refused_owners[owner];}
              throw std::runtime_error("Native TU1.5 body wake requires native owner="+std::to_string(owner));
            },observe,stats?&stats->world_activation:nullptr);
          if(stats){++stats->contact_body_wake_calls;++stats->native_world_activation_calls;}
        }catch(...){if(stats){++stats->contact_body_wake_refusals;++stats->native_world_activation_refusals;}throw;}
      };
      native_tu15_update_bound_contact_body(m,manager,row,false,reversed,datum,slot,
        {body_data+0x900u,body_data+0x910u},current,state.x87_depth,native,
        pair_deletion_scope,observe,stats?&stats->contact_body:nullptr);
    };''';s=s[:start]+new+s[end:];(stage/r).write_text(s)
 r=SHARED[3];s=before[r];anchor='    result.native_contact_entity_calls=contact_entities.calls;';lines=''.join('    result.native_contact_body_'+k+'=world_integration.'+v+';\n'for k,v in FIELDS.items());lines+='''    result.native_contact_body_refused_owners=world_integration.contact_body.update.unqualified_owners;
    for(const auto&[owner,count]:world_integration.contact_body.native_only.owners)
      result.native_contact_body_refused_owners[owner]+=count;
    for(const auto&[owner,count]:world_integration.contact_body_wake_refused_owners)
      result.native_contact_body_refused_owners[owner]+=count;
''';s=once(s,anchor,lines+anchor);(stage/r).write_text(s)
 r=SHARED[4];s=before[r];anchor='        {"native_contact_entity_calls",native_frames.native_contact_entity_calls},';assert s.count(anchor)==2;lines=''.join('        {"native_contact_body_'+k+'",native_frames.native_contact_body_'+k+'},\n'for k in FIELDS);lines+='        {"native_contact_body_refused_owners",native_frames.native_contact_body_refused_owners},\n';s=s.replace(anchor,lines+anchor)
 gate='''  if(std::getenv("HALO2_REQUIRE_NATIVE_CONTACT_BODY_UPDATE")) {
    const bool native_body=native_frames.native_contact_body_calls>0&&
      native_frames.native_contact_body_attempts==native_frames.native_contact_body_calls&&
      native_frames.native_contact_body_refusals==0&&native_frames.native_contact_body_unqualified_owners==0&&
      native_frames.native_contact_body_refused_owners.empty()&&native_frames.native_contact_body_unsupported_fp_profiles==0&&
      native_frames.native_contact_body_wake_attempts==native_frames.native_contact_body_wake_calls&&
      native_frames.native_contact_body_wake_refusals==0&&native_frames.native_world_activation_residual_calls==0&&
      !native_frames.residual_owner_calls.contains(0x1d77d0u);
    report["native_contact_body_update_gate"]=native_body;passed&=native_body;
  }
''';s=once(s,'  if(std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_LIST_AGENT_DESTRUCTOR")) {',gate+'  if(std::getenv("HALO2_REQUIRE_NATIVE_REVERSED_LIST_AGENT_DESTRUCTOR")) {');(stage/r).write_text(s)
 r=SHARED[5];s=before[r]+'\n# Sole native1D77D0 semantic owner.\ntarget_sources(halo2_native_pair_consumer PRIVATE src/physics/native_tu15_contact_body_update.cpp)\n';(stage/r).write_text(s)
 # Replace the prior opaque1D77D0 test side effects with declared, consistent
 # native manager/body input. No runtime rows, flags or results are supplied.
 r=SHARED[6];s=before[r];start=s.index('    else if(target==0x1d77d0u)');end=s.index('    else if(target==worldCollisionListener)',start);s=s[:start]+s[end:]
 s=once(s,'m.put(world+0x92ccu,1);','m.put(world+0x92ccu,2);')
 s=once(s,'for(auto row:{world+0x9400u,world+0x9648u})','for(auto row:{world+0x9400u,world+0x9448u})')
 anchor='    m.put(world+0x7fc4u,world+0x7fd0u);';extra='''    // Declared native contact DATA, replacing a former opaque Body callback.
    m.put(world+0x70d4u,0x323040u);m.put(world+0x70d8u,0x323020u);
    m.put(world+0x70b0u,world+0xe000u);m.put(world+0xe000u+2u*4u,0u);
    m.put(world+0x7120u,world+0xe200u);
    m.put(world+0xe200u,0u);m.put(world+0x92b0u,world+0xe300u);
    m.put(world+0xe300u+7u*96u+0x40u,world+0xe800u);
    m.put(world+0xe83cu,world+0xe900u);m.put(world+0xe900u,world+0xea00u);
    m.put(world+0xea18u,0x31dd70u);m.put(world+0xe92cu,0x3f800000u);
    m.put(0x46a0d0u,0x3f800000u);m.put(0x46a0dcu,0xbf800000u);
    m.put(0x46a240u,0x3f800000u);m.put(0x46a238u,0u);
    m.put(0x51df54u,world+0xeb00u);m.put(world+0xeb08u,100u);
''';s=once(s,anchor,extra+anchor)
 s=once(s,'m.get(world+0x77f0u)==2&&','stats.contact_body.completed==2&&stats.contact_body.refusals==0&&stats.contact_body.native_only.refused==0&&')
 s=once(s,'std::count(calls.begin(),calls.end(),0x1d77d0u)==2','!stats.residual_owner_calls.contains(0x1d77d0u)')
 (stage/r).write_text(s)
 (a.output/'delta.patch').write_text(''.join(''.join(difflib.unified_diff(before[r].splitlines(True),(stage/r).read_text().splitlines(True),fromfile='a/'+r,tofile='b/'+r))for r in SHARED))
 save(a.output/'source-pins.json',dict(qualified=False,root_reviewed=False,production_applied=False,base_source_root=str(a.source_root),base_hashes=base,delta_files=SHARED,new_files=NEW,inherited_files=['tests/helpers/native_world_convex_factory_prefix_tu15.hpp','tests/helpers/native_world_island_merge_prefix_tu15.hpp','tests/helpers/native_contact_process_binding_model_tu15.hpp'],staged_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Native1D77D0 through exact qualified native-only helper; sole activation builder315CB0 additive export; independent low retained DATA; unknown owners count+throw before CPU transport. Prior opaque completion Body callback replaced by explicit declared native DATA, all other prior World models retained.'))
 print(dict(prepared=True,files=len(SHARED)+len(NEW)))
if __name__=='__main__':main()
