#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Original regular bitmap geometry after selected sprite/anchor/effect inputs.
// UV bounds use source order left, right, top, bottom. Registration contains
// authored fractions; axes and anchor are the actual prior effect outputs.
struct HudBitmapGeometryInput {
  std::array<float, 2> anchor, horizontal_axis, vertical_axis;
  float bitmap_width, bitmap_height;
  std::array<float, 4> sprite_bounds;
  std::array<float, 2> registration;
  std::uint16_t flags;
};
struct HudBitmapGeometryConstants {
  float stretch; // Original base XBE45DC00, separately verified later operands.
  float mirror;  // Original base XBE45DC0C.
};
struct HudBitmapQuad {
  // Original order: top left, top right, bottom right, bottom left. Mirroring
  // moves positions, retaining the same original UV order and vertex indices.
  std::array<std::array<float, 2>, 4> positions;
  std::array<float, 4> sprite_bounds;
};
struct HudBitmapQuads {
  std::array<HudBitmapQuad, 4> quads;
  unsigned count;
};
// Original22AFF1 common scalar construction plus regular bitmap UV flips,
// stretch and four-request mirror branch. Actual bitmap selection/visibility,
// special-type shading, effect producers, clipping, sampling and GPU remain
// caller-owned; this function supplies no readiness or presentation result.
HudBitmapQuads source_hud_bitmap_geometry(const HudBitmapGeometryInput &,
                                         const HudBitmapGeometryConstants &);
} // namespace halo2
