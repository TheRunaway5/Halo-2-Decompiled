#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
namespace halo2 {
// WholeBBE00. Local object coordinates resolve through the actual parent's
// signed node/transform offset. Outputs publish individually, retaining aliases.
void native_campaign_object_world_point_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual BBE00 caller: retain its conditional saved EDI word when resolving
// an attached object's local coordinates. The numeric owner is shared above.
void native_campaign_object_world_point_frame_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t destination,std::uint32_t stack,
    std::uint32_t saved_edi,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual private three-word caller output; no retained scratch allocation.
std::array<std::uint32_t,3> native_campaign_object_world_point_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t object,NativeTu15PhysicsFrameFp &);
}
