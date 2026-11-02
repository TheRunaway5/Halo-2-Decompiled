#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintEffectLocationStats {
    std::uint64_t point_leaf_calls{},bsp_nodes{},location_calls{},no_active_bsp{},outside_bsp{},
        key_calls{},missing_cluster{},missing_key{},halfspace_tests{},keys_selected{},unsupported_profiles{};
};
// Actual14CDB0, starting at the caller's node. Position is ordinary read-only
// vector DATA; all packed node/plane operands retain their actual widths.
std::uint32_t native_tu15_constraint_effect_point_leaf(const SourcePhysicsContactMemory&,
    std::uint32_t tree,std::uint32_t node,std::uint32_t position,NativeTu15PhysicsFrameFp&,
    NativeTu15ConstraintEffectLocationStats* = nullptr);
// Actual11E600. The8-byte location is caller-owned output DATA.
void native_tu15_constraint_effect_location(SourcePhysicsContactMemory&,std::uint32_t position,
    std::uint32_t output,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintEffectLocationStats* = nullptr);
// Actual11E850. Optional output is an ordinary2-byte key, never a retained
// CPU register. It remains unwritten on failure and when the pointer is null.
bool native_tu15_constraint_effect_location_key(SourcePhysicsContactMemory&,std::uint32_t location,
    std::uint32_t position,std::uint32_t optional_key,NativeTu15PhysicsFrameFp&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintEffectLocationStats* = nullptr);
} // namespace halo2
