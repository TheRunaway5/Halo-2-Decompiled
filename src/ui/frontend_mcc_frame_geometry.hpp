#pragma once
#include "halo2/frontend.hpp"
#include <array>
#include <openssl/sha.h>
#include <span>

namespace halo2::detail {
// Exact MCC decoded frame owner; custom artwork must retain its own mapping.
inline bool stock_mcc_frame_pixels(const Texture &texture) {
  if (texture.width != 2048 || texture.height != 1303 || texture.format != 11 ||
      texture.pixels.size() != std::size_t(2048) * 1303 * 4)
    return false;
  constexpr std::array<unsigned char, 32> expected{
      0x44, 0xc4, 0x2e, 0xa2, 0xff, 0xd6, 0x2c, 0x13, 0xbb, 0x4a, 0xff,
      0xb9, 0x35, 0x59, 0x07, 0x32, 0xfb, 0xb9, 0xe8, 0xbb, 0xa8, 0xa1,
      0xcb, 0x62, 0x2c, 0x74, 0x0f, 0x2b, 0x7a, 0x19, 0xfd, 0x1a};
  std::array<unsigned char, 32> digest{};
  return SHA256(reinterpret_cast<const unsigned char *>(texture.pixels.data()),
                texture.pixels.size(), digest.data()) &&
         digest == expected;
}

namespace mcc_frame_geometry {
struct Point {
  float logical, source;
};
// Bar contour landmarks extracted from the original1024x512 reference and
// the selected MCC2048x1303 owner. The original page has only640 active
// columns; its remaining384 columns are padding. MCC's page has different
// padding and spacing, so treating it as uniform image density moves both bars.
// Numeric contour evidence: .local/mcc-frame-anchor-20261019/*-contours.json.
// No original pixels or cache are loaded by this projection.
inline constexpr std::array header_x{Point{0, 0},      Point{363, 1054},
                                     Point{391, 1108}, Point{405, 1136},
                                     Point{410, 1144}, Point{640, 2048}};
inline constexpr std::array header_y{Point{0, 0},    Point{31, 69},
                                     Point{38, 82},  Point{44, 95},
                                     Point{59, 124}, Point{240, 651.5f}};
inline constexpr std::array footer_x{Point{0, 0},      Point{99, 566},
                                     Point{337, 1008}, Point{455, 1228},
                                     Point{467, 1250}, Point{640, 2048}};
inline constexpr std::array footer_y{Point{240, 651.5f}, Point{421, 1058},
                                     Point{439, 1091},   Point{449, 1113},
                                     Point{462, 1135},   Point{512, 1303}};
template <class Emit> void regions(Emit emit) {
  const auto grid = [&](auto &xs, auto &ys) {
    for (unsigned y = 1; y < ys.size(); ++y)
      for (unsigned x = 1; x < xs.size(); ++x)
        emit(xs[x - 1].logical, ys[y - 1].logical,
             xs[x].logical - xs[x - 1].logical,
             ys[y].logical - ys[y - 1].logical, xs[x - 1].source / 2048.f,
             ys[y - 1].source / 1303.f, xs[x].source / 2048.f,
             ys[y].source / 1303.f);
  };
  grid(header_x, header_y);
  grid(footer_x, footer_y);
}
template <class Emit> void edge_regions(Emit emit) {
  for (const auto ys :
       {std::span<const Point>{header_y}, std::span<const Point>{footer_y}})
    for (unsigned y = 1; y < ys.size(); ++y)
      emit(ys[y - 1].logical, ys[y].logical - ys[y - 1].logical,
           ys[y - 1].source / 1303.f, ys[y].source / 1303.f);
}
} // namespace mcc_frame_geometry
} // namespace halo2::detail
