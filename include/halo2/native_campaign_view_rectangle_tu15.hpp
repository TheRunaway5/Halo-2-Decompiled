#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <optional>

namespace halo2 {
struct NativeCampaignViewRectangleTu15 {std::array<std::uint32_t,4> bounds{};};
// Actual disjoint private-output profile of2B8B4; shares the complete
// algorithm, with native storage for the rectangle and no source stack.
NativeCampaignViewRectangleTu15 native_campaign_view_rectangle_value_tu15(
    SourcePhysicsContactMemory &,std::uint32_t camera,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Complete original2F950. Camera+30/+38 are signed short rectangles;
// destination is the actual four-float collection rectangle. Reads remain
// lazy across publications, including aliases and the global modifier flag.
// Masked SIMD exceptions are required; x87 controls/status are untouched.
bool native_campaign_view_rectangle_tu15(SourcePhysicsContactMemory &,
    std::uint32_t camera,std::uint32_t destination,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
}
