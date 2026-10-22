#include "halo2/biped_prephysics.hpp"
#include "halo2/biped_phase_runtime.hpp"
#include "halo2/unit_animation_runtime.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
using FP = SourcePhysicsFactoryFloatingPoint;
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
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint32_t v) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(v >> (i * 8));
  m.write(p, s.written);
  if (o)
    o(s);
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 65535u) * 12 + 8);
}
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 65535u) * 16 + 8);
}
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t u(float v) { return std::bit_cast<std::uint32_t>(v); }
__m128 load(const M &m, std::uint32_t p) { return _mm_set_ss(f(r(m, p))); }
std::uint32_t word(__m128 v) { return u(_mm_cvtss_f32(v)); }
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original biped pre-physics FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original biped pre-physics x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::int32_t rounded(__m128 value) {
  const float input = _mm_cvtss_f32(value);
  std::int32_t result{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfistpl %0"
               : "=m"(result)
               : "m"(input)
               : "st", "memory");
#endif
  return result;
}
std::uint32_t call(M &m, const SourceBipedPrePhysicsServices &s,
                   SourceBipedPhaseCall c, const O &o) {
  const auto *runtime =
      c.entry == 0xddbc0 && s.physical_input
          ? &*s.physical_input
          : ((c.entry == 0xe1800 || c.entry == 0xe4f40 || c.entry == 0xdf7c0 ||
              c.entry == 0xbc910 || c.entry == 0xe63c0) &&
                     s.phase_runtime
                 ? &*s.phase_runtime
                 : nullptr);
  if (runtime) {
    FP fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error(
        "Required original biped input FP backend unavailable");
#endif
    return source_call_biped_runtime_1_1(m, c, fp, *runtime, o);
  }
  if (c.entry == 0x110b00 || c.entry == 0x1cf310 || c.entry == 0x1cf360 ||
      c.entry == 0x1d1bd0) {
    FP fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error(
        "Required original animation FP backend unavailable");
#endif
    return source_call_unit_animation_selection_1_1(m, c, fp, s.required_owner,
                                                    o);
  }
  if (!s.required_owner)
    throw std::runtime_error(
        "Required original biped pre-physics callee unavailable");
  return s.required_owner(m, c, o);
}
__m128 normalize(M &m, std::uint32_t p, const O &o) {
  const float x = f(r(m, p)), y = f(r(m, p + 4)), z = f(r(m, p + 8)),
              epsilon = f(r(m, 0x46504c));
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
  const auto inverse = _mm_div_ss(load(m, 0x465030), _mm_set_ss(length));
  w(m, o, 0x30c1b, p, 4, word(_mm_mul_ss(_mm_set_ss(x), inverse)));
  w(m, o, 0x30c2c, p + 4, 4, word(_mm_mul_ss(inverse, _mm_set_ss(y))));
  w(m, o, 0x30c31, p + 8, 4, word(_mm_mul_ss(inverse, _mm_set_ss(z))));
  return _mm_set_ss(length);
}
} // namespace
void source_update_biped_dead_outputs_1_1(M &m, std::uint32_t packet,
                                          std::uint32_t up,
                                          std::uint32_t forward,
                                          std::uint32_t esp, FP fp,
                                          const O &o) {
  Scope scope(fp);
  if (r(m, packet + 0x14, 1) & 0x40u) {
    constexpr std::array<std::uint32_t, 3> vs{0x1f0cfc, 0x1f0d01, 0x1f0d07},
        as{0x1f0d12, 0x1f0d17, 0x1f0d1d};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, vs[i], forward + i * 4, 4, r(m, packet + 0xf4 + i * 4));
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, as[i], up + i * 4, 4, r(m, packet + 0x100 + i * 4));
    return;
  }
  const auto one = load(m, 0x465030), minus_one = load(m, 0x46503c);
  auto dot = _mm_add_ss(
      _mm_add_ss(_mm_mul_ss(load(m, packet + 0x108), load(m, packet + 0xb4)),
                 _mm_mul_ss(load(m, packet + 0x104), load(m, packet + 0xb0))),
      _mm_mul_ss(load(m, packet + 0x100), load(m, packet + 0xac)));
  if (_mm_comigt_ss(minus_one, dot))
    dot = minus_one;
  else if (_mm_comigt_ss(dot, one))
    dot = one;
  // The original epsilon subtraction is x87, before float storage.
  if (double(f(r(m, 0x46504c))) >
      std::abs(double(_mm_cvtss_f32(dot)) - double(_mm_cvtss_f32(one))))
    return;
  const auto value = _mm_cvtss_f32(dot);
  if (!std::isfinite(value) || value < -1.f || value > 1.f)
    throw std::runtime_error(
        "Required original329168 CRT angle error owner unavailable");
  float angle{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  std::uint16_t saved{};
  asm volatile("fnstcw %0" : "=m"(saved));
  const std::uint16_t temporary = (saved & 0x300u) | 0x7fu;
  // 329168->329185: sqrt((1+x)*(1-x)), then FPATAN.32E529
  // temporarily clears the rounding mode while preserving precision.
  asm volatile("fldcw %0" ::"m"(temporary) : "memory");
  if (value == -1.f)
    asm volatile("fldpi");
  else if (value == 1.f)
    asm volatile("fldz");
  else
    asm volatile(
        "flds %0\n\tfld1\n\tfadd %%st(1)\n\tfld1\n\tfsub %%st(2)\n\t"
        "fmulp %%st, %%st(1)\n\tfsqrt\n\tfxch %%st(1)\n\tfpatan" ::"m"(value)
        : "st", "memory");
  asm volatile("fldcw %1\n\tfstps %0"
               : "=m"(angle)
               : "m"(saved)
               : "st", "memory");
#endif
  if (angle == f(r(m, 0x465048)))
    return;
  const auto axis = esp - 12;
  const auto ax =
      _mm_sub_ss(_mm_mul_ss(load(m, packet + 0x104), load(m, packet + 0xb4)),
                 _mm_mul_ss(load(m, packet + 0xb0), load(m, packet + 0x108)));
  const auto ay =
      _mm_sub_ss(_mm_mul_ss(load(m, packet + 0x108), load(m, packet + 0xac)),
                 _mm_mul_ss(load(m, packet + 0x100), load(m, packet + 0xb4)));
  const auto az =
      _mm_sub_ss(_mm_mul_ss(load(m, packet + 0xb0), load(m, packet + 0x100)),
                 _mm_mul_ss(load(m, packet + 0x104), load(m, packet + 0xac)));
  w(m, o, 0x1f0a49, axis, 4, word(ax));
  w(m, o, 0x1f0a73, axis + 4, 4, word(ay));
  w(m, o, 0x1f0a91, axis + 8, 4, word(az));
  if (_mm_cvtss_f32(normalize(m, axis, o)) == f(r(m, 0x465048)))
    return;
  float sine{}, cosine{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %2\n\tfsin\n\tfstps %0\n\tflds %2\n\tfcos\n\tfstps %1"
               : "=m"(sine), "=m"(cosine)
               : "m"(angle)
               : "st", "memory");
#endif
  constexpr std::array<std::uint32_t, 3> as{0x1f0ac7, 0x1f0acf, 0x1f0ad8},
      vs{0x1f0ae5, 0x1f0aea, 0x1f0af0};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, as[i], up + i * 4, 4, r(m, packet + 0x100 + i * 4));
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, vs[i], forward + i * 4, 4, r(m, packet + 0xf4 + i * 4));
  const auto x = load(m, axis), y = load(m, axis + 4), z = load(m, axis + 8),
             sn = _mm_set_ss(sine), cs = _mm_set_ss(cosine);
  for (unsigned which = 0; which < 2; ++which) {
    const auto out = which ? forward : up;
    const auto a = load(m, out), b = load(m, out + 4), c = load(m, out + 8);
    auto projection =
        which ? _mm_add_ss(_mm_add_ss(_mm_mul_ss(a, x), _mm_mul_ss(c, z)),
                           _mm_mul_ss(b, y))
              : _mm_add_ss(_mm_add_ss(_mm_mul_ss(c, z), _mm_mul_ss(b, y)),
                           _mm_mul_ss(a, x));
    projection = _mm_mul_ss(projection, _mm_sub_ss(one, cs));
    const auto first = _mm_sub_ss(
        _mm_add_ss(_mm_mul_ss(a, cs), _mm_mul_ss(projection, x)),
        _mm_mul_ss(_mm_sub_ss(_mm_mul_ss(b, z), _mm_mul_ss(c, y)), sn));
    const auto second = _mm_sub_ss(
        _mm_add_ss(_mm_mul_ss(b, cs), _mm_mul_ss(y, projection)),
        _mm_mul_ss(_mm_sub_ss(_mm_mul_ss(c, x), _mm_mul_ss(a, z)), sn));
    const auto third = _mm_sub_ss(
        _mm_add_ss(_mm_mul_ss(c, cs), _mm_mul_ss(z, projection)),
        _mm_mul_ss(_mm_sub_ss(_mm_mul_ss(a, y), _mm_mul_ss(b, x)), sn));
    w(m, o, which ? 0x1f0ca5 : 0x1f0bac, out, 4, word(first));
    w(m, o, which ? 0x1f0cd9 : 0x1f0bd2, out + 4, 4, word(second));
    w(m, o, which ? 0x1f0cde : 0x1f0bfc, out + 8, 4, word(third));
  }
  normalize(m, up, o);
  normalize(m, forward, o);
}
void source_update_biped_physical_outputs_1_1(
    M &m, std::uint32_t packet, std::uint32_t physical, std::uint32_t up,
    std::uint32_t forward, std::uint32_t esp, FP fp,
    const SourceBipedPrePhysicsServices &s, const O &o) {
  switch (r(m, physical, 1)) {
  case 1:
    call(m, s,
         {.entry = 0x1f4e20,
          .entry_esp = esp - 16,
          .eax = packet,
          .ecx = up,
          .edx = physical + 0x10,
          .ebx = forward,
          .arguments = {up}},
         o);
    return;
  case 2:
    call(m, s,
         {.entry = 0x1fe3e0,
          .entry_esp = esp - 16,
          .eax = packet,
          .ecx = up,
          .edx = physical + 0x10,
          .ebx = forward,
          .arguments = {physical + 0x10}},
         o);
    return;
  case 3:
    source_update_biped_dead_outputs_1_1(m, packet, up, forward, esp - 16, fp,
                                         o);
    return;
  case 4:
  case 5:
  case 6:
    return;
  default:
    throw std::runtime_error(
        "Undefined original1E9480 physical-state dispatch");
  }
}
std::uint32_t source_biped_unmarked_parent_1_1(const M &m, std::uint32_t d) {
  while (d != 0xffffffffu) {
    const auto p = object(m, d);
    if (!(r(m, p + 4) & 0x4000000u))
      break;
    d = r(m, p + 0x14);
  }
  return d;
}
std::uint8_t source_biped_action_class_1_1(std::uint32_t a) {
  switch (a) {
  case 0xa00000f:
  case 0x900000e:
  case 0x400000c:
    return 0;
  case 0x800001e:
  case 0x9000015:
  case 0x9000016:
  case 0xa000014:
  case 0xa000017:
  case 0xd000042:
    return 1;
  default:
    return 2;
  }
}
std::uint8_t
source_update_biped_recovery_1_1(M &m, std::uint32_t d, std::uint32_t esp,
                                 FP fp, const SourceBipedPrePhysicsServices &s,
                                 const O &o) {
  const auto p = object(m, d), frame = esp - 16;
  if (r(m, p + 0x39d, 1) && r(m, p + 0x3dc, 1) == 3 &&
      (r(m, tag(m, r(m, p)) + 0x1f0) & 0x400u) && !(r(m, p + 0x10a, 1) & 4u) &&
      r(m, p + 0x39d, 1) == 1) {
    call(m, s, {.entry = 0xe6f60, .entry_esp = frame - 4, .eax = d}, o);
    source_request_unit_animation_1_1(
        m, {d, {0x7000001, 0x7000101, 0x7000101, 0x7000001}, 0, 0}, 2,
        frame - 32, fp, s.required_owner, o);
    if (d != 0xffffffffu) {
      const auto current = object(m, d);
      if (sx(m, current + 0x112, 2) != -1) {
        if (r(m, current + 4) & 0x20000000u)
          call(m, s,
               {.entry = 0xc0930,
                .entry_esp = frame - 8,
                .eax = d,
                .arguments = {0}},
               o);
        w(m, o, 0xe4480, current + 4, 4, r(m, current + 4) & 0xdfffffffu);
      }
    }
    if (r(m, p + 0x3dc, 1) == 2)
      call(m, s, {.entry = 0x1fe8f0, .entry_esp = frame - 4, .eax = p + 0x3ec},
           o);
  }
  if (!r(m, p + 0x39d, 1))
    return 0;
  w(m, o, 0xe44a7, p + 0x39d, 1, r(m, p + 0x39d, 1) - 1);
  return 1;
}
std::uint8_t source_select_biped_dead_action_1_1(
    M &m, std::uint32_t d, std::uint32_t action, std::uint32_t physical,
    std::uint32_t esp, FP fp, const SourceBipedPrePhysicsServices &s,
    const O &o) {
  Scope scope(fp);
  const auto p = object(m, d), definition = tag(m, r(m, p)), frame = esp - 0x1c;
  const auto animation = p + std::uint32_t(sx(m, p + 0x12a, 2));
  if (source_biped_airborne_restriction_1_1(m, d, fp) &&
      !(r(m, p + 0xc1, 1) & 1u)) {
    const auto current = object(m, d);
    if (r(m, current + 4) & 0x20000000u) {
      if (d != 0xffffffffu && sx(m, current + 0x112, 2) != -1) {
        if (r(m, current + 4) & 0x20000000u)
          call(m, s,
               {.entry = 0xc0930,
                .entry_esp = frame - 8,
                .eax = d,
                .arguments = {0}},
               o);
        w(m, o, 0xe5366, current + 4, 4, r(m, current + 4) & 0xdfffffffu);
      }
      call(m, s,
           {.entry = 0xbb240,
            .entry_esp = frame - 8,
            .esi = d,
            .arguments = {0x3e88b439}},
           o);
      source_request_unit_animation_1_1(
          m, {d, {0x7000101, 0x7000101, 0x7000101, 0xd000042}, 0, 0}, 0x210,
          frame - 32, fp, s.required_owner, o);
      return 1;
    }
  }
  if ((r(m, animation) == 0xffffffffu || sx(m, animation + 6, 2) == -1 ||
       (r(m, animation + 0x11, 1) & 10u)) &&
      !(r(m, object(m, d) + 4) & 0x20000000u) &&
      r(m, animation + 0x7c) != 0xd000042) {
    const auto current = object(m, d);
    if (d != 0xffffffffu && sx(m, current + 0x112, 2) != -1)
      w(m, o, 0xe53eb, current + 4, 4, r(m, current + 4) | 0x20000000u);
    return 0;
  }
  if (r(m, p + 0x34b, 1)) {
    w(m, o, 0xe540e, action, 4, 0x7000101);
    w(m, o, 0xe5414, action + 4, 4, 0xc000043);
    w(m, o, 0xe541b, action + 8, 1, 0);
    w(m, o, 0xe541f, action + 9, 1, 0);
    return r(m, p + 0x34b, 1) == 1 ? 1 : 0;
  }
  std::uint8_t changed = 0;
  if (source_biped_airborne_restriction_1_1(m, d, fp) &&
      (call(m, s,
            {.entry = 0x110b00,
             .entry_esp = frame - 12,
             .eax = d,
             .arguments = {0x7000101, 0xd000042}},
            o) &
       255u)) {
    if (r(m, animation + 0x7c) == 0xd000042)
      call(m, s, {.entry = 0xe4f40, .entry_esp = frame - 4, .eax = d}, o);
    if (r(m, animation + 0x6c, 1) & 1u)
      w(m, o, 0xe546e, animation + 0x6c, 1, r(m, animation + 0x6c, 1) & 0xfeu);
    w(m, o, 0xe5476, action + 4, 4, 0xd000042);
    changed = 1;
  } else {
    if (r(m, animation + 0x7c) == 0xd000042) {
      w(m, o, 0xe5495, p + 0x3a0, 4, 0);
      w(m, o, 0xe54a6, physical + 0x28, 4, 0);
      source_update_biped_physical_outputs_1_1(m, physical, p + 0x3dc, p + 0x7c,
                                               p + 0x70, frame - 8, fp, s, o);
    }
    w(m, o, 0xe54b8, action + 4, 4, 0xc000043);
  }
  if (!(r(m, p + 0x10a, 1) & 4u) && (r(m, definition + 0x1f0) & 0x400u) &&
      r(m, object(m, d) + 0x39d, 1)) {
    w(m, o, 0xe54fd, action, 4, 0x6000087);
    w(m, o, 0xe5503, action + 4, 4, 0x400000c);
    w(m, o, 0xe550a, action + 8, 1, 1);
    w(m, o, 0xe550e, action + 9, 1, 1);
    return 1;
  }
  w(m, o, 0xe551e, action + 9, 1, 0);
  return changed;
}
void source_update_biped_action_release_1_1(
    M &m, std::uint32_t d, std::uint32_t esp, FP fp,
    const SourceBipedPrePhysicsServices &s, const O &o) {
  Scope scope(fp);
  const auto p = object(m, d);
  const auto cls = sx(m, p + 0x34a, 1);
  bool elapsed = false;
  if (cls == 0) {
    if (sx(m, p + 0x39b, 1) > 0) {
      const auto next = std::uint8_t(r(m, p + 0x39b, 1) + 1);
      w(m, o, 0xe6360, p + 0x39b, 1, next);
      const auto seconds = _mm_mul_ss(
          _mm_cvtsi32_ss(_mm_setzero_ps(), std::bit_cast<std::int8_t>(next)),
          load(m, r(m, 0x518444) + 4));
      if (_mm_comigt_ss(seconds, load(m, 0x4650d8))) {
        w(m, o, 0xe637f, p + 0x39b, 1, 0);
        elapsed = true;
      }
    }
  } else
    w(m, o, cls == 1 ? 0xe633e : 0xe62b5, p + 0x39b, 1, cls == 1 ? 1 : 0);
  if (!elapsed) {
    const auto animation = p + std::uint32_t(sx(m, p + 0x12a, 2)),
               a = r(m, animation + 0x7c);
    if (a != 0x900000e && a != 0xa00000f)
      return;
    if (r(m, animation) != 0xffffffffu && sx(m, animation + 6, 2) != -1) {
      const auto rate =
          _mm_mul_ss(load(m, animation + 0x1c), load(m, 0x4650dc));
      if (_mm_cvtss_f32(rate) != 0.f)
        return;
    }
  }
  auto scale = load(m, 0x465030);
  if (elapsed) {
    scale = _mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, p + 0x39f, 1)),
                       load(m, 0x451d90));
    if (_mm_comigt_ss(_mm_setzero_ps(), scale))
      scale = _mm_setzero_ps();
    else if (_mm_comigt_ss(scale, load(m, 0x465030)))
      scale = load(m, 0x465030);
  }
  call(m, s,
       {.entry = 0xe63c0,
        .entry_esp = esp - 24,
        .edx = 2,
        .arguments = {d, 3, word(scale)}},
       o);
  w(m, o, 0xe63aa, p + 0x39f, 1, 0);
}
std::uint8_t
source_update_biped_active_actions_1_1(M &m, std::uint32_t d, std::uint32_t esp,
                                       const SourceBipedPrePhysicsServices &s,
                                       const O &o) {
  const auto p = object(m, d), bank = p + std::uint32_t(sx(m, p + 0x346, 2));
  std::uint8_t active = 0;
  for (unsigned i = 0; i < 60; ++i) {
    const auto mask = 1u << (i & 31), at = bank + 4 + (i >> 5) * 4;
    if (!(r(m, at) & mask))
      continue;
    const auto entry = r(m, r(m, 0x46ec68 + i * 4) + 4);
    if (!entry)
      continue;
    if (call(m, s, {.entry = entry, .entry_esp = esp - 32, .arguments = {d, i}},
             o) &
        255u)
      active = 1;
    else
      w(m, o, 0xe7ed3, at, 4, r(m, at) & ~mask);
  }
  return active;
}
std::uint8_t source_update_biped_prephysics_1_1(
    M &m, std::uint32_t d, std::uint32_t esp, FP fp,
    const SourceBipedPrePhysicsServices &s, const O &o) {
  Scope scope(fp);
  const auto frame = esp - 0x17cu, p = object(m, d),
             definition = tag(m, r(m, p));
  const auto parent = source_biped_unmarked_parent_1_1(m, r(m, p + 0x14));
  const auto animation = p + std::uint32_t(sx(m, p + 0x12a, 2));
  if (r(m, p + 0xb4) == 0xffffffffu)
    return 0;
  // These are real caller-local action and physical packets. Lower source
  // owners see the same addresses on each tick and supply their own fields.
  w(m, o, 0xde9ce, frame + 0x18, 4, 0);
  w(m, o, 0xde9d2, frame + 0x1c, 4, 0);
  w(m, o, 0xde9d7, frame + 0x20, 4, 0);
  w(m, o, 0xde9db, frame + 0x18, 4, 0x7000101);
  w(m, o, 0xde9e3, frame + 0x1c, 4, 0x400000c);
  auto changed = source_update_biped_recovery_1_1(m, d, frame - 8, fp, s, o);
  if (parent != 0xffffffffu) {
    const auto header = r(m, r(m, 0x4e78d0) + 0x44) + (parent & 65535u) * 12;
    const auto type = r(m, header + 3, 1);
    if (r(m, p + 0x3dc, 1) == 6)
      call(m, s, {.entry = 0xe6f60, .entry_esp = frame - 4, .eax = d}, o);
    if (type == 1) {
      const auto vehicle = r(m, header + 8);
      call(m, s,
           {.entry = 0xe49b0,
            .entry_esp = frame - 8,
            .esi = vehicle,
            .arguments = {d}},
           o);
      if (r(m, r(m, 0x4edf18) + 0xc, 1) != 4)
        call(m, s, {.entry = 0xe4d30, .entry_esp = frame - 4, .eax = d}, o);
      if (r(m, r(m, 0x4edf18) + 0xc, 1) != 4 && r(m, vehicle + 0x34b, 1) > 0) {
        auto limit = std::uint32_t(rounded(_mm_mul_ss(
            _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, r(m, 0x518444) + 2, 2)),
            load(m, 0x465194))));
        const auto seat = sx(m, p + 0x1fc, 2);
        if (seat != -1) {
          const auto seat_row =
              r(m, tag(m, r(m, object(m, r(m, p + 0x14)))) + 0x1cc) +
              std::uint32_t(seat) * 0xb0;
          if (_mm_comigt_ss(load(m, seat_row + 0x20), load(m, 0x465048)))
            limit = call(m, s,
                         {.entry = 0x148160,
                          .entry_esp = frame - 8,
                          .arguments = {r(m, seat_row + 0x20)}},
                         o);
        }
        if (std::int32_t(r(m, vehicle + 0x34b, 1)) > si(limit))
          call(m, s,
               {.entry = 0xe7ef0,
                .entry_esp = frame - 8,
                .ecx = 0x1e,
                .arguments = {d}},
               o);
      }
      changed = 1;
    } else if (type == 0) {
      changed = 1;
      const bool dead = r(m, object(m, parent) + 0x10a, 1) & 4u;
      w(m, o, 0xdeb4d, frame + 0x1c, 4, dead ? 0x700003d : 0x1000003c);
    }
  } else {
    if ((r(m, p + 0x10a, 1) & 4u) ||
        (r(m, p + 0x3dc, 1) != 2 && !(r(m, definition + 0x264) & 8u))) {
      w(m, o, 0xdeb8a, p + 0x158, 4, 0);
      const auto magnitude = normalize(m, p + 0x150, o);
      if (_mm_cvtss_f32(magnitude) == f(r(m, 0x465048))) {
        const auto v = r(m, 0x46fc48);
        w(m, o, 0xdebac, p + 0x150, 4, r(m, v));
        w(m, o, 0xdebb1, p + 0x154, 4, r(m, v + 4));
        w(m, o, 0xdebb7, p + 0x158, 4, r(m, v + 8));
      }
    }
    w(m, o, 0xdebc2, p + 0x34a, 1,
      source_biped_action_class_1_1(r(m, animation + 0x7c)));
    const auto x = load(m, p + 0x1b0), y = load(m, p + 0x1b4),
               z = load(m, p + 0x1b8);
    const auto squared = _mm_add_ss(
        _mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)), _mm_mul_ss(z, z));
    if (_mm_comigt_ss(load(m, 0x4654c0), squared)) {
      const auto v = r(m, 0x46fc44);
      w(m, o, 0xdec11, p + 0x1b0, 4, r(m, v));
      w(m, o, 0xdec1a, p + 0x1b4, 4, r(m, v + 4));
      w(m, o, 0xdec23, p + 0x1b8, 4, r(m, v + 8));
    }
    for (unsigned i = 0; i < 2; ++i) {
      const auto at = p + 0x399 + i;
      if (r(m, p + 0x348, 1) & (1u << i)) {
        if (sx(m, at, 1) < 127)
          w(m, o, i ? 0xdec6b : 0xdec3e, at, 1, r(m, at, 1) + 1);
        changed = 1;
      } else
        w(m, o, i ? 0xdec78 : 0xdec4b, at, 1, 0);
    }
    w(m, o, 0xdec88, frame + 0x7c, 2, 0xffff);
    w(m, o, 0xdec90, frame + 0x7e, 2, 0xffff);
    changed |= std::uint8_t(call(m, s,
                                 {.entry = 0xe1800,
                                  .entry_esp = frame - 12,
                                  .arguments = {d, frame + 0x18}},
                                 o));
    call(m, s,
         {.entry = 0xddbc0,
          .entry_esp = frame - 12,
          .arguments = {d, frame + 0x2c}},
         o);
    source_update_biped_physical_outputs_1_1(
        m, frame + 0x2c, p + 0x3dc, p + 0x7c, p + 0x70, frame - 8, fp, s, o);
    changed |= source_update_biped_movement_1_1(
        m, d, frame + 0x18, frame + 0x2c, frame - 16, fp, s, o);
    const auto state = r(m, p + 0x3dc, 1);
    if (state == 3) {
      if (s.death_admission) {
        auto admission = *s.death_admission;
        if (s.object_velocity) {
          admission.update_activation = [&](auto datum) {
            source_biped_physics_activation_1_1(
                m, datum, false, s.object_velocity->activation, o);
          };
          admission.impulse = [&](auto component, auto slot, auto vector) {
            source_add_biped_body_velocity_1_1(
                m, component, std::uint32_t(slot), vector, frame - 8 - 0x60, fp,
                s.object_velocity->body, o);
          };
        }
        if (s.ragdoll_runtime) {
          admission.rebuild_ragdoll = [&](auto datum) {
            return source_rebuild_biped_ragdoll_runtime_1_1(
                m, datum, frame - 8 - 0x60, fp, *s.ragdoll_runtime, o);
          };
          admission.maintain_placement = [&](auto datum) {
            source_maintain_biped_ragdoll_runtime_1_1(
                m, datum, frame - 8 - 0x5c, fp, *s.ragdoll_runtime, o);
          };
        }
        changed |=
            source_admit_biped_death_1_1(m, d, frame - 8, fp, admission, o);
      } else
        changed |= std::uint8_t(call(
            m, s, {.entry = 0xe2520, .entry_esp = frame - 8, .arguments = {d}},
            o));
      changed |= source_select_biped_dead_action_1_1(
          m, d, frame + 0x18, frame + 0x2c, frame - 12, fp, s, o);
    } else if (state == 1) {
      if (source_biped_airborne_restriction_1_1(m, d, fp))
        call(m, s,
             {.entry = 0xe5700,
              .entry_esp = frame - 12,
              .arguments = {d, frame + 0x18}},
             o);
      else if (sx(m, p + 0x34c, 2) != -1)
        call(m, s,
             {.entry = 0xe5a80,
              .entry_esp = frame - 12,
              .arguments = {d, frame + 0x18}},
             o);
      else if (r(m, p + 0x348, 1) & 2u)
        call(m, s, {.entry = 0xe5c10, .entry_esp = frame - 4, .esi = d}, o);
      changed = 1;
    }
    source_update_biped_action_release_1_1(m, d, frame - 4, fp, s, o);
  }
  call(m, s, {.entry = 0xdf7c0, .entry_esp = frame - 4, .eax = d}, o);
  if ((r(m, p + 0x10a, 1) & 4u) && (r(m, p + 0xc1, 1) & 1u)) {
    if (call(m, s, {.entry = 0x115600, .entry_esp = frame - 4, .eax = d}, o) &
        255u) {
      call(m, s, {.entry = 0xbc910, .entry_esp = frame - 4, .eax = d}, o);
      changed = 1;
    } else
      w(m, o, 0xded8b, p + 0x348, 1, r(m, p + 0x348, 1) & 0xfcu);
  } else {
    w(m, o, 0xded9d, p + 0xbc, 4, r(m, r(m, 0x518444) + 8));
    call(m, s, {.entry = 0xbc910, .entry_esp = frame - 4, .eax = d}, o);
    changed = 1;
    const auto child = r(m, object(m, d) + 0xd4);
    if (child != 0xffffffffu)
      call(m, s,
           {.entry = 0xb6760, .entry_esp = frame - 12, .arguments = {child, 4}},
           o);
  }
  const auto active =
      source_update_biped_active_actions_1_1(m, d, frame - 8, s, o);
  const SourceRuntimeOwner animation_owner =
      [&](M &memory, const SourceRuntimeCall &request, const O &observe) {
        if (s.phase_runtime &&
            (request.entry == 0xc04f0 || request.entry == 0xe8050))
          return source_call_biped_runtime_1_1(memory, request, fp,
                                               *s.phase_runtime, observe);
        if (!s.required_owner)
          throw std::runtime_error(
              "Required original biped animation lower owner unavailable");
        return s.required_owner(memory, request, observe);
      };
  const auto action = source_update_unit_animation_1_1(
      m, d, frame + 0x18, frame - 12, fp, animation_owner, o);
  return changed | active | action;
}
} // namespace halo2
