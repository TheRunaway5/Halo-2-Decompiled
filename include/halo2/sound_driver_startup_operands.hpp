#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace halo2 {
// Only immutable executable operands. The startup caller's separate mutable
// routing_plugin_identity requires retained live plugin ownership.
struct SoundDriverStartupImmutableOperands {
  std::uint32_t one_bits;
  std::array<std::byte,12> basis_forward, basis_up;
};
} // namespace halo2
