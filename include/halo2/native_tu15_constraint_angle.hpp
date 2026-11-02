#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintAngleStats {
    std::uint64_t calls{},completed{},complemented{},negative_second{},negative_first{},unsupported_depth{};
};
// Actual37C6B0 authored approximate angle. The caller owns one active native
// x87 environment; this operation leaves its real extended result on ST0 for
// the source caller's FST/unwrap/FSTP sequence. It does not install an FP image,
// return a reconstructed register bank, or substitute FPATAN/libm. At most
// three new stack entries are live; older caller values are not consumed.
void native_tu15_constraint_angle_active(SourcePhysicsContactMemory&,
    std::uint32_t first_float_bits,std::uint32_t second_float_bits,
    unsigned actual_preserved_x87_depth,NativeTu15ConstraintAngleStats* = nullptr);
} // namespace halo2
