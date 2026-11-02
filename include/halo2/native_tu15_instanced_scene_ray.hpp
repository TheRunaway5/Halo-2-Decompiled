#pragma once
#include "halo2/native_tu15_mopp_private_ray_setup.hpp"
#include "halo2/native_tu15_instanced_ray_lifetime.hpp"
namespace halo2 {
struct NativeTu15InstancedSceneRayRequest {
    std::uint32_t cluster{},mask{},bsp_filter{},origin{},displacement{},result{};
};
struct NativeTu15InstancedSceneRayScratch {
    // Distinct, caller-owned,16-aligned DATA. No native object escapes return.
    std::uint32_t identity{};
    static constexpr std::uint32_t bytes=0x1a0u;
};
struct NativeTu15InstancedSceneRayServices {
    NativeTu15MoppPrivateRayServices traversal;
    NativeTu15InstancedRayLifetimeServices lifetime;
};
struct NativeTu15InstancedSceneRayStats {
    std::uint64_t calls{},completed{},refusals{},empty_clusters{},positive_clusters{},
        shape_initializations{},shape_casts{},shape_destructions{},hits{},
        unbound_allocation_modes{};
    NativeTu15MoppPrivateRayActivity setup;
};
// Source24DC90. The one unwritten private ray word is intentionally not a
// native value. Every traversal/shape consumer must use the supplied guarded
// memory; any read of that word refuses. No raw private-stack equivalence or
// arbitrary observer equivalence is implied by this retained-DATA interface.
bool native_tu15_query_instanced_scene_ray(SourcePhysicsContactMemory&,
    const NativeTu15InstancedSceneRayRequest&,NativeTu15InstancedSceneRayScratch,
    NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
    const NativeTu15InstancedSceneRayServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15InstancedSceneRayStats* = nullptr);
} // namespace halo2
