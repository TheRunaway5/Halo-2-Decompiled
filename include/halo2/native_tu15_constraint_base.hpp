#pragma once
#include "halo2/native_tu15_constraint_producer.hpp"
namespace halo2 {
struct NativeTu15ConstraintBaseStats {
    std::uint64_t calls{},rows{},coupled_rows{},aggregate_pairs{},aggregate_axes{},basis_repairs{};
};
// Actual329720, cdecl6. All six retained identities/values are explicit.
// The call-free native algorithm constructs the real rows and metadata; no
// recorded Jacobian, callback output or cursor is an input. Private x87 depth
// reaches seven, so at most one genuine incoming value may remain preserved.
void native_tu15_build_constraint_base(SourcePhysicsContactMemory&,
    const NativeTu15ConstraintBaseBuildRequest&,NativeTu15PhysicsFrameFp&,
    unsigned actual_preserved_x87_depth,const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintBaseStats* = nullptr);
} // namespace halo2
