#pragma once
#include "halo2/source_power_special_tu15.hpp"
namespace halo2 {
// Source37D948 in its caller-owned discarded-FPU-state domain: at least one
// binary64 argument is infinite and neither is NaN. Result and domain event
// are ordinary DATA. The existing positive-exponent owner is reused.
SourcePowerSpecialTu15Result native_tu15_authored_function_infinite_power(
  std::uint64_t base_bits,std::uint64_t exponent_bits);
} // namespace halo2
