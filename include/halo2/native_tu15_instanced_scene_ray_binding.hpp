#pragma once
#include "halo2/native_tu15_instanced_scene_ray.hpp"
#include "halo2/native_tu15_instanced_ray_host_callbacks.hpp"
#include "halo2/native_tu15_mopp_ray_traversal.hpp"
#include "halo2/native_tu15_camera_instanced_query.hpp"
#include <map>
namespace halo2 {
struct NativeTu15BoundInstancedRayScratch {
    std::uint32_t identity{},bytes{};
    static constexpr std::uint32_t required_bytes=0x800u;
};
struct NativeTu15BoundInstancedRayActivity {
    std::uint64_t attempts{},completed{},refusals{},unsupported_scratch{},
        provider_calls{},forward_calls{},collector_calls{},instance_calls{},unknown_owners{};
    std::map<std::uint32_t,std::uint64_t>refused_owners;
    NativeTu15InstancedSceneRayStats scene;
    NativeTu15MoppRayTraversalStats traversal;
    NativeTu15InstancedRayCallbackStats callbacks;
    NativeTu15CameraInstanceQueryStats instance;
};
// Retained-DATA production composition. Only known native owners are routed;
// no source instruction executor exists here. Source-private body/key and
// unconsumed hit packet are typed native DATA, never a reconstructed frame.
// Every selected provider receives the SAME unprovided-field guarded memory.
// Arbitrary raw private-store observers are outside this interface's domain.
inline bool native_tu15_query_bound_instanced_scene_ray(SourcePhysicsContactMemory&memory,
    const NativeTu15InstancedSceneRayRequest&request,NativeTu15BoundInstancedRayScratch scratch,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourcePhysicsContactRecordObserver&observe={},
    NativeTu15BoundInstancedRayActivity*activity=nullptr){
    NativeTu15BoundInstancedRayActivity local;auto&out=activity?*activity:local;++out.attempts;
    try{
        if((scratch.identity&15u)||scratch.bytes<scratch.required_bytes||
            std::uint64_t(scratch.identity)+scratch.bytes>0x100000000ull){
            ++out.unsupported_scratch;throw std::runtime_error("Native instanced scene query requires distinct aligned owned DATA");}
        const auto reject=[&](std::uint32_t owner){++out.unknown_owners;++out.refused_owners[owner];
            throw std::runtime_error("Native instanced scene query requires native owner="+std::to_string(owner));};
        NativeTu15InstancedSceneRayServices services;
        services.lifetime.release_code=[&](auto,auto owner,auto,auto&){reject(owner);};
        services.lifetime.destroy_collection=[&](auto,auto owner,auto,auto&){reject(owner);};
        services.lifetime.allocation_mode=[&](auto,auto,auto&){reject(0x2e12a0u);};
        services.traversal.traverse=[&](SourcePhysicsContactMemory&guarded,auto cache,auto code,
            const auto&variables,const auto&bounds,auto&current,auto older){
            NativeTu15MoppRayTraversalServices traversal;
            traversal.filter=[&](auto,auto owner,auto,auto,auto,auto&)->bool{reject(owner);return false;};
            traversal.cast_direct=[&](auto,auto owner,auto,auto,auto&)->bool{reject(owner);return false;};
            traversal.child_shape=[&](auto collection,auto owner,auto key,auto&)->NativeTu15MoppChildShape{
                if(owner!=0x24dec0u)reject(owner);++out.provider_calls;
                return{native_tu15_resolve_instanced_ray_child(guarded,collection,key,
                    scratch.identity+0x1a0u,observe,&out.callbacks)};
            };
            traversal.cast_collector=[&](auto,auto owner,auto,NativeTu15MoppRayBody body,auto collector,auto&state){
                if(owner!=0x24def0u)reject(owner);++out.forward_calls;
                NativeTu15InstancedRayHostForwardServices forward;
                forward.collect_key=[&](auto target,auto receiver,auto key,auto&child_fp){
                    if(target!=0x24df20u)reject(target);++out.collector_calls;
                    NativeTu15InstancedRayCallbackServices callback;
                    callback.query_instance=[&](auto instance,auto mask,auto filter,auto origin,
                        auto delta,auto result,auto&query_fp)->bool{
                        ++out.instance_calls;
                        return native_tu15_query_camera_instance(guarded,
                            {instance,mask,filter,origin,delta,result},
                            {scratch.identity+0x200u,scratch.identity+0x234u,scratch.identity+0x240u,
                             scratch.identity+0x24cu,scratch.identity+0x680u},query_fp,older,observe,&out.instance);
                    };
                    native_tu15_collect_instanced_scene_ray_key(guarded,receiver,key,child_fp,
                        older,callback,observe,&out.callbacks);
                };
                native_tu15_forward_instanced_child_ray_key(guarded,body.primitive_key,collector,
                    state,forward,observe,&out.callbacks);
            };
            auto current_bounds=bounds;
            native_tu15_traverse_mopp_ray(guarded,cache,code,variables,current_bounds,current,
                older,traversal,observe,&out.traversal);
        };
        const auto result=native_tu15_query_instanced_scene_ray(memory,request,{scratch.identity},
            fp,depth,services,observe,&out.scene);++out.completed;return result;
    }catch(...){++out.refusals;throw;}
}
} // namespace halo2
