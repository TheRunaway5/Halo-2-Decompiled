#pragma once
#include "halo2/native_tu15_function_curves.hpp"
#include "halo2/native_tu15_function_control_point.hpp"
namespace halo2 {
struct NativeTu15FunctionEvaluationExtendedStats {
  std::uint64_t calls{},null_functions{},empty_functions{},invalid_kinds{},
    lower_clamps{},upper_clamps{},unsupported_kinds{},unsupported_fp_profiles{},
    power_calls{},control_point_queries{},unsupported_power_profiles{};
  std::array<std::uint64_t,11> kinds{};
  NativeTu15FunctionCurveStats curves{};
};
// Source13DE70's authored 8-byte reference. This separate extension preserves
// the frozen kind0/1 owner while its additional authored math is qualified.
// Kinds9/10 use native x87 math in the normal finite power domain. Exceptional
// CRT publication, overflow/underflow, or unsupported FP profiles refuse.
std::uint32_t native_tu15_evaluate_authored_function_extended(
  SourcePhysicsContactMemory&,std::uint32_t function_reference,
  std::uint32_t first_parameter_bits,std::uint32_t second_parameter_bits,
  NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
  NativeTu15FunctionEvaluationExtendedStats* = nullptr);
} // namespace halo2
