#pragma once
#include "halo2/native_tu15_contact_candidates_binding.hpp"
#include "halo2/native_tu15_convex_agent_binding.hpp"
#include "halo2/native_tu15_stack_arena.hpp"
#include "halo2/native_tu15_compound_shape_bounds.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include "halo2/native_tu15_contact_producer_native_providers.hpp"
#include "halo2/native_tu15_moving_shape_process_binding.hpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
namespace {
using namespace halo2;
constexpr std::uint32_t PARENT_DATA=0x74000000u;
struct ParentActivities {
 NativeTu15ProducerProviderActivity providers;
 NativeTu15ContactCandidateBindingActivity candidates;
 NativeTu15ConvexSimplexBindingActivity solver;
 NativeTu15ContactPublicationBindingActivity publication;
 NativeTu15ContactPointBindingActivity removal;
 NativeTu15ContactCreationGameplayStats gameplay;
 NativeTu15PoolArrayServiceActivity pool;
 NativeTu15MovingShapeBindingActivity moving;
};
void run_native_parent(SourcePhysicsContactMemory&storage,const J&input,
 NativeTu15PhysicsFrameFp&fp,ParentActivities&stats){
 const SourcePhysicsContactRecordObserver o;
 NativeTu15ConvexSimplexMemory m(storage);NativeTu15PairDeletionNativeScope scope;
 NativeTu15PairDeletionNativeScopeGuard all_native(scope);
 const auto unbound=[&](std::uint32_t owner){scope.reject_unconverted(owner);throw std::runtime_error("Native parent requires owner="+std::to_string(owner));};
 NativeTu15ContactCandidateBindingProviders providers;providers.owner_memory=&m;
 NativeTu15ArrayAllocatorMemory router(m);NativeTu15ArrayAllocatorServices heap;
 heap.allocate_owner=[&](const auto&r,auto&){if(r.owner!=0x233250u)unbound(r.owner);NativeTu15PoolMemory pool(m,r.receiver,o);return native_tu15_pool_allocate(pool,r.bytes);};
 heap.release_owner=[&](const auto&r,auto&){if(r.owner!=0x233420u)unbound(r.owner);NativeTu15PoolMemory pool(m,r.receiver,o);if(!native_tu15_pool_release(pool,r.data,r.bytes))unbound(r.owner);};
 heap.default_heap=[&](auto&)->std::uint32_t{unbound(0x149930u);return 0;};
 providers.allocation.allocate=[&](const auto&r,auto&f){if(r.owner!=0x149da0u)unbound(r.owner);return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,f,heap);};
 providers.allocation.release=[&](const auto&r,auto&f){if(r.owner!=0x149e00u)unbound(r.owner);native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,f,heap);};
 providers.buffers.allocate_stack_block=[&](auto arena,auto owner,auto bytes,auto&f){if(owner.identity!=0x2e10a0u)unbound(owner.identity);return native_tu15_allocate_stack_arena(m,arena.identity,bytes,f,providers.allocation,o);};
 providers.buffers.release_stack_block=[&](auto arena,auto owner,auto block,auto bytes,auto&f){if(owner.identity!=0x2e1110u)unbound(owner.identity);native_tu15_release_stack_arena(m,arena.identity,block,bytes,f,providers.allocation,o);};
 providers.buffers.release_owned_components=[&](auto arena,auto owner,auto block,auto bytes,auto tag,auto&f){providers.allocation.release({arena.identity,owner.identity,block,bytes,tag},f);};
 providers.bounds.child_bounds=[&](auto shape,auto owner,auto transform,auto margin,auto output,auto&f){
  if(owner==0x2e5e80u)native_tu15_motion_shape_bounds(m,shape,transform,margin,output,f,providers.bounds,o);
  else if(owner==0x2e9600u)native_tu15_forward_shape_bounds(m,shape,transform,margin,output,f,providers.bounds);
  else if(owner==0x2e8870u)native_tu15_transformed_shape_bounds(m,shape,transform,[&]{return margin;},[&]{return output;},{0x74004300u},f,providers.bounds,o);
  else if(owner==0x306760u)native_tu15_convex_box_bounds(m,shape,transform,margin,output,f,0,o);
  else if(owner==0x2e8120u)native_tu15_endpoint_shape_bounds(m,shape,transform,margin,output,f,0,o);
  else unbound(owner);
 };
 const auto kind=[&](auto,auto owner,auto&)->std::uint32_t{if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();if(auto type=native_tu15_activation_type(owner.identity))return *type;unbound(owner.identity);return 0;};
 providers.agents.classify_shape=kind;
 providers.agents.create_agent=[&](auto owner,auto first,auto second,auto,auto user,auto&f){if(owner.identity!=0x2e4eb0u)unbound(owner.identity);return native_tu15_create_bound_convex_contact_agent(m,{[&]{return first.identity;},[&]{return second.identity;},[&]{return user;}},{0x74005000u,0x74005100u,0x74005200u,0x74006000u,0x74007000u,0x74008000u},f,0,o);};
 providers.filter.classify_shape=kind;providers.filter.motion_type=[&](auto,auto owner,auto&f){return kind(0,owner,f);};
 const NativeTu15CollisionResolveScratch resolve{{0x74004400u},{0x74004420u},{0x74004440u},{0x74004450u},0x74004460u};
 providers.filter.resolve_child=[&](auto collection,auto owner,auto key,auto output,auto&f){return native_tu15_resolve_bound_contact_candidate_child(m,collection,owner,key,output,{0x74004500u},resolve,f,o,&stats.candidates);};
 providers.release_agent=[&](auto,auto owner,auto&){unbound(owner.identity);};
 const auto p=PARENT_DATA;
 NativeTu15ProducerNativeProviders native;native.owner_memory=&m;native.candidates=providers;
 const NativeTu15ContactShapeScratch support_child{p+0x3600u,p+0x3610u,nullptr},support{p+0x3500u,p+0x3510u,&support_child};
 native.geometry=native_tu15_make_bound_convex_contact_services(m,
  {{p+0x3800u,p+0x3804u,p+0x3808u,p+0x3810u,p+0x3820u,
   {p+0x3000u,p+0x3100u,p+0x3140u,{p+0x2800u,{p+0x2600u,p+0x2604u,
    {p+0x2000u,p+0x2010u,p+0x2030u,p+0x2060u,p+0x2020u}}}}},support},0,{},&stats.solver);
 NativeTu15ContactPublicationNativeServices published;
 published.creation.allocation=[&](auto&view){return native_tu15_make_pool_array_services(view,{},&stats.pool);};
 published.creation.notify_listener=[&](auto&view,auto listener,auto owner,auto event,auto&current){
  native_tu15_dispatch_contact_creation_gameplay_listener(view,listener,owner,event,
   {p+0x6000u,p+0x6004u,p+0x6008u,p+0x6010u,p+0x6020u,p+0x6030u,p+0x6034u,p+0x6038u},current,0,{},&stats.gameplay);
 };
 published.remove_point=[&](auto&view,auto manager,auto id,auto&current){
  const auto allocation=native_tu15_make_pool_array_services(view,{},&stats.pool);
  native_tu15_remove_bound_contact_point(view,manager,id,{p+0x5200u,p+0x5220u,p+0x5240u,p+0x5220u},current,0,allocation,{},&stats.removal);
 };
 native.publication=native_tu15_make_contact_publication_services(m,
  {{p+0x5000u},{p+0x5100u,p+0x5120u,p+0x5140u}},0,published,scope,&stats.publication);
 native.append_record=[&](auto&view,auto array,auto constructor,auto&current){
  const auto allocation=native_tu15_make_pool_array_services(view,{},&stats.pool);
  (void)native_tu15_append_contact_record(view,array,constructor,current,allocation);
 };
 native.process_rejected_pair=[&](auto&view,auto child,auto owner,auto first,auto second,auto context,auto output,auto&current){
  if(owner.identity!=0x2e5320u)unbound(owner.identity);
  native_tu15_process_bound_moving_shape_contact(view,{child.identity},first,second,context,output,
   {p+0x8000u,0x8000u},current,0,{},scope,&stats.moving);
 };
 const NativeTu15ContactShapeScratch cache_child{p+0x7120u,p+0x7130u,nullptr},cache_support{p+0x7100u,p+0x7110u,&cache_child};
 const NativeTu15ProducerProviderScratch scratch{
  {{p},{{p+0x4000u},p+0x4100u},{p+0x4500u},resolve,{p+0x4600u}},
  {p+0x4500u},resolve,
  {{p+0x370u},{p+0x1000u,p+0x1300u},{{p+0x7000u},cache_support},{{p+0x7040u},cache_support}},
  {p+0x4000u},{p+0x370u}};
 const auto services=native_tu15_make_producer_native_services(m,scratch,0,native,scope,{},&stats.providers);
 native_tu15_produce_contacts(m,{input.at("agent")},{input.at("first")},{input.at("second")},
  {input.at("configuration")},{input.at("output")},{p+0x400u,p+0x4a00u},fp,0,services);
}
}
