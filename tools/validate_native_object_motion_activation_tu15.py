#!/usr/bin/env python3
"""Original B9800 versus explicit-context native velocity/wake composition."""
import argparse,json,os,re,shutil,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_native_contact_agent_dispatch_tu15 import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--case',default='.*');p.add_argument('--binding',action='store_true');p.add_argument('--transport',action='store_true');p.add_argument('--manager',action='store_true');p.add_argument('--policy',action='store_true');p.add_argument('--policy-native-bounds',action='store_true');p.add_argument('--binding-source-overlay',type=Path);p.add_argument('--live-depths',nargs='+',type=int,default=[0]);a=p.parse_args();a.binding|=a.transport;a.policy|=a.policy_native_bounds;assert all(0<=depth<=5 for depth in a.live_depths)and(a.transport or a.live_depths==[0])and not((a.manager or a.policy)and a.binding)and not(a.manager and a.policy)and(a.binding or a.binding_source_overlay is None);a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 new_policy_binding=a.binding and 'native_tu15_body_policy.hpp' in ((a.binding_source_overlay/'include/halo2/native_tu15_object_activation_residuals.hpp')if a.binding_source_overlay else(ROOT/'include/halo2/native_tu15_object_activation_residuals.hpp')).read_text()
 new_constraint_binding=a.binding and 'native_tu15_body_constraint_update.hpp' in ((a.binding_source_overlay/'include/halo2/native_tu15_object_activation_residuals.hpp')if a.binding_source_overlay else(ROOT/'include/halo2/native_tu15_object_activation_residuals.hpp')).read_text()
 new_constraint_services=a.binding and 'native_tu15_model_constraint_lookup.hpp' in ((a.binding_source_overlay/'include/halo2/native_tu15_object_activation_residuals.hpp')if a.binding_source_overlay else(ROOT/'include/halo2/native_tu15_object_activation_residuals.hpp')).read_text()
 new_owners=a.binding and(a.binding_source_overlay is not None or 'native_tu15_activation_payload_filter.hpp'in(ROOT/'include/halo2/native_tu15_object_activation_residuals.hpp').read_text());units=['object_motion','island_activation','activation_agents','activation_services','contact_agent_factory','array_allocator','array_growth']+(['object_activation_residuals']if a.binding else[])+(['object_velocity_transport','object_velocity_handoff']if a.transport else[])+(['activation_contact_manager']if a.manager else[])+(['body_policy']if a.policy or new_policy_binding else[])+(['shape_bounds','centered_shape_bounds','compound_shape_bounds','sphere_shape_bounds','geometry']if a.policy_native_bounds or new_policy_binding else[])+(['activation_payload_filter','activation_contact_manager']if new_owners else[])+(['body_constraint_update']if new_constraint_binding else[])+(['model_constraint_lookup','model_constraint_capacity']if new_constraint_services else[]);units+=['gameplay_policy_destruction']if 'body_policy'in units else[];files=[ROOT/f'src/physics/native_tu15_{name}.cpp'for name in units]+[ROOT/f'include/halo2/native_tu15_{name}.hpp'for name in units]+[ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',ROOT/'tools/observe_native_activation_agents_tu15.cpp',ROOT/'tools/observe_native_object_motion_activation_tu15.cpp',Path(__file__)]+([ROOT/'src/physics/vehicles/physics_vehicle_update_generated.cpp',ROOT/'src/physics/vehicles/physics_vehicle_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_world_runtime_internal.hpp',ROOT/'src/physics/runtime/physics_contact_runtime_internal.hpp']if a.transport else[])
 if new_constraint_binding:files.extend([ROOT/'src/core/native_tu15_data_array_allocate.cpp',ROOT/'include/halo2/native_tu15_data_array_allocate.hpp'])
 if a.binding_source_overlay:
  a.binding_source_overlay=a.binding_source_overlay.resolve();assert a.binding_source_overlay.is_relative_to(ROOT);files.extend([a.binding_source_overlay/'src/physics/native_tu15_object_activation_residuals.cpp',a.binding_source_overlay/'include/halo2/native_tu15_object_activation_residuals.hpp'])
  if(a.binding_source_overlay/'src/physics/native_tu15_object_velocity_transport.cpp').exists():files.append(a.binding_source_overlay/'src/physics/native_tu15_object_velocity_transport.cpp')
 pins={str(f.relative_to(ROOT)):sha(f)for f in files};save(a.output/'report.json',report)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];driver=scratch/'driver.cpp';body=(ROOT/'tools/observe_native_object_motion_activation_tu15.cpp').read_text()
  if a.transport:
   body=body.replace('linear_data=sp+0x100u,angular_data=sp+0x120u','linear_data=sp+0x1cu,angular_data=sp+0x34u').replace('put(sp,0x1ccacau)','put(sp,0xf2096u)').replace('original.go(0x1ccaca)','original.go(0xf2096)')
   first=body.index(' NativeTu15ArrayAllocatorMemory allocation_memory(memory);');last=body.index('\n std::size_t different{},compared{};');replacement='''
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ObjectVelocityTransportActivity transport;NativeTu15ObjectVelocityHandoffActivity handoff;
 SourcePhysicsContactRuntimeAbi actual{};actual.gp={std::uint32_t(object_entry.rax),std::uint32_t(object_entry.rcx),std::uint32_t(object_entry.rdx),std::uint32_t(object_entry.rbx),std::uint32_t(object_entry.rsp),std::uint32_t(object_entry.rbp),std::uint32_t(object_entry.rsi),std::uint32_t(object_entry.rdi)};actual.eflags=object_entry.eflags;actual.fs_base=capture.at("before").at("fs_base");actual.x87_depth=incoming_depth;actual.x87_status=before.swd;actual.floating_point=SourcePhysicsFactoryFloatingPoint{before.cwd,before.mxcsr};for(unsigned n=0;n<incoming_depth;++n)std::memcpy(&actual.x87[n],reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10);for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)actual.xmm[n][k]=before.xmm_space[n*4u+k];const auto outer_before=actual;
 NativeTu15ObjectVelocityTransportPolicy policy{private_first,private_end,private_first+0x1000u,sp-private_first-0x2000u,true,nullptr,nullptr,[](auto){}};
 const NativeTu15ObjectVelocityOwnerExecutor execute=[&](auto owner,auto&call,const auto&){
  auto r=native.regs();r.rax=call.gp[0];r.rcx=call.gp[1];r.rdx=call.gp[2];r.rbx=call.gp[3];r.rsp=call.gp[4];r.rbp=call.gp[5];r.rsi=call.gp[6];r.rdi=call.gp[7];r.eflags=call.eflags;r.rip=owner;request(PTRACE_SETREGS,native.pid,nullptr,&r);auto f=native.fp();f.cwd=call.floating_point->x87_control_word;f.swd=call.x87_status;f.mxcsr=call.floating_point->mxcsr;f.ftw=0;std::memset(f.st_space,0,sizeof(f.st_space));for(unsigned n=0;n<call.x87_depth;++n){std::memcpy(reinterpret_cast<std::byte*>(f.st_space)+n*16u,&call.x87[n],10);f.ftw|=1u<<(((f.swd>>11u)+n)&7u);}for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)f.xmm_space[n*4u+k]=call.xmm[n][k];request(PTRACE_SETFPREGS,native.pid,nullptr,&f);const auto before_sp=call.gp[4];native.go(0xf2096u);r=native.regs();f=native.fp();call.gp={std::uint32_t(r.rax),std::uint32_t(r.rcx),std::uint32_t(r.rdx),std::uint32_t(r.rbx),std::uint32_t(r.rsp),std::uint32_t(r.rbp),std::uint32_t(r.rsi),std::uint32_t(r.rdi)};call.eflags=r.eflags;call.floating_point=SourcePhysicsFactoryFloatingPoint{f.cwd,f.mxcsr};call.x87_status=f.swd;call.x87_depth=std::popcount(unsigned(f.ftw)&255u);for(unsigned n=0;n<call.x87_depth;++n)std::memcpy(&call.x87[n],reinterpret_cast<const std::byte*>(f.st_space)+n*16u,10);for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)call.xmm[n][k]=f.xmm_space[n*4u+k];bridge.calls.push_back(J{{"entry",owner},{"before_esp",before_sp},{"after_esp",call.gp[4]},{"after_fp",fp_words(f)}});
 };
 SourcePhysicsContactRecordObserver observed;
 NativeTu15ObjectVelocityHandoffBinding route(memory,[&](auto&bank,const auto&call,auto&state,const auto&observer){native_tu15_run_object_velocity_handoff(bank,call,state,policy,execute,observer,&transport);},&handoff);
 SourcePhysicsContactRuntimeServices call_services;const SourcePhysicsFactoryFloatingPoint controls{before.cwd,before.mxcsr};
 {halo2::physics_vehicle_runtime_detail::FpScope scope(controls);halo2::physics_vehicle_runtime_detail::VehicleState caller(memory,observed,call_services,SourcePhysicsContactImage::TitleUpdate15);caller.load(actual);check(handoff_native_object_velocity(caller),"Actual F2091 native handoff was not selected");actual=caller.save();}
 fp={actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr};wake_stats=transport.binding.activation;agent_stats=transport.binding.agents;service_stats=transport.binding.services;const std::string legacy_refusal="Transport exercises the qualified explicit native owner";
 check(actual.gp[4]==outer_before.gp[4]+8u&&actual.xmm==outer_before.xmm,"Transport lost actual return or changed dead SIMD state");for(unsigned n=0;n<8;++n)if(n!=4)check(actual.gp[n]==outer_before.gp[n],"Transport fabricated a guest volatile-register postimage");
 check(expected_fp.ftw==before.ftw&&actual.x87_depth==incoming_depth,"Original/native velocity owner changed preserved live x87 depth");for(unsigned n=0;n<incoming_depth;++n){check(std::memcmp(reinterpret_cast<const std::byte*>(expected_fp.st_space)+n*16u,reinterpret_cast<const std::byte*>(before.st_space)+n*16u,10)==0,"Original velocity owner changed a declared incoming extended value");check(std::memcmp(&actual.x87[n],&outer_before.x87[n],10)==0,"Native velocity transport changed a declared incoming extended value");}
 report.update(J{{"actual_F2091_generated_helper_exercised",true},{"declared_incoming_x87_depth",incoming_depth},{"original_and_native_preserve_live_extended_values",true},{"native_handoff_attempts",handoff.attempts},{"native_handoff_completed",handoff.completed},{"native_handoff_refusals",handoff.refusals},{"native_transport_attempts",transport.attempts},{"native_transport_completed",transport.completed},{"native_transport_refusals",transport.refusals},{"native_transport_unqualified_owners",transport.unqualified_owner_calls},{"binding_residual_owners",transport.binding.residual_owners},{"binding_residual_calls",transport.binding.residual_calls},{"outer_gp_simd_preserved_except_RET4",true}});
''';body=body[:first]+replacement+body[last:]
   generated=(ROOT/'src/physics/vehicles/physics_vehicle_update_generated.cpp').read_text();start=generated.index('static bool handoff_native_object_velocity(VehicleState&s) {');end=generated.index('\n}',start)+2;helper=generated[start:end];prefix='#include "halo2/native_tu15_object_velocity_transport.hpp"\n#include "halo2/native_tu15_object_velocity_handoff.hpp"\n#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"\nusing halo2::physics_vehicle_runtime_detail::VehicleState;\n'+prefix+helper+'\n'
  elif a.binding:
   first=body.index(' NativeTu15ArrayAllocatorMemory allocation_memory(memory);');last=body.index('\n std::size_t different{},compared{};');replacement='''
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ObjectActivationBindingActivity activity;
 const NativeTu15ObjectActivationResidualCall invoke=[&](auto owner,auto arguments,auto receiver,unsigned pop,auto&state){
  const auto returned=0x1ccacau,who=receiver.value_or(0u);std::uint32_t result{};
  switch(arguments.size()){
  case0:result=bridge.call(owner,who,{},returned,state);break;
  case1:result=bridge.call(owner,who,{arguments[0]},returned,state);break;
  case2:result=bridge.call(owner,who,{arguments[0],arguments[1]},returned,state);break;
  case3:result=bridge.call(owner,who,{arguments[0],arguments[1],arguments[2]},returned,state);break;
  case4:result=bridge.call(owner,who,{arguments[0],arguments[1],arguments[2],arguments[3]},returned,state);break;
  default:throw std::runtime_error("Unqualified object binding argument count");}
  const auto&call=bridge.calls.back();check(call.at("after_esp").get<std::uint32_t>()==call.at("before_esp").get<std::uint32_t>()+4u+pop,"Actual remaining object owner RET-pop differs");return result;
 };
 const auto legacy_refusal=native_tu15_object_velocities_unsupported(memory,object,with_linear?&linear:nullptr,with_angular?&angular:nullptr);
 native_tu15_set_object_velocities_with_residuals(memory,object,with_linear?&linear:nullptr,with_angular?&angular:nullptr,fp,0,{arena+0x100u,arena+0x104u},invoke,{},&activity);
 wake_stats=activity.activation;agent_stats=activity.agents;service_stats=activity.services;
 std::uint64_t owner_total{};for(const auto&[owner,count]:activity.residual_owners)owner_total+=count;check(owner_total==activity.residual_calls&&activity.residual_calls==bridge.calls.size(),"Object binding residual identities/counts differ");
 report.update(J{{"reusable_native_binding_exercised",true},{"binding_setters",activity.setters},{"binding_native_factories",activity.native_agent_factories},{"binding_native_shape_getters",activity.native_shape_type_getters},{"binding_native_motion_getters",activity.native_motion_type_getters},{"binding_native_policy_getters",activity.native_policy_type_getters},{"binding_native_pool_allocations",activity.native_pool_allocations},{"binding_native_pool_releases",activity.native_pool_releases},{"binding_residual_calls",activity.residual_calls},{"binding_residual_owners",activity.residual_owners},{"binding_unconverted_factories",activity.unconverted_agent_factories},{"binding_unconverted_shape_getters",activity.unconverted_shape_type_getters},{"binding_unconverted_motion_getters",activity.unconverted_motion_type_getters},{"binding_unconverted_policy_getters",activity.unconverted_policy_type_getters}});
''';replacement=replacement.replace('case0:','case 0:').replace('case1:','case 1:').replace('case2:','case 2:').replace('case3:','case 3:').replace('case4:','case 4:');body=body[:first]+replacement+body[last:]
  if new_policy_binding:
   owner='transport.binding'if a.transport else'activity';needle='\n std::size_t different{},compared{};';assert body.count(needle)==1
   body=body.replace(needle,'\n report.update(J{{"binding_native_body_policy_calls",'+owner+'.body_policy.calls},{"binding_native_body_policy_constructors",'+owner+'.body_policy.constructors},{"binding_native_policy_bounds_owners",'+owner+'.native_policy_bounds_owners},{"binding_unconverted_policy_bounds_owners",'+owner+'.unconverted_policy_bounds_owners},{"binding_unconverted_policy_types",'+owner+'.body_policy.unconverted_type_queries},{"binding_unconverted_policy_destructions",'+owner+'.body_policy.unconverted_destructions}});'+needle,1)
   if not a.transport:
    needle='{arena+0x100u,arena+0x104u},invoke';assert body.count(needle)==1
    body=body.replace(needle,'{arena+0x100u,arena+0x104u,arena+0x200u,arena+0x400u,0x1000u},invoke',1)
  if new_constraint_binding:
   owner='transport.binding'if a.transport else'activity';needle='\n std::size_t different{},compared{};';assert body.count(needle)==1
   body=body.replace(needle,'\n report.update(J{{"binding_native_body_constraint_calls",'+owner+'.body_constraints.calls},{"binding_native_body_constraint_flag_skips",'+owner+'.body_constraints.flag_skips},{"binding_native_body_constraint_inactive_skips",'+owner+'.body_constraints.inactive_skips},{"binding_native_body_constraint_empty_banks",'+owner+'.body_constraints.empty_banks},{"binding_native_body_constraint_rows",'+owner+'.body_constraints.rows},{"binding_unbound_body_constraint_owners",'+owner+'.body_constraints.dependent_owners},{"binding_unbound_body_constraint_services",'+owner+'.body_constraints.unbound_services}});'+needle,1)
  if new_constraint_services:
   owner='transport.binding'if a.transport else'activity';needle='\n std::size_t different{},compared{};';assert body.count(needle)==1
   body=body.replace(needle,'\n report.update(J{{"binding_native_constraint_lookup_calls",'+owner+'.constraint_lookup.calls},{"binding_native_constraint_lookup_candidates",'+owner+'.constraint_lookup.candidates},{"binding_native_constraint_capacity_calls",'+owner+'.constraint_capacity.calls},{"binding_unbound_constraint_removals",'+owner+'.constraint_capacity.unbound_removals}});'+needle,1)
  if new_owners and not a.transport:
   needle='wake_stats=activity.activation;agent_stats=activity.agents;service_stats=activity.services;';assert body.count(needle)==1
   body=body.replace(needle,needle+'\n report.update(J{{"binding_native_manager_factories",activity.native_manager_factories},{"binding_unconverted_manager_factories",activity.unconverted_manager_factories},{"binding_native_manager_registrations",activity.managers.registrations},{"binding_native_payload_filters",activity.payload_filter.calls},{"binding_payload_native_motion_queries",activity.payload_filter.native_motion_queries},{"binding_payload_unconverted_motion_queries",activity.payload_filter.unconverted_motion_queries}});',1)
  if a.policy:
   prefix='#include "halo2/native_tu15_body_policy.hpp"\n'+prefix
   needle=' agents.filter_pair=';assert body.count(needle)==1
   body=body.replace(needle,''' NativeTu15BodyPolicyStats policy_stats;NativeTu15BodyPolicyServices policy_services;policy_services.allocation=allocation;
 policy_services.bounds.child_bounds=[&](auto shape,auto owner,auto transform,auto margin,auto output,auto&fp){bridge.call(owner,shape.identity,{transform.identity,margin,output.identity},0x1ccacau,fp);};
 policy_services.policy_kind=[&](auto current,auto owner,auto&fp){return bridge.call(owner.identity,current,{},0x1ccacau,fp);};
 policy_services.destroy_policy=[&](auto current,auto owner,auto flags,auto&fp){bridge.call(owner.identity,current,{flags},0x1ccacau,fp);};
 policy_services.release_base_lock=[&](auto current,auto flags,auto&fp){bridge.call(0x2e12a0u,current,{flags},0x1ccacau,fp);};
 agents.filter_pair=''',1)
   needle='agents.mark_body=[&](auto current,auto flags,auto&fp){bridge.call(0x31b730u,current.identity,{flags},0x282aa0u,fp);};';assert body.count(needle)==1
   body=body.replace(needle,'agents.mark_body=[&](auto current,auto flags,auto&fp){native_tu15_select_body_policy(memory,current,flags,{{arena+0x200u}},fp,0,policy_services,{},&policy_stats);};',1)
   needle='\n std::size_t different{},compared{};';assert body.count(needle)==1
   body=body.replace(needle,'\n report.update(J{{"native_body_policy_calls",policy_stats.calls},{"native_body_policy_constructors",policy_stats.constructors},{"native_body_policy_bounds",policy_stats.shape_bounds_calls},{"native_body_policy_installs",policy_stats.installs},{"native_body_policy_default_installs",policy_stats.default_installs},{"native_body_policy_native_type_queries",policy_stats.native_type_queries},{"native_body_policy_unconverted_type_queries",policy_stats.unconverted_type_queries},{"native_body_policy_native_destructions",policy_stats.native_destructions},{"native_body_policy_unconverted_destructions",policy_stats.unconverted_destructions}});'+needle,1)
   if a.policy_native_bounds:
    prefix=''.join('#include "halo2/native_tu15_'+name+'.hpp"\n'for name in('shape_bounds','centered_shape_bounds','compound_shape_bounds','sphere_shape_bounds'))+prefix
    needle=' policy_services.bounds.child_bounds=[&](auto shape,auto owner,auto transform,auto margin,auto output,auto&fp){bridge.call(owner,shape.identity,{transform.identity,margin,output.identity},0x1ccacau,fp);};';assert body.count(needle)==1
    body=body.replace(needle,''' std::map<std::uint32_t,std::uint64_t>native_policy_bounds,residual_policy_bounds;auto bounds_cursor=arena+0x1800u;
 std::function<void(NativeTu15BoundsShape,std::uint32_t,NativeTu15BoundsTransform,std::uint32_t,NativeTu15BoundsOutput,NativeTu15PhysicsFrameFp&)>shape_bounds;
 shape_bounds=[&](auto shape,std::uint32_t owner,auto transform,std::uint32_t margin,auto output,auto&fp){
  if(owner==0x306760u){++native_policy_bounds[owner];native_tu15_convex_box_bounds(memory,shape,transform,margin,output,fp,0);}
  else if(owner==0x307e30u){++native_policy_bounds[owner];native_tu15_centered_shape_bounds(memory,shape,transform,margin,output,fp,0);}
  else if(owner==0x2e8d90u){++native_policy_bounds[owner];native_tu15_sphere_shape_bounds(memory,shape,transform,margin,output,fp,0);}
  else if(owner==0x2e8120u){++native_policy_bounds[owner];native_tu15_endpoint_shape_bounds(memory,shape,transform,margin,output,fp,0);}
  else if(owner==0x306170u||owner==0x2e8870u){
   ++native_policy_bounds[owner];check(bounds_cursor>=arena+0x400u,"Native policy bounds exhausted caller-owned data storage");const auto previous=bounds_cursor;bounds_cursor-=64u;
   try{if(owner==0x306170u)native_tu15_compound_shape_bounds(memory,shape,transform,margin,output,{bounds_cursor},fp,policy_services.bounds);
    else native_tu15_transformed_shape_bounds(memory,shape,transform,[margin]{return margin;},[output]{return output;},{bounds_cursor},fp,policy_services.bounds);}
   catch(...){bounds_cursor=previous;throw;}bounds_cursor=previous;
  }
  else if(owner==0x2e9600u){++native_policy_bounds[owner];native_tu15_forward_shape_bounds(memory,shape,transform,margin,output,fp,policy_services.bounds);}
  else if(owner==0x2e5e80u){++native_policy_bounds[owner];native_tu15_motion_shape_bounds(memory,shape,transform,margin,output,fp,policy_services.bounds);}
  else{++residual_policy_bounds[owner];bridge.call(owner,shape.identity,{transform.identity,margin,output.identity},0x1ccacau,fp);}
 };
 policy_services.bounds.child_bounds=shape_bounds;''',1)
    needle='\n std::size_t different{},compared{};';assert body.count(needle)==1
    body=body.replace(needle,'\n report.update(J{{"native_policy_bounds_owners",native_policy_bounds},{"residual_policy_bounds_owners",residual_policy_bounds}});'+needle,1)
  if a.manager:
   prefix='#include "halo2/native_tu15_activation_contact_manager.hpp"\n'+prefix
   needle=' agents.filter_pair=';assert body.count(needle)==1
   body=body.replace(needle,''' NativeTu15ActivationContactManagerStats manager_stats;NativeTu15ActivationContactManagerServices manager_services;manager_services.allocation=allocation;
 manager_services.destroy_body=[&](auto current,auto owner,auto flags,auto&fp){bridge.call(owner.identity,current.identity,{flags},0x1ccacau,fp);};
 manager_services.release_base_lock=[&](auto current,auto flags,auto&fp){bridge.call(0x2e12a0u,current,{flags},0x1ccacau,fp);};
 agents.filter_pair=''',1)
   needle='agents.create_manager=[&](auto factory,auto owner,auto first,auto second,auto context,auto&fp){';assert body.count(needle)==1
   body=body.replace(needle,needle+'if(owner.identity==0x3232e0u)return native_tu15_create_activation_contact_manager(memory,factory,first,second,fp,manager_services,{},&manager_stats);',1)
   needle='\n std::size_t different{},compared{};';assert body.count(needle)==1
   body=body.replace(needle,'\n report.update(J{{"native_manager_factories",manager_stats.factories},{"native_manager_constructors",manager_stats.constructors},{"native_manager_collectors",manager_stats.collectors},{"native_manager_registrations",manager_stats.registrations},{"native_manager_growths",manager_stats.array_growths},{"native_manager_unregistrations",manager_stats.unregistrations},{"native_manager_body_destructions",manager_stats.body_destructions},{"native_manager_storage_releases",manager_stats.storage_releases},{"native_manager_collector_destructions",manager_stats.collector_destructions}});'+needle,1)
  driver.write_text('#include "halo2/native_tu15_object_motion.hpp"\n#include "halo2/native_tu15_object_activation_residuals.hpp"\n#include "halo2/native_tu15_activation_services.hpp"\n#include "halo2/native_tu15_contact_agent_factory.hpp"\n#include "halo2/native_tu15_array_allocator.hpp"\n'+prefix+body);base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458';contexts=[];cases=[]
  for name in('gemini-column0','gemini-column1'):
   variations=(2,3)if a.transport else range(5)
   case_name=lambda island,variation,depth:f'{name}_{island}_{variation}'+(f'_d{depth}'if a.live_depths!=[0]else'')
   if not any(re.fullmatch(a.case,case_name(island,variation,depth))for island in(0,12)for variation in variations for depth in a.live_depths):continue
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';replay=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(replay)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(replay/'original-hardware').exists(),r.stderr[-2000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,ordinary_full_frame_reference_exit=r.returncode))
   for island in(0,12):
    for variation in variations:
     for depth in a.live_depths:
      if re.fullmatch(a.case,case_name(island,variation,depth)):cases.append(dict(name=case_name(island,variation,depth),elf=str(replay/'original-hardware'),capture=str(capture/'capture.json'),island=island,variation=variation,depth=depth))
  assert cases;project=scratch/'project';project.mkdir()
  for name in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{name}.cpp',project/(name+'.cpp'))
  if new_constraint_binding:shutil.copyfile(ROOT/'src/core/native_tu15_data_array_allocate.cpp',project/'data_array_allocate.cpp')
  if a.binding_source_overlay:
   shutil.copyfile(a.binding_source_overlay/'src/physics/native_tu15_object_activation_residuals.cpp',project/'object_activation_residuals.cpp');target=project/'include/halo2';target.mkdir(parents=True);shutil.copyfile(a.binding_source_overlay/'include/halo2/native_tu15_object_activation_residuals.hpp',target/'native_tu15_object_activation_residuals.hpp')
   if a.transport and(a.binding_source_overlay/'src/physics/native_tu15_object_velocity_transport.cpp').exists():shutil.copyfile(a.binding_source_overlay/'src/physics/native_tu15_object_velocity_transport.cpp',project/'object_velocity_transport.cpp')
  shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');variants={};mutation_unit='activation_contact_manager'if a.manager else'body_policy'if a.policy else'object_motion';text=(ROOT/f'src/physics/native_tu15_{mutation_unit}.cpp').read_text()
  mutations=(('wrong_manager_type','word(0x32325bu,p,0x41f124u)','word(0x32325bu,p,0x41f125u)'),('wrong_listener_identity','entries+current_count*4u,listener','entries+current_count*4u,listener+4u'),('wrong_body_reference','word(body+6u,2)+1u,2);append_listener','word(body+6u,2)+2u,2);append_listener'))if a.manager else(('skip_native_activation','native_tu15_activate_physics_body(m, {b}, context->floating_point, context->services,\n                                       context->observer, context->activity);','(void)b;'),('wrong_object_velocity','w3(m, o + 0x88, *linear);','w3(m, o + 0x8c, *linear);'))
  if a.policy:mutations=(('wrong_policy_threshold','p+0x50u,0x3c23d70au','p+0x50u,0x3c23d70bu'),('wrong_policy_extent','word(0x31e7d1u,p+8u,result)','word(0x31e7d1u,p+12u,result)'),('wrong_policy_reference','word(0x1e10adu,policy+6u,word(policy+6u,2)+1u,2)','word(0x1e10adu,policy+6u,word(policy+6u,2)+2u,2)'))
  for name,needle,replacement in mutations:
   assert text.count(needle)==1,name;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  negative_case=next((c for c in cases if c['island']==12 and c['variation']==(2 if a.transport else 0)),None)
  if negative_case is None:variants={}
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nj=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and(j.get("different_bytes",0)>0 or not j.get("fp_equal",True)),(r.returncode,j)\nprint("Actual original B9800 rejects native velocity/wake mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(NativeObjectActivation LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT "{driver}")
add_library(dependencies OBJECT {' '.join(name+'.cpp'for name in (units+(['data_array_allocate']if new_constraint_binding else[]))if name!=mutation_unit)})
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE "{project}/include" "{ROOT}/include" "{ROOT}/src")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retained=[]
  for variant in('native',*variants):
   filename=mutation_unit+'.cpp'if variant=='native'else variant+'.cpp';cmake+=f'add_executable(object_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {filename})\ntarget_include_directories(object_{variant} PRIVATE "{project}/include" "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features(object_{variant} PRIVATE cxx_std_20)\ntarget_compile_options(object_{variant} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for c in cases if variant=='native'else[negative_case]:
    name=variant+'_'+c['name'];output='evidence/'+name+'.json';retained.append(output);command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:object_{variant}> "{c["elf"]}" "{c["capture"]}" {c["island"]} {c["variation"]+c["depth"]*16} "${{CMAKE_BINARY_DIR}}/{output}"';cmake+=f'add_test(NAME object_{name} COMMAND {command})\nset_tests_properties(object_{name} PROPERTIES TIMEOUT 45)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','object_native',*sum((['--target','object_'+name]for name in variants),[]),'--filter','^object_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',path]for path in retained),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,reusable_native_binding=a.binding,actual_F2091_transport=a.transport,native_contact_manager=a.manager,native_body_policy=a.policy,native_policy_bounds=a.policy_native_bounds,new_native_predicate_and_manager=new_owners,native_policy_binding=new_policy_binding,native_constraint_binding=new_constraint_binding,native_constraint_services=new_constraint_services,binding_source_overlay=str(a.binding_source_overlay)if a.binding_source_overlay else None,declared_incoming_x87_depths=a.live_depths,compiler_results=results,contexts=contexts,physical_cases_per_compiler=len(cases),negative_controls=list(variants),source_hashes=pins,current_sources_unchanged=unchanged,image_sha256=NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],native_parent_receives_guest_register_state=False,production_native_only_closure=False,remaining=['complete ordered reads/stores and fault/mutable-callback/alias domains','native remaining manager/agent factories,query and body-policy lifetime/body update services','actual production boundary and visible multiplayer'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
