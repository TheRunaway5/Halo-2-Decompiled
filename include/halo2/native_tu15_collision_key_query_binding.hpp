#pragma once
#include "halo2/native_tu15_collision_key_query.hpp"
#include "halo2/native_tu15_mopp_aabb_private_cache.hpp"
#include "halo2/native_tu15_mopp_aabb_traversal.hpp"
#include "halo2/native_tu15_mopp_oriented_aabb_setup.hpp"
namespace halo2 {
struct NativeTu15CollisionKeyQueryBindingScratch {
    NativeTu15CollisionKeyQueryScratch matrices;
    std::uint32_t private_cache{};
};
struct NativeTu15CollisionKeyQueryBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},tree_queries{},oriented_queries{},unknown_instance_owners{};
    NativeTu15MoppAabbTraversalStats traversal;
};
// Observes retained query DATA only. This explicit contract does not request
// events/preimages for the hidden call-local MOPP cache or CPU control cells.
// A generic raw instruction/store observer needs a separate qualified owner.
struct NativeTu15CollisionKeyQueryObservation {SourcePhysicsContactRecordObserver retained_stores;};
// The sole native1F6D10 service composition. No original execution callback is
// accepted. Allocation is its actual typed owner; unknown instance methods
// refuse. All cache consumers use the same call-local read-before-definition
// guard. Raw private-cache observer/address-fault equivalence remains separate.
inline void native_tu15_query_bound_collision_keys(SourcePhysicsContactMemory&memory,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t array,
    NativeTu15CollisionKeyQueryBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ArrayGrowthServices&allocation,const NativeTu15CollisionKeyQueryObservation&observation = {},
    NativeTu15CollisionKeyQueryBindingActivity*activity = nullptr){
    const auto&observe=observation.retained_stores;
    if(activity)++activity->attempts;
    try{
        const auto matrix_end=std::uint64_t(scratch.matrices.identity)+0x100u;
        const auto cache_end=std::uint64_t(scratch.private_cache)+64u;
        if((scratch.matrices.identity&15u)||(scratch.private_cache&15u)||matrix_end>0x100000000ull||
            cache_end>0x100000000ull||(scratch.matrices.identity<cache_end&&scratch.private_cache<matrix_end))
            throw std::runtime_error("Native collision key query requires separate bounded matrix/cache DATA");
        NativeTu15MoppAabbTraversalServices traversal;traversal.allocation=allocation;
        NativeTu15CollisionKeyQueryServices services;
        services.query_shape=[&](auto current_shape,auto current_bounds,auto keys,auto&state){
            if(activity)++activity->tree_queries;
            NativeTu15MoppAabbPrivateCache guarded(memory,scratch.private_cache);
            NativeTu15MoppAabbSetupServices setup;
            setup.traverse=[&](auto cache,auto program,const auto&variables,auto&current,auto older){
                native_tu15_traverse_mopp_aabb(guarded,cache,program,variables,current,older,traversal,observe,
                    activity?&activity->traversal:nullptr);
            };
            native_tu15_query_mopp_aabb_shape(guarded,current_shape,current_bounds,keys,
                scratch.private_cache,state,depth,setup);
        };
        services.query_instance=[&](auto current_shape,auto owner,auto transform,auto extents,auto tolerance,auto keys,auto&state){
            if(owner!=0x2e9640u){if(activity)++activity->unknown_instance_owners;
                throw std::runtime_error("Native collision key query requires actual instance native owner="+std::to_string(owner));}
            if(activity)++activity->oriented_queries;
            NativeTu15MoppAabbPrivateCache guarded(memory,scratch.private_cache);
            NativeTu15MoppAabbSetupServices setup;
            setup.traverse=[&](auto cache,auto program,const auto&variables,auto&current,auto older){
                native_tu15_traverse_mopp_aabb(guarded,cache,program,variables,current,older,traversal,observe,
                    activity?&activity->traversal:nullptr);
            };
            native_tu15_query_mopp_oriented_shape(guarded,current_shape,transform,extents,tolerance,keys,
                scratch.private_cache,state,depth,setup);
        };
        native_tu15_query_collision_keys(memory,shape,bounds,array,scratch.matrices,fp,depth,services,observe);
        if(activity)++activity->completed;
    }catch(...){if(activity)++activity->refusals;throw;}
}
} // namespace halo2
