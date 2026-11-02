#pragma once
#include "halo2/native_tu15_contact_candidates.hpp"
#include "halo2/native_tu15_contact_candidate_buffers.hpp"
#include "halo2/native_tu15_collision_key_query_binding.hpp"
#include "halo2/native_tu15_collision_component.hpp"
#include "halo2/native_tu15_collection_filter.hpp"
#include "halo2/native_tu15_shape_bounds.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
namespace halo2 {
struct NativeTu15ContactCandidateBindingScratch {
    NativeTu15ContactCandidateScratch candidate; //0x3D0 retained bytes, including inline keys
    NativeTu15CollisionKeyQueryBindingScratch keys;
    NativeTu15CollisionResolveArguments resolve_arguments; //eight ordinary key/output DATA bytes
    NativeTu15CollisionResolveScratch resolve;
    NativeTu15CollectionFilterScratch filter;
};
struct NativeTu15ContactCandidateBindingProviders {
    // Root constructs all callbacks against this same memory view. The native
    // scope below rejects any unconverted provider before its CPU transport.
    const SourcePhysicsContactMemory* owner_memory{};
    NativeTu15ArrayGrowthServices allocation;
    NativeTu15ContactBufferServices buffers;
    NativeTu15ShapeBoundsServices bounds;
    NativeTu15ContactAgentDispatchServices agents;
    NativeTu15CollectionFilterServices filter;
    std::function<void(NativeTu15ContactAgent,NativeTu15ContactVirtualOwner,
        NativeTu15PhysicsFrameFp&)> release_agent;
};
struct NativeTu15ContactCandidateBindingObservation {
    // Retained candidate/query DATA only. Raw hidden MOPP-cache/control-cell
    // preimages and private address-fault observation are separate domains.
    SourcePhysicsContactRecordObserver retained_stores;
};
struct NativeTu15ContactCandidateBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},bounds_calls{},key_queries{},
        resolutions{},filters{},dispatches{},agent_releases{},sorts{},
        buffer_constructions{},buffer_destructions{},reserves{},unqualified_owners{},unsupported_depths{};
    std::map<std::uint32_t,std::uint64_t> refused_owners;
    NativeTu15ContactCandidateStats candidate;
    NativeTu15CollisionKeyQueryBindingActivity keys;
    NativeTu15CollisionSurfaceStats surfaces;
};
namespace native_candidate_binding_detail {
inline void put(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value) {
    std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8u*i));m.write(p,b);
}
inline void refuse(NativeTu15ContactCandidateBindingActivity*a,std::uint32_t owner,const char*phase) {
    if(a){++a->unqualified_owners;++a->refused_owners[owner];}
    throw std::runtime_error(std::string("Native contact candidate requires qualified ")+phase+" owner="+std::to_string(owner));
}
}
// Actual1F7C10 ordinary key/output formals. These are semantic DATA, not a GP
// or control-frame image. The receiver is unused by this authenticated owner.
inline NativeTu15ContactChild native_tu15_resolve_bound_contact_candidate_child(
    SourcePhysicsContactMemory&memory,NativeTu15ContactChild collection,
    NativeTu15ContactVirtualOwner owner,std::uint32_t key,std::uint32_t output,
    NativeTu15CollisionResolveArguments arguments,NativeTu15CollisionResolveScratch scratch,
    NativeTu15PhysicsFrameFp&fp,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15ContactCandidateBindingActivity*activity=nullptr) {
    (void)collection;
    if(owner.identity!=0x1f7c10u)native_candidate_binding_detail::refuse(activity,owner.identity,"surface resolver");
    native_candidate_binding_detail::put(memory,arguments.identity,key);
    native_candidate_binding_detail::put(memory,arguments.identity+4u,output);
    if(activity)++activity->resolutions;
    return {native_tu15_resolve_surface_component(memory,arguments,scratch,fp,observe,
        activity?&activity->surfaces:nullptr).identity};
}
// Sole native575-site2F36F0 algorithm and its actual typed service closure.
// Providers are existing native owners; no executor/GP/expected-data callback
// is accepted. The same guard spans every bound helper and provider call.
inline void native_tu15_update_bound_contact_candidates(
    SourcePhysicsContactMemory&memory,NativeTu15ContactProducer producer,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext context,NativeTu15ContactVirtualOwner factory,
    NativeTu15ContactCandidateBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const NativeTu15ContactCandidateBindingProviders&providers,
    NativeTu15PairDeletionNativeScope&native_only_scope,
    const NativeTu15ContactCandidateBindingObservation&observation={},
    NativeTu15ContactCandidateBindingActivity*activity=nullptr,
    NativeTu15PairDeletionNativeScopeActivity*scope_activity=nullptr) {
    NativeTu15PairDeletionNativeScopeGuard guard(native_only_scope,scope_activity);
    if(activity)++activity->attempts;
    const auto&observe=observation.retained_stores;
    try {
        if(depth){if(activity)++activity->unsupported_depths;
            throw std::runtime_error("Native contact candidate binding requires witnessed empty live x87 DATA");}
        if(providers.owner_memory!=&memory)
            throw std::runtime_error("Native contact candidate providers require their same memory view");
        NativeTu15ContactCandidateServices services;
        services.bounds=[&](auto shape,auto owner,auto transform,auto margin,auto output,auto&current) {
            if(owner.identity!=0x2e5e80u||!providers.bounds.child_bounds)
                native_candidate_binding_detail::refuse(activity,owner.identity,"shape bounds");
            if(activity)++activity->bounds_calls;
            providers.bounds.child_bounds({shape.identity},owner.identity,{transform},margin,{output},current);
        };
        services.query_keys=[&](auto shape,auto owner,auto bounds,auto keys,auto&current) {
            if(owner.identity!=0x1f6d10u)native_candidate_binding_detail::refuse(activity,owner.identity,"key query");
            if(activity)++activity->key_queries;
            native_tu15_query_bound_collision_keys(memory,shape.identity,bounds,keys,
                scratch.keys,current,depth,providers.allocation,{observe},activity?&activity->keys:nullptr);
        };
        services.resolve_child=[&](auto collection,auto owner,auto key,auto output,auto,auto&current) {
            return native_tu15_resolve_bound_contact_candidate_child(memory,collection,owner,key,output,
                scratch.resolve_arguments,scratch.resolve,current,observe,activity);
        };
        services.filter=[&](auto receiver,auto owner,auto output,auto,auto a,auto b,
            auto parent,auto key,auto,auto&current) {
            if(owner.identity!=0x1cbe70u)native_candidate_binding_detail::refuse(activity,owner.identity,"collection filter");
            if(activity)++activity->filters;
            // Actual RET24 arguments3/4 keep their ABI order. The shared
            // filter names those ordinary inputs second/first, respectively.
            return native_tu15_filter_collection_child(memory,receiver,output,a,b,{parent},key,
                scratch.filter,current,providers.filter,observe);
        };
        services.create_agent=[&](auto owner,auto a,auto b,auto settings,auto data,auto,auto&current) {
            if(owner.identity!=0x2f3140u)native_candidate_binding_detail::refuse(activity,owner.identity,"agent dispatch");
            if(activity)++activity->dispatches;
            return native_tu15_dispatch_contact_agent(memory,a,b,settings,data,current,providers.agents);
        };
        services.release_agent=[&](auto agent,auto owner,auto,auto&current) {
            if(!providers.release_agent)native_candidate_binding_detail::refuse(activity,owner.identity,"agent release");
            if(activity)++activity->agent_releases;providers.release_agent(agent,owner,current);
        };
        services.sort_keys=[&](auto keys,auto low,auto high,auto comparator,auto&) {
            if(activity)++activity->sorts;native_tu15_sort_contact_keys(memory,keys,low,high,comparator,observe);
        };
        services.initialize_entries=[&](auto header,auto count,auto&current) {
            if(activity)++activity->buffer_constructions;
            (void)native_tu15_initialize_contact_entries(memory,{header},count,current,providers.buffers,observe);
        };
        services.destroy_entries=[&](auto header,auto&current) {
            if(activity)++activity->buffer_destructions;
            native_tu15_destroy_contact_entries(memory,{header},current,providers.buffers,observe);
        };
        services.reserve_entries=[&](auto header,auto count,auto stride,auto&current) {
            if(activity)++activity->reserves;
            native_tu15_reserve_array(memory,{header},count,stride,current,providers.allocation,observe);
        };
        services.release=[&](auto allocator,auto owner,auto data,auto bytes,auto tag,auto,auto&current) {
            if(owner.identity!=0x149e00u||!providers.allocation.release)
                native_candidate_binding_detail::refuse(activity,owner.identity,"array release");
            providers.allocation.release({allocator,owner.identity,data,bytes,tag},current);
        };
        services.allocate=[&](auto allocator,auto owner,auto bytes,auto tag,auto&current) {
            if(owner.identity!=0x149da0u||!providers.allocation.allocate)
                native_candidate_binding_detail::refuse(activity,owner.identity,"array allocation");
            return providers.allocation.allocate({allocator,owner.identity,bytes,tag},current);
        };
        native_tu15_update_contact_candidates(memory,producer,first,second,context,factory,
            scratch.candidate,fp,depth,services,observe,activity?&activity->candidate:nullptr);
        if(activity)++activity->completed;
    } catch(...) {if(activity)++activity->refusals;throw;}
}
} // namespace halo2
