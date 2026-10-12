#include "halo2/menu_world_depth.hpp"
#include "halo2/cache.hpp"
#include <cmath>

namespace halo2 {
namespace {
constexpr float original_w_max = 16777215.f; // Literal45DD40, word4B7FFFFF.
float clip_distance(float reciprocal, float distance) {
  // Each original MULSS rounds independently. Do not reassociate into
  // distance/far or fuse the product with the subsequent scaling.
  const float product = reciprocal * distance;
  float plane = product * original_w_max;
  if (plane < 0.f) plane = 0.f;
  else if (plane > original_w_max) plane = original_w_max;
  return plane; // Equality preserves the original negative-zero word.
}
}
MenuWorldDepthClipPlanes menu_world_depth_clip_planes(
    const MenuWorldDepthClipInput &input) {
  if (!std::isfinite(input.near_distance) ||
      !std::isfinite(input.far_distance) || !(input.far_distance > 0.f))
    throw FormatError("menu depth clip distances require finite near and positive finite far");
  if (input.boundary != MenuWorldDepthBoundary::none &&
      input.boundary != MenuWorldDepthBoundary::near_plane &&
      input.boundary != MenuWorldDepthBoundary::far_plane)
    throw FormatError("unsupported menu depth clip boundary mode");
  if (input.boundary != MenuWorldDepthBoundary::none &&
      !std::isfinite(input.boundary_distance))
    throw FormatError("menu depth boundary distance must be finite");
  const float reciprocal = 1.f / input.far_distance; // Original1347B DIVSS.
  if (!std::isfinite(reciprocal))
    throw FormatError("unsupported menu depth far distance reciprocal overflow");
  MenuWorldDepthClipPlanes result{
      clip_distance(reciprocal, input.near_distance),
      clip_distance(reciprocal, input.far_distance)};
  if (input.boundary == MenuWorldDepthBoundary::near_plane)
    result.minimum_w = clip_distance(reciprocal, input.boundary_distance);
  else if (input.boundary == MenuWorldDepthBoundary::far_plane)
    result.maximum_w = clip_distance(reciprocal, input.boundary_distance);
  if (input.scaled_projection)
    result.minimum_w *= .15000000596046448f; // Literal4407B8, original12D7A.
  return result;
}
} // namespace halo2
