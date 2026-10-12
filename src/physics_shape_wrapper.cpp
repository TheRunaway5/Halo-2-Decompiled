#include "halo2/physics_shape_wrapper.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>
namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
[[noreturn]] void missing(const char *s) { throw std::runtime_error(s); }
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (i * 8);
  return v;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint64_t v) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(v >> (8 * i));
  m.write(p, s.written);
  if (o)
    o(s);
}
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t u(float v) { return std::bit_cast<std::uint32_t>(v); }
__m128 scalar(std::uint32_t v) { return _mm_set_ss(f(v)); }
__m128 vector(const M &m, std::uint32_t p) {
  std::array<std::uint32_t, 4> a{};
  for (unsigned n = 0; n < 4; ++n)
    a[n] = r(m, p + n * 4);
  return _mm_castsi128_ps(
      _mm_loadu_si128(reinterpret_cast<const __m128i *>(a.data())));
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP p) : mx(_mm_getcsr()) {
    if ((p.x87_control_word & 0x3f) != 0x3f || (p.mxcsr & 0x1f80) != 0x1f80)
      missing("Required unmasked source floating-point exception owner "
              "unavailable");
    std::fegetenv(&env);
    _mm_setcsr(p.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(p.x87_control_word) : "memory");
#else
    missing("Required original x87 wrapper arithmetic backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t squared(std::uint32_t a) {
  float out;
  const float v = f(a);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfld %%st(0)\n\tfmul %%st(1), %%st(0)\n\tfstps "
               "%0\n\tfstp %%st(0)"
               : "=m"(out)
               : "m"(v)
               : "memory");
#else
  missing("Required original x87 wrapper square unavailable");
#endif
  return u(out);
}
std::uint32_t difference_squared(std::uint32_t a, std::uint32_t b) {
  float out;
  const float x = f(a), y = f(b);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfsubs %2\n\tfld %%st(0)\n\tfmul %%st(1), "
               "%%st(0)\n\tfstps %0\n\tfstp %%st(0)"
               : "=m"(out)
               : "m"(x), "m"(y)
               : "memory");
#else
  missing("Required original x87 wrapper difference square unavailable");
#endif
  return u(out);
}
void method(const M &m, std::uint32_t p, unsigned slot, std::uint32_t target) {
  if (r(m, r(m, p) + slot) != target)
    missing("Required original wrapper virtual owner unavailable");
}
std::uint32_t allocate(M &m, const O &o, std::uint32_t size = 0x40) {
  method(m, r(m, 0x487638), 0x10, 0x148a10);
  if (r(m, 0x480d98) != 2)
    missing("Required original wrapper allocator mode unavailable");
  const auto allocator = r(m, 0x480da0);
  method(m, allocator, 0x10, 0x22e950);
  const auto p =
      source_allocate_physics_body_small_block_1_1(m, allocator, size, o);
  if (!p)
    missing("Required original alternate wrapper allocator unavailable");
  return p;
}
std::uint32_t kind(const M &m, std::uint32_t body) {
  const auto target = r(m, r(m, r(m, body + 0x3c)) + 0x18);
  if (target == 0x31b250)
    return 7;
  if (target == 0x9d900)
    return 4;
  if (target == 0x9b560)
    return 5;
  if (target == 0x31b260)
    return 6;
  missing("Required original virtual motion-kind owner unavailable");
}
__m128 length(__m128 v) {
  v = _mm_mul_ps(v, v);
  const auto y = _mm_add_ss(_mm_shuffle_ps(v, v, 0x55), v);
  return _mm_sqrt_ss(_mm_add_ss(_mm_shuffle_ps(v, v, 0xaa), y));
}
struct ShapeMatrix {
  __m128 column[4];
};
ShapeMatrix matrix(const M &m, std::uint32_t p) {
  return {
      {vector(m, p), vector(m, p + 16), vector(m, p + 32), vector(m, p + 48)}};
}
ShapeMatrix compose(const ShapeMatrix &a, const ShapeMatrix &b) {
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
std::uint32_t extents(M &m, std::uint32_t shape, const ShapeMatrix &a,
                      const SourcePhysicsShapeWrapperScratch &s, const O &o) {
  const auto type = source_physics_shape_type_1_1(m, shape);
  if (type == 21)
    return extents(m, r(m, shape + 0xc), compose(a, matrix(m, shape + 0x10)), s,
                   o);
  // Original184AB5/184AC0: the convex wrapper's class methodA0120 returns
  // 23. Its underlying shape is at+30; it contributes no extra transform.
  if (type == 23)
    return extents(m, r(m, shape + 0x30), a, s, o);
  if (type != 4 && type != 6 && type != 7 && type != 8)
    missing("Required original shape extent-dispatch owner unavailable");
  const auto branch = r(m, 0x1850c4 + type - 4, 1);
  if (r(m, 0x1850a8 + branch * 4) != (type == 4   ? 0x184b09u
                                      : type == 6 ? 0x184cb0u
                                      : type == 7 ? 0x184d87u
                                                  : 0x184e6bu))
    missing("Required original shape extent-dispatch table owner unavailable");
  __m128 span, result;
  if (type == 4) {
    const auto radius = scalar(r(m, shape + 0xc));
    result = _mm_add_ss(radius, length(a.column[3]));
    span = _mm_mul_ss(radius, scalar(r(m, 0x46507c)));
    const auto minimum = scalar(r(m, s.extent_minimum));
    w(m, o, 0x184b6f, s.extent_minimum, 4,
      _mm_comigt_ss(minimum, radius) ? r(m, shape + 0xc)
                                   : r(m, s.extent_minimum));
    const auto maximum = scalar(r(m, s.extent_maximum));
    w(m, o, 0x184b8f, s.extent_maximum, 4,
      _mm_comigt_ss(maximum, span) ? r(m, s.extent_maximum)
                                 : u(_mm_cvtss_f32(span)));
    return u(_mm_cvtss_f32(result));
  }
  if (type == 7) {
    const auto radial =
        length(_mm_sub_ps(vector(m, shape + 0x10), vector(m, shape + 0x20)));
    result = _mm_add_ss(_mm_mul_ss(radial, scalar(r(m, 0x46502c))),
                        length(a.column[3]));
    result = _mm_add_ss(result, scalar(r(m, shape + 0xc)));
    span = _mm_mul_ss(scalar(r(m, shape + 0xc)), scalar(r(m, 0x46507c)));
  } else {
    const auto y = scalar(r(m, shape + 0x14)), z = scalar(r(m, shape + 0x18));
    const auto yz = _mm_comigt_ss(y, z) ? z : y;
    const auto x = scalar(r(m, shape + 0x10));
    span = _mm_mul_ss(_mm_comigt_ss(x, yz) ? yz : x, scalar(r(m, 0x46507c)));
    const auto translated =
        length(type == 6 ? a.column[3]
                         : _mm_add_ps(vector(m, shape + 0x20), a.column[3]));
    result = _mm_add_ss(translated, length(vector(m, shape + 0x10)));
  }
  const auto old_min = r(m, s.extent_minimum), old_max = r(m, s.extent_maximum);
  const auto min_site = type == 6   ? 0x184c86u
                        : type == 7 ? 0x184e45u
                                    : 0x184f5du;
  const auto max_greater = _mm_comigt_ss(scalar(old_max), span);
  const auto max_site = type == 6   ? (max_greater ? 0x184c9du : 0x184b8fu)
                        : type == 7 ? 0x184e58u
                                    : 0x184f70u;
  w(m, o, min_site, s.extent_minimum, 4,
    _mm_comigt_ss(scalar(old_min), span) ? u(_mm_cvtss_f32(span)) : old_min);
  w(m, o, max_site, s.extent_maximum, 4,
    max_greater ? old_max : u(_mm_cvtss_f32(span)));
  return u(_mm_cvtss_f32(result));
}
void write_vector(M &m, const O &o, std::uint32_t site, std::uint32_t p,
                  __m128 v) {
  std::array<std::uint64_t, 2> raw{};
  _mm_storeu_ps(reinterpret_cast<float *>(raw.data()), v);
  w(m, o, site, p, 8, raw[0]);
  w(m, o, site, p + 8, 8, raw[1]);
}
// Complete2E4E10 constructor. Allocation and transform copy remain the
// separate1DADF0 owner, including its actual child reference transfer.
void construct_transform(M &m, std::uint32_t p, std::uint32_t child,
                         const O &o) {
  w(m, o, 0x2e4e18, p + 0xc, 4, child);
  w(m, o, 0x2e4e1b, p + 6, 2, 1);
  w(m, o, 0x2e4e21, p + 8, 4, 0);
  w(m, o, 0x2e4e28, p, 4, 0x45c5a0);
  write_vector(m, o, 0x2e4e31, p + 16, _mm_setzero_ps());
  write_vector(m, o, 0x2e4e35, p + 32, _mm_setzero_ps());
  write_vector(m, o, 0x2e4e39, p + 48, _mm_setzero_ps());
  w(m, o, 0x2e4e42, p + 0x10, 4, 0x3f800000);
  w(m, o, 0x2e4e45, p + 0x24, 4, 0x3f800000);
  w(m, o, 0x2e4e48, p + 0x38, 4, 0x3f800000);
  write_vector(m, o, 0x2e4e4b, p + 64, _mm_setzero_ps());
  w(m, o, 0x2e4e55, child + 6, 2, r(m, child + 6, 2) + 1u);
}
} // namespace
std::uint32_t
source_physics_convex_extents_1_1(M &m, std::uint32_t shape,
                                  const SourcePhysicsShapeWrapperScratch &s,
                                  FP fp, const O &o) {
  Scope scope(fp);
  const auto basis = r(m, 0x46fc70), p = s.transform;
  // Original184510 matrix packet writes in source order. The temporary
  // translation vector is copied to+30 by the original MOVAPS at1845C2.
  for (const auto a :
       std::array<std::array<std::uint32_t, 3>, 9>{{{0x184528, 0x10, 0x10},
                                                    {0x184533, 0x14, 0x14},
                                                    {0x18453e, 0x18, 0x18},
                                                    {0x184549, 0x20, 0x1c},
                                                    {0x184554, 0, 4},
                                                    {0x18455f, 0x24, 0x20},
                                                    {0x18456a, 4, 8},
                                                    {0x184575, 0x28, 0x24},
                                                    {0x18458a, 8, 0xc}}})
    w(m, o, a[0], p + a[1], 4, r(m, basis + a[2]));
  w(m, o, 0x18459f, p + 0xc, 4, 0);
  w(m, o, 0x1845a5, p + 0x1c, 4, 0);
  w(m, o, 0x1845ab, p + 0x2c, 4, 0);
  // Unicorn exposes the one original16-byte MOVAPS as two ordered8-byte
  // writes, matching the existing contact/factory observer convention.
  w(m, o, 0x1845c2, p + 0x30, 8,
    std::uint64_t(r(m, basis + 0x28)) |
        (std::uint64_t(r(m, basis + 0x2c)) << 32));
  w(m, o, 0x1845c2, p + 0x38, 8, r(m, basis + 0x30));
  w(m, o, 0x1845d0, s.extent_minimum, 4, r(m, 0x452044));
  w(m, o, 0x1845e1, s.extent_maximum, 4, r(m, 0x447020));
  return extents(m, shape, matrix(m, p), s, o);
}
std::uint32_t source_construct_physics_shape_wrapper_1_1(
    M &m, std::uint32_t shape, std::uint32_t body, std::uint32_t scalar_word,
    const SourcePhysicsShapeWrapperScratch &s, FP fp, const O &o) {
  Scope scope(fp);
  const auto p = allocate(m, o);
  w(m, o, 0x1dada0, p + 4, 2, 0x40);
  w(m, o, 0x2e2541, p + 0xc, 4, r(m, 0x46938c));
  w(m, o, 0x2e2547, p + 6, 2, 1);
  w(m, o, 0x2e2550, p + 8, 4, 0);
  w(m, o, 0x2e2557, p, 4, 0x418930);
  w(m, o, 0x2e255d, p + 0x30, 4, shape);
  w(m, o, 0x2e2563, p + 0x10, 8, 0);
  w(m, o, 0x2e2563, p + 0x18, 8, 0);
  w(m, o, 0x2e256a, shape + 6, 2, r(m, shape + 6, 2) + 1u);
  w(m, o, 0x2e256e, p + 0x20, 4, squared(0x41200000));
  w(m, o, 0x2e2574, p + 0x34, 4, body);
  w(m, o, 0x2e2580, p + 0x24, 4, 0x41200000);
  w(m, o, 0x2e2583, p + 0x28, 4, 0x40a00000);
  w(m, o, 0x2e258e, p + 0x2c, 4,
    difference_squared(0x41200000, r(m, 0x465060)));
  w(m, o, 0x2e2594, p + 0xc, 4, r(m, shape + 0xc));
  const auto config = s.configuration;
  w(m, o, 0x2e5762, config, 4, 0);
  w(m, o, 0x2e572d, config + 4, 4, r(m, 0x46938c));
  w(m, o, 0x2e25a8, config + 8, 4, 0);
  w(m, o, 0x2e25af, config + 0xc, 4, 0x41a00000);
  w(m, o, 0x2e25b6, config + 0x10, 4, 0x40a00000);
  w(m, o, 0x1dadc0, config + 0xc, 4, scalar_word);
  w(m, o, 0x1dadd1, config + 8, 4, shape);
  const auto part =
      u(_mm_cvtss_f32(_mm_mul_ss(scalar(scalar_word), scalar(r(m, 0x465090)))));
  w(m, o, 0x1dadd5, config + 0x10, 4, part);
  w(m, o, 0x2e5786, p + 8, 4, r(m, config));
  w(m, o, 0x2e2271, p + 0x24, 4, r(m, config + 0xc));
  w(m, o, 0x2e227b, p + 0x20, 4, squared(r(m, config + 0xc)));
  w(m, o, 0x2e2284, p + 0x28, 4, r(m, config + 0x10));
  w(m, o, 0x2e228e, p + 0x2c, 4,
    difference_squared(r(m, p + 0x24), r(m, config + 0x10)));
  return p;
}
std::uint32_t source_prepare_physics_shape_wrapper_1_1(
    M &m, std::uint32_t shape, std::uint32_t body, std::uint32_t caller,
    const SourcePhysicsShapeWrapperScratch &s, FP fp, const O &o) {
  Scope scope(fp);
  source_physics_convex_extents_1_1(m, shape, s, fp, o);
  auto value = _mm_mul_ss(scalar(r(m, s.extent_maximum)), scalar(caller));
  value = _mm_mul_ss(value, scalar(r(m, 0x46502c)));
  value = _mm_add_ss(value, scalar(r(m, 0x486564)));
  const auto ticks = std::int32_t(
      std::bit_cast<std::int16_t>(std::uint16_t(r(m, r(m, 0x518444) + 2, 2))));
  value = _mm_mul_ss(value, _mm_cvtsi32_ss(_mm_setzero_ps(), ticks));
  const auto type = source_physics_shape_type_1_1(m, shape);
  if (type == 21) {
    const auto child = source_construct_physics_shape_wrapper_1_1(
        m, r(m, shape + 0xc), body, u(_mm_cvtss_f32(value)), s, fp, o);
    const auto wrapped = allocate(m, o, 0x50);
    construct_transform(m, wrapped, child, o);
    for (unsigned i = 0; i < 4; ++i)
      write_vector(m, o, 0x1db14b + i * 8, wrapped + 0x10 + i * 16,
                   vector(m, shape + 0x10 + i * 16));
    w(m, o, 0x1db167, child + 6, 2, r(m, child + 6, 2) - 1u);
    if (r(m, child + 6, 2) == 0)
      missing("Required original27F9F0 transformed wrapper child lifetime "
              "owner unavailable");
    return wrapped;
  }
  return source_construct_physics_shape_wrapper_1_1(
      m, shape, body, u(_mm_cvtss_f32(value)), s, fp, o);
}
std::uint32_t source_commit_physics_shape_wrapper_1_1(
    M &m, std::uint32_t component, std::uint32_t body, std::uint32_t caller,
    const SourcePhysicsShapeWrapperScratch &s, FP fp, const O &o) {
  if (r(m, 0x486577, 1) == 0 || kind(m, body) == 7)
    return body;
  if (kind(m, body) == 6)
    return body;
  const auto shape = r(m, body + 0xc);
  const auto wrapped = source_prepare_physics_shape_wrapper_1_1(
      m, shape, body, caller, s, fp, o);
  w(m, o, 0x1dad2b, shape + 6, 2, r(m, shape + 6, 2) - 1u);
  if (r(m, shape + 6, 2) == 0)
    missing(
        "Required original27F9F0 deferred-delete/destructor owner unavailable");
  w(m, o, 0x1dad67, body + 0xc, 4, wrapped);
  w(m, o, 0x1dad71, component + 4, 4, r(m, component + 4) | 0x10u);
  return body;
}
} // namespace halo2
