#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintAuxiliaryMathStats {
    std::uint64_t calls{},normalized{},small_vectors{},pack_calls{},body_velocity_calls{},
        default_velocities{},unsupported_profiles{};
};
// Actual30BD0: the tiny-vector branch leaves its input unchanged. Five native
// x87 temporaries admit preserved depths0..3; return is ordinary scalar bits.
std::uint32_t native_tu15_normalize_constraint_axis(SourcePhysicsContactMemory&,
    std::uint32_t vector,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintAuxiliaryMathStats* = nullptr);
// Actual18BBC0, source clamp/quantization/32-bit packing with one native x87
// temporary. The input vector is read-only; no component result is supplied.
std::uint32_t native_tu15_pack_constraint_direction(const SourcePhysicsContactMemory&,
    std::uint32_t vector,NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    NativeTu15ConstraintAuxiliaryMathStats* = nullptr);
// Actual1D8C80, admitted private-output caller. Values are raw vector DATA;
// MOVSS source operands do not perform floating-point arithmetic.
std::array<std::uint32_t,3> native_tu15_constraint_body_linear_velocity(
    const SourcePhysicsContactMemory&,NativeTu15PhysicsComponent,std::uint32_t body_slot,
    NativeTu15ConstraintAuxiliaryMathStats* = nullptr);
} // namespace halo2
