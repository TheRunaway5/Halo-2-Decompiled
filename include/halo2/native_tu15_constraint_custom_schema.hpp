#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
namespace halo2 {
struct NativeTu15ConstraintCustomSchemaStats {
    std::uint64_t calls{};
    std::array<std::uint64_t,3> owners{};
};
// Only the three authenticated retail TU1.5 built-in callback owners qualify.
// A different registered schema owner requires its own native implementation.
bool native_tu15_owns_constraint_custom_schema(NativeTu15PhysicsVirtualOwner);
// Actual cdecl callback(row,payload). These three built-ins use only payload;
// row is retained in the typed API because it is part of the true call ABI.
void native_tu15_apply_constraint_custom_schema(SourcePhysicsContactMemory&,
    NativeTu15PhysicsVirtualOwner,std::uint32_t row,std::uint32_t payload,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintCustomSchemaStats* = nullptr);
} // namespace halo2
