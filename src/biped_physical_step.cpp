#include "halo2/biped_phase_runtime.hpp"
#include "halo2/biped_prephysics.hpp"
#include "halo2/physics_body_matrix.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cstring>
#include <emmintrin.h>
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
    v |= std::to_integer<unsigned>(b[i]) << (8 * i);
  return v;
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::int32_t sx(const M &m, std::uint32_t p, unsigned n = 2) {
  const auto v = r(m, p, n);
  return n == 1 ? std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))
                : std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v)));
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t v,
       unsigned n = 4) {
  SourcePhysicsContactRecordStore e{site, p, {}, {}};
  e.before.resize(n);
  e.written.resize(n);
  m.read(p, e.before);
  for (unsigned i = 0; i < n; ++i)
    e.written[i] = std::byte(v >> (8 * i));
  m.write(p, e.written);
  if (o)
    o(e);
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 65535) * 12 + 8);
}
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
__m128 load(const M &m, std::uint32_t p) { return _mm_set_ss(f(r(m, p))); }
std::uint32_t word(__m128 v) {
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original physical-step FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original physical-step x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t call(M &m, const SourceBipedPrePhysicsServices &s,
                   SourceBipedPhaseCall q, const O &o) {
  if (q.entry == 0xbc910 && s.phase_runtime) {
    FP fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error(
        "Required original physical marking FP backend unavailable");
#endif
    return source_call_biped_runtime_1_1(m, q, fp, *s.phase_runtime, o);
  }
  if (q.entry == 0x1c8430 && s.object_velocity) {
    if (!q.eax || !q.ebx || q.arguments.size() != 2)
      throw std::runtime_error(
          "Missing defined original physical velocity input");
    source_biped_object_velocity_1_1(m, *q.eax, *q.ebx, q.arguments[0],
                                     std::uint8_t(q.arguments[1]), q.entry_esp,
                                     *s.object_velocity, o);
    return 0; // The source physical caller ignores this owner's EAX.
  }
  if (!s.required_owner)
    throw std::runtime_error(
        "Required original physical-step callee unavailable");
  return s.required_owner(m, q, o);
}
} // namespace
void source_step_biped_dead_1_1(M &m, std::uint32_t dead,
                                std::uint32_t physical, std::uint32_t out,
                                std::uint32_t esp, FP fp,
                                const SourceBipedPrePhysicsServices &s,
                                const O &o) {
  Scope scope(fp);
  const auto frame = ((esp - 4) & ~15u) - 0xe0u;
  const auto cd = r(m, physical + 0x18),
             component = r(m, r(m, 0x526248) + 0x44) + (cd & 65535u) * 0xa0;
  const auto velocity = out + 0xc;
  const bool awake = r(m, r(m, component + 0x70) + 0x44, 1) != 0;
  w(m, o, 0x1f00b1, frame + 0x17, awake, 1);
  constexpr std::uint32_t copy[]{0x1f00c4, 0x1f00c9, 0x1f00cf};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, copy[i], velocity + 4 * i, r(m, physical + 0x124 + 4 * i));
  w(m, o, 0x1f00d8, frame + 0x16, 0, 1);
  w(m, o, 0x1f00dd, frame + 0x50, component);
  w(m, o, 0x1f00e1, frame + 0x2c, velocity);
  if (awake) {
    const auto body = r(m, component + 0x70);
    const auto scale = load(m, 0x4651a0);
    constexpr std::uint32_t locals[]{0x1f0118, 0x1f0139, 0x1f013f},
        stores[]{0x1f011e, 0x1f012b, 0x1f0145};
    // The source retains all three scaled values before restoring them at
    // the tail; these are body-owned movement, not a native gravity vector.
    for (unsigned i = 0; i < 3; ++i) {
      const auto v = _mm_mul_ss(load(m, body + 0x30 + 4 * i), scale);
      w(m, o, locals[i], frame + 0x54 + 4 * i, word(v));
      w(m, o, stores[i], velocity + 4 * i,
        word(_mm_sub_ss(load(m, velocity + 4 * i), v)));
    }
  }
  bool support_motion = false;
  if (r(m, dead + 0x14) != 0xffffffffu) {
    const auto d = r(m, dead + 0x14);
    const auto pool = r(m, 0x4e78d0), index = d & 65535u;
    std::uint32_t support = 0;
    if (si(index) < si(r(m, pool + 0x38))) {
      const auto header = r(m, pool + 0x44) + r(m, pool + 0x24) * index;
      if (r(m, header, 2) && sx(m, header) == (si(d) >> 16))
        support = r(m, header + 8);
    }
    if (!support)
      call(m, s,
           {.entry = 0x1f0d30,
            .entry_esp = frame - 8,
            .ecx = dead,
            .arguments = {r(m, component + 8)}},
           o);
  } else if (r(m, physical + 0xc) != 0xffffffffu) {
    const auto parent = object(m, r(m, physical + 0xc)),
               parent_cd = r(m, parent + 0xb4),
               pc = r(m, r(m, 0x526248) + 0x44) + (parent_cd & 65535u) * 0xa0;
    const auto root = sx(m, pc + 0x18, 1);
    if (root >= 0 && root < si(r(m, pc + 0x74))) {
      call(m, s,
           {.entry = 0x1d4410,
            .entry_esp = frame - 8,
            .eax = std::uint32_t(root),
            .ecx = physical + 0xe8,
            .esi = frame + 0x1c,
            .arguments = {pc}},
           o);
      constexpr std::uint32_t diffs[]{0x1f021e, 0x1f022f, 0x1f024e},
          stores[]{0x1f0282, 0x1f0288, 0x1f028b};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, diffs[i], frame + 0x34 + 4 * i,
          word(_mm_sub_ss(load(m, frame + 0x1c + 4 * i),
                          load(m, dead + 0x50 + 4 * i))));
      const auto x = load(m, dead + 0x50), y = load(m, dead + 0x54),
                 z = load(m, dead + 0x58);
      const auto square = _mm_add_ss(
          _mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)), _mm_mul_ss(z, z));
      support_motion = _mm_comigt_ss(square, _mm_setzero_ps());
      w(m, o, 0x1f026b, frame + 0x16, 1, 1);
      if (!support_motion)
        w(m, o, 0x1f0272, frame + 0x16, 0, 1);
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, stores[i], dead + 0x50 + 4 * i, r(m, frame + 0x1c + 4 * i));
    }
  } else {
    constexpr std::uint32_t stores[]{0x1f029b, 0x1f02a0, 0x1f02a6};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, stores[i], dead + 0x50 + 4 * i, r(m, r(m, 0x46fc44) + 4 * i));
  }
  auto square = [](__m128 x, __m128 y, __m128 z) {
    return _mm_add_ss(_mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)),
                      _mm_mul_ss(z, z));
  };
  auto hypot_word = [&](std::uint32_t xword, std::uint32_t yword) {
    const auto x = f(xword), y = f(yword);
    float magnitude;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %1\n\tfmuls %1\n\tflds %2\n\tfmuls "
                 "%2\n\tfaddp\n\tfsqrt\n\tfstps %0"
                 : "=m"(magnitude)
                 : "m"(x), "m"(y)
                 : "st", "st(1)", "memory");
#else
    throw std::runtime_error(
        "Required original dead-step x87 magnitude owner unavailable");
#endif
    return std::bit_cast<std::uint32_t>(magnitude);
  };
  const auto clock = r(m, 0x518444);
  if (r(m, dead + 0xc, 1) &&
      (_mm_comigt_ss(load(m, 0x4650ec), load(m, out + 0x14)) ||
       _mm_comigt_ss(load(m, 0x4650ec), load(m, physical + 0x12c)))) {
    auto decay = _mm_mul_ss(load(m, clock + 4), load(m, 0x46507c));
    const auto magnitude_word = hypot_word(r(m, velocity), r(m, out + 0x10));
    w(m, o, 0x1f02f5, frame + 0x18, magnitude_word);
    const auto magnitude = _mm_set_ss(f(magnitude_word));
    if (_mm_comigt_ss(magnitude, decay)) {
      decay = _mm_sub_ss(load(m, 0x465030), _mm_div_ss(decay, magnitude));
      w(m, o, 0x1f031c, velocity, word(_mm_mul_ss(load(m, velocity), decay)));
      w(m, o, 0x1f0329, out + 0x10,
        word(_mm_mul_ss(load(m, out + 0x10), decay)));
    } else {
      w(m, o, 0x1f0330, velocity, 0);
      w(m, o, 0x1f0334, out + 0x10, 0);
    }
  }
  if (_mm_comigt_ss(square(load(m, physical + 0x138), load(m, physical + 0x13c),
                           load(m, physical + 0x140)),
                    load(m, 0x465558))) {
    w(m, o, 0x1f0389, out, r(m, out));
    auto x = _mm_sub_ss(
        _mm_sub_ss(
            _mm_mul_ss(load(m, physical + 0xf4), load(m, physical + 0x138)),
            _mm_mul_ss(load(m, physical + 0xf8), load(m, physical + 0x13c))),
        load(m, physical + 0x124));
    auto y = _mm_sub_ss(
        _mm_add_ss(
            _mm_mul_ss(load(m, physical + 0x138), load(m, physical + 0xf8)),
            _mm_mul_ss(load(m, physical + 0xf4), load(m, physical + 0x13c))),
        load(m, physical + 0x128));
    w(m, o, 0x1f03e3, frame + 0x20, word(y));
    w(m, o, 0x1f03f1, frame + 0x1c, word(x));
    const auto magnitude_word = hypot_word(word(y), word(x));
    w(m, o, 0x1f0408, frame + 0x18, magnitude_word);
    auto limit = _mm_mul_ss(
        load(m, clock + 4),
        load(m, physical + ((r(m, physical + 0x14, 1) & 4u) ? 0x148 : 0x144)));
    if (_mm_comigt_ss(_mm_set_ss(f(magnitude_word)), limit)) {
      limit = _mm_div_ss(limit, _mm_set_ss(f(magnitude_word)));
      x = _mm_mul_ss(limit, x);
      y = _mm_mul_ss(limit, y);
    }
    w(m, o, 0x1f045a, velocity, word(_mm_add_ss(load(m, velocity), x)));
    w(m, o, 0x1f045e, out + 0x10, word(_mm_add_ss(load(m, out + 0x10), y)));
  }
  if (!(r(m, physical + 0x14, 1) & 2u) &&
      !_mm_comigt_ss(load(m, 0x4650fc),
                     square(load(m, velocity), load(m, velocity + 4),
                            load(m, velocity + 8)))) {
    w(m, o, 0x1f04ab, dead + 0xe, 0, 1);
    w(m, o, 0x1f04b6, dead + 0xf, std::min(r(m, dead + 0xf, 1) + 1, 254u), 1);
  } else {
    w(m, o, 0x1f04cc, dead + 0xe, std::min(r(m, dead + 0xe, 1) + 1, 254u), 1);
    w(m, o, 0x1f04cf, dead + 0xf, 0, 1);
  }
  if (r(m, physical + 0xb9, 1))
    w(m, o, 0x1f04dd, dead + 0x13, 0, 1);
  else if (r(m, physical + 0xb8, 1) && r(m, dead + 0x13, 1) < 2) {
    const auto extension = _mm_add_ss(load(m, 0x486564), load(m, 0x4650e0));
    w(m, o, 0x1f051f, frame + 0x1c, word(extension));
    w(m, o, 0x1f0525, frame + 0x30, 0xffffffffu);
    constexpr std::uint32_t zero_sites[]{0x1f052d, 0x1f0532, 0x1f0537};
    for (unsigned i = 0; i < 3; ++i) {
      SourcePhysicsContactRecordStore e{
          zero_sites[i], frame + 0x60 + i * 16, {}, std::vector<std::byte>(16)};
      e.before.resize(16);
      m.read(e.address, e.before);
      m.write(e.address, e.written);
      if (o)
        o(e);
    }
    auto candidate = r(m, dead + 0x13, 1) + 1;
    w(m, o, 0x1f053f, frame + 0x28, candidate);
    std::uint32_t selected = 0xffffffffu;
    for (; candidate < 3; ++candidate) {
      const auto shape_row = r(m, r(m, physical + 8) + 0x24) + candidate * 0x80;
      auto radius = load(m, 0x486564);
      const auto inner = _mm_sub_ss(load(m, shape_row + 0x2c), extension);
      w(m, o, 0x1f0575, frame + 0x18, word(radius));
      if (!_mm_comigt_ss(radius, inner)) {
        radius = inner;
        w(m, o, 0x1f057d, frame + 0x18, word(radius));
      }
      w(m, o, 0x2e5516, frame + 0x46, 1, 2);
      w(m, o, 0x2e551c, frame + 0x48, 0);
      w(m, o, 0x2e5523, frame + 0x4c, word(radius));
      w(m, o, 0x2e5526, frame + 0x40, 0x45c5c8);
      constexpr std::uint32_t pos_sites[]{0x1f059f, 0x1f05b0, 0x1f05c1};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, pos_sites[i], frame + 0x90 + i * 4,
          r(m, physical + 0xe8 + i * 4));
      w(m, o, 0x1f05cd, frame + 0x9c, 0);
      SourcePhysicsContactRecordStore position{
          0x1f05ed, frame + 0x90, {}, std::vector<std::byte>(16)};
      position.before.resize(16);
      m.read(position.address, position.before);
      std::array<std::byte, 16> center{};
      m.read(shape_row + 0x70, center);
      __m128 a, b;
      std::memcpy(&a, position.before.data(), 16);
      std::memcpy(&b, center.data(), 16);
      a = _mm_add_ps(a, b);
      std::memcpy(position.written.data(), &a, 16);
      m.write(position.address, position.written);
      if (o)
        o(position);
      const auto one = r(m, 0x465030);
      w(m, o, 0x1f05fb, frame + 0xb4, r(m, frame + 0x64));
      w(m, o, 0x1f060a, frame + 0xb8, r(m, frame + 0x68));
      w(m, o, 0x1f0619, frame + 0xac, one);
      w(m, o, 0x1f0622, frame + 0xb0, one);
      w(m, o, 0x1f062b, frame + 0xbc, r(m, frame + 0x70));
      w(m, o, 0x1f063a, frame + 0xc0, one);
      w(m, o, 0x1f0643, frame + 0xd0, one);
      w(m, o, 0x1f0656, frame + 0xc4, r(m, frame + 0x78));
      w(m, o, 0x1f0668, frame + 0xd4, r(m, frame + 0x90));
      w(m, o, 0x1f067f, frame + 0xc8, r(m, frame + 0x80));
      w(m, o, 0x1f0691, frame + 0xd8, r(m, frame + 0x94));
      w(m, o, 0x1f06ab, frame + 0xcc, r(m, frame + 0x84));
      w(m, o, 0x1f06b4, frame + 0xdc, r(m, frame + 0x98));
      if (!(call(m, s,
                 {.entry = 0x1c8b40,
                  .entry_esp = frame - 16,
                  .eax = frame + 0xac,
                  .arguments = {frame + 0x40, r(m, physical + 0x18),
                                r(m, physical + 0x24)}},
                 o) &
            255u)) {
        selected = candidate;
        w(m, o, 0x1f06ca, frame + 0x30, candidate);
      }
      w(m, o, 0x1f06d4, frame + 0x40, 0x459d98);
      if (r(m, frame + 0x45, 1) & 0x80u) {
        w(m, o, 0x1f06de, frame + 0x45, r(m, frame + 0x45, 1) & 0x7fu, 1);
        call(m, s,
             {.entry = 0x2dd7f0,
              .entry_esp = frame - 8,
              .ecx = frame + 0x40,
              .arguments = {0}},
             o);
      }
      w(m, o, 0x1f06ff, frame + 0x40, 0x459d94);
      w(m, o, 0x1f0707, frame + 0x28, candidate + 1);
    }
    if (selected != 0xffffffffu) {
      w(m, o, 0x1f0738, dead + 0x13, selected, 1);
      call(m, s,
           {.entry = 0x1c70d0,
            .entry_esp = frame - 12,
            .ecx = dead,
            .arguments = {r(m, component + 8), 0}},
           o);
    }
  }
  const auto count = r(m, component + 0x8c);
  if (!count)
    w(m, o, 0x1f0860, dead + 0x10, 0, 1);
  else {
    auto root = sx(m, component + 0x18, 1);
    if (root < 0 || root >= si(r(m, component + 0x74)))
      root = -1;
    bool supported = true;
    for (std::int32_t i = 0; i < si(count); ++i) {
      const auto row = r(m, component + 0x88) + std::uint32_t(i) * 0x48;
      if (r(m, row + 0x14) != 0xffffffffu &&
          r(m, row + 0x18) != r(m, physical + 0xc)) {
        supported = false;
        break;
      }
    }
    if (!supported || root == -1)
      w(m, o, 0x1f0857, dead + 0x10, 0, 1);
    else {
      const auto lower = load(m, 0x465974), upper = load(m, 0x46508c);
      auto clamp = [&](__m128 v, __m128 lo) {
        if (_mm_comigt_ss(lo, v))
          return lo;
        if (_mm_comigt_ss(v, upper))
          return upper;
        return v;
      };
      auto equal = [](__m128 a, __m128 b) {
        return _mm_comieq_ss(a, b) &&
               !(_mm_movemask_ps(_mm_cmpunord_ss(a, b)) & 1);
      };
      const auto x = load(m, velocity), y = load(m, out + 0x10),
                 z = load(m, out + 0x14);
      if (equal(clamp(x, lower), x) && equal(clamp(y, lower), y)) {
        const auto zlower = _mm_sub_ss(
            _mm_sub_ss(_mm_setzero_ps(), load(m, r(m, 0x526254))), upper);
        if (equal(clamp(z, zlower), z))
          w(m, o, 0x1f084f, dead + 0x10,
            std::min(r(m, dead + 0x10, 1) + 1, 254u), 1);
      }
    }
  }
  if (support_motion) {
    constexpr std::uint32_t stores[]{0x1f087a, 0x1f0889, 0x1f0899};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, stores[i], velocity + 4 * i,
        word(_mm_add_ss(load(m, velocity + 4 * i),
                        load(m, frame + 0x34 + 4 * i))));
  }
  if (awake) {
    constexpr std::uint32_t stores[]{0x1f08b0, 0x1f08bf, 0x1f08cf};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, stores[i], velocity + 4 * i,
        word(_mm_add_ss(load(m, velocity + 4 * i),
                        load(m, frame + 0x54 + 4 * i))));
  }
}
void source_select_biped_physical_movement_1_1(M &m, std::uint32_t state,
                                               std::uint32_t physical,
                                               std::uint32_t out, FP fp,
                                               const O &o) {
  Scope scope(fp);
  if (r(m, physical + 0x14, 1) & 0x20u) {
    w(m, o, 0x1e94eb, out + 4, 1);
    w(m, o, 0x1e94ee, out + 8, 1, 1);
    return;
  }
  const auto mode = r(m, state, 1);
  const auto zero = _mm_setzero_ps();
  auto absolute = [](__m128 x) {
    return _mm_and_ps(x, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
  };
  if (mode == 1) {
    bool nonzero = false;
    for (unsigned i = 0; i < 3; ++i) {
      const auto v = load(m, physical + 0xdc + 4 * i);
      if (!_mm_comieq_ss(v, zero) ||
          (_mm_movemask_ps(_mm_cmpunord_ss(v, v)) & 1)) {
        nonzero = true;
        break;
      }
    }
    if (!nonzero)
      return;
    const auto x = load(m, physical + 0xdc), y = load(m, physical + 0xe0);
    if (_mm_comigt_ss(absolute(y), absolute(x)))
      w(m, o, _mm_comigt_ss(zero, y) ? 0x1f5349 : 0x1f5351, out + 4,
        _mm_comigt_ss(zero, y) ? 2 : 3);
    else
      w(m, o, _mm_comigt_ss(zero, x) ? 0x1f5362 : 0x1f536a, out + 4,
        _mm_comigt_ss(zero, x) ? 5 : 4);
  } else if (mode == 2) {
    w(m, o, 0x1fe878, out + 4, 1);
    const auto x = load(m, physical + 0xdc), y = load(m, physical + 0xe0),
               threshold = load(m, 0x46526c);
    if (_mm_comigt_ss(absolute(y), absolute(x))) {
      if (_mm_comigt_ss(threshold, y))
        w(m, o, 0x1fe8a2, out + 4, 2);
      else if (_mm_comigt_ss(y, load(m, 0x46504c)))
        w(m, o, 0x1fe8bb, out + 4, 3);
    } else {
      if (_mm_comigt_ss(threshold, x))
        w(m, o, 0x1fe8cc, out + 4, 5);
      else if (_mm_comigt_ss(x, load(m, 0x46504c)))
        w(m, o, 0x1fe8e5, out + 4, 4);
    }
  } else if (mode == 3) {
    return;
  } else if (mode == 4 || mode == 5) {
    if (r(m, state + 0x38, 1)) {
      const auto z = load(m, physical + 0xe4);
      const auto value =
          _mm_comigt_ss(zero, z) ? 11u : (_mm_comigt_ss(z, zero) ? 10u : 1u);
      w(m, o, value == 11 ? 0x1ffff5 : (value == 10 ? 0x20000a : 0x200012),
        out + 4, value);
    }
  } else if (mode == 6) {
    if (r(m, physical + 0xbc, 1) &&
        _mm_comige_ss(load(m, physical + 0xd8), load(m, 0x4650d8)))
      w(m, o, 0x1efdff, out + 4, 4);
  } else
    throw std::runtime_error("Undefined original1E94E0 state dispatch");
}
void source_step_biped_physical_1_1(M &m, std::uint32_t state,
                                    std::uint32_t physical, std::uint32_t out,
                                    std::uint32_t esp, FP fp,
                                    const SourceBipedPrePhysicsServices &s,
                                    const O &o) {
  Scope scope(fp);
  const auto frame = ((esp - 4) & ~15u) - 0xa0;
  w(m, o, 0x1e8f6f, out, 0);
  constexpr std::uint32_t upsites[]{0x1e8f85, 0x1e8f8a, 0x1e8f90},
      forwardsites[]{0x1e8f9e, 0x1e8fa3, 0x1e8fae},
      zerosites[]{0x1e8fbc, 0x1e8fc1, 0x1e8fc7},
      positionsites[]{0x1e8fe4, 0x1e8fe9, 0x1e8fef},
      tailsites[]{0x1e8ff7, 0x1e8ffc, 0x1e9002};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, upsites[i], out + 0x24 + 4 * i, r(m, physical + 0x100 + 4 * i));
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, forwardsites[i], out + 0x18 + 4 * i, r(m, physical + 0xf4 + 4 * i));
  w(m, o, 0x1e8fb8, frame + 0x20, out + 0xc);
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, zerosites[i], out + 0xc + 4 * i, r(m, r(m, 0x46fc44) + 4 * i));
  w(m, o, 0x1e8fd0, out + 4, 0);
  w(m, o, 0x1e8fd7, out + 8, 0, 1);
  w(m, o, 0x1e8fdb, out + 0x54, 0, 1);
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, positionsites[i], out + 0x58 + 4 * i, r(m, r(m, 0x46fc28) + 4 * i));
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, tailsites[i], out + 0x64 + 4 * i, r(m, r(m, 0x46fc44) + 4 * i));
  if (r(m, state + 1, 1))
    w(m, o, 0x1e9011, state + 1, r(m, state + 1, 1) - 1, 1);
  const auto mode = r(m, state, 1);
  if (mode == 1 || mode == 2)
    call(m, s,
         {.entry = mode == 1 ? 0x1f3e10u : 0x1fe1d0u,
          .entry_esp = frame - 16,
          .edi = physical,
          .arguments = {state + 0x10, out, physical}},
         o);
  else if (mode == 3)
    source_step_biped_dead_1_1(m, state + 0x10, physical, out, frame - 12, fp,
                               s, o);
  else if (mode == 4 || mode == 5)
    call(m, s,
         {.entry = 0x1ffe70,
          .entry_esp = frame - 8,
          .ecx = physical,
          .edx = out,
          .arguments = {state + 0x10}},
         o);
  else if (mode == 6) {
    const auto v =
        _mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, r(m, 0x518444) + 2)),
                   load(m, 0x452000));
    w(m, o, 0x1e905b, frame + 0x14, word(v));
    const auto value = f(word(v));
    std::int32_t ticks;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("flds %1\n\tfistpl %0"
                 : "=m"(ticks)
                 : "m"(value)
                 : "st", "memory");
#else
    throw std::runtime_error(
        "Required original physical-step FIST owner unavailable");
#endif
    w(m, o, 0x1e9065, frame + 0x18, std::uint32_t(ticks));
    w(m, o, 0x1e906d, state + 1, std::uint32_t(ticks), 1);
    call(m, s,
         {.entry = 0x1ef0e0,
          .entry_esp = frame - 12,
          .eax = physical,
          .arguments = {state + 0x10, out}},
         o);
  } else
    throw std::runtime_error("Undefined original1E8F60 state dispatch");
  source_select_biped_physical_movement_1_1(m, state, physical, out, fp, o);
  auto virtual_type = [&](std::uint32_t p) {
    return call(
        m, s, {.entry = r(m, r(m, p) + 0x14), .entry_esp = frame - 4, .ecx = p},
        o);
  };
  const auto shape = r(m, physical + 0x14c);
  if (shape) {
    const auto cd = r(m, physical + 0x18),
               component = r(m, r(m, 0x526248) + 0x44) + (cd & 65535u) * 0xa0;
    const auto body = r(m, r(m, component + 0x70) + 0x40);
    w(m, o, 0x1e90bd, frame + 0x18, component);
    w(m, o, 0x1e90c7, frame + 0x14, body);
    const auto child = r(m, body + 0xc);
    const auto type = virtual_type(child);
    bool matches =
        child == shape || (type == 0x17 && r(m, child + 0x30) == shape);
    if (!matches) {
      const auto shape_type = virtual_type(shape);
      if (shape_type == 7) {
        auto radius = load(m, 0x486564);
        const auto extension = _mm_add_ss(radius, load(m, 0x4650e0));
        const auto inner = _mm_sub_ss(load(m, shape + 0xc), extension);
        if (!_mm_comigt_ss(radius, inner))
          radius = inner;
        w(m, o, 0x1e92f5, frame + 0x1c, word(extension));
        w(m, o, 0x1e92fb, frame + 0x14, r(m, 0x486564));
        if (!_mm_comigt_ss(load(m, 0x486564), inner))
          w(m, o, 0x1e9303, frame + 0x14, word(inner));
        call(m, s,
             {.entry = 0x2e4620,
              .entry_esp = frame - 16,
              .ecx = frame + 0x30,
              .arguments = {shape + 0x10, shape + 0x20, word(radius)}},
             o);
        source_physics_body_matrix_1_1(
            m, component, 0, frame + 0x6c, ((frame - 8) & ~15u) - 0x40,
            [&](M &mem, std::uint32_t transform, std::uint32_t offset,
                const O &observe) {
              call(mem, s,
                   {.entry = 0x2e0b60,
                    .entry_esp = ((frame - 8) & ~15u) - 0x48,
                    .ecx = transform,
                    .arguments = {offset}},
                   observe);
            },
            o);
        w(m, o, 0x1e9354, frame + 0x9c,
          word(_mm_add_ss(_mm_mul_ss(extension, load(m, 0x46502c)),
                          load(m, frame + 0x9c))));
        const bool blocked =
            call(m, s,
                 {.entry = 0x1c8b40,
                  .entry_esp = frame - 16,
                  .eax = frame + 0x6c,
                  .arguments = {frame + 0x30, r(m, physical + 0x18),
                                r(m, physical + 0x24)}},
                 o) &
            255u;
        w(m, o, blocked ? 0x1e9382 : 0x1e936f, out,
          blocked ? (r(m, out) & ~2u) : (r(m, out) | 2u));
        w(m, o, 0xdd97b, frame + 0x30, 0x459d98);
        if (r(m, frame + 0x34, 2) & 0x8000u) {
          w(m, o, 0xdd98a, frame + 0x34, r(m, frame + 0x34, 2) & 0x7fffu, 2);
          call(m, s,
               {.entry = 0x2dd7f0,
                .entry_esp = frame - 16,
                .ecx = frame + 0x30,
                .arguments = {0}},
               o);
        }
        w(m, o, 0xdd993, frame + 0x30, 0x459d94);
      } else {
        const auto second_type = virtual_type(shape);
        matches = second_type == 4;
        if (!matches && virtual_type(shape) == 0x15)
          matches = virtual_type(r(m, shape + 0xc)) == 4;
        w(m, o, 0x1e90f2, out, matches ? (r(m, out) | 2u) : (r(m, out) & ~2u));
      }
    } else
      w(m, o, 0x1e90f2, out, r(m, out) | 2u);
  }
  if (!(r(m, r(m, physical + 8), 1) & 0x20u))
    return;
  const auto d = r(m, state + 4), velocity = out + 0xc;
  call(m, s,
       {.entry = 0xb8630,
        .entry_esp = frame - 8,
        .eax = d,
        .ecx = velocity,
        .edi = 0,
        .arguments = {0}},
       o);
  call(m, s,
       {.entry = 0x1c8430,
        .entry_esp = frame - 12,
        .eax = d,
        .ebx = velocity,
        .arguments = {0, 1}},
       o);
  const auto x = load(m, velocity), y = load(m, velocity + 4),
             z = load(m, velocity + 8);
  const auto square = _mm_add_ss(_mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)),
                                 _mm_mul_ss(z, z));
  if (_mm_comigt_ss(square, load(m, 0x46504c))) {
    call(m, s,
         {.entry = 0xbaa80, .entry_esp = frame - 8, .arguments = {d}, .dl = 0},
         o);
    call(m, s, {.entry = 0xb8250, .entry_esp = frame - 4, .eax = d}, o);
    call(m, s, {.entry = 0xbc910, .entry_esp = frame - 4, .eax = d}, o);
  }
  const auto dt = load(m, r(m, 0x518444) + 4);
  constexpr std::uint32_t sites[]{0x1e91db, 0x1e91e1, 0x1e91e7};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], frame + 0x24 + 4 * i,
      word(_mm_add_ss(_mm_mul_ss(load(m, velocity + 4 * i), dt),
                      load(m, physical + 0xe8 + 4 * i))));
  auto p = object(m, d);
  const bool updating = r(m, p + 5, 1) & 1u;
  w(m, o, 0x1e91ed, frame + 0x14, d);
  if (updating)
    call(m, s, {.entry = 0xb96a0, .entry_esp = frame - 8, .arguments = {d}}, o);
  bool valid = true;
  if (r(m, p + 0x14) == 0xffffffffu) {
    for (unsigned i = 0; i < 3; ++i) {
      const auto v = load(m, frame + 0x24 + 4 * i);
      if (!_mm_comige_ss(v, load(m, 0x46594c)) ||
          !_mm_comige_ss(load(m, 0x465490), v)) {
        valid = false;
        break;
      }
    }
  }
  if (valid) {
    constexpr std::uint32_t stores[]{0x1e924e, 0x1e9254, 0x1e925c};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, stores[i], p + 0x64 + 4 * i, r(m, frame + 0x24 + 4 * i));
    const auto child = r(m, object(m, d) + 0xd4);
    if (child != 0xffffffffu)
      call(m, s,
           {.entry = 0xb6760, .entry_esp = frame - 12, .arguments = {child, 2}},
           o);
  }
  call(m, s,
       {.entry = 0x1c83b0,
        .entry_esp = frame - 12,
        .eax = d,
        .arguments = {0, 0}},
       o);
  if (updating)
    call(m, s, {.entry = 0xb94f0, .entry_esp = frame - 12, .arguments = {d, 0}},
         o);
  call(m, s, {.entry = 0xbc910, .entry_esp = frame - 4, .eax = d}, o);
}
} // namespace halo2
