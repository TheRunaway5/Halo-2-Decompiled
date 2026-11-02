#pragma once
#include "halo2/native_tu15_convex_agent_factory.hpp"
#include "halo2/native_tu15_contact_shape.hpp"
#include "halo2/native_tu15_array_allocator.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include <stdexcept>
namespace halo2 {
struct NativeTu15ConvexAgentBindingScratch {
    std::uint32_t relative_transform{},cache{},wrapper_scalar{},wrapper_dots{},child_scalar{},child_dots{};
};
struct NativeTu15ConvexAgentBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},pool_allocations{},pool_releases{},
        class_queries{},centers{},supports{},encodings{};
};
// Qualified class23, class8, sphere class4 and authored polygon class24;
// empty-live-x87 domain. Every
// dependency is native; an unsupported allocator/shape/depth refuses explicitly.
inline NativeTu15ContactAgent native_tu15_create_bound_convex_contact_agent(
    SourcePhysicsContactMemory&memory,const NativeTu15ConvexAgentArguments&arguments,
    NativeTu15ConvexAgentBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ConvexAgentBindingActivity*activity=nullptr){
    if(activity)++activity->attempts;
    try{
        if(incoming_x87_depth)throw std::runtime_error("Native TU1.5 convex factory requires its qualified empty live x87 domain");
        NativeTu15ArrayAllocatorMemory router(memory);NativeTu15ArrayAllocatorServices heap;
        heap.allocate_owner=[&](const auto&r,auto&){
            if(r.owner!=0x233250u)throw std::runtime_error("Native TU1.5 convex factory requires qualified pool allocation");
            NativeTu15PoolMemory pool(memory,r.receiver,observe);const auto p=native_tu15_pool_allocate(pool,r.bytes);if(activity)++activity->pool_allocations;return p;
        };
        heap.release_owner=[&](const auto&r,auto&){
            if(r.owner!=0x233420u)throw std::runtime_error("Native TU1.5 convex factory requires qualified pool release");
            NativeTu15PoolMemory pool(memory,r.receiver,observe);if(!native_tu15_pool_release(pool,r.data,r.bytes))throw std::runtime_error("Native TU1.5 convex factory requires qualified large free");if(activity)++activity->pool_releases;
        };
        heap.default_heap=[](auto&)->std::uint32_t{throw std::runtime_error("Native TU1.5 convex factory requires qualified default heap");};
        NativeTu15ConvexAgentServices services;
        services.allocation.allocate=[&](const auto&r,auto&state){if(r.owner!=0x149da0u)throw std::runtime_error("Native TU1.5 convex factory requires qualified allocation route");return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,state,heap);};
        services.allocation.release=[&](const auto&r,auto&state){if(r.owner!=0x149e00u)throw std::runtime_error("Native TU1.5 convex factory requires qualified release route");native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,state,heap);};
        services.shape_class=[&](auto,auto owner,auto&){
            if(owner.identity==0xa9520u){
                if(activity)++activity->class_queries;
                return native_tu15_contact_child_class_23();
            }
            if(owner.identity==0xab4b0u){
                if(activity)++activity->class_queries;
                return native_tu15_contact_child_class_24();
            }
            if(owner.identity==0xad390u||owner.identity==0x2e8cf0u){
                if(const auto kind=native_tu15_activation_type(owner.identity)){
                    if(activity)++activity->class_queries;
                    return *kind;
                }
            }
            throw std::runtime_error("Native TU1.5 convex factory requires a qualified shape class owner: "+std::to_string(owner.identity));
        };
        services.cache.shape_center=[&](auto shape,auto owner,auto output,auto&){native_tu15_contact_shape_center(memory,shape,owner,output,observe);if(activity)++activity->centers;};
        services.cache.support_vertex=[&](auto shape,auto owner,auto direction,auto output,auto&state){const NativeTu15ContactShapeScratch child{scratch.child_scalar,scratch.child_dots,nullptr};native_tu15_contact_shape_support(memory,shape,owner,direction,output,{scratch.wrapper_scalar,scratch.wrapper_dots,&child},state,0,observe);if(activity)++activity->supports;};
        services.cache.vertex_encoding=[&](auto,auto owner,auto&){
            if(activity)++activity->encodings;
            if(owner.identity==0x1cb030u)return native_tu15_contact_shape_vertex_encoding();
            if(owner.identity==0xad740u){
                if(const auto encoding=native_tu15_activation_type(owner.identity))return *encoding;
            }
            throw std::runtime_error("Native TU1.5 convex factory requires qualified encoding owner");
        };
        const auto agent=native_tu15_create_convex_contact_agent(memory,arguments,{scratch.relative_transform,{scratch.cache}},fp,services,observe);
        if(activity)++activity->completed;return agent;
    }catch(...){if(activity)++activity->refusals;throw;}
}
// Actual2F83B0 binding. Qualified sphere/capsule/convex input owners.
// Unsupported support/encoding owners still refuse; classes are owner keyed.
inline NativeTu15ContactAgent native_tu15_create_bound_gsk_contact_agent(
    SourcePhysicsContactMemory&memory,const NativeTu15ConvexAgentArguments&arguments,
    NativeTu15ConvexAgentBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned incoming_x87_depth,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ConvexAgentBindingActivity*activity=nullptr){
    if(activity)++activity->attempts;
    try{
        if(incoming_x87_depth)throw std::runtime_error("Native TU1.5 GSK factory requires its qualified empty live x87 domain");
        NativeTu15ArrayAllocatorMemory router(memory);NativeTu15ArrayAllocatorServices heap;
        heap.allocate_owner=[&](const auto&r,auto&){
            if(r.owner!=0x233250u)throw std::runtime_error("Native TU1.5 GSK factory requires qualified pool allocation");
            NativeTu15PoolMemory pool(memory,r.receiver,observe);const auto p=native_tu15_pool_allocate(pool,r.bytes);if(activity)++activity->pool_allocations;return p;
        };
        heap.release_owner=[&](const auto&r,auto&){
            if(r.owner!=0x233420u)throw std::runtime_error("Native TU1.5 GSK factory requires qualified pool release");
            NativeTu15PoolMemory pool(memory,r.receiver,observe);if(!native_tu15_pool_release(pool,r.data,r.bytes))throw std::runtime_error("Native TU1.5 GSK factory requires qualified large free");if(activity)++activity->pool_releases;
        };
        heap.default_heap=[](auto&)->std::uint32_t{throw std::runtime_error("Native TU1.5 GSK factory requires qualified default heap");};
        NativeTu15ConvexAgentServices services;
        services.allocation.allocate=[&](const auto&r,auto&state){if(r.owner!=0x149da0u)throw std::runtime_error("Native TU1.5 GSK factory requires qualified allocation route");return native_tu15_route_array_allocation(router,r.bytes,r.memory_class,state,heap);};
        services.allocation.release=[&](const auto&r,auto&state){if(r.owner!=0x149e00u)throw std::runtime_error("Native TU1.5 GSK factory requires qualified release route");native_tu15_route_array_release(router,r.data,r.bytes,r.memory_class,state,heap);};
        services.shape_class=[&](auto,auto owner,auto&){
            if(activity)++activity->class_queries;
            if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();
            if(owner.identity==0xad390u || owner.identity==0x31edf0u || owner.identity==0x31ede0u || owner.identity==0x2e8cf0u) {
                if(const auto kind=native_tu15_activation_type(owner.identity))return *kind;
            }
            throw std::runtime_error("Native TU1.5 GSK factory requires qualified shape-class owner="+std::to_string(owner.identity));
        };
        services.cache.shape_center=[&](auto shape,auto owner,auto output,auto&){native_tu15_contact_shape_center(memory,shape,owner,output,observe);if(activity)++activity->centers;};
        services.cache.support_vertex=[&](auto shape,auto owner,auto direction,auto output,auto&state){const NativeTu15ContactShapeScratch child{scratch.child_scalar,scratch.child_dots,nullptr};native_tu15_contact_shape_support(memory,shape,owner,direction,output,{scratch.wrapper_scalar,scratch.wrapper_dots,&child},state,0,observe);if(activity)++activity->supports;};
        services.cache.vertex_encoding=[&](auto,auto owner,auto&){
            if(activity)++activity->encodings;
            if(owner.identity==0x1cb030u)return native_tu15_contact_shape_vertex_encoding();
            // Actual31D520 (mov eax,2;ret) and AD740 (mov eax,1;ret).
            if(owner.identity==0x31d520u || owner.identity==0xad740u) {
                if(const auto encoding=native_tu15_activation_type(owner.identity))return *encoding;
            }
            throw std::runtime_error("Native TU1.5 GSK factory requires qualified encoding owner="+std::to_string(owner.identity));
        };
        const auto agent=native_tu15_create_gsk_contact_agent(memory,arguments,{scratch.relative_transform,{scratch.cache}},fp,services,observe);
        if(activity)++activity->completed;return agent;
    }catch(...){if(activity)++activity->refusals;throw;}
}
} // namespace halo2
