#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace halo2 {
// Native presentation extension: foreground widgets retain the authored
// centered 640x480 canvas. Only explicitly marked background art spans width.
struct FrontendCanvas {
  float scale{}, left{}, top{}, width{}, height{}, surface_width{}, surface_height{};
  float background_logical_width() const { return surface_width / scale; }
  std::array<float, 4> source(bool background) const {
    return background ? std::array<float, 4>{0, top / surface_height, 1, height / surface_height}
                      : std::array<float, 4>{left / surface_width, top / surface_height,
                                             width / surface_width, height / surface_height};
  }
};
inline FrontendCanvas frontend_canvas(std::uint32_t width, std::uint32_t height) {
  if (!width || !height) throw std::invalid_argument("empty frontend drawable");
  const float w = float(width), h = float(height);
  const float scale = std::min(w / 640.f, h / 480.f);
  return {scale, (w - 640.f * scale) * .5f, (h - 480.f * scale) * .5f,
          640.f * scale, 480.f * scale, w, h};
}
} // namespace halo2
