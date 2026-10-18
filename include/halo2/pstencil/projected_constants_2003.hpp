#pragma once
#include "halo2/render_program.hpp"

namespace halo2::pstencil {
// May2003 A3CF0's projective/pyramid C15..20 producer. The current asset
// adapter supplies its completed shape, frame and explicit falloff selector;
// this function preserves the original unwritten C16.w and C20.xy lanes.
std::array<ShaderVector, 6>
projected_constants_2003(const std::array<std::uint32_t, 31> &shape,
                         const std::array<float, 16> &frame, unsigned falloff,
                         float constant20_w,
                         const std::array<ShaderVector, 6> &previous);
} // namespace halo2::pstencil
