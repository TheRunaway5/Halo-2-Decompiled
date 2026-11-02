#pragma once
#include "halo2/native_tu15_constraint_point_basis.hpp"
#include "halo2/native_tu15_body_constraint_update.hpp"
namespace halo2 {
struct NativeTu15ConstraintGeometryOutputs {
    std::uint32_t normal{},position{},first_axis{},second_axis{},first_extent{},second_extent{};
};
struct NativeTu15ConstraintGeometryScratch {
    // Two separate36-byte basis DATA buffers, disjoint from all inputs and
    // outputs. They are temporary values, never a guest CPU frame.
    std::uint32_t first_basis{},second_basis{};
};
struct NativeTu15ConstraintGeometryStats {
    std::uint64_t calls{},retained_node_frames{},simple_axes{},alternate_axes{},measured_bounds{},
        local_rows{},matching_rows{},second_basis_selected{},unsupported_profiles{};
    NativeTu15ModelConstraintUpdateStats frames;
    NativeTu15ConstraintPointBasisStats bases;
    NativeTu15ConstraintAuxiliaryMathStats normalize;
};
// Actual22E4F0. The datum is the constraint instance datum, not a second
// body slot. All vectors/scalars are ordinary caller-owned output DATA.
void native_tu15_constraint_geometry_frame(SourcePhysicsContactMemory&,
    std::uint32_t instance,std::uint32_t constraint_datum,std::uint32_t first_body_slot,
    NativeTu15ConstraintGeometryOutputs,bool include_pending_points,
    NativeTu15ConstraintGeometryScratch,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintGeometryStats* = nullptr);
} // namespace halo2
