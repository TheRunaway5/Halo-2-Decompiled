#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15ConstraintMotionCompareStats {std::uint64_t calls{},missing_second{},comparisons{};};
// Actual22EDE0. Compare genuine object linear-velocity squared magnitudes in
// the original source association; a None second datum returns false.
bool native_tu15_second_constraint_body_moves_faster(const SourcePhysicsContactMemory&,
    std::uint32_t first_component_datum,std::uint32_t second_component_datum,
    NativeTu15PhysicsFrameFp&,NativeTu15ConstraintMotionCompareStats* = nullptr);
} // namespace halo2
