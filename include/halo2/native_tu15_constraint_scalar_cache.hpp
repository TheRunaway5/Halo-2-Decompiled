#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintScalarCacheStats {
    std::uint64_t calls{},already_retained{},cooldown_checks{},cooldown_retained{},published{};
};
// Actual22FE30. Retain the source scalar envelope using its genuine clock and
// cooldown fields, native SSE and native x87 integer rounding.
void native_tu15_update_constraint_cached_scalar(SourcePhysicsContactMemory&,
    std::uint32_t instance,std::uint32_t scalar_bits,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintScalarCacheStats* = nullptr);
} // namespace halo2
