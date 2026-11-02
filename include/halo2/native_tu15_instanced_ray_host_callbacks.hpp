#pragma once
#include "halo2/native_tu15_instanced_ray_callbacks.hpp"
namespace halo2 {
// Source30AA70 defines an ordinary typed ray-body key. Source24DF20 consumes
// only this key from that body. This overload shares the numeric algorithm,
// leaving the public numeric read/alias/observer contract unchanged.
void native_tu15_collect_instanced_scene_ray_key(SourcePhysicsContactMemory&,
    std::uint32_t collector,std::uint32_t raw_key,NativeTu15PhysicsFrameFp&,unsigned depth,
    const NativeTu15InstancedRayCallbackServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15InstancedRayCallbackStats* = nullptr);
struct NativeTu15InstancedRayHostForwardServices {
    std::function<void(std::uint32_t owner,std::uint32_t collector,std::uint32_t raw_key,
        NativeTu15PhysicsFrameFp&)>collect_key;
};
// Known24DEF0 caller-private hit+14 is defined but never consumed by24DF20.
// Keep that value as native DATA; no private CPU/control stack is serialized.
// The known collector must be proved not to consume the omitted hit packet.
void native_tu15_forward_instanced_child_ray_key(SourcePhysicsContactMemory&,
    std::uint32_t raw_key,std::uint32_t collector,NativeTu15PhysicsFrameFp&,
    const NativeTu15InstancedRayHostForwardServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15InstancedRayCallbackStats* = nullptr);
} // namespace halo2
