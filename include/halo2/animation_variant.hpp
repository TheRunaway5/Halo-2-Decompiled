#pragma once
#include "halo2/unit_animation_selection.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
struct AnimationVariantConstants {
  std::uint32_t one_bits, rng_scale_bits, unsigned_conversion_correction_bits;
};
struct AnimationVariantRngStore {
  std::uint32_t instruction;
  unsigned stream;
  std::uint32_t before, after;
  friend bool operator==(const AnimationVariantRngStore &,
                         const AnimationVariantRngStore &) = default;
};
struct AnimationVariantServices {
  SourceAnimationGraphMemory memory;
  AnimationVariantConstants constants;
  // Complete original1E13C0. Called with its actual master graph address and
  // packed request, including nested definition lookups and final selection.
  // Missing ownership throws; this module provides no resource success.
  std::function<void(std::uint32_t graph_address, std::uint32_t packed)>
      resource;
  std::function<void(const AnimationVariantRngStore &)> rng_store;
};
// Original1E1020 plus1E0FC0/1DE890/1DE9B0. The two raw streams are caller-owned;
// this module never initializes, seeds or chooses a replacement RNG stream.
// Every required resource call precedes the corresponding output return.
std::uint32_t choose_source_animation_variant(
    std::uint32_t graph_address, std::uint32_t packed, std::uint8_t stream0,
    std::array<std::uint32_t, 2> &rng, const AnimationVariantServices &);
// Complete1CEDA0: manager+68 graph and+6C bit1 own graph/stream dispatch.
std::uint32_t select_source_animation_variant(
    std::span<const std::byte> manager, std::uint32_t packed,
    std::array<std::uint32_t, 2> &rng, const AnimationVariantServices &);
} // namespace halo2
