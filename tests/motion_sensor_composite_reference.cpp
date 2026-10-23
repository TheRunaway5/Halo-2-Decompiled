// Frozen pre-optimization host compositor, source SHA-256
// 5618997563b135f586b500918cd7427fa97e3bc6f1603d824d80281137291f8d.
// Differential oracle; this is not an independent Xbox framebuffer fixture.
#include "halo2/motion_sensor.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
Texture motion_sensor_reference_composite(const MotionSensorRenderPlan &plan,
                                          const MotionSensorArtwork &art,
                                          Vec3 color, unsigned raster_size) {
  if (raster_size < 64 || raster_size > 512 || raster_size % 64)
    throw FormatError(
        "Motion-sensor raster size must be a multiple of64 in64..512");
  const float density = float(raster_size) / 64;
  for (const auto *t : {&art.sweep, &art.mask, &art.blip, &art.custom})
    if (!t->width || !t->height || t->format != 11 ||
        t->pixels.size() < std::size_t(t->width) * t->height * 4)
      throw FormatError("Original motion-sensor artwork must be decoded RGBA");
  auto sample = [](const Texture &t, float u, float v, float lod,
                   float border_alpha = 0.f) {
    const auto level = t.mips.empty()
                           ? 0u
                           : unsigned(std::clamp(int(std::floor(lod + .5f)), 0,
                                                 int(t.mips.size() - 1)));
    const auto width = t.mips.empty() ? t.width : t.mips[level].width,
               height = t.mips.empty() ? t.height : t.mips[level].height;
    const auto offset = t.mips.empty() ? 0 : t.mips[level].offset;
    const float x = u * width - .5f, y = v * height - .5f;
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = x - ix, fy = y - iy;
    std::array<float, 4> out{};
    for (int j = 0; j < 2; ++j)
      for (int i = 0; i < 2; ++i) {
        const int px = ix + i, py = iy + j;
        const float weight = (i ? fx : 1 - fx) * (j ? fy : 1 - fy);
        // 254200 selects border addressing.2548F0 changes its ARGB border
        // to46000000 before the sweep and retains it through the mask. RGB
        // is black, but alpha70/255 preserves contacts beyond the pulse ring.
        if (px < 0 || py < 0 || px >= int(width) || py >= int(height)) {
          out[3] += border_alpha * weight;
          continue;
        }
        for (unsigned c = 0; c < 4; ++c)
          out[c] += std::to_integer<unsigned>(
                        t.pixels[offset + (py * width + px) * 4 + c]) /
                    255.f * weight;
      }
    return out;
  };
  Texture out;
  out.width = out.height = raster_size;
  out.format = 11;
  out.pixels.resize(std::size_t(raster_size) * raster_size * 4);
  auto quantize = [](float v) {
    return std::clamp(std::round(v * 255.f), 0.f, 255.f) / 255.f;
  };
  std::vector<std::array<float, 4>> target(std::size_t(raster_size) *
                                           raster_size);
  if (plan.visible) {
    for (const auto &b : plan.blips) {
      // 254490: color is multiplied by the history scale, while the quad's
      //  half extent is intensity*2. These two arguments are not
      //  interchangeable.
      const float extent = b.intensity * 2;
      if (!(extent > 0))
        continue;
      const auto &texture = b.custom ? art.custom : art.blip;
      // NV2A screen coordinates have four fractional bits, truncated toward
      // zero. The original-game capture's passthrough shader preserves this
      // conversion (xemu0.8.136 pgraph/glsl/vsh.c:roundScreenCoords). Round
      // each edge independently before coverage and interpolation, not the
      // center.
      auto screen = [](float value) { return std::trunc(value * 16.f) / 16.f; };
      const float x0 = screen(b.point.x - extent),
                  x1 = screen(b.point.x + extent),
                  y0 = screen(b.point.y - extent),
                  y1 = screen(b.point.y + extent);
      if (x1 <= x0 || y1 <= y0)
        continue;
      const int left = std::max(0, int(std::ceil(x0 * density - .5f))),
                right = std::min(int(raster_size),
                                 int(std::ceil(x1 * density - .5f))),
                top = std::max(0, int(std::ceil(y0 * density - .5f))),
                bottom = std::min(int(raster_size),
                                  int(std::ceil(y1 * density - .5f)));
      for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x) {
          const auto texel =
              sample(texture, ((x + .5f) / density - x0) / (x1 - x0),
                     (y1 - (y + .5f) / density) / (y1 - y0),
                     std::log2(std::max(
                         float(texture.width) / ((x1 - x0) * density),
                         float(texture.height) / ((y1 - y0) * density))));
          auto &pixel = target[y * raster_size + x];
          const std::array<float, 3> rgb{b.color.x, b.color.y, b.color.z};
          for (unsigned c = 0; c < 3; ++c)
            pixel[c] = quantize(pixel[c] + texel[c] * b.scale * rgb[c]);
        }
    }
    for (unsigned y = 0; y < raster_size; ++y)
      for (unsigned x = 0; x < raster_size; ++x) {
        const float u = (x + .5f) / raster_size, v = (y + .5f) / raster_size;
        const auto sweep = sample(
            art.sweep, .5f + plan.pulse * (.5f - u),
            .5f + plan.pulse * (v - .5f),
            std::log2(plan.pulse * (raster_size == 64 ? 1.f
                                                      : float(art.sweep.width) /
                                                            raster_size)),
            70.f / 255);
        auto &pixel = target[y * raster_size + x];
        const std::array<float, 3> rgb{color.x, color.y, color.z};
        for (unsigned c = 0; c < 3; ++c)
          pixel[c] = quantize(sweep[c] * rgb[c] + pixel[c] * sweep[3]);
        // 254200 clears the A8R8G8B8 target to00000000, then selects
        //  COLORWRITEENABLE=00010101.2548F0 retains that RGB-only mask through
        //  sweep and mask. Their source alpha controls RGB blending, but must
        //  not become destination alpha: the final ONE/INVSRCALPHA pass would
        //  otherwise darken the authored background and expose a square pulse.
        const float mask = sample(art.mask, 1 - u, v, 0, 70.f / 255)[3];
        for (auto &channel : pixel)
          channel = quantize(channel * mask);
      }
  }
  for (unsigned p = 0; p < target.size(); ++p)
    for (unsigned c = 0; c < 4; ++c)
      out.pixels[p * 4 + c] =
          std::byte(unsigned(std::round(target[p][c] * 255)));
  return out;
}
} // namespace halo2
