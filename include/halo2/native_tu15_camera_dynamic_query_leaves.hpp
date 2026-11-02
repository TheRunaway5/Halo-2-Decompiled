#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Actual16B870. Header and object are current caller-resolved DATA identities.
bool native_tu15_admit_camera_query_object(const SourcePhysicsContactMemory&,
    std::uint32_t datum,std::uint32_t header,std::uint32_t object,
    std::uint32_t mask,std::uint32_t ignore_first,std::uint32_t ignore_second);
// Actual16B9A0. Lazy transform/plane reads preserve source alias/store order.
void native_tu15_publish_camera_collision(SourcePhysicsContactMemory&,
    std::uint32_t destination,std::uint32_t optional_transform,std::uint32_t collision,
    NativeTu15PhysicsFrameFp&,const SourcePhysicsContactRecordObserver& = {});
// ActualDB030. Host-value form shares the same DATA resolver with the numeric
// public output; neither exposes a CPU register or private control frame.
std::uint16_t native_tu15_camera_collision_material(const SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t material_index);
std::uint32_t native_tu15_camera_collision_material_to(SourcePhysicsContactMemory&,
    std::uint32_t object,std::uint32_t material_index,std::uint32_t output,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
