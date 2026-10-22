#include "halo2/physics_body_factory.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
std::uint32_t r(const M &m, std::uint32_t a, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(a, std::span(b).first(n));
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return x;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t a, unsigned n,
       std::uint64_t v) {
  SourcePhysicsContactRecordStore s{site, a, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(a, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(v >> (8 * i));
  m.write(a, s.written);
  if (o)
    o(s);
}
[[noreturn]] void missing(const char *s) { throw std::runtime_error(s); }
float f(std::uint32_t x) { return std::bit_cast<float>(x); }
std::uint32_t u(float x) { return std::bit_cast<std::uint32_t>(x); }
__m128 v(const M &m, std::uint32_t p) {
  std::array<std::uint32_t, 4> a{};
  for (unsigned n = 0; n < 4; ++n)
    a[n] = r(m, p + 4 * n);
  return _mm_castsi128_ps(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(a.data())));
}
void vector(M &m, const O &o, std::uint32_t site, std::uint32_t p, __m128 x) {
  std::array<std::uint64_t, 2> a{};
  _mm_storeu_ps(reinterpret_cast<float *>(a.data()), x);
  w(m, o, site, p, 8, a[0]);
  w(m, o, site, p + 8, 8, a[1]);
}
struct FpScope {
  unsigned mx;
  std::fenv_t env;
  FpScope(FP p) : mx(_mm_getcsr()) {
    if ((p.x87_control_word & 0x3f) != 0x3f || (p.mxcsr & 0x1f80) != 0x1f80)
      missing("Unmasked source FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(p.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(p.x87_control_word) : "memory");
#else
    missing("Original x87 factory math backend unavailable");
#endif
  }
  ~FpScope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
long double ext(float x) { return static_cast<long double>(x); }
// All x87 expressions execute with the original supplied precision/rounding;
// every binary32 spill below is an explicit source FSTP boundary.
float spill(long double x) {
  volatile float z = static_cast<float>(x);
  return z;
}
long double add(long double a, long double b) {
  volatile long double z = a + b;
  return z;
}
long double sub(long double a, long double b) {
  volatile long double z = a - b;
  return z;
}
long double mul(long double a, long double b) {
  volatile long double z = a * b;
  return z;
}
long double div(long double a, long double b) {
  volatile long double z = a / b;
  return z;
}
long double root(long double a) {
  long double z;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fldt %1\n\tfsqrt\n\tfstpt %0" : "=m"(z) : "m"(a) : "memory");
  return z;
#else
  (void)a;
  missing("Original x87 FSQRT backend unavailable");
#endif
}
void method(const M &m, std::uint32_t p, unsigned slot, std::uint32_t target) {
  if (r(m, r(m, p) + slot) != target)
    missing("Required original factory virtual owner unavailable");
}
std::uint32_t allocate(M &m, std::uint32_t n, const O &o) {
  method(m, r(m, 0x487638), 0x10, 0x148a10);
  if (r(m, 0x480d98) != 2)
    missing("Required original factory allocator mode unavailable");
  const auto a = r(m, 0x480da0);
  method(m, a, 0x10, 0x22e950);
  const auto p = source_allocate_physics_body_small_block_1_1(m, a, n, o);
  if (!p)
    missing("Required original alternate factory allocator unavailable");
  return p;
}
__m128 transformed(const M &m, std::uint32_t matrix, __m128 point) {
  const auto z = _mm_shuffle_ps(point, point, 0xaa),
             y = _mm_shuffle_ps(point, point, 0x55),
             x = _mm_shuffle_ps(point, point, 0);
  const auto tail =
      _mm_add_ps(_mm_mul_ps(v(m, matrix + 0x20), z), v(m, matrix + 0x30));
  return _mm_add_ps(_mm_add_ps(_mm_mul_ps(v(m, matrix), x),
                               _mm_mul_ps(v(m, matrix + 0x10), y)),
                    tail);
}
float length_squared(__m128 x) {
  x = _mm_mul_ps(x, x);
  auto y = _mm_shuffle_ps(x, x, 0x55);
  y = _mm_add_ss(y, x);
  auto z = _mm_shuffle_ps(x, x, 0xaa);
  return _mm_cvtss_f32(_mm_add_ss(z, y));
}
void center(M &m, std::uint32_t motion, std::uint32_t p, FP fp, const O &o) {
  FpScope scope(fp);
  w(m, o, 0x31fd64, motion + 0x6c, 4,
    u(spill(add(ext(f(r(m, motion + 0x6c))), ext(f(r(m, 0x418460)))))));
  const auto x = v(m, p);
  vector(m, o, 0x31fd70, motion + 0x20, x);
  w(m, o, 0x31fd73, motion + 0x2c, 4, u(spill(ext(f(r(m, motion + 0x2c))))));
  const auto preserved = r(m, motion + 0x7c);
  vector(m, o, 0x2e0be5, motion + 0x70, transformed(m, motion + 0x80, x));
  w(m, o, 0x31fd8d, motion + 0x7c, 4, preserved);
}
void inertia(M &m, std::uint32_t motion, std::uint32_t p, FP fp, const O &o) {
  FpScope scope(fp);
  const auto one = ext(f(r(m, 0x465030)));
  const auto y = div(one, ext(f(r(m, p + 0x14)))),
             z = div(one, ext(f(r(m, p + 0x28)))),
             x = div(one, ext(f(r(m, p))));
  const auto old = r(m, motion + 0xec);
  w(m, o, 0x3213ba, motion + 0xe0, 4, u(spill(x)));
  w(m, o, 0x3213c2, motion + 0xe4, 4, u(spill(y)));
  w(m, o, 0x3213c8, motion + 0xe8, 4, u(spill(z)));
  w(m, o, 0x3213ce, motion + 0xec, 4, old);
}
void mass(M &m, std::uint32_t motion, std::uint32_t word, FP fp, const O &o) {
  FpScope scope(fp);
  method(m, motion, 0x20, 0x321220);
  const auto inv = u(spill(div(ext(f(r(m, 0x465030))), ext(f(word)))));
  w(m, o, 0x321226, motion + 0x2c, 4, inv);
  w(m, o, 0x321229, motion + 0xec, 4, inv);
}
void auxiliary_size(M &m, std::uint32_t aux, std::uint32_t shape,
                    std::uint32_t output, FP fp, const O &o) {
  if (shape) {
    source_physics_shape_bounds_1_1(m, shape, 0x469340, 0, output, fp, o);
    FpScope scope(fp);
    const auto delta = _mm_sub_ps(v(m, output + 16), v(m, output));
    const auto half = _mm_set1_ps(0.5f);
    w(m, o, 0x31ac41, aux + 8, 4, u(length_squared(_mm_mul_ps(half, delta))));
  } else
    w(m, o, 0x31ac4e, aux + 8, 4, 0xbf800000);
}
void auxiliary(M &m, std::uint32_t body, std::uint32_t mode,
               const SourcePhysicsBodyFactoryScratch &s, FP fp, const O &o) {
  if (mode != 2)
    missing("Required unmeasured alternate solver auxiliary owner unavailable");
  const auto p = allocate(m, 0x60, o);
  w(m, o, 0x317c40, p + 4, 2,
    0x60); // exact site checked by original differential
  source_construct_physics_body_auxiliary_1_1(m, p, body, s.auxiliary_bounds, fp, o);
  w(m, o, 0x1dc7ed, p + 6, 2, r(m, p + 6, 2) + 1);
  const auto old = r(m, body + 0x54);
  if (old)
    missing("Required old solver auxiliary lifetime owner unavailable");
  w(m, o, 0x1dc827, body + 0x54, 4, p);
  w(m, o, 0x317c6c, p + 6, 2, r(m, p + 6, 2) - 1);
}
void radius(M &m, std::uint32_t body, std::uint32_t output, FP fp, const O &o) {
  source_physics_shape_bounds_1_1(m, r(m, body + 0xc), 0x469340, 0, output, fp,
                                  o);
  FpScope scope(fp);
  const auto motion = r(m, body + 0x3c);
  const auto lo = v(m, output), hi = v(m, output + 16);
  const auto first = _mm_cvtss_f32(
      _mm_sqrt_ss(_mm_set_ss(length_squared(_mm_sub_ps(hi, lo)))));
  const auto second = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(length_squared(
      _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(0.5f), _mm_add_ps(lo, hi)),
                 v(m, motion + 0x20))))));
  const auto x = add(mul(ext(first), ext(f(r(m, 0x46502c)))), ext(second));
  w(m, o, 0x317de6, motion + 0xc, 4, u(spill(x)));
}
} // namespace
void source_construct_physics_body_auxiliary_1_1(M &m, std::uint32_t p,
                                                 std::uint32_t body,
                                                 std::uint32_t bounds, FP fp,
                                                 const O &o) {
  w(m, o, 0x31acb5, p + 6, 2, 1);
  w(m, o, 0x31acbb, p, 4, 0x41b0fc);
  for (const auto a : std::array<std::array<std::uint32_t, 3>, 4>{
           {{0x31acc1, 0x50, 0x3c23d70a}, {0x31acc8, 0x54, 0x3ba3d70a},
            {0x31accf, 0x58, 0x3dcccccd}, {0x31acd6, 0x5c, 0x3e4ccccd}}})
    w(m, o, a[0], p + a[1], 4, a[2]);
  vector(m, o, 0x31acef, p + 0x10,
         _mm_castsi128_ps(_mm_set1_epi32(0x7f7fffff)));
  vector(m, o, 0x31acf3, p + 0x30,
         _mm_castsi128_ps(_mm_set1_epi32(0x7f7fffff)));
  vector(m, o, 0x31acfa, p + 0x20, _mm_setzero_ps());
  w(m, o, 0x31ad03, p + 0x2c, 4, 0x3f800000);
  vector(m, o, 0x31ad06, p + 0x40, _mm_setzero_ps());
  w(m, o, 0x31ad0a, p + 0x4c, 4, 0x3f800000);
  auxiliary_size(m, p, r(m, body + 0xc), bounds, fp, o);
}
std::uint32_t source_host_x86_rsqrtss_estimate(std::uint32_t word) {
  // A real platform instruction provider, not an emulator result lookup.
  // Caller retains source MXCSR; Xbox estimate parity remains unproved.
  return u(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(f(word)))));
}

std::uint32_t
source_resolve_physics_shape_selector_1_1(const M &m, std::uint32_t p,
                                          std::uint32_t selector) {
  const auto type = std::bit_cast<std::int16_t>(std::uint16_t(selector));
  const auto index = std::uint32_t(
      std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(selector >> 16))));
  switch (type) {
  case 0:
    return r(m, p + 0x4c) + index * 0x80 + 0x30;
  case 1:
    return r(m, p + 0x5c) + index * 0x50 + 0x20;
  case 2:
    return r(m, p + 0x64) + index * 0x90 + 0x40;
  case 3:
    return r(m, p + 0x6c) + index * 0x60 + 0x20;
  case 4:
    return r(m, p + 0x74) + index * 0x100 + 0x20;
  case 5:
    return r(m, p + 0x54) + index * 0xb0 + 0x20;
  case 6:
    return r(m, p + 0x10c) + index * 0x20;
  case 14:
    return r(m, p + 0x94) + index * 0x38;
  case 15:
    return r(m, p + 0xa4) + index * 0x14;
  default:
    return 0;
  }
}
void source_initialize_physics_model_1_1(M &m, std::uint32_t datum,
                                         std::uint8_t clear, const O &o) {
  const auto p = r(m, r(m, 0x4eb114) + (datum & 0xffff) * 16 + 8);
  const auto each = [&](unsigned offset, unsigned stride, const auto &body) {
    for (std::uint32_t i = 0; std::bit_cast<std::int32_t>(i) <
                              std::bit_cast<std::int32_t>(r(m, p + offset));
         ++i)
      body(r(m, p + offset + 4) + i * stride);
  };
  if (!clear)
    each(0x38, 0x90, [&](std::uint32_t row) {
      w(m, o, 0x1ee3cf, row + 0x38, 4,
        source_resolve_physics_shape_selector_1_1(m, p, r(m, row + 0x38)));
    });
  each(0x48, 0x80, [&](std::uint32_t row) {
    const auto shape = row + 0x20, transform = row + 0x30;
    if (shape) {
      w(m, o, clear ? 0x1ee405 : 0x1ee421, shape + 8, 4, clear ? 0 : row);
      w(m, o, clear ? 0x1ee408 : 0x1ee424, shape, 4, 0x45c670);
      w(m, o, clear ? 0x1ee40e : 0x1ee42a, shape + 6, 2, 0x80);
    }
    if (transform) {
      if (!clear)
        w(m, o, 0x1ee437, transform + 12, 4, shape);
      w(m, o, 0x1ee43a, transform + 8, 4, 0);
      w(m, o, 0x1ee43d, transform, 4, 0x45c648);
      w(m, o, 0x1ee443, transform + 6, 2, 0x80);
    }
  });
  const auto primitive = [&](unsigned offset, unsigned stride, std::uint32_t vt,
                             std::array<std::uint32_t, 3> bind,
                             std::array<std::uint32_t, 3> reset) {
    each(offset, stride, [&](std::uint32_t row) {
      const auto shape = row + 0x20;
      if (shape) {
        const auto sites = clear ? reset : bind;
        w(m, o, sites[0], shape + 8, 4, clear ? 0 : row);
        w(m, o, sites[1], shape, 4, vt);
        w(m, o, sites[2], shape + 6, 2, 0x80);
      }
    });
  };
  primitive(0x50, 0xb0, 0x45d208, {0x1ee48a, 0x1ee48d, 0x1ee493},
            {0x1ee472, 0x1ee475, 0x1ee47b});
  primitive(0x58, 0x50, 0x45c6b0, {0x1ee4da, 0x1ee4dd, 0x1ee4e3},
            {0x1ee4c2, 0x1ee4c5, 0x1ee4cb});
  each(0x60, 0x90, [&](std::uint32_t row) {
    const auto shape = row + 0x20, transform = row + 0x40;
    if (shape) {
      if (clear)
        w(m, o, 0x1ee518, shape + 6, 2, 1);
      w(m, o, clear ? 0x1ee51c : 0x1ee532, shape + 8, 4, clear ? 0 : row);
      w(m, o, clear ? 0x1ee51f : 0x1ee535, shape, 4, 0x45d098);
      if (!clear)
        w(m, o, 0x1ee53b, shape + 6, 2, 0x80);
    }
    if (transform) {
      if (!clear)
        w(m, o, 0x1ee548, transform + 12, 4, shape);
      w(m, o, 0x1ee54b, transform + 8, 4, 0);
      w(m, o, 0x1ee54e, transform, 4, 0x45c648);
      w(m, o, 0x1ee554, transform + 6, 2, 0x80);
    }
  });
  primitive(0x68, 0x60, 0x45d1c8, {0x1ee59f, 0x1ee5a2, 0x1ee5a8},
            {0x1ee587, 0x1ee58a, 0x1ee590});
  std::uint32_t vertices = 0, planes = 0;
  each(0x70, 0x100, [&](std::uint32_t row) {
    const auto shape = row + 0x20;
    if (clear) {
      if (shape) {
        w(m, o, 0x1ee5ec, shape + 8, 4, 0);
        w(m, o, 0x1ee5ef, shape, 4, 0x45d188);
        w(m, o, 0x1ee5f5, shape + 6, 2, 0x80);
      }
      return;
    }
    const auto external = (r(m, shape + 0x38) & 0x7fffffff) > 3;
    const auto bank =
        external ? r(m, p + 0x7c) + vertices * 0x30 : shape + 0x40;
    const auto planebank = r(m, p + 0x84) + planes * 16;
    w(m, o, 0x1ee630, shape + 8, 4, row);
    w(m, o, 0x1ee633, shape + 0x30, 4, bank);
    w(m, o, 0x1ee636, shape + 0xd4, 4, planebank);
    w(m, o, 0x1ee63c, shape, 4, 0x45d188);
    w(m, o, 0x1ee642, shape + 6, 2, 0x80);
    if (external)
      vertices += r(m, shape + 0x34);
    planes += r(m, shape + 0xd8);
  });
  std::uint32_t list_offset = 0;
  each(0x90, 0x38, [&](std::uint32_t row) {
    if (clear) {
      if (row) {
        w(m, o, 0x1ee6b9, row + 8, 4, 0);
        w(m, o, 0x1ee6bc, row, 4, 0x45d140);
        w(m, o, 0x1ee6c2, row + 6, 2, 0x80);
      }
      return;
    }
    const auto external = (r(m, row + 0x14) & 0x7fffffff) > 4;
    const auto bank = external ? r(m, p + 0x9c) + list_offset * 8 : row + 0x18;
    w(m, o, 0x1ee701, row + 8, 4, 0);
    w(m, o, 0x1ee704, row + 12, 4, bank);
    w(m, o, 0x1ee709, row, 4, 0x45d140);
    w(m, o, 0x1ee70f, row + 6, 2, 0x80);
    if (r(m, 0x45d168) != 0x302730)
      missing("Required original list-shape count owner unavailable");
    for (std::uint32_t i = 0; std::bit_cast<std::int32_t>(i) <
                              std::bit_cast<std::int32_t>(r(m, row + 16));
         ++i) {
      w(m, o, 0x1ee73a, bank + i * 8, 4,
        source_resolve_physics_shape_selector_1_1(m, p, r(m, bank + i * 8)));
      w(m, o, 0x1ee73d, bank + i * 8 + 4, 4, 0);
      method(m, row, 0x28, 0x302730);
    }
    if (external)
      list_offset += r(m, row + 16);
  });
  std::uint32_t mopp_offset = 0;
  each(0xa0, 0x14, [&](std::uint32_t row) {
    if (clear) {
      if (row) {
        w(m, o, 0x1ee7c0, row + 8, 4, 0);
        w(m, o, 0x1ee7c3, row, 4, 0x45d0d8);
        w(m, o, 0x1ee7c9, row + 6, 2, 0x80);
      }
      return;
    }
    const auto child =
        r(m, p + 0x94) + std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(
                             std::uint16_t(r(m, row + 12) >> 16)))) *
                             0x38;
    const auto code = r(m, p + 0xac) + mopp_offset;
    w(m, o, 0x1ee7ee, row + 16, 4, code);
    w(m, o, 0x1ee7f1, row + 12, 4, child);
    w(m, o, 0x1ee7f4, row + 8, 4, 0);
    w(m, o, 0x1ee7fb, row, 4, 0x45d0d8);
    w(m, o, 0x1ee801, row + 6, 2, 0x80);
    mopp_offset += (r(m, code + 0x20) + 15) & 0xfffffff0u;
  });
  each(0x108, 0x20, [&](std::uint32_t row) {
    const auto shape = row + 0x14;
    if (clear) {
      if (row) {
        w(m, o, 0x1ee85c, row + 8, 4, 0);
        w(m, o, 0x1ee85f, row, 4, 0x45cfc4);
        w(m, o, 0x1ee865, row + 6, 2, 0x80);
      }
      if (shape) {
        w(m, o, 0x1ee86d, shape + 8, 4, 0);
        w(m, o, 0x1ee870, shape + 6, 2, 0x80);
        w(m, o, 0x1ee8ba, shape, 4, 0x45d10c);
      }
      return;
    }
    const auto saved = r(m, shape + 8);
    const auto child =
        source_resolve_physics_shape_selector_1_1(m, p, r(m, row + 12));
    w(m, o, 0x1ee893, row + 12, 4, child);
    w(m, o, 0x1ee89b, row + 6, 2, 0x80);
    w(m, o, 0x1ee89f, row + 8, 4, 0);
    w(m, o, 0x1ee8a6, row + 16, 4, shape);
    w(m, o, 0x1ee8a9, row, 4, 0x45cfc4);
    w(m, o, 0x1ee8af, shape + 8, 4, saved);
    w(m, o, 0x1ee8b2, shape + 6, 2, 0x80);
    w(m, o, 0x1ee8ba, shape, 4, 0x45d10c);
  });
}
void source_initialize_physics_convex_model_1_1(M &m, std::uint32_t datum,
                                                const O &o) {
  source_initialize_physics_model_1_1(m, datum, 0, o);
}

void source_physics_quaternion_matrix_1_1(M &m, std::uint32_t out,
                                          std::uint32_t p, FP fp, const O &o) {
  FpScope scope(fp);
  const auto x = f(r(m, p)), y = f(r(m, p + 4)), z = f(r(m, p + 8)),
             q = f(r(m, p + 12));
  const auto x2 = add(ext(x), ext(x)), y2 = add(ext(y), ext(y));
  const auto z2 = spill(add(ext(z), ext(z)));
  const auto xx = spill(mul(x2, ext(x))), xy = spill(mul(y2, ext(x))),
             xz = spill(mul(ext(z2), ext(x))), yy = spill(mul(y2, ext(y))),
             yz = spill(mul(ext(z2), ext(y))), zz = spill(mul(ext(z2), ext(z))),
             xw = spill(mul(x2, ext(q)));
  const auto yw = mul(y2, ext(q)), zw = mul(ext(z2), ext(q)),
             one = ext(f(r(m, 0x465030)));
  const std::array<float, 12> a = {spill(sub(one, add(ext(zz), ext(yy)))),
                                   spill(add(ext(xy), zw)),
                                   spill(sub(ext(xz), yw)),
                                   0,
                                   spill(sub(ext(xy), zw)),
                                   spill(sub(one, add(ext(zz), ext(xx)))),
                                   spill(add(ext(xw), ext(yz))),
                                   0,
                                   spill(add(yw, ext(xz))),
                                   spill(sub(ext(yz), ext(xw))),
                                   spill(sub(one, add(ext(yy), ext(xx)))),
                                   0};
  const std::array<std::uint32_t, 12> sites = {
      0x378b24, 0x378b2b, 0x378b34, 0x378b37, 0x378b3f, 0x378b52,
      0x378b5d, 0x378b60, 0x378b67, 0x378b72, 0x378b83, 0x378b86};
  for (unsigned n = 0; n < 12; ++n)
    w(m, o, sites[n], out + n * 4, 4, u(a[n]));
}
void source_physics_matrix_quaternion_1_1(M &m, std::uint32_t out,
                                          std::uint32_t p, FP fp, const O &o) {
  FpScope scope(fp);
  const auto at = [&](unsigned n) { return ext(f(r(m, p + n))); };
  const auto trace = add(add(at(0x14), at(0)), at(0x28)),
             one = ext(f(r(m, 0x465030))), half = ext(f(r(m, 0x46502c)));
  if (trace > ext(f(r(m, 0x465048)))) {
    const auto s = root(add(trace, one)), inv = div(half, s);
    w(m, o, 0x2e0897, out, 4, u(spill(mul(sub(at(0x18), at(0x24)), inv))));
    w(m, o, 0x2e08a1, out + 4, 4, u(spill(mul(sub(at(0x20), at(8)), inv))));
    w(m, o, 0x2e08ac, out + 8, 4, u(spill(mul(sub(at(4), at(0x10)), inv))));
    w(m, o, 0x2e08b7, out + 12, 4, u(spill(mul(s, half))));
  } else {
    unsigned i = 0;
    if (at(0x14) > at(0))
      i = 1;
    if (at(0x28) > at(i * 20))
      i = 2;
    const std::array<unsigned, 3> next{1, 2, 0};
    const auto j = next[i], k = next[j];
    const auto sum = add(at(k * 20), at(j * 20));
    const auto s = root(add(sub(at(i * 20), sum), one));
    const auto inv = div(half, s);
    w(m, o, 0x2e093e, out + i * 4, 4, u(spill(mul(s, half))));
    w(m, o, 0x2e094f, out + 12, 4,
      u(spill(mul(sub(at((k + j * 4) * 4), at((j + k * 4) * 4)), inv))));
    w(m, o, 0x2e095a, out + j * 4, 4,
      u(spill(mul(add(at((i + j * 4) * 4), at((j + i * 4) * 4)), inv))));
    w(m, o, 0x2e096b, out + k * 4, 4,
      u(spill(mul(add(at((i + k * 4) * 4), at((k + i * 4) * 4)), inv))));
  }
}
void source_physics_normalize_quaternion_1_1(
    M &m, std::uint32_t p, const SourcePhysicsReciprocalSqrtEstimate &estimate,
    FP fp, const O &o) {
  FpScope scope(fp);
  auto q = v(m, p), square = _mm_mul_ps(q, q);
  auto sum = _mm_add_ps(square, _mm_shuffle_ps(square, square, 0x4e));
  sum = _mm_add_ps(sum, _mm_shuffle_ps(sum, sum, 0xb1));
  if (!estimate)
    missing("Required Xbox RSQRTSS estimate provider unavailable");
  const auto inv = _mm_set_ss(f(estimate(u(_mm_cvtss_f32(sum)))));
  auto t = _mm_mul_ss(sum, inv);
  t = _mm_mul_ss(t, inv);
  const auto difference = _mm_sub_ss(_mm_set_ss(f(r(m, 0x465044))), t);
  auto gain = _mm_mul_ss(_mm_set_ss(f(r(m, 0x46502c))), inv);
  gain = _mm_mul_ss(gain, difference);
  gain = _mm_shuffle_ps(gain, gain, 0);
  vector(m, o, 0x1dc7b5, p, _mm_mul_ps(q, gain));
}
namespace {
// 2E09E0 columns. Temporary matrices are calculations owned by this call;
// the retained output remains the caller's original AABB packet.
struct ShapeMatrix {
  __m128 column[4];
};
ShapeMatrix shape_matrix(const M &m, std::uint32_t p) {
  return {{v(m, p), v(m, p + 16), v(m, p + 32), v(m, p + 48)}};
}
ShapeMatrix compose_shape_matrix(const ShapeMatrix &a, const ShapeMatrix &b) {
  ShapeMatrix out{};
  for (unsigned i = 0; i < 4; ++i) {
    const auto q = b.column[i];
    const auto x = _mm_mul_ps(a.column[0], _mm_shuffle_ps(q, q, 0));
    const auto y = _mm_mul_ps(a.column[1], _mm_shuffle_ps(q, q, 0x55));
    const auto z = _mm_mul_ps(a.column[2], _mm_shuffle_ps(q, q, 0xaa));
    out.column[i] = _mm_add_ps(_mm_add_ps(x, y), z);
  }
  out.column[3] = _mm_add_ps(out.column[3], a.column[3]);
  return out;
}
__m128 transform_shape_point(const ShapeMatrix &a, __m128 q) {
  const auto x = _mm_mul_ps(a.column[0], _mm_shuffle_ps(q, q, 0));
  const auto y = _mm_mul_ps(a.column[1], _mm_shuffle_ps(q, q, 0x55));
  const auto z = _mm_mul_ps(a.column[2], _mm_shuffle_ps(q, q, 0xaa));
  return _mm_add_ps(_mm_add_ps(x, y), _mm_add_ps(z, a.column[3]));
}
void shape_bounds(M &m, std::uint32_t shape, const ShapeMatrix &matrix,
                  std::uint32_t margin, std::uint32_t out, const O &o) {
  const auto target = r(m, r(m, shape) + 0x18);
  if (target == 0x2e2380) {
    shape_bounds(m, r(m, shape + 0x30), matrix, margin, out, o);
    auto expansion = _mm_setzero_ps();
    if (const auto body = r(m, shape + 0x34))
      if (const auto world = r(m, body + 8))
        expansion = _mm_mul_ps(_mm_set1_ps(f(r(m, world + 0x150))),
                               v(m, r(m, body + 0x3c) + 0x40));
    const auto hi = v(m, out + 16), lo = v(m, out);
    vector(m, o, 0x2e23e8, out + 16,
           _mm_max_ps(_mm_add_ps(hi, expansion), hi));
    vector(m, o, 0x2e23f5, out,
           _mm_min_ps(_mm_add_ps(lo, expansion), lo));
    return;
  }
  if (target == 0x2e4dd0) {
    const auto composed =
        compose_shape_matrix(matrix, shape_matrix(m, shape + 0x10));
    shape_bounds(m, r(m, shape + 0xc), composed, margin, out, o);
    return;
  }
  if (target == 0x2e52e0) {
    const auto radi = spill(add(ext(f(margin)), ext(f(r(m, shape + 0xc)))));
    vector(m, o, 0x2e530a, out,
           _mm_sub_ps(matrix.column[3], _mm_set1_ps(radi)));
    vector(m, o, 0x2e5314, out + 16,
           _mm_add_ps(matrix.column[3], _mm_set1_ps(radi)));
    return;
  }
  if (target != 0x303e30 && target != 0x2e4670 && target != 0x3049b0)
    missing(
        "Required original resident shape-bounds virtual owner unavailable");
  const auto radi = spill(add(ext(f(margin)), ext(f(r(m, shape + 0xc)))));
  if (target == 0x2e4670) {
    const auto a = transform_shape_point(matrix, v(m, shape + 0x10));
    const auto b = transform_shape_point(matrix, v(m, shape + 0x20));
    vector(m, o, 0x2e4707, out + 16, _mm_max_ps(a, b));
    vector(m, o, 0x2e470e, out,
           _mm_sub_ps(_mm_min_ps(a, b), _mm_set1_ps(radi)));
    vector(m, o, 0x2e4718, out + 16,
           _mm_add_ps(v(m, out + 16), _mm_set1_ps(radi)));
    return;
  }
  const auto a = v(m, shape + 0x10), mask = v(m, 0x45cac0);
  auto z = _mm_mul_ps(_mm_shuffle_ps(a, a, 0xaa), matrix.column[2]);
  z = _mm_add_ps(_mm_and_ps(z, mask), _mm_set1_ps(radi));
  const auto x =
      _mm_and_ps(_mm_mul_ps(_mm_shuffle_ps(a, a, 0), matrix.column[0]), mask);
  const auto y = _mm_and_ps(
      _mm_mul_ps(_mm_shuffle_ps(a, a, 0x55), matrix.column[1]), mask);
  const auto extent = _mm_add_ps(_mm_add_ps(x, y), z);
  const bool box = target == 0x3049b0;
  vector(m, o, box ? 0x30494e : 0x1d1c66, out + 16, extent);
  vector(m, o, box ? 0x30495c : 0x1d1c74, out,
         _mm_xor_ps(extent, v(m, 0x418ae0)));
  const auto point = box ? matrix.column[3]
                         : transform_shape_point(matrix, v(m, shape + 0x20));
  vector(m, o, box ? 0x30496a : 0x1d1cb7, out + 16,
         _mm_add_ps(v(m, out + 16), point));
  vector(m, o, box ? 0x304978 : 0x1d1cc1, out, _mm_add_ps(v(m, out), point));
}
} // namespace
std::uint32_t source_physics_shape_type_1_1(const M &m, std::uint32_t shape) {
  switch (r(m, r(m, shape) + 0x14)) {
  case 0x9d900:
    return 4;
  case 0x9d8e0:
    return 8;
  case 0x31b250:
    return 7;
  case 0x31b260:
    return 6;
  case 0x2e4d20:
    return 21;
  case 0xa0120:
    return 23;
  default:
    missing("Required original resident shape-class virtual owner unavailable");
  }
}
void source_physics_shape_bounds_1_1(M &m, std::uint32_t shape,
                                     std::uint32_t matrix, std::uint32_t margin,
                                     std::uint32_t out, FP fp, const O &o) {
  FpScope scope(fp);
  shape_bounds(m, shape, shape_matrix(m, matrix), margin, out, o);
}
void source_physics_convex_bounds_1_1(M &m, std::uint32_t shape,
                                      std::uint32_t matrix,
                                      std::uint32_t margin, std::uint32_t out,
                                      FP fp, const O &o) {
  method(m, shape, 0x18, 0x303e30);
  source_physics_shape_bounds_1_1(m, shape, matrix, margin, out, fp, o);
}
std::uint32_t source_construct_physics_motion_1_1(M &m, std::uint32_t p,
                                                  std::uint32_t pos,
                                                  std::uint32_t quat,
                                                  std::uint32_t mode, FP fp,
                                                  const O &o) {
  if (mode != 4 && mode != 5 && mode != 7)
    missing("Required alternate motion constructor unavailable");
  w(m, o, 0x317a58, p + 8, 4, 0);
  w(m, o, 0x317a61, p, 4, 0x41a968);
  vector(m, o, 0x317a67, p + 0x40, _mm_setzero_ps());
  vector(m, o, 0x317a6b, p + 0x50, _mm_setzero_ps());
  w(m, o, 0x317a6f, p + 0xc, 4, 0x3f800000);
  w(m, o, 0x317a76, p + 0x14, 4, 0);
  w(m, o, 0x317a79, p + 0x18, 4, 0);
  vector(m, o, 0x317a7c, p + 0x20, _mm_setzero_ps());
  vector(m, o, 0x317a83, p + 0x60, v(m, pos));
  w(m, o, 0x317a87, p + 0x6c, 4, 0);
  vector(m, o, 0x317a8d, p + 0x70, v(m, pos));
  vector(m, o, 0x317a97, p + 0xb0, v(m, pos));
  vector(m, o, 0x317aac, p + 0x30, v(m, quat));
  source_physics_quaternion_matrix_1_1(m, p + 0x80, quat, fp, o);
  w(m, o, 0x317ab9, p + 0x7c, 4, 0);
  if (mode == 4 || mode == 5) {
    w(m, o, 0x321340, p, 4, 0x41b8f0);
    for (unsigned n = 0; n < 4; ++n)
      w(m, o, 0x321346 + n * 6, p + 0xe0 + n * 4, 4, 0x3f800000);
    vector(m, o, 0x321361, p + 0xf0, _mm_setzero_ps());
    vector(m, o, 0x321368, p + 0x100, _mm_setzero_ps());
    vector(m, o, 0x32136f, p + 0xc0, _mm_setzero_ps());
    vector(m, o, 0x321376, p + 0xd0, _mm_setzero_ps());
    w(m, o, 0x32137d, p + 0x10, 4, 0xbf800000);
    if (mode == 5)
      w(m, o, 0x320e64, p, 4, 0x41b870);
  } else {
    w(m, o, 0x31f9a7, p, 4, 0x41b770);
    w(m, o, 0x31f9ad, p + 0xc0, 4, 0);
    w(m, o, 0x31f9b3, p + 0x10, 4, 0);
    w(m, o, 0x31fd44, p + 0x2c, 4, 0);
    w(m, o, 0x31f9bf, p + 0x7c, 4, 0x3f800000);
    w(m, o, 0x320cf7, p, 4, 0x41b7f0);
    w(m, o, 0x320cfd, p + 0x10, 4, 0);
    w(m, o, 0x31fd44, p + 0x2c, 4, 0);
    w(m, o, 0x320d09, p + 0x7c, 4, 0x3f800000);
  }
  return p;
}
std::uint32_t source_allocate_physics_motion_1_1(
    M &m, const SourcePhysicsMotionConstruction &in, FP fp, const O &o) {
  if (in.mode != 4 && in.mode != 5)
    missing("Required unmeasured motion selection owner unavailable");
  const std::uint32_t n = 0x110u;
  const auto p = allocate(m, n, o);
  w(m, o, in.mode == 5 ? 0x317f80 : 0x317f1c, p + 4, 2, n);
  source_construct_physics_motion_1_1(m, p, in.position, in.quaternion, in.mode,
                                      fp, o);
  if (in.mode == 4 || in.mode == 5) {
    method(m, p, 0x2c, 0x321390);
    inertia(m, p, in.inertia, fp, o);
    method(m, p, 0x3c, 0x31fd50);
    center(m, p, in.center, fp, o);
    method(m, p, 0x1c, 0x321050);
    mass(m, p, in.mass_word, fp, o);
  }
  return p;
}
std::uint32_t source_construct_physics_rigid_body_1_1(
    M &m, std::uint32_t p, std::uint32_t info,
    const SourcePhysicsBodyFactoryScratch &s, FP fp, const O &o) {
  source_construct_physics_entity_1_1(m, p, info, o);
  w(m, o, 0x318128, p, 4, 0x41a950);
  const auto mode = r(m, info + 0xb4, 1);
  if (mode == 7) {
    w(m, o, 0x318145, p + 0x40, 1, 1);
    const auto q = allocate(m, 0xd0, o);
    w(m, o, 0x31815b, q + 4, 2, 0xd0);
    source_construct_physics_motion_1_1(m, q, info + 0x20, info + 0x30, 7, fp,
                                        o);
    w(m, o, 0x318179, p + 0x3c, 4, q);
    w(m, o, 0x31817f, p + 0x14, 4, q + 0x60);
  } else {
    const auto q = source_allocate_physics_motion_1_1(m,
                                                      {info + 0x20, info + 0x30,
                                                       info + 0x60, info + 0x90,
                                                       r(m, info + 0xa0), mode},
                                                      fp, o);
    w(m, o, 0x3181a9, p + 0x3c, 4, q);
    if (r(m, p + 0x44))
      missing("Required positive body wakeup owner unavailable");
    method(m, q, 0x50, 0x31fe90);
    vector(m, o, 0x31fe97, q + 0x40, v(m, info + 0x40));
    if (r(m, p + 0x44))
      missing("Required positive body wakeup owner unavailable");
    method(m, q, 0x54, 0x31fea0);
    vector(m, o, 0x31fea7, q + 0x50, v(m, info + 0x50));
    w(m, o, 0x31820f, q + 0x14, 4, r(m, info + 0xa4));
    w(m, o, 0x31821b, q + 0x18, 4, r(m, info + 0xa8));
    w(m, o, 0x318224, p + 0x14, 4, q + 0x60);
    auxiliary(m, p, r(m, info + 0xb5, 1), s, fp, o);
    if (r(m, p + 0xc))
      radius(m, p, s.radius_bounds, fp, o);
  }
  w(m, o, 0x31824a, p + 0x4c, 4, r(m, info + 0xac));
  w(m, o, 0x318257, p + 0x50, 4, r(m, info + 0xb0));
  return p;
}
std::uint32_t source_create_physics_rigid_body_1_1(
    M &m, std::uint32_t component, const std::array<std::uint32_t, 14> &a,
    const SourcePhysicsBodyFactoryScratch &s, FP fp,
    const SourcePhysicsReciprocalSqrtEstimate &estimate, const O &o) {
  source_initialize_physics_body_info_1_1(m, s.info, o);
  source_physics_matrix_quaternion_1_1(m, s.quaternion, a[9], fp, o);
  const auto idx = r(m, component + 8) & 0xffffu, pool = r(m, 0x4e78d0),
             body = r(m, r(m, pool + 0x44) + idx * 12 + 8);
  const auto mode = (r(m, body + 0xc0, 1) & 0x20) ? 7 : a[8];
  const auto identity = a[10] == 10 ? 0x802 : idx + 1;
  source_physics_normalize_quaternion_1_1(m, s.quaternion, estimate, fp, o);
  vector(m, o, 0x1dab44, s.info + 0x90, v(m, a[3]));
  w(m, o, 0x1dab51, s.info + 0xac, 4, a[4]);
  w(m, o, 0x1dab5f, s.info + 0xb0, 4, a[5]);
  vector(m, o, 0x1dab6b, s.info + 0x60, v(m, a[6]));
  vector(m, o, 0x1dab77, s.info + 0x70, v(m, a[6] + 16));
  vector(m, o, 0x1dab86, s.info + 0x80, v(m, a[6] + 32));
  w(m, o, 0x1dab93, s.info + 0xa0, 4, a[7]);
  vector(m, o, 0x1daba0, s.info + 0x20, v(m, a[9] + 0x30));
  w(m, o, 0x1dabac, s.info + 4, 4, a[0]);
  w(m, o, 0x1dabb0, s.info + 0xb4, 1, mode);
  vector(m, o, 0x1dabb7, s.info + 0x30, v(m, s.quaternion));
  w(m, o, 0x1dabd5, s.info + 0xa4, 4, a[12]);
  w(m, o, 0x1dabe3, s.info, 4,
    (identity << 16) | (std::uint8_t(a[11]) ? 2 : a[10]));
  w(m, o, 0x1dabe7, s.info + 0xa8, 4, a[13]);
  if (!std::uint8_t(a[2]) || std::uint8_t(a[1])) {
    const auto shape_type = source_physics_shape_type_1_1(m, a[0]);
    if (!std::uint8_t(a[2]) && shape_type == 19)
      w(m, o, 0x1dac03, s.info + 4, 4, r(m, a[0] + 0xc));
    if (std::uint8_t(a[1]) &&
        source_physics_shape_type_1_1(m, r(m, s.info + 4)) == 21)
      missing(
          "Required original transformed single-body splice owner unavailable");
  }
  const auto p = allocate(m, 0xa0, o);
  w(m, o, 0x1dacac, p + 4, 2, 0xa0);
  source_construct_physics_rigid_body_1_1(m, p, s.info, s, fp, o);
  if (std::bit_cast<std::int32_t>(r(m, s.info + 0x14)) >= 0)
    missing("Required temporary property bank cleanup owner unavailable");
  return p;
}
} // namespace halo2
