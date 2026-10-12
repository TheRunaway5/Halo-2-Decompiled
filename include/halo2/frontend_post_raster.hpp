#pragma once
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace halo2 {
struct FrontendPostRaster {
  std::uint32_t width{}, height{};
  std::array<double, 2> required{};
  // Full drawable -> padded low-resolution texture coordinates.
  std::array<float, 4> crop{};
  // Padded low-resolution texture -> full drawable source coordinates.
  std::array<float, 4> source{};
  // Expand completed original haze rows to the padded raster footprint.
  std::array<float, 2> scale{};
};

// Native presentation around a centered 640x480 source image. Keep the
// original low-resolution texels and tap spacing; add whole pairs of outer
// texels and crop their fractional outer coverage when composing the frame.
inline FrontendPostRaster frontend_post_raster(std::uint32_t width,
    std::uint32_t height, std::uint32_t base_width, std::uint32_t base_height) {
  if (!width || !height || !base_width || !base_height ||
      (base_width & 1u) || (base_height & 1u))
    throw std::invalid_argument("frontend post raster requires nonempty surface and even source dimensions");
  struct Axis { std::uint32_t padded; double required; };
  const auto axis = [](std::uint32_t base, std::uint64_t numerator,
                       std::uint64_t denominator) -> Axis {
    if (numerator > std::numeric_limits<std::uint64_t>::max() / base)
      throw std::overflow_error("frontend post raster ratio is too large");
    const auto product = std::uint64_t(base) * numerator;
    const auto pair = denominator * 2;
    // Integer ceiling avoids an extra pair at exact rational boundaries.
    const auto half = product / pair + (product % pair != 0);
    if (half > std::numeric_limits<std::uint32_t>::max() / 2)
      throw std::overflow_error("frontend post raster exceeds addressable dimensions");
    return {std::uint32_t(half * 2), double(product) / double(denominator)};
  };
  const auto wide = std::uint64_t(width) * 3;
  const auto tall = std::uint64_t(height) * 4;
  const auto x = wide > tall ? axis(base_width, wide, tall) : axis(base_width, 1, 1);
  const auto y = tall > wide ? axis(base_height, tall, wide) : axis(base_height, 1, 1);
  const double pad_x = (double(x.padded) - x.required) * .5;
  const double pad_y = (double(y.padded) - y.required) * .5;
  return {x.padded, y.padded, {x.required, y.required},
      {float(pad_x / x.padded), float(pad_y / y.padded),
       float(x.required / x.padded), float(y.required / y.padded)},
      {pad_x == 0 ? 0.f : float(-pad_x / x.required),
       pad_y == 0 ? 0.f : float(-pad_y / y.required),
       float(x.padded / x.required), float(y.padded / y.required)},
      {float(x.padded) / base_width, float(y.padded) / base_height}};
}
} // namespace halo2
