#pragma once
#include "halo2/native_tu15_contact_query.hpp"
#include "halo2/native_tu15_convex_contact_query.hpp"
#include "halo2/native_tu15_contact_cache.hpp"
#include "halo2/native_tu15_contact_shape.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include "halo2/native_tu15_pair_deletion_native_scope.hpp"
#include <string>
namespace halo2 {
struct NativeTu15ContactQueryCacheBindingScratch {
    NativeTu15ContactCacheScratch vectors;
    NativeTu15ContactShapeScratch support;
};
struct NativeTu15ContactQueryBindingScratch {
    NativeTu15ContactQueryScratch query;
    NativeTu15ConvexContactScratch convex;
    NativeTu15ContactQueryCacheBindingScratch convex_cache,triangle_cache;
};
struct NativeTu15ContactQueryBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},classifications{},convex_caches{},
        triangle_caches{},centers{},supports{},encodings{},convex_queries{};
    NativeTu15ContactQueryStats query;
    NativeTu15PairDeletionNativeScopeActivity native_only;
};
// Retained DATA observation only. This does not request physical CPU/private
// control store preimages or grant a private scratch-address fault domain.
struct NativeTu15ContactQueryObservation {SourcePhysicsContactRecordObserver retained_stores;};
// Sole2E4FE0→2F8630/cache/shape composition. geometry.solve_simplex is the
// genuine native30E930 service requirement; absence refuses at that boundary.
// There is no original executor or GP/SIMD execution-state interface here.
inline std::uint32_t native_tu15_query_bound_contact(SourcePhysicsContactMemory&memory,
    NativeTu15ContactChild child,std::uint32_t valid_result,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext context,std::uint32_t output,
    NativeTu15ContactQueryBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15ConvexContactServices&geometry,
    NativeTu15PairDeletionNativeScope&scope,
    const NativeTu15ContactQueryObservation&observation={},
    NativeTu15ContactQueryBindingActivity*activity=nullptr){
    NativeTu15ContactQueryBindingActivity local;auto&out=activity?*activity:local;
    ++out.attempts;NativeTu15PairDeletionNativeScopeGuard guard(scope,&out.native_only);
    const auto&observe=observation.retained_stores;
    try{
        if(depth)throw std::runtime_error("Native contact query binding requires its qualified empty live x87 domain");
        NativeTu15ContactQueryServices services;
        services.classify_shape=[&](auto,auto owner,auto&){
            ++out.classifications;
            if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();
            if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();
            if(const auto kind=native_tu15_activation_type(owner.identity))return *kind;
            throw std::runtime_error("Native contact query requires its shape class owner="+std::to_string(owner.identity));
        };
        services.initialize_cache=[&](auto owner,auto cache,auto a,auto b,auto transform,auto&current){
            if(owner!=0x309060u&&owner!=0x309150u)
                throw std::runtime_error("Native contact query requires its cache owner="+std::to_string(owner));
            const auto&storage=owner==0x309060u?scratch.convex_cache:scratch.triangle_cache;
            NativeTu15ContactCacheServices cached;
            cached.shape_center=[&](auto shape,auto method,auto destination,auto&){
                native_tu15_contact_shape_center(memory,shape,method,destination,observe);++out.centers;
            };
            cached.support_vertex=[&](auto shape,auto method,auto direction,auto destination,auto&state){
                native_tu15_contact_shape_support(memory,shape,method,direction,destination,
                    storage.support,state,0,observe);++out.supports;
            };
            cached.vertex_encoding=[&](auto,auto method,auto&){
                ++out.encodings;
                if(method.identity==0x1cb030u)return native_tu15_contact_shape_vertex_encoding();
                if(method.identity==0x31d520u||method.identity==0xad740u)
                    return *native_tu15_activation_type(method.identity);
                throw std::runtime_error("Native contact query requires its encoding owner="+std::to_string(method.identity));
            };
            if(owner==0x309060u){
                native_tu15_initialize_convex_contact_cache(memory,cache,a,b,transform,
                    storage.vectors,current,cached,observe);++out.convex_caches;
            }else{
                native_tu15_initialize_triangle_contact_cache(memory,cache,a,b,transform,
                    storage.vectors,current,cached,observe);++out.triangle_caches;
            }
        };
        services.query_convex=[&](auto receiver,auto valid,auto a,auto b,auto settings,auto destination,auto&current){
            ++out.convex_queries;
            native_tu15_query_convex_contact(memory,receiver,valid,a,b,settings,destination,
                scratch.convex,current,0,geometry,observe);
        };
        const auto result=native_tu15_query_contact(memory,child,valid_result,first,second,
            context,output,scratch.query,fp,0,services,observe,&out.query);
        ++out.completed;return result;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
