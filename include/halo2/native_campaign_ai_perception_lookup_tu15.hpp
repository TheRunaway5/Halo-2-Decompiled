#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Whole267520. Find the actual first matching property/attached-owner alias;
// optionally create it through real native group classification/allocation and
// fanout. A self object, inactive actor or true refusal remains FFFFFFFF.
std::uint32_t native_campaign_ai_perception_lookup_tu15(
    SourcePhysicsContactMemory &,std::uint32_t object,std::uint32_t actor,
    bool create,NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
}
