#include "halo2/source_point_collision.hpp"
#include <bit>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
SourcePointCollisionScratch
source_point_collision_scratch_1_1(std::uint32_t entry) {
  return {entry - 8, entry - 4, entry - 0x2c};
}
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (i * 8);
  return x;
}
std::int32_t si(std::uint32_t x) { return std::bit_cast<std::int32_t>(x); }
std::int32_t sx(const M &m, std::uint32_t p, unsigned n) {
  const auto x = r(m, p, n);
  return n == 1 ? std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x)))
                : std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x)));
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t x) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(4);
  s.written.resize(4);
  m.read(p, s.before);
  for (unsigned i = 0; i < 4; ++i)
    s.written[i] = std::byte(x >> (i * 8));
  m.write(p, s.written);
  if (o)
    o(s);
}
[[noreturn]] void missing(const char *s) { throw std::runtime_error(s); }
float f(std::uint32_t x) { return std::bit_cast<float>(x); }
std::uint32_t u(float x) { return std::bit_cast<std::uint32_t>(x); }
__m128 v(std::uint32_t x) { return _mm_set_ss(f(x)); }
std::uint32_t word(__m128 x) { return u(_mm_cvtss_f32(x)); }
struct Scope {
  unsigned old;
  explicit Scope(FP p) : old(_mm_getcsr()) {
    if ((p.mxcsr & 0x1f80) != 0x1f80)
      missing("Required original point-query FP exception owner unavailable");
    _mm_setcsr(p.mxcsr);
  }
  ~Scope() { _mm_setcsr(old); }
};
float plane(const M &m, std::uint32_t p, std::uint32_t point) {
  auto x = _mm_add_ss(_mm_mul_ss(v(r(m, p + 8)), v(r(m, point + 8))),
                      _mm_mul_ss(v(r(m, p + 4)), v(r(m, point + 4))));
  x = _mm_add_ss(x, _mm_mul_ss(v(r(m, p)), v(r(m, point))));
  return _mm_cvtss_f32(_mm_sub_ss(x, v(r(m, p + 12))));
}
bool sphere(const M &m, std::uint32_t center, std::uint32_t radius,
            std::uint32_t point, bool z_before_y) {
  const auto x = _mm_sub_ss(v(r(m, center)), v(r(m, point))),
             y = _mm_sub_ss(v(r(m, center + 4)), v(r(m, point + 4))),
             z = _mm_sub_ss(v(r(m, center + 8)), v(r(m, point + 8)));
  auto sum = _mm_add_ss(_mm_mul_ss(x, x),
                        z_before_y ? _mm_mul_ss(z, z) : _mm_mul_ss(y, y));
  sum = _mm_add_ss(sum, z_before_y ? _mm_mul_ss(y, y) : _mm_mul_ss(z, z));
  const auto rad = v(r(m, radius));
  return _mm_cvtss_f32(_mm_mul_ss(rad, rad)) >= _mm_cvtss_f32(sum);
}
std::uint32_t spatial_first(M &m, std::uint32_t cluster,
                            const SourcePointCollisionScratch &s, const O &o) {
  const auto token =
      r(m, r(m, 0x4e58b0) +
               std::uint32_t(std::int32_t(
                   std::bit_cast<std::int16_t>(std::uint16_t(cluster)))) *
                   4);
  w(m, o, 0xb9841, s.iterator, token);
  if (token == 0xffffffffu)
    return token;
  const auto pool = r(m, 0x4e58b4),
             row = r(m, pool + 0x44) + (token & 0xffffu) * r(m, pool + 0x24);
  const auto next = r(m, row + 8);
  w(m, o, 0xb986f, s.record_pointer, row + 12);
  w(m, o, 0xb9889, s.iterator, next);
  return r(m, row + 4);
}
std::uint32_t spatial_next(M &m, const SourcePointCollisionScratch &s,
                           const O &o) {
  const auto token = r(m, s.iterator);
  if (token == 0xffffffffu)
    return token;
  const auto pool = r(m, 0x4e58b4),
             row = r(m, pool + 0x44) + (token & 0xffffu) * r(m, pool + 0x24);
  const auto next = r(m, row + 8);
  w(m, o, 0xb98d1, s.record_pointer, row + 12);
  w(m, o, 0xb98eb, s.iterator, next);
  return r(m, row + 4);
}
std::uint16_t reject_flags(std::uint32_t flags) {
  std::uint32_t out = ((flags & 0x80000u) | 0x40000u) >> 18;
  if (flags & 0x40000u)
    out |= 4;
  if (flags & 0x40000000u)
    out |= 8;
  if (flags & 0x100000u)
    out |= 16;
  if (flags & 0x400000u)
    out |= 32;
  if (flags & 0x200000u)
    out |= 64;
  return std::uint16_t(out);
}
} // namespace
std::uint32_t source_point_collision_leaf_1_1(const M &m, std::uint32_t bsp,
                                              std::uint32_t point,
                                              std::uint32_t root, FP fp) {
  Scope scope(fp);
  const auto nodes = r(m, bsp + 4), planes = r(m, bsp + 12);
  auto node = root;
  do {
    const auto p = nodes + (node & 0x7fffffu) * 8,
               geometry = planes + std::uint32_t(sx(m, p, 2)) * 16;
    const auto side = std::uint32_t(plane(m, geometry, point) >= 0.f);
    node = std::uint32_t(si(r(m, p + side * 3 + 1)) >> 8);
  } while (!(node & 0x800000u));
  return node == 0xffffffffu ? node : node & 0x7fffffu;
}
void source_point_collision_inverse_1_1(M &m, std::uint32_t matrix,
                                        std::uint32_t point, std::uint32_t out,
                                        FP fp, const O &o) {
  Scope scope(fp);
  const auto scale = f(r(m, matrix));
  if (scale == 0.f) {
    constexpr std::array sites{0x143f4fu, 0x143f53u, 0x143f58u};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, sites[i], out + i * 4, 0);
    return;
  }
  __m128 xyz[3];
  for (unsigned i = 0; i < 3; ++i)
    xyz[i] = _mm_sub_ss(v(r(m, point + i * 4)), v(r(m, matrix + 0x28 + i * 4)));
  if (scale != f(r(m, 0x465030))) {
    const auto inv = _mm_div_ss(v(r(m, 0x465030)), v(r(m, matrix)));
    for (auto &x : xyz)
      x = _mm_mul_ss(inv, x);
  }
  constexpr std::array sites{0x143ef8u, 0x143f1fu, 0x143f47u};
  for (unsigned i = 0; i < 3; ++i) {
    auto x = _mm_add_ss(_mm_mul_ss(v(r(m, matrix + 12 + i * 12)), xyz[2]),
                        _mm_mul_ss(v(r(m, matrix + 8 + i * 12)), xyz[1]));
    x = _mm_add_ss(x, _mm_mul_ss(v(r(m, matrix + 4 + i * 12)), xyz[0]));
    w(m, o, sites[i], out + i * 4, word(x));
  }
}
bool source_point_collision_instance_admitted_1_1(const M &m, std::uint32_t def,
                                                  std::uint32_t index,
                                                  std::uint32_t flags) {
  const auto count = si(r(m, def + 0x98));
  if (count < 1 ||
      (r(m, r(m, def + 0x9c) + std::uint32_t(count) * 8 - 4, 1) & 0x10u))
    return false;
  if (flags & 0x40000u) {
    const auto bsp = r(m, 0x4e7918);
    if (si(r(m, bsp + 0xc4)) > 0) {
      const auto runtime = r(m, bsp + 0xc8);
      if (runtime && (si(index) < 0 || si(index) >= si(r(m, runtime + 0x40)) ||
                      sx(m, r(m, runtime + 0x44) + index * 4, 2) == -1))
        return false;
    }
  }
  return true;
}
bool source_point_collision_instance_1_1(M &m, std::uint32_t index,
                                         std::uint32_t point,
                                         std::uint32_t flags,
                                         std::uint32_t scratch, FP fp,
                                         const O &o) {
  Scope scope(fp);
  const auto bsp = r(m, 0x4e7918), instance = r(m, bsp + 0x144) + index * 0x58,
             definition = r(m, bsp + 0x13c) +
                          std::uint32_t(sx(m, instance + 0x34, 2)) * 0xc8;
  if (!source_point_collision_instance_admitted_1_1(m, definition, index,
                                                    flags) ||
      !sphere(m, instance + 0x3c, instance + 0x48, point, true))
    return false;
  source_point_collision_inverse_1_1(m, instance, point, scratch, fp, o);
  return source_point_collision_leaf_1_1(m, definition + 0x70, scratch, 0,
                                         fp) == 0xffffffffu;
}
bool source_point_collision_1_1(M &m, std::uint32_t flags, std::uint32_t point,
                                std::uint32_t excluded_a,
                                std::uint32_t excluded_b,
                                const SourcePointCollisionScratch &s, FP fp,
                                const SourcePointCollisionServices &services,
                                const O &o) {
  Scope scope(fp);
  const auto leaf =
      source_point_collision_leaf_1_1(m, r(m, 0x4e790c), point, 0, fp);
  if (leaf == 0xffffffffu)
    return true;
  if (!(flags & 14u))
    return false;
  const auto bsp = r(m, 0x4e7918),
             cluster = std::uint32_t(sx(m, r(m, bsp + 0x30) + leaf * 8, 2));
  bool collision = false;
  if (flags & 2u) {
    const auto fog = sx(m, r(m, bsp + 0xa0) + cluster * 0xb0 + 0x70, 1);
    if (fog != -1) {
      const auto index = std::uint32_t(fog) & 127u;
      if (fog < 0) {
        const auto p = r(m, bsp + 0x68) + index * 0x18;
        if (sx(m, p + 2, 2) != -1 && plane(m, p + 4, point) < 0.f)
          collision = true;
      } else if (sx(m, r(m, r(m, 0x4e7920) + 0x34c) + index * 16 + 12, 2) != -1)
        collision = true;
    }
  }
  if ((flags & 4u) && !collision) {
    const auto row = r(m, bsp + 0xa0) + cluster * 0xb0;
    for (std::uint32_t i = 0; si(i) < si(r(m, row + 0x98)); ++i) {
      const auto index = std::uint32_t(sx(m, r(m, row + 0x9c) + i * 2, 2));
      if (source_point_collision_instance_1_1(m, index, point, flags,
                                              s.instance_local_point, fp, o)) {
        collision = true;
        break;
      }
    }
  }
  if (!(flags & 8u) || collision)
    return collision;
  w(m, o, 0x16ab15, s.record_pointer, 0);
  const auto rejected = reject_flags(flags);
  const auto types = (flags >> 4) & 0x1fffu;
  for (auto datum = spatial_first(m, cluster, s, o); datum != 0xffffffffu;
       datum = spatial_next(m, s, o)) {
    const auto p = r(m, s.record_pointer);
    if (!(types & (1u << (r(m, p, 1) & 31u))) || (r(m, p + 2, 2) & rejected) ||
        !sphere(m, p + 4, p + 16, point, false) || datum == excluded_a ||
        datum == excluded_b)
      continue;
    if (!services.object)
      missing(
          "Required original16A8A0 model point-containment owner unavailable");
    if (services.object({datum, flags, point, excluded_a, excluded_b}))
      return true;
  }
  return false;
}
} // namespace halo2
