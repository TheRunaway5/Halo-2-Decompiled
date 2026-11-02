#pragma once
#include "halo2/native_tu15_moving_shape_process.hpp"
#include "halo2/native_tu15_convex_contact_process.hpp"
#include "halo2/native_tu15_convex_simplex_solver_binding.hpp"
#include "halo2/native_tu15_contact_cache.hpp"
#include "halo2/native_tu15_contact_publication_binding.hpp"
#include "halo2/native_tu15_contact_point_binding.hpp"
#include "halo2/native_tu15_contact_creation_gameplay.hpp"
#include "halo2/native_tu15_pool_array_services.hpp"
#include "halo2/native_tu15_contact_agent_factory.hpp"
#include "halo2/native_tu15_activation_services.hpp"
#include <string>

namespace halo2 {
struct NativeTu15MovingShapeBindingScratch {std::uint32_t identity{},bytes{};};
struct NativeTu15MovingShapeNativeServices {
    // A selected wake must use the same DATA view and a separately owned
    // native activation arena. Absence is a named requirement, never fallback.
    std::function<void(SourcePhysicsContactMemory&,std::uint32_t world,
        std::uint32_t island,NativeTu15PhysicsFrameFp&)> activate_island;
    NativeTu15ContactCreationGameplayServices gameplay;
    std::function<std::uint64_t()> timestamp_counter;
};
struct NativeTu15MovingShapeBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},resource_refusals{},
        depth_refusals{},fp_refusals{},unprovided_reads{},classifications{},cache_initializations{},
        contact_publications{},distance_publications{},activation_requests{},
        unqualified_owners{},flag_domain_refusals{};
    std::map<std::uint32_t,std::uint64_t> refused_owners;
    NativeTu15MovingShapeProcessStats moving;
    NativeTu15ConvexProcessStats convex;
    NativeTu15ConvexSimplexBindingActivity solver;
    NativeTu15ContactPublicationBindingActivity publication;
    NativeTu15ContactPublicationStats contact;
    NativeTu15ContactDistanceStats distance;
    NativeTu15ContactPointBindingActivity removal;
    NativeTu15ContactCreationGameplayStats gameplay;
    NativeTu15PoolArrayServiceActivity pool;
    NativeTu15PairDeletionNativeScopeActivity native_only;
};
// Retained DATA/FP contract. Caller memory may observe retained public writes;
// no raw-private store/preimage/PC observer is accepted. The caller owns an
// aligned32KiB interval and its full synchronous lifetime. Any native wake
// provider must use disjoint DATA. No generic executor or CPU frame is used.
inline void native_tu15_process_bound_moving_shape_contact(
    SourcePhysicsContactMemory&storage,NativeTu15ContactAgent agent,
    NativeTu15ContactPayload first,NativeTu15ContactPayload second,
    NativeTu15ContactContext context,NativeTu15ContactOutput output,
    NativeTu15MovingShapeBindingScratch scratch,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const NativeTu15MovingShapeNativeServices&native,
    NativeTu15PairDeletionNativeScope&native_only,
    NativeTu15MovingShapeBindingActivity*activity=nullptr) {
    NativeTu15MovingShapeBindingActivity local;auto&out=activity?*activity:local;
    ++out.attempts;NativeTu15PairDeletionNativeScopeGuard scope(native_only,&out.native_only);
    try {
        const auto p=scratch.identity;
        if(!p||(p&15u)||scratch.bytes<0x8000u||std::uint64_t(p)+0x8000u>0x100000000ull) {
            ++out.resource_refusals;
            throw std::runtime_error("Native moving process requires owned aligned32KiB DATA");
        }
        if(depth){++out.depth_refusals;throw std::runtime_error("Native moving process requires qualified empty live x87");}
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u)||
            (fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u)) {
            ++out.fp_refusals;throw std::runtime_error("Native moving process requires its qualified masked FP domain");
        }
        NativeTu15ConvexSimplexMemory view(storage);
        struct AvailabilityActivity {
            const NativeTu15ConvexSimplexMemory&memory;NativeTu15MovingShapeBindingActivity&activity;
            ~AvailabilityActivity(){activity.unprovided_reads+=memory.refused_reads();}
        } availability{view,out};
        const auto refuse=[&](std::uint32_t owner,const char*phase)->void {
            ++out.unqualified_owners;++out.refused_owners[owner];
            throw std::runtime_error(std::string("Native moving process requires native ")+phase+" owner="+std::to_string(owner));
        };
        NativeTu15MovingShapeProcessServices services;
        services.classify_shape=[&](auto,auto owner,auto&)->std::uint32_t {
            ++out.classifications;
            if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();
            if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();
            if(const auto kind=native_tu15_activation_type(owner.identity))return*kind;
            refuse(owner.identity,"shape classification");return 0;
        };
        services.initialize_cache=[&](auto owner,auto cache,auto a,auto b,auto relative,auto&current) {
            if(owner!=0x309060u&&owner!=0x309150u)refuse(owner,"cache initialization");
            ++out.cache_initializations;
            const NativeTu15ContactShapeScratch child{p+0x7120u,p+0x7130u,nullptr},shape{p+0x7100u,p+0x7110u,&child};
            NativeTu15ContactCacheServices cache_services;
            cache_services.shape_center=[&](auto value,auto method,auto destination,auto&){native_tu15_contact_shape_center(view,value,method,destination);};
            cache_services.support_vertex=[&](auto value,auto method,auto direction,auto destination,auto&state){native_tu15_contact_shape_support(view,value,method,direction,destination,shape,state,0);};
            cache_services.vertex_encoding=[&](auto,auto method,auto&)->std::uint32_t {
                if(method.identity==0x1cb030u)return native_tu15_contact_shape_vertex_encoding();
                if(method.identity==0x31d520u||method.identity==0xad740u)
                    return *native_tu15_activation_type(method.identity);
                refuse(method.identity,"vertex encoding");return 0;
            };
            if(owner==0x309060u)native_tu15_initialize_convex_contact_cache(view,cache,a,b,relative,{p+0x7000u},current,cache_services);
            else native_tu15_initialize_triangle_contact_cache(view,cache,a,b,relative,{p+0x7000u},current,cache_services);
        };
        services.process_convex=[&](auto receiver,auto a,auto b,auto settings,auto destination,auto&current) {
            const NativeTu15ContactShapeScratch child_shape{p+0x3600u,p+0x3610u,nullptr},shape{p+0x3500u,p+0x3510u,&child_shape};
            NativeTu15ConvexProcessServices convex;
            convex.geometry=native_tu15_make_bound_convex_contact_services(view,
                {{p+0x3800u,p+0x3804u,p+0x3808u,p+0x3810u,p+0x3820u,
                    {p+0x3000u,p+0x3100u,p+0x3140u,{p+0x2800u,{p+0x2600u,p+0x2604u,
                        {p+0x2000u,p+0x2010u,p+0x2030u,p+0x2060u,p+0x2020u}}}}},shape},
                0,native.timestamp_counter,&out.solver);
            // Source2F8B60 initializes this flag. The selected qualification
            // does not admit the independently unqualified flag1 continuation.
            const auto solve=convex.geometry.solve_simplex;
            convex.geometry.solve_simplex=[&,solve](auto work,auto x,auto y,auto relative,auto cache,auto plane,auto&state) {
                std::array<std::byte,1>flag{};view.read(work+0xcu,flag);
                if(flag[0]!=std::byte{}){++out.flag_domain_refusals;refuse(0x30e930u,"flag1 simplex continuation");}
                return solve(work,x,y,relative,cache,plane,state);
            };
            NativeTu15ContactPublicationNativeServices publication_native;
            publication_native.creation.allocation=[&](auto&memory){return native_tu15_make_pool_array_services(memory,{},&out.pool);};
            publication_native.creation.activate_island=[&](auto&memory,auto world,auto island,auto&state) {
                ++out.activation_requests;
                if(!native.activate_island)refuse(0x316d40u,"island activation");
                native.activate_island(memory,world,island,state);
            };
            publication_native.creation.notify_listener=[&](auto&memory,auto listener,auto owner,auto event,auto&state) {
                native_tu15_dispatch_contact_creation_gameplay_listener(memory,listener,owner,event,
                    {p+0x6000u,p+0x6004u,p+0x6008u,p+0x6010u,p+0x6020u,p+0x6030u,p+0x6034u,p+0x6038u},
                    state,0,native.gameplay,&out.gameplay);
            };
            publication_native.remove_point=[&](auto&memory,auto manager,auto point,auto&state) {
                const auto allocation=native_tu15_make_pool_array_services(memory,{},&out.pool);
                native_tu15_remove_bound_contact_point(memory,manager,point,
                    {p+0x5200u,p+0x5220u,p+0x5240u,p+0x5220u},state,0,allocation,{},&out.removal);
            };
            const auto publication=native_tu15_make_contact_publication_services(view,
                {{p+0x5000u},{p+0x5100u,p+0x5120u,p+0x5140u}},0,publication_native,native_only,&out.publication);
            convex.publish_contact=[&](auto x,auto y,auto data,auto packet,auto manager,auto records,auto count,auto&state) {
                ++out.contact_publications;
                native_tu15_publish_contact(view,x,y,data,packet,manager,records,count,{p+0x4000u},state,0,publication,{},&out.contact);
            };
            convex.publish_distance=[&](auto x,auto y,auto data,auto cutoff,auto records,auto count,auto result,auto manager,auto&state) {
                ++out.distance_publications;
                native_tu15_publish_contact_distances(view,x,y,data,cutoff,records,count,result,manager,state,0,publication.removal,{},&out.distance);
            };
            convex.cleanup.remove_point=publication.removal.remove_point;
            native_tu15_process_convex_contact_agent(view,{receiver.identity},a,b,settings,{destination},
                {p+0x1000u,p+0x1300u},current,0,convex,{},&out.convex);
        };
        native_tu15_process_moving_shape_contact(view,{agent.identity},first,second,context,
            output.identity,{p},fp,0,services,{},&out.moving);
        ++out.completed;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
