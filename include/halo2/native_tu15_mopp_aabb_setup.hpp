#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
// Semantic fields actually initialized by30A1E0 and consumed by309940.
// The two uninitialized alignment words at0C/1C are deliberately absent.
struct NativeTu15MoppAabbVariables {
    std::array<std::uint32_t,3> upper{},lower{};
    std::array<std::uint32_t,6> translation_and_properties{};
};
struct NativeTu15MoppAabbSetupServices {
    std::function<void(std::uint32_t cache,std::uint32_t program,
        const NativeTu15MoppAabbVariables&,NativeTu15PhysicsFrameFp&,
        unsigned incoming_x87_depth)> traverse; // genuine309940
};
// Actual30A1E0. Cache and bounds are caller-owned DATA; values are derived
// using native hardware x87 rounding, including the intermediate float store.
void native_tu15_prepare_mopp_aabb(SourcePhysicsContactMemory&,
    std::uint32_t cache,std::uint32_t code,std::uint32_t bounds,
    std::uint32_t key_array,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15MoppAabbSetupServices&,
    const SourcePhysicsContactRecordObserver& = {});
// Actual2E9670; private cache backing must already exist for the full call.
void native_tu15_query_mopp_aabb_shape(SourcePhysicsContactMemory&,
    std::uint32_t shape,std::uint32_t bounds,std::uint32_t key_array,
    std::uint32_t cache,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeTu15MoppAabbSetupServices&,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
