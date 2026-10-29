#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/native_campaign_object_markers_tu15.hpp"
namespace halo2 {
// WholeBAD60: resolve real authored object markers or publish the original
// fallback pose. The return is AX16; missing named markers remain zero even
// though the original still writes a fallback record.
std::uint16_t native_campaign_object_marker_request_tu15(SourcePhysicsContactMemory &,
    std::uint32_t object,bool direct,std::uint32_t marker,std::int16_t capacity,
    std::uint32_t destination,NativeTu15PhysicsFrameFp &);
struct NativeCampaignObjectMarkerRequestValueTu15 {
    std::uint16_t count{};
    NativeCampaignObjectMarkerValueTu15 value;
};
// The actual1EBCB0 caller requests exactly one record in private storage.
// This overload owns that native value without a synthetic source address.
NativeCampaignObjectMarkerRequestValueTu15 native_campaign_object_marker_request_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t object,bool direct,
    std::uint32_t marker,NativeTu15PhysicsFrameFp &);
}
