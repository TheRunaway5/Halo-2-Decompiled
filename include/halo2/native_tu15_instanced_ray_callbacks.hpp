#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15InstancedRayCallbackServices {
    std::function<bool(std::uint32_t instance,std::uint32_t mask,std::uint32_t bsp_filter,
        std::uint32_t origin,std::uint32_t displacement,std::uint32_t result,
        NativeTu15PhysicsFrameFp&)>query_instance;
    std::function<void(std::uint32_t owner,std::uint32_t collector,std::uint32_t body,
        std::uint32_t hit_packet,NativeTu15PhysicsFrameFp&)>collect;
};
struct NativeTu15InstancedRayCallbackStats {
    std::uint64_t provider_calls{},provider_null{},forward_calls{},collector_calls{},
        duplicate_instances{},instance_queries{},accepted_instances{},refusals{},
        unbound_query{},unbound_collect{};
};
// Source24DEC0. The supplied20B child DATA remains owned through its cast.
std::uint32_t native_tu15_resolve_instanced_ray_child(SourcePhysicsContactMemory&,
    std::uint32_t collection,std::uint32_t primitive_key,std::uint32_t child_data,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15InstancedRayCallbackStats* = nullptr);
// Source24DEF0. Only hit_packet+14 is defined by the caller. A collector may
// consume only proven fields; known24DF20 never reads this packet at all.
void native_tu15_forward_instanced_child_ray(SourcePhysicsContactMemory&,
    std::uint32_t body,std::uint32_t collector,std::uint32_t hit_packet,
    NativeTu15PhysicsFrameFp&,const NativeTu15InstancedRayCallbackServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15InstancedRayCallbackStats* = nullptr);
// Source24DF20, ordinary raw instance DATA and current mask/filter/ray/result.
void native_tu15_collect_instanced_scene_ray(SourcePhysicsContactMemory&,
    std::uint32_t collector,std::uint32_t body,NativeTu15PhysicsFrameFp&,unsigned depth,
    const NativeTu15InstancedRayCallbackServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15InstancedRayCallbackStats* = nullptr);
} // namespace halo2
