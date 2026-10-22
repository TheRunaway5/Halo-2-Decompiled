#include "halo2/biped_death_transition.hpp"
#include <bit>
#include <cfenv>
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
  std::uint32_t x = 0;
  for (unsigned i = 0; i < n; ++i)
    x |= std::to_integer<std::uint32_t>(b[i]) << (i * 8);
  return x;
}
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, std::uint32_t x,
       unsigned n = 4) {
  SourcePhysicsContactRecordStore s{site, p, {}, {}};
  s.before.resize(n);
  s.written.resize(n);
  m.read(p, s.before);
  for (unsigned i = 0; i < n; ++i)
    s.written[i] = std::byte(x >> (8 * i));
  m.write(p, s.written);
  if (o)
    o(s);
}
template <class F, class... A> void need(const F &f, const char *s, A... a) {
  if (!f)
    throw std::runtime_error(s);
  f(a...);
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12 + 8);
}
__m128 at(const M &m, std::uint32_t p) {
  return _mm_set_ss(std::bit_cast<float>(r(m, p)));
}
std::uint32_t bits(__m128 v) {
  return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v));
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(FP fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 63u) != 63u || (fp.mxcsr & 0x1f80u) != 0x1f80u)
      throw std::runtime_error(
          "Required original death transition FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original death transition x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
} // namespace
void source_initialize_biped_ground_state_1_1(M &m, std::uint32_t p, const O &o) {
  const auto zero = r(m, 0x46fc44);
  w(m, o, 0x1f3daa, p + 0xc, 0xffffffffu);
  constexpr std::array zero_sites{0x1f3db5u, 0x1f3dbau, 0x1f3dc0u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, zero_sites[i], p + i * 4, r(m, zero + i * 4));
  const auto up = r(m, 0x46fc50);
  w(m, o, 0x1f3dc9, p + 0x62, 0, 1);
  constexpr std::array up_sites{0x1f3dd2u, 0x1f3dd7u, 0x1f3dddu};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, up_sites[i], p + 0x64 + i * 4, r(m, up + i * 4));
  const auto support = r(m, 0x484de0, 2), identity = r(m, 0x46fc70);
  w(m, o, 0x1f3ded, p + 0x60, support, 2);
  constexpr std::array none_sites{0x1f3df1u, 0x1f3df4u, 0x1f3df7u, 0x1f3dfau};
  constexpr std::array none_offsets{0x58u, 0x5cu, 0x1cu, 0x20u};
  for (unsigned i = 0; i < 4; ++i)
    w(m, o, none_sites[i], p + none_offsets[i], 0xffffffffu);
  w(m, o, 0x1f3e05, p + 0x70, 0);
  for (unsigned i = 0; i < 13; ++i)
    w(m, o, 0x1f3e0a, p + 0x24 + i * 4, r(m, identity + i * 4));
}
void source_initialize_biped_dead_state_1_1(M &m, std::uint32_t p, const O &o) {
  const auto up = r(m, 0x46fc50);
  constexpr std::array up_sites{0x1eff6du, 0x1eff72u, 0x1eff78u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, up_sites[i], p + i * 4, r(m, up + i * 4));
  const auto zero = r(m, 0x46fc44);
  constexpr std::array byte_offsets{0xdu,  0xcu,  0xeu, 0xfu,
                                    0x10u, 0x11u, 0x13u};
  for (unsigned i = 0; i < byte_offsets.size(); ++i)
    w(m, o, 0x1eff84 + i * 3, p + byte_offsets[i], 0, 1);
  w(m, o, 0x1eff9c, p + 0x14, 0xffffffffu);
  w(m, o, 0x1eff9f, p + 0x18, 0xffffffffu);
  w(m, o, 0x1effa9, p + 0x50, r(m, zero));
  w(m, o, 0x1effae, p + 0x54, r(m, zero + 4));
  const auto z = r(m, zero + 8);
  w(m, o, 0x1effb4, p + 0x12, 0, 1);
  w(m, o, 0x1effb7, p + 0x58, z);
  w(m, o, 0x1effba, p + 0x60, 0xffffffffu);
  w(m, o, 0x1effbd, p + 0x64, 0xffffffffu);
  w(m, o, 0x1effc5, p + 0x68, r(m, zero));
  w(m, o, 0x1effca, p + 0x6c, r(m, zero + 4));
  const auto tail = r(m, zero + 8);
  w(m, o, 0x1effd1, p + 0x5c, 0xffffffffu);
  w(m, o, 0x1effd6, p + 0x70, tail);
}
void source_biped_physical_state_1_1(
    M &m, std::uint32_t p, std::uint8_t destination, std::uint32_t anchor,
    FP fp, const SourceBipedPhysicalStateServices &services, const O &o) {
  const auto before = r(m, p, 1);
  if (before > 6 || destination < 1 || destination > 6)
    throw std::runtime_error(
        "Required original physical state dispatch outside recovered0..6 bank");
  if (before == 4 || before == 5)
    need(services.leave,
         "Required original1FFD10 physical state exit unavailable", r(m, p + 4),
         p + 0x10);
  w(m, o, 0x1e8e8b, p, destination, 1);
  if (destination == 1)
    source_initialize_biped_ground_state_1_1(m, p + 0x10, o);
  else if (destination == 3)
    source_initialize_biped_dead_state_1_1(m, p + 0x10, o);
  else if (destination == 2) {
    const auto zero = r(m, 0x46fc44);
    w(m, o, 0x1e8ea5, p + 0x10, 0);
    constexpr std::array sites{0x1e8eb1u, 0x1e8eb6u, 0x1e8ebcu};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, sites[i], p + 0x14 + i * 4, r(m, zero + i * 4));
  } else {
    if (destination == 4 || destination == 5)
      source_biped_world_position_1_1(m, r(m, p + 4), anchor, fp, o);
    need(services.initialize,
         "Required original physical state packet constructor unavailable",
         destination, p, anchor);
  }
  const auto after = r(m, p, 1);
  if (before != after && (before == 3 || after == 3 || before == 4 ||
                          before == 5 || after == 4 || after == 5))
    source_rebuild_object_physics_1_1(m, r(m, p + 4), 0, services.rebuild, o);
}
void source_transition_biped_dead_1_1(
    M &m, std::uint32_t datum, std::uint32_t anchor, FP fp,
    const SourceBipedPhysicalStateServices &services, const O &o) {
  const auto unit = object(m, datum);
  source_biped_physical_state_1_1(m, unit + 0x3dc, 3, anchor, fp, services, o);
  if (source_biped_airborne_restriction_1_1(m, datum, fp)) {
    w(m, o, 0xe6df3, unit + 0x3f9, 0, 1);
    w(m, o, 0xe6df9, unit + 0x3f8, 0, 1);
  }
}
std::uint8_t source_biped_default_physical_state_1_1(const M &m,
                                                   std::uint32_t d) {
  const auto p = object(m, d);
  if (r(m, p + 0x10a, 1) & 4u)
    return 3;
  const auto definition =
      r(m, r(m, 0x4eb114) + (r(m, p) & 0xffffu) * 16 + 8);
  return std::uint8_t(1 + ((r(m, definition + 0x264) >> 4) & 1u));
}
void source_reset_biped_physical_state_1_1(
    M &m, std::uint32_t d, std::uint32_t anchor, FP fp,
    const SourceBipedPhysicalStateServices &services, const O &o) {
  const auto state = source_biped_default_physical_state_1_1(m, d);
  if (state == 3) {
    source_transition_biped_dead_1_1(m, d, anchor, fp, services, o);
    return;
  }
  const auto p = object(m, d);
  source_biped_physical_state_1_1(m, p + 0x3dc, state, anchor, fp, services, o);
  if (state == 2) {
    w(m, o, 0xe6da7, p + 0x34c, 0xffffu, 2);
    w(m, o, 0xe6db0, p + 0x34e, 0, 1);
    w(m, o, 0xe6db6, p + 0x34f, 0, 1);
  }
}
void source_construct_biped_physical_state_1_1(
    M &m, std::uint32_t d, std::uint32_t anchor, FP fp,
    const SourceBipedPhysicalStateServices &services, const O &o) {
  const auto p = object(m, d);
  const auto definition =
      r(m, r(m, 0x4eb114) + (r(m, p) & 0xffffu) * 16 + 8);
  w(m, o, 0xdfb41, p + 0x348, 0, 2);
  constexpr std::array first_sites{0xdfb48u, 0xdfb4eu, 0xdfb54u, 0xdfb5au};
  constexpr std::array first_offsets{0x34au, 0x34bu, 0x34eu, 0x34fu};
  for (unsigned i = 0; i < 4; ++i)
    w(m, o, first_sites[i], p + first_offsets[i], 0, 1);
  w(m, o, 0xdfb60, p + 0x34c, 0xffffu, 2);
  constexpr std::array none_sites{0xdfb67u, 0xdfb6du, 0xdfb73u};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, none_sites[i], p + 0x38c + i * 4, 0xffffffffu);
  constexpr std::array byte_sites{0xdfb79u, 0xdfb7fu, 0xdfb85u, 0xdfb8bu,
                                  0xdfb91u, 0xdfb98u, 0xdfb9eu};
  for (unsigned i = 0; i < 7; ++i)
    w(m, o, byte_sites[i], p + 0x398 + i, i == 4 ? 0x7fu : 0u, 1);
  w(m, o, 0xdfba4, p + 0x2c4, 0);
  w(m, o, 0xdfbac, p + 0x3a0, 0);
  w(m, o, 0xdfbb4, p + 0x3a4, 0);
  source_biped_world_position_1_1(m, d, p + 0x368, fp, o);
  const auto current = object(m, d);
  constexpr std::array reset_sites{0xdfbd6u, 0xdfbdcu, 0xdfbe2u,
                                   0xdfbe8u, 0xdfbeeu, 0xdfbf4u};
  constexpr std::array reset_offsets{0x354u, 0x358u, 0x35cu,
                                     0x360u, 0x364u, 0x350u};
  for (unsigned i = 0; i < 6; ++i)
    w(m, o, reset_sites[i], current + reset_offsets[i], 0xffffffffu);
  const bool authored = r(m, definition + 0x1f0) & 2u;
  w(m, o, authored ? 0xdfc06 : 0xdfc0f, p + 0x348,
    authored ? r(m, p + 0x348, 1) | 8u : r(m, p + 0x348, 1) & 0xf7u, 1);
  w(m, o, 0xdfc1a, p + 0x3d8, 0xffffffffu);
  w(m, o, 0xdfc20, p + 0x3e0, d);
  w(m, o, 0xdfc26, p + 0x3dc, 0, 1);
  w(m, o, 0xdfc2c, p + 0x3e4, 0xffffffffu);
  w(m, o, 0xdfc32, p + 0x3e8, 0xffffffffu);
  source_reset_biped_physical_state_1_1(m, d, anchor, fp, services, o);
}
void source_biped_death_transition_1_1(
    M &m, const SourceBipedDeathTransitionRequest &q, std::uint32_t entry,
    FP fp, const SourceBipedDeathTransitionServices &services, const O &o) {
  Scope scope(fp);
  const auto unit = object(m, q.object), frame = entry - 0x18,
             vector = frame + 0xc;
  if (r(m, unit + 0x3dc, 1) != 3) {
    if (r(m, unit + 0x14) == 0xffffffffu &&
        source_biped_airborne_restriction_1_1(m, q.object, fp)) {
      need(services.orientation,
           "Required originalBB0C0 death clearance orientation unavailable",
           q.object, vector);
      const auto up = r(m, 0x46fc50);
      const auto yz = _mm_add_ss(_mm_mul_ss(at(m, up + 4), at(m, vector + 4)),
                                 _mm_mul_ss(at(m, up + 8), at(m, vector + 8)));
      const auto projection =
          _mm_add_ss(yz, _mm_mul_ss(at(m, up), at(m, vector)));
      if (_mm_comigt_ss(projection, at(m, 0x4650e0))) {
        const auto definition =
            r(m, r(m, 0x4eb114) + (r(m, unit) & 0xffffu) * 16 + 8);
        const auto two = at(m, 0x46507c);
        const auto clearance =
            _mm_sub_ss(at(m, definition + 0x26c),
                       _mm_mul_ss(at(m, definition + 0x270), two));
        if (_mm_comigt_ss(clearance, at(m, 0x465048))) {
          const auto cap = _mm_mul_ss(at(m, 0x486564), two);
          const auto amount = _mm_comigt_ss(clearance, cap) ? cap : clearance;
          source_biped_world_position_1_1(m, q.object, vector, fp, o);
          w(m, o, 0xe6154, vector + 8,
            bits(_mm_add_ss(at(m, vector + 8), amount)));
          need(services.relocate,
               "Required originalB8490 death clearance relocation unavailable",
               q.object, vector);
        }
      }
    }
    source_transition_biped_dead_1_1(m, q.object, vector, fp, services.physical,
                                     o);
    if (q.direction3 && q.source_object != 0xffffffffu) {
      const auto clock = r(m, 0x518444);
      w(m, o, 0xe6178, unit + 0x450, q.source_object);
      w(m, o, 0xe6181, unit + 0x44c, r(m, clock + 8));
      constexpr std::array sites{0xe618fu, 0xe6194u, 0xe619au};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, sites[i], unit + 0x454 + i * 4, r(m, q.direction3 + i * 4));
    }
  }
  if (q.forced_animation) {
    const auto manager = unit + std::int32_t(std::bit_cast<std::int16_t>(
                                    std::uint16_t(r(m, unit + 0x12a, 2))));
    need(services.forced_animation,
         "Required original110BF0 forced death animation owner unavailable",
         q.object);
    need(services.update_animation,
         "Required original1D1940 forced death animation update unavailable",
         q.object);
    w(m, o, 0xe61d6, manager + 0x6c, r(m, manager + 0x6c, 1) | 1u, 1);
    w(m, o, 0xe61da, manager + 0x65, 0, 1);
    w(m, o, 0xe61de, manager + 0x64, 0, 1);
    w(m, o, 0xe61e2, manager + 0x67, 0, 1);
    w(m, o, 0xe61e6, unit + 0x349, r(m, unit + 0x349, 1) | 4u, 1);
  }
}
} // namespace halo2
