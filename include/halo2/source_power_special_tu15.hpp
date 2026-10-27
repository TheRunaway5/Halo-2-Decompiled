#pragma once
#include <cstdint>
namespace halo2 {
struct SourcePowerSpecialTu15Result {
  std::uint64_t result_bits{};
  bool report_domain_error{};
};
// TU1.5 37D948 in the positive-exponent lift domain. The caller's original
// FNSAVE/FRSTOR discards this classifier's internal arithmetic status. Input and
// result are binary64 words, with all classification expressed semantically.
SourcePowerSpecialTu15Result source_power_special_positive_tu15(
    std::uint64_t base_bits,std::uint64_t exponent_bits);
}
