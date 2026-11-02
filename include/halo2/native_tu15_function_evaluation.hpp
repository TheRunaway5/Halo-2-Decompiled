#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
namespace halo2 {
struct NativeTu15FunctionEvaluationStats {
  std::uint64_t calls{},null_functions{},empty_functions{},invalid_kinds{},kind_zero{},kind_one{},
    lower_clamps{},upper_clamps{},unsupported_kinds{},unsupported_fp_profiles{};
};
// Source13DE70's reference is exactly8B{signed byte-count,blob identity}.
// Sourcekind0/common clamp andkind1 are admitted natively, including genuine
// null/empty/invalid-kind returns. Other authored kinds remain explicit native
// work and throw; this API never invokes an original CPU or supplies outputs.
// This admitted domain is SSE-only and does not consume a live x87 value.
std::uint32_t native_tu15_evaluate_authored_function(SourcePhysicsContactMemory&,
  std::uint32_t function_reference,std::uint32_t first_parameter_bits,
  std::uint32_t second_parameter_bits,NativeTu15PhysicsFrameFp&,
  unsigned actual_preserved_x87_depth,NativeTu15FunctionEvaluationStats* = nullptr);
} // namespace halo2
