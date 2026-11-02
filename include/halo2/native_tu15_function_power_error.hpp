#pragma once
#include "halo2/source_math_error_tu15.hpp"
namespace halo2 {
struct NativeTu15FunctionPowerErrorStats {
  std::uint64_t pole_errors{},precision_errors{},delegated_errors{},refused_profiles{};
};
// Adds source CRT kind2/pole and kind8/precision to the existing native policy.
// This numerical backend runs under its caller's current native x87 scope,
// restores caller_control at the source phase, and uses typed errno storage.
double native_tu15_authored_function_power_error_current(unsigned kind,
  double value,std::uint16_t caller_control,const SourceMathErrorServicesTu15& = {},
  NativeTu15FunctionPowerErrorStats* = nullptr);
} // namespace halo2
