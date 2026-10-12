#pragma once
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
inline Vec3 operator+(Vec3 a, Vec3 b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vec3 operator-(Vec3 a, Vec3 b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator/(Vec3 a, float s) { return a * (1 / s); }
inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalized(Vec3 a) {
  const float n = length(a);
  return n > 1e-8f ? a / n : Vec3{};
}
// NV2A D3DVSDT_NORMPACKED3: signed 11/11/10-bit components divided by
// 1023/1023/511. Unlike normalized SHORT, the negative endpoint is not clamped.
inline Vec3 unpack_normal(std::uint32_t packed) {
  const auto axis = [](std::uint32_t value, unsigned bits) {
    const auto sign = 1u << (bits - 1), mask = (1u << bits) - 1;
    const auto integer = std::int32_t((value & mask) ^ sign) - std::int32_t(sign);
    return float(integer) / float(sign - 1);
  };
  return {axis(packed, 11), axis(packed >> 11, 11), axis(packed >> 22, 10)};
}
inline Vec3 direction(float yaw, float pitch = 0) {
  return {std::cos(yaw) * std::cos(pitch), std::sin(yaw) * std::cos(pitch),
          std::sin(pitch)};
}
// Scenario Euler angles use the retail 0x141CE0 matrix, whose rotation order
// differs from the free-look camera. Columns are forward, left and up.
struct AuthoredBasis {
  Vec3 forward, left, up;
  Vec3 rotate(Vec3 v) const { return forward * v.x + left * v.y + up * v.z; }
};
AuthoredBasis source_euler_basis(Vec3);
inline AuthoredBasis authored_basis(Vec3 r) { return source_euler_basis(r); }
inline Vec3 minimum(Vec3 a, Vec3 b) {
  return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}
inline Vec3 maximum(Vec3 a, Vec3 b) {
  return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}
} // namespace halo2
