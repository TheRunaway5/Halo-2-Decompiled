#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
namespace halo2 {
//26F940: publish the actual query target timestamp (or NONE), then recompute
// its score through native26FA40. Reached required-owner failures propagate.
void native_campaign_ai_vision_tracking_timestamp_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t perception,bool tracked,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
//299860: actual owner-list attachment selection with strict first-minimum
// distance and authored flag/preferred-object weighting.
std::uint32_t native_campaign_ai_vision_attachment_select_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t owner,std::uint32_t reference_body,
    std::uint32_t preferred,NativeTu15PhysicsFrameFp &);
//CDCC0: retain source read/store ordering for raw destinations; the actual
// vision caller's private output can instead remain typed native coordinates.
void native_campaign_ai_vision_unit_vector_copy_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,std::uint32_t destination);
std::array<std::uint32_t,3> native_campaign_ai_vision_unit_vector_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t object);
//111FD0: original signed unit-extension identity/validity query.
std::uint32_t native_campaign_ai_vision_unit_extension_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t object);
//26DF70: original facing/distance bucket. The vision callers' second vector
// and distance are private typed values, with no source stack reconstruction.
std::uint8_t native_campaign_ai_vision_facing_class_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t direction,
    const std::array<std::uint32_t,3>& facing,std::uint32_t distance_bits,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
std::uint8_t native_campaign_ai_vision_facing_class_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t direction,std::uint32_t facing,
    std::uint32_t distance_bits,NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
}
