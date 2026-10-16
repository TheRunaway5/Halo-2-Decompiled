#pragma once
#include "halo2/native_runtime_address.hpp"
#include <cstdint>

namespace halo2 {
class PhysicsLiveWorldTu15;

// Actual source identities and borrowed native spans for one activated BSP.
// Native addresses do not extend their backing lifetime. A world restore or
// backing replacement requires activation/rebinding at its renderer boundary.
struct SourceLiveShadowStorageTu15 {
    std::uint32_t bsp{},plane_rows{},plane_count{};
    std::uint32_t plane_storage{},index_storage{},hash{},allocator{};
    NativeRuntimeAddress bsp_native{},plane_rows_native{};
    NativeRuntimeAddress plane_storage_native{},index_storage_native{};
    NativeRuntimeAddress hash_native{},buckets_native{},nodes_native{};
    bool operator==(const SourceLiveShadowStorageTu15&) const = default;
};

// Explicit renderer BSP activation only. Cold storage runs whole1E0F0;
// existing storage runs whole22690 before whole1E1A0 inserts the actual BSP
// planes. This is not a per-view or per-frame initializer.
SourceLiveShadowStorageTu15 source_live_shadow_storage_activate_tu15(
    PhysicsLiveWorldTu15&);

// Read-only frame admission against the retained activation identities and
// native backing. This never clears, allocates or repopulates source storage.
void source_live_shadow_storage_validate_tu15(
    PhysicsLiveWorldTu15&,const SourceLiveShadowStorageTu15&);

// Rebind borrowed native addresses after an independently admitted same-world
// restore. Source BSP/allocation identities, counts, allocator and extents must
// still match. This neither authenticates an imported bank nor activates it.
SourceLiveShadowStorageTu15 source_live_shadow_storage_rebind_tu15(
    PhysicsLiveWorldTu15&,const SourceLiveShadowStorageTu15&);
}
