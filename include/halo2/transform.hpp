#pragma once
#include "halo2/cache.hpp"
#include <array>

namespace halo2 {
// Retail transform4x3: uniform scale, three rotation columns, translation.
struct Transform {
  float scale = 1;
  std::array<Vec3, 3> basis{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
  Vec3 position;
};
// Original 1420F0, 141590 and 142A60 arithmetic and operation order.
Transform camera_transform(Vec3 position, Vec3 forward, Vec3 up);
Transform inverse_transform(const Transform &);
Transform compose_transform(const Transform &, const Transform &);
} // namespace halo2
