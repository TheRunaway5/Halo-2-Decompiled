#include "halo2/biped_ragdoll_admission.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::int32_t sx(const M &m, std::uint32_t p, unsigned n) {
  const auto v = r(m, p, n);
  return n == 1 ? std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))
                : std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v)));
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t v) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(4);
  s.written.resize(4);
  m.read(p, s.before);
  for (unsigned i = 0; i < 4; ++i)
    s.written[i] = std::byte(v >> (8 * i));
  m.write(p, s.written);
  if (o)
    o(s);
}
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t u(float v) { return std::bit_cast<std::uint32_t>(v); }
__m128 scalar(std::uint32_t v) { return _mm_set_ss(f(v)); }
std::uint32_t word(__m128 v) { return u(_mm_cvtss_f32(v)); }
__m128 load(const M &m, std::uint32_t p) { return scalar(r(m, p)); }
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(SourcePhysicsFactoryFloatingPoint fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original corpse impulse FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original corpse impulse x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 0xffffu) * 16 + 8);
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12 + 8);
}
std::uint32_t component(const M &m, std::uint32_t p) {
  return r(m, r(m, 0x526248) + 0x44) + (r(m, p + 0xb4) & 0xffffu) * 160;
}
std::uint32_t motion(const M &m, std::uint32_t c, std::uint32_t slot) {
  return r(m, r(m, r(m, c + 0x70) + slot * 0x60 + 0x40) + 0x3c);
}
__m128 mass(const M &m, std::uint32_t c, std::uint32_t slot) {
  // Complete1D45A0: zero inverse mass returns one. Reciprocal mass is
  // clamped from below to one, with the original unordered comparison.
  const auto inverse = load(m, motion(m, c, slot) + 0x2c);
  const auto one = load(m, 0x465030);
  if (_mm_cvtss_f32(inverse) == 0.f)
    return one;
  const auto value = _mm_div_ss(one, inverse);
  return _mm_cvtss_f32(one) > _mm_cvtss_f32(value) ? one : value;
}
std::uint32_t random_range(M &m, std::uint32_t seed, std::uint32_t low,
                           std::uint32_t high, const O &o) {
  const auto next = r(m, seed) * 0x19660du + 0x3c6ef35fu;
  w(m, o, 0x259be, seed, next);
  const auto integer = std::int32_t(next >> 16);
  const auto scale = f(r(m, 0x4650c0)), lo = f(low), hi = f(high);
  float result{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fildl %1\n\tfmuls %2\n\tflds %4\n\tfsubs %3\n\t"
               "fmulp %%st, %%st(1)\n\tfadds %3\n\tfstps %0"
               : "=m"(result)
               : "m"(integer), "m"(scale), "m"(lo), "m"(hi)
               : "st", "memory");
#endif
  return u(result);
}
__m128 normalize(M &m, std::uint32_t p, const O &o) {
  // Literal30BD0 x87 products/sum/FSQRT. Its epsilon test uses the
  // unrounded x87 length, while the reciprocal uses the stored float.
  const float x = f(r(m, p)), y = f(r(m, p + 4)), z = f(r(m, p + 8));
  const float epsilon = f(r(m, 0x46504c));
  float length{};
  unsigned char below{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %4\n\tflds %3\n\tflds %2\n\tfld %%st(0)\n\t"
               "fmul %%st(1)\n\tfld %%st(2)\n\tfmul %%st(3)\n\t"
               "faddp %%st, %%st(1)\n\tfld %%st(3)\n\tfmul %%st(4)\n\t"
               "faddp %%st, %%st(1)\n\tfsqrt\n\tfstp %%st(3)\n\t"
               "fstp %%st(0)\n\tfstp %%st(0)\n\tfsts %0\n\tfabs\n\t"
               "flds %5\n\tfcomip %%st(1), %%st\n\tseta %1\n\tfstp %%st(0)"
               : "=m"(length), "=qm"(below)
               : "m"(x), "m"(y), "m"(z), "m"(epsilon)
               : "st", "cc", "memory");
#endif
  if (below)
    return _mm_setzero_ps();
  const auto inverse = _mm_div_ss(load(m, 0x465030), scalar(u(length)));
  w(m, o, 0x30c1b, p, word(_mm_mul_ss(scalar(u(x)), inverse)));
  w(m, o, 0x30c2c, p + 4, word(_mm_mul_ss(inverse, scalar(u(y)))));
  w(m, o, 0x30c31, p + 8, word(_mm_mul_ss(inverse, scalar(u(z)))));
  return scalar(u(length));
}
__m128 projection(const M &m, std::uint32_t c, std::uint32_t slot,
                  std::uint32_t frame) {
  const auto p = motion(m, c, slot) + 0x70;
  const auto x = _mm_mul_ss(_mm_sub_ss(load(m, p), load(m, frame + 0x2c)),
                            load(m, frame + 0x44));
  const auto z = _mm_mul_ss(_mm_sub_ss(load(m, p + 8), load(m, frame + 0x34)),
                            load(m, frame + 0x4c));
  const auto y = _mm_mul_ss(_mm_sub_ss(load(m, p + 4), load(m, frame + 0x30)),
                            load(m, frame + 0x48));
  return _mm_add_ss(_mm_add_ss(x, z), y);
}
} // namespace

std::int32_t source_biped_contact_body_slot_1_1(const M &m, std::uint32_t d,
                                                std::int32_t node) {
  // Complete185700. The first authored valid node is accepted even when
  // greater than the requested node; subsequent replacements obey <=node.
  if (node == -1)
    return -1;
  const auto p = object(m, d), c = component(m, p);
  const auto phmo = tag(m, r(m, tag(m, r(m, tag(m, r(m, p)) + 0x38)) + 0x24));
  const auto rigid = r(m, phmo + 0x3c);
  std::int32_t best = -1, result = -1;
  for (std::int32_t slot = 0; slot < si(r(m, c + 0x74)); ++slot) {
    const auto b = r(m, c + 0x70) + std::uint32_t(slot) * 0x60;
    for (std::int32_t i = 0; i < si(r(m, b + 0x4c)); ++i) {
      const auto id = sx(m, r(m, b + 0x48) + std::uint32_t(i), 1);
      const auto current = sx(m, rigid + std::uint32_t(id) * 0x90, 2);
      if (current != -1 &&
          (best == -1 || (current <= node && best < current))) {
        best = current;
        result = slot;
      }
    }
  }
  return result;
}

bool source_biped_contact_velocity_1_1(
    M &m, std::uint32_t d, std::int32_t node, std::uint32_t frame,
    SourcePhysicsFactoryFloatingPoint fp,
    const SourceBipedBodyVelocityDelta &delta, const O &o) {
  Scope scope(fp);
  const auto selected = source_biped_contact_body_slot_1_1(m, d, node);
  w(m, o, 0xe27f2, frame + 0x24, std::uint32_t(selected));
  if (selected == -1)
    return false;
  const auto p = object(m, d), c = component(m, p), def = tag(m, r(m, p));
  w(m, o, 0xe281b, frame + 0x1c, def);
  const auto seed = r(m, 0x4ee9d8);
  w(m, o, 0xe282b, frame + 0x20,
    random_range(m, seed, 0x3fc00000, 0x40500000, o));
  w(m, o, 0xe2840, frame + 0x28,
    random_range(m, seed, 0x3f400000, 0x3fc00000, o));
  const auto zero = r(m, 0x46fc28);
  auto x = load(m, zero), y = load(m, zero + 4), z = load(m, zero + 8);
  w(m, o, 0xe2855, frame + 0x2c, word(x));
  w(m, o, 0xe2869, frame + 0x30, word(y));
  w(m, o, 0xe2876, frame + 0x34, word(z));
  auto weight = _mm_setzero_ps();
  w(m, o, 0xe289d, frame + 0x14, word(weight));
  const auto selected_com = motion(m, c, std::uint32_t(selected)) + 0x70;
  constexpr std::array copy_sites{0xe28a3u, 0xe28a9u, 0xe28afu};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, copy_sites[i], frame + 0x44 + i * 4, r(m, selected_com + i * 4));
  const auto count = si(r(m, c + 0x74));
  auto body = r(m, c + 0x70) + 0x40;
  if (count > 0)
    w(m, o, 0xe28ba, frame + 0x18, body);
  for (std::int32_t slot = 0; slot < count; ++slot) {
    const auto value =
        _mm_cvtsi32_ss(_mm_setzero_ps(), _mm_cvttss_si32(mass(m, c, slot)));
    const auto com = r(m, r(m, body) + 0x3c) + 0x70;
    x = _mm_add_ss(_mm_mul_ss(load(m, com), value), x);
    y = _mm_add_ss(_mm_mul_ss(load(m, com + 4), value), y);
    z = _mm_add_ss(_mm_mul_ss(load(m, com + 8), value), z);
    weight = _mm_add_ss(value, weight);
    w(m, o, 0xe2918, frame + 0x14, word(weight));
    body += 0x60;
    w(m, o, 0xe291e, frame + 0x18, body);
  }
  const auto inverse = _mm_div_ss(load(m, 0x465030), weight);
  x = _mm_mul_ss(x, inverse);
  y = _mm_mul_ss(y, inverse);
  z = _mm_mul_ss(z, inverse);
  const auto ax = _mm_sub_ss(load(m, frame + 0x44), x);
  const auto ay = _mm_sub_ss(load(m, frame + 0x48), y);
  const auto az = _mm_sub_ss(load(m, frame + 0x4c), z);
  w(m, o, 0xe295e, frame + 0x2c, word(x));
  w(m, o, 0xe2964, frame + 0x30, word(y));
  w(m, o, 0xe296a, frame + 0x34, word(z));
  w(m, o, 0xe2970, frame + 0x44, word(ax));
  w(m, o, 0xe2976, frame + 0x48, word(ay));
  w(m, o, 0xe297c, frame + 0x4c, word(az));
  if (!(_mm_cvtss_f32(normalize(m, frame + 0x44, o)) > f(r(m, 0x4650e0))))
    return false;
  const auto nx = load(m, frame + 0x38), ny = load(m, frame + 0x3c),
             nz = load(m, frame + 0x40);
  const auto bx = load(m, frame + 0x44), by = load(m, frame + 0x48),
             bz = load(m, frame + 0x4c);
  const auto dot = _mm_add_ss(
      _mm_add_ss(_mm_mul_ss(bx, nx), _mm_mul_ss(nz, bz)), _mm_mul_ss(ny, by));
  w(m, o, 0xe29f1, frame + 0x38, word(_mm_sub_ss(nx, _mm_mul_ss(bx, dot))));
  w(m, o, 0xe29f7, frame + 0x3c, word(_mm_sub_ss(ny, _mm_mul_ss(by, dot))));
  w(m, o, 0xe29fd, frame + 0x40, word(_mm_sub_ss(nz, _mm_mul_ss(bz, dot))));
  if (_mm_cvtss_f32(normalize(m, frame + 0x38, o)) == 0.f)
    return false;
  const auto authored = load(m, def + 0x258), cap = load(m, 0x46518c);
  const auto speed =
      _mm_mul_ss(_mm_cvtss_f32(authored) > _mm_cvtss_f32(cap) ? cap : authored,
                 load(m, frame + 0x20));
  constexpr std::array scale_sites{0xe2a59u, 0xe2a5fu, 0xe2a65u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, scale_sites[i], frame + 0x38 + i * 4,
      word(_mm_mul_ss(speed, load(m, frame + 0x38 + i * 4))));
  auto maximum = _mm_setzero_ps();
  for (std::int32_t slot = 0; slot < count; ++slot) {
    const auto v = projection(m, c, slot, frame);
    if (!(_mm_cvtss_f32(maximum) > _mm_cvtss_f32(v)))
      maximum = v;
  }
  w(m, o, 0xe2ad5, frame + 0x14, 0);
  if (count <= 0)
    return false;
  w(m, o, 0xe2aeb, frame + 0x20, word(_mm_div_ss(load(m, 0x465030), maximum)));
  for (std::int32_t slot = 0; slot < si(r(m, c + 0x74)); ++slot) {
    const auto v = projection(m, c, slot, frame);
    const auto sign = load(m, _mm_cvtss_f32(v) < 0.f ? 0x465098 : 0x465030);
    auto scale = _mm_mul_ss(_mm_mul_ss(sign, load(m, frame + 0x20)), v);
    if (slot == selected)
      scale = _mm_mul_ss(scale, load(m, 0x46507c));
    w(m, o, 0xe2ba4, frame + 0x50,
      word(_mm_mul_ss(load(m, frame + 0x38), scale)));
    w(m, o, 0xe2baa, frame + 0x54,
      word(_mm_mul_ss(load(m, frame + 0x3c), scale)));
    w(m, o, 0xe2bb0, frame + 0x58,
      word(_mm_add_ss(_mm_mul_ss(load(m, frame + 0x40), scale),
                      load(m, frame + 0x28))));
    if (!delta)
      throw std::runtime_error(
          "Required original1D4A00 corpse velocity owner unavailable");
    delta(c, slot, frame + 0x50);
    w(m, o, 0xe2bc8, frame + 0x14, std::uint32_t(slot + 1));
  }
  return true;
}
} // namespace halo2
