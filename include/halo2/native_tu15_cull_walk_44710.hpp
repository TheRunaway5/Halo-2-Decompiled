#pragma once
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
// Actual 44710 service boundary. Frustum entries and sphere calls execute
// synchronously, in source order, against the same retained memory. The
// services own geometry/masks and may change later units or association words.
struct NativeTu15CullWalkServices {
    std::function<void(std::uint32_t unit,std::uint32_t entry,NativeTu15PhysicsFrameFp&)> frustum;
    std::function<void(std::uint32_t unit,const std::array<std::uint32_t,3>& center,
        std::uint32_t radius,NativeTu15PhysicsFrameFp&)> sphere;
};
// Whole 44710 parent and 44680's four-word iterator. Initial mode, context,
// prefix and end are pinned; association rows use the current global context.
// Each iterator word is pinned only when first reached. No deferred mask merge
// or repeated unit-eligibility test may be inserted between association calls.
void native_tu15_cull_walk_44710(const SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,const NativeTu15CullWalkServices&);
class PhysicsLiveWorldTu15;
// Returning false leaves the entry with its genuine 44E80 owner. A native
// entry returning true must publish all effects before the next service.
using NativeTu15CullFrustumEntry=std::function<bool(std::uint32_t,std::uint32_t,NativeTu15PhysicsFrameFp&)>;
// Returns the final arithmetic context of this standalone source call. A
// later prepare_source_call starts a fresh caller, as for the literal root.
NativeTu15PhysicsFrameFp source_native_cull_walk_tu15(PhysicsLiveWorldTu15&,
    const NativeTu15CullFrustumEntry& = {});
}
