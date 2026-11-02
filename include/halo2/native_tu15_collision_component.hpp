#pragma once
#include "halo2/native_tu15_collision_surface_shape.hpp"
namespace halo2 {
// Source186E70: authored collision-channel visibility. FFFFFFFF is the source
// global-instance sentinel; the data owner preserves unaligned bitmap reads.
bool native_tu15_collision_channel_enabled(SourcePhysicsContactMemory&,
    std::uint32_t channel,std::uint32_t instance);
// Source1F76A0: decode the authored component key and test its actual source
// visibility/object-generation fields. Unsupported encoded kinds are refused.
bool native_tu15_collision_component_enabled(SourcePhysicsContactMemory&,
    std::uint32_t encoded_key);
struct NativeTu15CollisionResolveArguments {std::uint32_t identity{};}; //key,output data fields
struct NativeTu15CollisionResolveScratch {
    NativeTu15CollisionSurfaceArguments unplaced_arguments,placed_arguments;
    NativeTu15CollisionSurfaceScratch unplaced_vertex,placed_vertex;
    std::uint32_t placed_resource{};
};
// Native1F7C10 surface-key paths1/5, including native1F76A0/186E70/1F5FE0.
// Remaining shared/object kinds2/3/4 require their own native owners and are
// explicit refusals, never emulated fallback. The caller owns the DATA packets.
NativeTu15ContactComponent native_tu15_resolve_surface_component(
    SourcePhysicsContactMemory&,NativeTu15CollisionResolveArguments,
    NativeTu15CollisionResolveScratch,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15CollisionSurfaceStats* = nullptr);
} // namespace halo2
