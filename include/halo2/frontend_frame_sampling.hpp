#pragma once
#include <cmath>
#include <optional>
#include <stdexcept>

namespace halo2 {
// Optional inward sampling limits at the visible logical canvas boundaries.
// The caller keeps the authored quad and UV interpolation unchanged. When a
// horizontal extension is present, its constant U and the center's local
// sampling limit must agree so linear filtering cannot reveal page padding.
struct FrontendFrameSampling {
  std::optional<float> left_u, right_u;
};
inline FrontendFrameSampling frontend_frame_sampling(
    float x, float width, float u0, float u1, unsigned texture_width) {
  if (!texture_width || !std::isfinite(x) || !std::isfinite(width) ||
      !std::isfinite(u0) || !std::isfinite(u1) ||
      !std::isfinite(x + width))
    throw std::invalid_argument("invalid frontend frame sampling bounds");
  FrontendFrameSampling result;
  // Existing bitmap extension admission excludes nonpositive scaled widths.
  // The actual original shell uses increasing U; reversed UVs are outside
  // this bounded extension helper rather than an invented orientation rule.
  if (width <= 0) return result;
  if (u1 <= u0)
    throw std::invalid_argument("unsupported frontend frame UV orientation");
  const auto boundary_u = [&](float boundary, float inward) {
    return u0 + (boundary - x) / width * (u1 - u0) +
           inward / float(texture_width);
  };
  if (x <= 0 && x + width >= 0)
    result.left_u = boundary_u(0, .5f);
  if (x <= 640 && x + width >= 640)
    result.right_u = boundary_u(640, -.5f);
  if (result.left_u && result.right_u && *result.left_u > *result.right_u)
    throw std::invalid_argument("frontend frame inward bounds cross");
  return result;
}
} // namespace halo2
