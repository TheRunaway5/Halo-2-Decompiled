#pragma once
#include <cstdint>

namespace halo2 {
// Literal executable operands. A supported image importer must supply these;
// this packet neither initializes a manager nor establishes resource residency.
struct UnitAnimationConstants {
  std::uint32_t one_bits, zero_bits, frame_subtractor_bits;
  std::uint32_t rng_scale_bits, unsigned_conversion_correction_bits;
};
} // namespace halo2
