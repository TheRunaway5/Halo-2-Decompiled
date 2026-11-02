#pragma once
#include "halo2/native_tu15_constraint_auxiliary_math.hpp"
namespace halo2 {
struct NativeTu15ConstraintPointBasisStats {
    std::uint64_t calls{},up_axes_selected{},unsupported_profiles{};
    NativeTu15ConstraintAuxiliaryMathStats normalize;
};
// Actual22E210 with native30BD0. Destination is36 bytes of caller-owned
// ordinary basis DATA, disjoint from the retained object/instance inputs.
void native_tu15_constraint_point_basis(SourcePhysicsContactMemory&,
    std::uint32_t component_datum,std::uint32_t instance,std::uint32_t destination,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintPointBasisStats* = nullptr);
} // namespace halo2
