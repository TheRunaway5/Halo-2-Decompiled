#include "halo2/transform.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
static_assert(sizeof(Transform) == 13 * sizeof(float));
Transform camera_transform(Vec3 position, Vec3 forward, Vec3 up) {
  return {1, {forward, cross(up, forward), up}, position};
}
Transform inverse_transform(const Transform &in) {
  if (in.scale == 0)
    return {0, {}, {}};
  Vec3 translation{0.f - in.position.x, 0.f - in.position.y,
                   0.f - in.position.z};
  const float scale = in.scale == 1.f ? 1.f : 1.f / in.scale;
  if (in.scale != 1.f)
    translation = translation * scale;
  const auto &b = in.basis;
  Transform out{scale,
                {{{b[0].x, b[1].x, b[2].x},
                  {b[0].y, b[1].y, b[2].y},
                  {b[0].z, b[1].z, b[2].z}}},
                {}};
  out.position.x =
      (out.basis[2].x * translation.z + out.basis[0].x * translation.x) +
      out.basis[1].x * translation.y;
  out.position.y =
      (out.basis[2].y * translation.z + out.basis[0].y * translation.x) +
      out.basis[1].y * translation.y;
  out.position.z =
      (out.basis[0].z * translation.x + out.basis[2].z * translation.z) +
      out.basis[1].z * translation.y;
  return out;
}
Transform compose_transform(const Transform &a, const Transform &b) {
  // Retail SSE sums basis columns as X + (Y + Z), but the translated point
  // as (X + Y) + Z. Keeping that distinction avoids a rounding change.
  const auto rotate_basis = [&](Vec3 p) {
    return Vec3{p.x * a.basis[0].x + (p.y * a.basis[1].x + p.z * a.basis[2].x),
                p.x * a.basis[0].y + (p.y * a.basis[1].y + p.z * a.basis[2].y),
                p.x * a.basis[0].z + (p.y * a.basis[1].z + p.z * a.basis[2].z)};
  };
  const auto rotate_position = [&](Vec3 p) {
    return Vec3{(p.x * a.basis[0].x + p.y * a.basis[1].x) + p.z * a.basis[2].x,
                (p.x * a.basis[0].y + p.y * a.basis[1].y) + p.z * a.basis[2].y,
                (p.x * a.basis[0].z + p.y * a.basis[1].z) + p.z * a.basis[2].z};
  };
  return {b.scale * a.scale,
          {rotate_basis(b.basis[0]), rotate_basis(b.basis[1]),
           rotate_basis(b.basis[2])},
          rotate_position(b.position) * a.scale + a.position};
}
} // namespace halo2

// Validation bridges for the original instruction comparisons.
extern "C" void halo2_transform_inverse(const float *input, float *output) {
  std::array<float, 13> words;
  std::copy_n(input, 13, words.begin());
  const auto result = std::bit_cast<std::array<float, 13>>(
      halo2::inverse_transform(std::bit_cast<halo2::Transform>(words)));
  std::copy(result.begin(), result.end(), output);
}
extern "C" void halo2_transform_compose(const float *a, const float *b,
                                        float *output) {
  std::array<float, 13> aw, bw;
  std::copy_n(a, 13, aw.begin());
  std::copy_n(b, 13, bw.begin());
  const auto result = std::bit_cast<std::array<float, 13>>(
      halo2::compose_transform(std::bit_cast<halo2::Transform>(aw),
                               std::bit_cast<halo2::Transform>(bw)));
  std::copy(result.begin(), result.end(), output);
}
extern "C" void halo2_camera_transform(const float *position,
                                       const float *forward, const float *up,
                                       float *output) {
  const auto result =
      std::bit_cast<std::array<float, 13>>(halo2::camera_transform(
          {position[0], position[1], position[2]},
          {forward[0], forward[1], forward[2]}, {up[0], up[1], up[2]}));
  std::copy(result.begin(), result.end(), output);
}
