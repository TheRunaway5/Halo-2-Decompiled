#include "halo2/frontend_post_raster.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>

namespace {
void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
void close(double actual, double expected, double tolerance, const char *message) {
  require(std::abs(actual - expected) < tolerance, message);
}
template <class F> void rejects(F function) {
  bool rejected = false;
  try { function(); } catch (const std::exception &) { rejected = true; }
  require(rejected, "invalid post raster is admitted");
}
}
int main() {
  try {
    struct Case { unsigned width, height, bloom_width, bloom_height, haze_width, haze_height; };
    const std::array cases{
        Case{640,480,160,120,320,240}, Case{960,720,160,120,320,240},
        Case{1280,720,214,120,428,240}, Case{1280,800,192,120,384,240},
        Case{1680,720,280,120,560,240}, Case{2560,720,428,120,854,240},
        Case{1000,600,200,120,400,240}, Case{1280,1024,160,128,320,256},
        Case{1919,1081,214,120,428,240}};
    unsigned pixel_checks{};
    for (const auto &c : cases) for (unsigned haze = 0; haze < 2; ++haze) {
      const unsigned base_width = haze ? 320 : 160, base_height = haze ? 240 : 120;
      const auto raster = halo2::frontend_post_raster(c.width,c.height,base_width,base_height);
      require(raster.width == (haze ? c.haze_width : c.bloom_width) &&
              raster.height == (haze ? c.haze_height : c.bloom_height),
              "post raster has incorrect padded dimensions");
      const double fit = std::min(double(c.width)/640., double(c.height)/480.);
      for (unsigned axis = 0; axis < 2; ++axis) {
        const unsigned base = axis ? base_height : base_width;
        const unsigned padded = axis ? raster.height : raster.width;
        const double surface = axis ? c.height : c.width;
        const double canvas = (axis ? 480. : 640.) * fit;
        const double spacing = canvas / base;
        const unsigned centered_start = (padded - base) / 2;
        require(padded >= base && !(padded & 1u) && !((padded - base) & 1u),
                "post raster changes the original center texel phase");
        close(raster.required[axis], base * surface / canvas, 1e-10,
              "post raster changes the required low-resolution coverage");
        require(raster.required[axis] <= padded && padded - raster.required[axis] < 2,
                "post raster adds more than one outer texel pair");
        close(double(raster.source[axis + 2]) * surface / padded, spacing, 1e-6,
              "threshold taps change their physical spacing");
        close(raster.scale[axis], double(padded)/base, 1e-6,
              "haze row scaling does not cover the padded footprint");
        // A canonical texel keeps its original physical location when the
        // threshold pass reads the expanded scene and the composite crops it.
        for (unsigned index = 0; index < base; ++index) {
          const double texel = centered_start + index + .5;
          const double physical = surface*.5 + (double(index)+.5-base*.5)*spacing;
          const double source = raster.source[axis] + texel/padded*raster.source[axis+2];
          close(source*surface, physical, .0002,
                "threshold pass moves an original center texel");
          const double crop = raster.crop[axis] + physical/surface*raster.crop[axis+2];
          close(crop*padded, texel, .0001,
                "composite lookup moves an original center texel");
          const double noise_clip = (crop*2.-1.)*raster.scale[axis];
          close(noise_clip, (double(index)+.5)*2./base-1., 1e-6,
                "haze noise coordinate changes inside the original center");
          ++pixel_checks;
        }
      }
      if (std::uint64_t(c.width)*3 == std::uint64_t(c.height)*4) {
        require(raster.required == std::array<double,2>{double(base_width),double(base_height)} &&
                raster.crop == std::array<float,4>{0,0,1,1} &&
                raster.source == std::array<float,4>{0,0,1,1} &&
                raster.scale[0] == 1 && raster.scale[1] == 1,
                "4:3 post raster changes the original identity mapping");
        require(std::bit_cast<std::uint32_t>(raster.crop[0]) == 0 &&
                std::bit_cast<std::uint32_t>(raster.crop[1]) == 0 &&
                std::bit_cast<std::uint32_t>(raster.source[0]) == 0 &&
                std::bit_cast<std::uint32_t>(raster.source[1]) == 0,
                "4:3 crop introduces signed zero");
      }
    }
    rejects([] { halo2::frontend_post_raster(0,720,160,120); });
    rejects([] { halo2::frontend_post_raster(1280,0,160,120); });
    rejects([] { halo2::frontend_post_raster(1280,720,0,120); });
    rejects([] { halo2::frontend_post_raster(1280,720,161,120); });
    rejects([] { halo2::frontend_post_raster(1280,720,160,121); });
    rejects([] { halo2::frontend_post_raster(0xffffffffu,1,160,120); });
    std::cout << "Padded frontend bloom/haze raster: " << pixel_checks
              << " centered texel, tap spacing and phase checks\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
