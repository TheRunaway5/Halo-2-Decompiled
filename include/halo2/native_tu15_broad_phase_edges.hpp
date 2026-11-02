#pragma once
#include "halo2/native_tu15_array_growth.hpp"

namespace halo2 {
struct NativeTu15BroadPhaseEdgeUpdate {
    std::uint32_t broad_phase{}, handles{}, bounds{}, count{};
    NativeTu15ArrayHeader created_pairs, removed_pairs;
};
struct NativeTu15BroadPhaseEdgeActivity {
    std::uint64_t objects{}, shifted_edges{}, created_pairs{}, removed_pairs{};
    std::uint64_t special_additions{}, special_removals{}, array_growths{};
};
// Semantic2EA380: four-lane AABB quantization, ordered sweep-edge updates and
// pair emission. All identities name ordinary DATA, including the mutable
// array headers. Sole2E3250 growth executes through its existing native API.
// No CPU register/frame image or result is accepted. This private candidate
// is UNQUALIFIED until its independent original comparison is complete.
void native_tu15_update_broad_phase_edges(SourcePhysicsContactMemory&,
    const NativeTu15BroadPhaseEdgeUpdate&, NativeTu15PhysicsFrameFp&,
    const NativeTu15ArrayGrowthServices&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15BroadPhaseEdgeActivity* = nullptr);
} // namespace halo2
