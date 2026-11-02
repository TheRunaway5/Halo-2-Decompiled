#pragma once
#include "halo2/native_tu15_camera_geometric_query.hpp"
#include "halo2/native_tu15_camera_dynamic_query.hpp"
#include "halo2/native_tu15_instanced_scene_ray_binding.hpp"
#include <map>
#include <stdexcept>
namespace halo2 {
struct NativeTu15CameraQueryBindingScratch {
    std::uint32_t identity{};
    static constexpr std::uint32_t bytes=0x10000u;
};
struct NativeTu15CameraQueryBindingActivity {
    std::uint64_t attempts{},completed{},refusals{},resource_refusals{};
    NativeTu15CameraGeometricQueryStats query;
    NativeTu15BoundInstancedRayActivity instanced;
    NativeTu15CameraDynamicStats dynamic;
    std::uint64_t unknown_owners()const{return instanced.unknown_owners+instanced.traversal.unbound_services+query.unbound_services+dynamic.unbound_data_owners;}
};
// The caller owns this aligned64KiB DATA region for the whole query. Each
// active object recursion has distinct DATA; no source CPU stack is modeled.
// The private MOPP unprovided word remains guarded through EVERY provider,
// forwarder, collector and primitive query. Unknown methods never call a CPU.
inline bool native_tu15_query_bound_camera_scene(SourcePhysicsContactMemory&memory,
    const NativeTu15CameraGeometricQuery&q,NativeTu15CameraQueryBindingScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,
    const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15CameraQueryBindingActivity*activity=nullptr){
    NativeTu15CameraQueryBindingActivity local;auto&out=activity?*activity:local;++out.attempts;
    try{
        const auto data=scratch.identity;
        if((data&15u)||data>0xffff0000u||depth){++out.resource_refusals;
            throw std::runtime_error("Native camera query requires aligned owned DATA and balanced Host FP");}
        NativeTu15CameraGeometricQueryServices parents;
        parents.instanced=[&](const NativeTu15CameraInstancedQuery&r,auto&current){
            return native_tu15_query_bound_instanced_scene_ray(memory,
                {r.cluster,r.mask,r.bsp_filter,r.origin,r.displacement,r.result},
                {data+0x1000u,0x800u},current,depth,observe,&out.instanced);
        };
        parents.dynamic=[&](const NativeTu15CameraDynamicQuery&r,auto&current){
            NativeTu15CameraDynamicServices services;
            services.acquire_frame=[&](unsigned recursion){
                if(recursion>=12u){++out.resource_refusals;
                    throw std::runtime_error("Native camera query requires a larger recursion DATA owner");}
                const auto p=data+0x4000u+recursion*0x1000u;
                return NativeTu15CameraDynamicFrame{p+0x200u,p+0x220u,p+0x1fcu,
                    {p,p+64u,p+52u,p+76u,p+116u}};
            };
            return native_tu15_query_dynamic_camera_object(memory,r,current,depth,services,
                observe,&out.dynamic);
        };
        const auto result=native_tu15_query_camera_geometry(memory,q,
            {data,data+0x500u,data+0x540u,data+0x580u},fp,depth,parents,observe,&out.query);
        ++out.completed;return result;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
