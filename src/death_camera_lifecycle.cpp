#include "halo2/death_camera_lifecycle.hpp"
#include <bit>
#include <cfenv>
#include <stdexcept>
#include <xmmintrin.h>

namespace halo2 {
namespace {
using M = SourcePhysicsContactMemory;
using O = SourcePhysicsContactRecordObserver;
constexpr auto none = 0xffffffffu;
std::uint32_t r(const M &m, std::uint32_t p, unsigned n = 4) {
  std::array<std::byte, 4> b{};
  m.read(p, std::span(b).first(n));
  std::uint32_t v{};
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<std::uint32_t>(b[i]) << (8 * i);
  return v;
}
std::int32_t si(std::uint32_t v) { return std::bit_cast<std::int32_t>(v); }
std::int32_t sh(std::uint32_t v) {
  return std::bit_cast<std::int16_t>(std::uint16_t(v));
}
float f(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t u(float v) { return std::bit_cast<std::uint32_t>(v); }
void w(M &m, const O &o, std::uint32_t site, std::uint32_t p, unsigned n,
       std::uint32_t v) {
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
std::uint32_t view_base(std::int32_t view) {
  return 0x4f0214u + std::uint32_t(view) * 0x140u;
}
std::uint32_t player_for_view(const M &m, std::int32_t view) {
  return view == -1 ? none
                    : r(m, r(m, 0x4f01f0) + std::uint32_t(view) * 4 + 12);
}
std::uint32_t player(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4f01f4) + 0x44) + (d & 0xffffu) * 0x21c;
}
std::uint32_t object(const M &m, std::uint32_t d) {
  return r(m, r(m, r(m, 0x4e78d0) + 0x44) + (d & 0xffffu) * 12 + 8);
}
std::uint32_t valid_unit(const M &m, std::uint32_t d) {
  if (d == none)
    return 0;
  const auto pool = r(m, 0x4e78d0), index = d & 0xffffu;
  if (std::int32_t(index) >= si(r(m, pool + 0x38)))
    return 0;
  const auto header = r(m, pool + 0x44) + index * r(m, pool + 0x24);
  const auto salt = sh(r(m, header, 2));
  if (!salt || salt != sh(d >> 16))
    return 0;
  if (!((1u << (r(m, header + 3, 1) & 31u)) & 3u))
    return 0;
  return r(m, header + 8);
}
std::uint32_t animation(const M &m, std::uint32_t p) {
  return p + std::uint32_t(sh(r(m, p + 0x12a, 2)));
}
std::uint32_t animation_state(const M &m, std::uint32_t p) {
  const auto a = animation(m, p);
  return r(m, a + 0x68) != none && r(m, a) != none && r(m, a + 6, 2) != 0xffffu
             ? r(m, a + 0x7c)
             : none;
}
struct Scope {
  unsigned mx;
  std::fenv_t env;
  explicit Scope(SourcePhysicsFactoryFloatingPoint fp) : mx(_mm_getcsr()) {
    if ((fp.x87_control_word & 0x3f) != 0x3f || (fp.mxcsr & 0x1f80) != 0x1f80)
      throw std::runtime_error(
          "Required original camera lifecycle FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original camera lifecycle x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
float random_value(std::int32_t value, float scale, float multiplier,
                   float bias, bool negate) {
  float out{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("fildl %1\n\tfmuls %2\n\tfmuls %3\n\tfadds %4\n\t"
               "cmpb $0, %5\n\tje 1f\n\tfchs\n\t1: fstps %0"
               : "=m"(out)
               : "m"(value), "m"(scale), "m"(multiplier), "m"(bias),
                 "qm"(negate)
               : "st", "cc", "memory");
#endif
  return out;
}
float random_yaw(std::int32_t value, float scale, float multiplier) {
  float out{};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  // 23FBB6 has no FADD; even a zero add would alter downward-rounded -0.
  asm volatile("fildl %1\n\tfmuls %2\n\tfmuls %3\n\tfstps %0"
               : "=m"(out)
               : "m"(value), "m"(scale), "m"(multiplier)
               : "st", "memory");
#endif
  return out;
}
} // namespace

SourceUnitCameraMode source_unit_camera_mode_1_1(const M &m, std::uint32_t d) {
  const auto p = valid_unit(m, d);
  if (!p || r(m, p + 0x14) == none)
    return {};
  const auto parent = object(m, r(m, p + 0x14)), state = animation_state(m, p);
  bool seat_camera = false;
  if ((1u << (r(m, parent + 0xaa, 1) & 31u)) & 3u) {
    const auto def = r(m, r(m, 0x4eb114) + (r(m, parent) & 0xffffu) * 16 + 8);
    const auto seat =
        r(m, def + 0x1cc) + std::uint32_t(sh(r(m, p + 0x1fc, 2))) * 0xb0;
    const auto flags = r(m, seat);
    seat_camera = (flags & 0x10u) != 0;
    if (flags & 0x40u) {
      const auto current = r(m, animation(m, p) + 0x7c);
      for (auto q = 0x46e8d0u; q < 0x46e8e4u; q += 4)
        if (r(m, q) == current)
          return {true, 1};
      if (state == 0x400004au)
        return {true, 3};
    }
  }
  return {seat_camera, 2};
}

void source_set_camera_mode_1_1(M &m, std::int32_t view, std::uint32_t entry,
                                bool transition, const O &o) {
  const auto base = view_base(view);
  const auto one = r(m, 0x465030);
  w(m, o, 0x15753a, base + 0xc, 4, entry);
  w(m, o, 0x15753d, base + 0x10c, 4, one);
  w(m, o, 0x157545, base + 0x108, 1, 0);
  if (transition && r(m, 0x4e7920)) {
    const auto controls = r(m, r(m, 0x4e791c) + 0xec);
    const auto duration = r(m, controls + (entry == 0x23ecb0   ? 0x10
                                           : entry == 0x23e210 ? 12
                                                               : 8));
    // UCOMISS/TEST AH44 keeps unordered durations as the original does.
    if (f(duration) != f(r(m, 0x465048)))
      w(m, o, 0x157592, base + 4, 4, duration);
  }
}

void source_restore_unit_camera_1_1(M &m, std::int32_t view, bool force,
                                    const O &o) {
  const auto base = view_base(view), own = player_for_view(m, view);
  const auto unit = own == none ? none : r(m, player(m, own) + 0x2c);
  const auto mode = source_unit_camera_mode_1_1(m, unit);
  if (!force && r(m, base + 0x58, 2) == mode.mode)
    return;
  if (mode.seat_camera) {
    if (force || r(m, base + 12) == 0x23e210u) {
      constexpr std::array sites{0x1570f5u, 0x1570f8u, 0x1570fbu, 0x1570feu};
      for (unsigned i = 0; i < 4; ++i)
        w(m, o, sites[i], base + 0x10 + i, 1, 0);
      w(m, o, 0x157101, base + 0x16, 2, 0);
      w(m, o, 0x157105, base + 0x14, 1, 0);
      w(m, o, 0x15710b, base + 0x24, 4, 0);
      w(m, o, 0x157110, base + 0x20, 4, 0);
      w(m, o, 0x15711f, base + 0x18, 4, none);
      w(m, o, 0x157126, base + 0x1c, 2, 0xffff);
      w(m, o, 0x157132, base + 0x28, 4, r(m, 0x465030));
      source_set_camera_mode_1_1(m, view, 0x23ecb0, !force, o);
      w(m, o, 0x15713c, base + 0x58, 2, mode.mode);
      return;
    }
  } else if (force || r(m, base + 12) == 0x23ecb0u) {
    w(m, o, 0x157160, base + 0x10, 4, 0);
    w(m, o, 0x157165, base + 0x14, 4, 0);
    source_set_camera_mode_1_1(m, view, 0x23e210, !force, o);
  }
  w(m, o, 0x157172, base + 0x58, 2, mode.mode);
}

void source_replace_death_camera_target_1_1(
    M &m, std::uint32_t camera, const SourceDeathCameraLifecycleServices &s,
    const O &o) {
  const auto game = r(m, 0x4edf18);
  const bool multiplayer = r(m, game + 8) == 2;
  bool team_only;
  if (multiplayer) {
    const auto engine = r(m, r(m, 0x4f10b8) + 0xc14);
    team_only = r(m, 0x566084 + engine * 4) && (r(m, game + 0x184, 1) & 1u);
  } else {
    if (!s.non_multiplayer_team_restriction)
      throw std::runtime_error(
          "Required original23FCA0 camera restriction owner unavailable");
    team_only = s.non_multiplayer_team_restriction(r(m, camera + 0x2c));
  }
  const auto players = source_death_camera_players(m, r(m, 0x4f01f4));
  const auto next = source_death_camera_next_player(
      players, r(m, camera + 0x2c), r(m, camera + 0x30), team_only);
  w(m, o, 0x23fef9, camera + 0x30, 4, next);
  const auto target = next == none ? none : r(m, player(m, next) + 0x2c);
  if (target != r(m, camera + 0x34) && target != none) {
    w(m, o, 0x23ff28, camera + 0x28, 4, r(m, 0x465044));
    w(m, o, 0x23ff2d, camera + 0x34, 4, target);
    w(m, o, 0x23ff30, camera + 0x3c, 1, 0);
  }
  w(m, o, 0x23ff44, camera + 0x38, 4, r(m, multiplayer ? 0x465068 : 0x465044));
}

void source_construct_death_camera_1_1(
    M &m, std::uint32_t camera, std::int32_t view, std::int32_t watch_mode,
    SourcePhysicsFactoryFloatingPoint fp,
    const SourceDeathCameraLifecycleServices &s, const O &o) {
  Scope scope(fp);
  const auto observer = view != -1 && r(m, 0x46fb64, 2) != 0xffffu
                            ? 0x4f125cu + std::uint32_t(view) * 0x358u
                            : 0;
  // A missing original observer is a required backing error, never a made-up
  // initial camera position. Preserve the actual original read of address0.
  constexpr std::array sites{0x23fb2fu, 0x23fb34u, 0x23fb3au};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, sites[i], camera + i * 4, 4, r(m, observer + i * 4));
  w(m, o, 0x23fb42, camera + 0x24, 4, r(m, 0x5563a4));
  const auto seed = r(m, 0x4ee9d8) + 4;
  const auto draw = [&](std::uint32_t site) {
    const auto value = r(m, seed) * 0x19660du + 0x3c6ef35fu;
    w(m, o, site, seed, 4, value);
    return std::int32_t(value >> 16);
  };
  const auto scale = f(r(m, 0x4650c0));
  const auto first = draw(0x23fb56);
  w(m, o, 0x23fb80, camera + 0x20, 4,
    u(random_value(first, scale, f(r(m, 0x465080)), f(r(m, 0x46507c)), false)));
  const auto second = draw(0x23fb92);
  w(m, o, 0x23fbb6, camera + 0x18, 4,
    u(random_yaw(second, scale, f(r(m, 0x4457bc)))));
  const auto third = draw(0x23fbc8);
  w(m, o, 0x23fbfb, camera + 0x28, 4, r(m, 0x465044));
  w(m, o, 0x23fc08, camera + 0x1c, 4,
    u(random_value(third, scale, f(r(m, 0x465640)), f(r(m, 0x46563c)), true)));
  const auto watch = watch_mode != -1                ? 0x452044u
                     : r(m, r(m, 0x4edf18) + 8) == 2 ? 0x465068u
                                                     : 0x465044u;
  w(m, o, 0x23fc31, camera + 0x38, 4, r(m, watch));
  const auto own = player_for_view(m, view);
  w(m, o, 0x23fc44, camera + 0x2c, 4, own);
  w(m, o, 0x23fc47, camera + 0x30, 4, own);
  w(m, o, 0x23fc4a, camera + 0x34, 4, none);
  w(m, o, 0x23fc51, camera + 0x3c, 1, 0);
  w(m, o, 0x23fc55, camera + 0x3d, 1, std::uint32_t(view));
  if (own == none)
    return;
  const auto row = player(m, own), current = r(m, row + 0x2c);
  if (current != none)
    w(m, o, 0x23fc79, camera + 0x34, 4, current);
  else {
    const auto previous = r(m, row + 0x30);
    if (previous != none)
      w(m, o, 0x23fc8a, camera + 0x34, 4, previous);
    else
      source_replace_death_camera_target_1_1(m, camera, s, o);
  }
}

void source_select_death_camera_1_1(M &m, std::int32_t view, bool reset,
                                    SourcePhysicsFactoryFloatingPoint fp,
                                    const SourceDeathCameraLifecycleServices &s,
                                    const O &o) {
  Scope scope(fp);
  const auto base = view_base(view);
  if (reset) {
    w(m, o, 0x1573c7, base + 0x10, 4, 0);
    w(m, o, 0x1573cc, base + 0x14, 4, 0);
    w(m, o, 0x1573da, base + 12, 4, 0x23e210);
    w(m, o, 0x1573e1, base + 0x10c, 4, r(m, 0x465030));
    w(m, o, 0x1573e9, base + 0x108, 1, 0);
    return;
  }
  const auto row = player(m, player_for_view(m, view));
  bool dead = r(m, row + 0x2c) == none;
  if (r(m, r(m, 0x4edf18) + 8) != 2)
    dead = dead && !(r(m, row + 2, 1) & 8u);
  if (r(m, r(m, 0x4f0204), 1) ||
      !(f(r(m, r(m, 0x518444) + 12)) > f(r(m, 0x465048))))
    return;
  source_restore_unit_camera_1_1(m, view, false, o);
  if (dead) {
    if (r(m, base + 12) != 0x23ff50u) {
      source_construct_death_camera_1_1(m, base + 0x10, view, -1, fp, s, o);
      source_set_camera_mode_1_1(m, view, 0x23ff50, true, o);
    }
  } else if (r(m, base + 12) == 0x23ff50u)
    source_restore_unit_camera_1_1(m, view, true, o);
}
} // namespace halo2
