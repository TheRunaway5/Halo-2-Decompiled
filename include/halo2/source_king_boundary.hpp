#pragma once
#include "halo2/source_king.hpp"
#include <optional>

namespace halo2 {
struct SourceKingBoundaryVertex {
  std::array<float,4> position{}; // Actual immediate attribute0, W=1.
  std::array<float,2> uv{}; // Actual semantic3 register1 for the authored hill.
  std::array<float,4> rgba{}; // Actual immediate attribute11.
};
struct SourceKingBoundary {
  std::array<SourceKingBoundaryVertex,66> strip;
  float inner{},outer{},uv_scale{},initial_u{};
  std::int32_t repeats{};
};
struct SourceKingBoundaryFloatingPoint {
  std::uint16_t x87_control_word=0x023f;
  std::uint32_t mxcsr=0x1fa0;
};
// Original19F680 distance prefix and19F8AD..19FC12 CPU emission arithmetic.
// The 22 original corridor cases establish the valid authored Lockout32-point
// contract and supplied FP controls. Graphics preparation/SDK restore and
// original GPU completion remain external services. Invalid/far hills yield
// no strip; absent arithmetic services or malformed native inputs throw.
std::optional<SourceKingBoundary> source_king_boundary(const SourceKingState&,
    const std::array<float,3>& camera,const std::array<float,3>& rgb,
    std::int32_t tick,std::int16_t tick_rate,SourceKingBoundaryFloatingPoint = {});
} // namespace halo2
