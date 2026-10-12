#pragma once
#include "halo2/render_program.hpp"
namespace halo2 {
struct RenderViewport {
  float left{}, top{}, width{}, height{};
  float render_scale = 1;
  bool scaled{}, scaling_disabled{};
  unsigned mode{};
};
// Retail 00012FA0 physical constants c8..c11. Viewport positions are pixels
// in the active render target; texture-specific texel sizes are separate.
std::array<ShaderVector,4> viewport_constants(RenderViewport);
// Expand an ordinary full-surface 640x480 layout into the original camera's
// signed-short edges, then derive its pixel size from those edges. This is a
// native resolution extension of the integer bank read by 12FA0, not the
// original render-scale or fixed mode2 target service.
RenderViewport full_surface_viewport(RenderViewport canonical_layout,
                                    std::uint32_t drawable_width,
                                    std::uint32_t drawable_height);
} // namespace halo2
