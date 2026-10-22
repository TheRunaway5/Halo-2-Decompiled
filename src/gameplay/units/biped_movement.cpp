#include "halo2/biped_phase_runtime.hpp"
#include "halo2/biped_prephysics.hpp"
#include "halo2/unit_animation_selection_runtime.hpp"
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
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::to_integer<unsigned>(b[i]) << (8 * i);
  return v;
}
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
std::uint32_t tag(const M &m, std::uint32_t d) {
  return r(m, r(m, 0x4eb114) + (d & 65535) * 16 + 8);
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
          "Required original movement FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original movement x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t call(M &m, const SourceBipedPrePhysicsServices &s,
                   SourceBipedPhaseCall q, const O &o) {
  if (q.entry == 0x1c8430 && s.object_velocity) {
    if (!q.eax || !q.ebx || q.arguments.size() != 2)
      throw std::runtime_error("Missing defined original biped velocity input");
    source_biped_object_velocity_1_1(m, *q.eax, *q.ebx, q.arguments[0],
                                     std::uint8_t(q.arguments[1]), q.entry_esp,
                                     *s.object_velocity, o);
    return 0; // The source movement caller ignores this owner's EAX.
  }
  if (s.phase_runtime &&
      (q.entry == 0x142e80 || q.entry == 0x143f60 || q.entry == 0x1d1290)) {
    FP fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error(
        "Required original biped producer FP backend unavailable");
#endif
    return source_call_biped_runtime_1_1(m, q, fp, *s.phase_runtime, o);
  }
  if (q.entry == 0x110b00 || q.entry == 0x1cf310 || q.entry == 0x1cf360 ||
      q.entry == 0x1d1bd0) {
    FP fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error(
        "Required original animation FP backend unavailable");
#endif
    return source_call_unit_animation_selection_1_1(m, q, fp, s.required_owner,
                                                    o);
  }
  if (!s.required_owner)
    throw std::runtime_error("Required original movement callee unavailable");
  return s.required_owner(m, q, o);
}
} // namespace
bool source_biped_physical_activation_1_1(const M &m, std::uint32_t d, FP fp) {
  Scope scope(fp);
  const auto p = object(m, d);
  if (r(m, p + 0x34b, 1))
    return false;
  bool result = false;
  const auto cd = r(m, p + 0xb4);
  if (cd != 0xffffffffu) {
    const auto c = r(m, r(m, 0x526248) + 0x44) + (cd & 65535) * 0xa0;
    result = r(m, r(m, c + 0x70) + 0x44, 1) && !(r(m, c + 4) & 0x40000u);
  }
  const auto state = r(m, p + 0x3dc, 1);
  if (state == 6)
    return true;
  if (state == 3)
    return result;
  const float x = f(r(m, p + 0x1b0)), y = f(r(m, p + 0x1b4)),
              z = f(r(m, p + 0x1b8)), epsilon = f(r(m, 0x46504c));
  unsigned char small = 0;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfmuls %1\n\tflds %2\n\tfmuls %2\n\tfaddp\n\tflds "
               "%3\n\tfmuls %3\n\tfaddp\n\tfabs\n\tflds %4\n\tfcomip "
               "%%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
               : "=qm"(small)
               : "m"(x), "m"(y), "m"(z), "m"(epsilon)
               : "st", "st(1)", "cc", "memory");
#endif
  if (!small || state == 4 || state == 5 ||
      source_biped_airborne_restriction_1_1(m, d, fp))
    return true;
  return result;
}
std::uint32_t source_biped_physical_fraction_1_1(M &m, std::uint32_t d,
                                                 std::uint32_t esp, FP fp,
                                                 const O &o) {
  Scope scope(fp);
  const auto p = object(m, d), scratch = esp - 4;
  w(m, o, 0xdee3c, scratch, r(m, tag(m, r(m, p)) + 0x250));
  const auto actor = r(m, p + 0x12c);
  if (actor != 0xffffffffu) {
    const auto a = r(m, r(m, 0x4fcdc8) + 0x44) + (actor & 65535) * 0x888;
    const auto pd = r(m, a + 0x858);
    if (pd != 0xffffffffu && !r(m, a + 7, 1)) {
      const auto prop = r(m, r(m, 0x509bec) + 0x44) + (pd & 65535) * 0xd4;
      if ((r(m, prop + 0x30, 2) == 0 || r(m, prop + 0x30, 2) == 1) &&
          _mm_comigt_ss(load(m, prop + 0x60), load(m, 0x465048)))
        w(m, o, 0x260d4c, scratch, r(m, prop + 0x60));
    }
  }
  auto value = _mm_mul_ss(load(m, r(m, 0x518444) + 4), load(m, scratch));
  const auto one = load(m, 0x465030);
  if (r(m, p + 0x134) & 0x800000u) {
    const auto limit = _mm_sub_ss(one, load(m, p + 0x2c4));
    if (_mm_comigt_ss(value, limit))
      value = limit;
  } else {
    const auto limit = load(m, p + 0x2c4);
    if (_mm_comigt_ss(value, limit))
      value = limit;
    value = _mm_sub_ss(_mm_setzero_ps(), value);
  }
  if (r(m, p + 0x3dc, 1) == 6 && r(m, p + 0x3f8, 1) &&
      (r(m, p + 0x348, 2) & 0x2000u)) {
    value = _mm_sub_ss(one, load(m, p + 0x2c4));
    if (!_mm_comigt_ss(one, value))
      value = one;
  }
  return word(value);
}
void source_biped_movement_action_1_1(
    M &m, std::uint32_t d, std::uint32_t modeout, std::uint32_t actionout,
    std::uint32_t byteout, std::uint32_t mode, std::uint8_t byte,
    std::uint32_t esp, const SourceBipedPrePhysicsServices &s, const O &o) {
  const auto p = object(m, d);
  if (sx(m, p + 0x34c) == 1)
    return;
  const auto state = r(m, p + 0x3dc, 1);
  if (state == 5)
    w(m, o, 0xe44f9, modeout, 0x50000cb);
  else if (state == 2 && (call(m, s,
                               {.entry = 0x1cf310,
                                .entry_esp = esp - 12,
                                .eax = p + std::uint32_t(sx(m, p + 0x12a)),
                                .esi = 0x6000542},
                               o) &
                          255u))
    w(m, o, 0xe451a, modeout, 0x6000542);
  if (r(m, actionout) != 0x400000cu)
    return;
  constexpr std::uint32_t values[]{0x400000c, 0xa000017, 0x9000016, 0xa000014,
                                   0x9000015, 0xd000035, 0xc000034, 0xd000032,
                                   0xc000033, 0x800001c, 0xa00001d};
  if (mode >= 1 && mode <= 11)
    w(m, o, 0xe4537 + (mode - 1) * 8, actionout, values[mode - 1]);
  if (mode)
    w(m, o, 0xe4599, byteout, byte, 1);
}
std::uint8_t
source_update_biped_movement_1_1(M &m, std::uint32_t d, std::uint32_t action,
                                 std::uint32_t physical, std::uint32_t esp,
                                 FP fp, const SourceBipedPrePhysicsServices &s,
                                 const O &o) {
  Scope scope(fp);
  const auto frame = esp - 0xe0u, p = object(m, d),
             definition = tag(m, r(m, p));
  const auto animation = p + std::uint32_t(sx(m, p + 0x12a)),
             extension = p + std::uint32_t(sx(m, p + 0x33e));
  w(m, o, 0xe3b72, p + 0x348, r(m, p + 0x348, 1) & 0xefu, 1);
  bool changed = source_biped_physical_activation_1_1(m, d, fp);
  if (changed && (r(m, p + 0xc1, 1) & 1u))
    call(m, s,
         {.entry = 0xbaa80, .entry_esp = frame - 8, .arguments = {d}, .dl = 0},
         o);
  if ((r(m, p + 0xc1, 1) & 1u) && (r(m, p + 0x10a, 1) & 4u) &&
      ((r(m, animation + 0x6c, 1) & 1u) || (r(m, extension, 1) & 4u)))
    return changed;
  source_step_biped_physical_1_1(m, p + 0x3dc, physical, frame + 0x30,
                                 frame - 12, fp, s, o);
  constexpr std::uint32_t forward_sites[]{0xe3bfc, 0xe3bfe, 0xe3c05},
      packet_sites[]{0xe3c0e, 0xe3c14, 0xe3c17},
      up_sites[]{0xe3c23, 0xe3c29, 0xe3c30};
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, forward_sites[i], p + 0x70 + 4 * i, r(m, frame + 0x48 + 4 * i));
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, packet_sites[i], physical + 0xf4 + 4 * i,
      r(m, frame + 0x48 + 4 * i));
  for (unsigned i = 0; i < 3; ++i)
    w(m, o, up_sites[i], p + 0x7c + 4 * i, r(m, frame + 0x54 + 4 * i));
  source_update_biped_physical_outputs_1_1(m, physical, p + 0x3dc, p + 0x7c,
                                           p + 0x70, frame - 8, fp, s, o);
  source_biped_movement_action_1_1(
      m, d, action, action + 4, action + 8, r(m, frame + 0x34),
      std::uint8_t(r(m, frame + 0x38)), frame - 16, s, o);
  auto state = r(m, p + 0x3dc, 1);
  if ((state == 4 || state == 5) && r(m, frame + 0x84, 1))
    call(m, s,
         {.entry = 0xb8490,
          .entry_esp = frame - 16,
          .eax = frame + 0x88,
          .ecx = 0,
          .edx = 0,
          .arguments = {d, 0, 0}},
         o);
  const auto requested = r(m, action + 4);
  if (requested == 0x400000cu && r(m, p + 0x34a, 1) == 1) {
    w(m, o, r(m, p + 0x3a9, 1) ? 0xe3cbb : 0xe3cc4, p + 0x3a8,
      r(m, p + 0x3a9, 1) ? 0 : r(m, p + 0x3a8, 1) + 1, 1);
    const auto elapsed =
        _mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), r(m, p + 0x3a8, 1)),
                   load(m, r(m, 0x518444) + 4));
    if (_mm_comigt_ss(load(m, 0x4650d8), elapsed)) {
      w(m, o, 0xe3cf9, action + 4, r(m, animation + 0x7c));
      call(m, s,
           {.entry = 0x1cf360,
            .entry_esp = frame - 12,
            .eax = animation,
            .edx = 0x400000c,
            .esi = frame + 0x24,
            .arguments = {0x7000101, 0x7000101}},
           o);
      if (r(m, frame + 0x26, 2) != 65535)
        call(m, s,
             {.entry = 0x1d1bd0,
              .entry_esp = frame - 8,
              .eax = animation,
              .arguments = {r(m, frame + 0x24)}},
             o);
    }
  }
  w(m, o, 0xe3d32, p + 0x3a9, source_biped_action_class_1_1(requested), 1);
  if (r(m, animation + 0x68) != 0xffffffffu && r(m, animation) != 0xffffffffu &&
      sx(m, animation + 6) != -1 && r(m, animation + 0x7c) != 0xe0000c2u &&
      (d == 0xffffffffu || !r(m, object(m, d) + 0xb3, 1))) {
    if (r(m, definition + 0x264) & 4u) {
      constexpr std::uint32_t copy[]{0xe3dae, 0xe3db9, 0xe3dbd},
          subtract[]{0xe3dd2, 0xe3de7, 0xe3dfc};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, copy[i], frame + 0x18 + 4 * i, r(m, frame + 0x3c + 4 * i));
      if (r(m, p + 0x3dc, 1) == 1)
        for (unsigned i = 0; i < 3; ++i)
          w(m, o, subtract[i], frame + 0x18 + 4 * i,
            word(_mm_sub_ss(load(m, frame + 0x18 + 4 * i),
                            load(m, frame + 0x94 + 4 * i))));
      call(m, s,
           {.entry = 0x142e80,
            .entry_esp = frame - 4,
            .eax = p + 0x7c,
            .ecx = frame + 0xac,
            .edx = p + 0x70},
           o);
      call(m, s,
           {.entry = 0x143f60,
            .entry_esp = frame - 4,
            .eax = frame + 0x18,
            .ecx = frame + 0xac,
            .edx = frame + 0xa0},
           o);
      call(m, s,
           {.entry = 0x1d1290,
            .entry_esp = frame - 8,
            .eax = d,
            .esi = animation,
            .arguments = {frame + 0xa0}},
           o);
    } else
      call(m, s,
           {.entry = 0x1d14f0,
            .entry_esp = frame - 12,
            .arguments = {d, p + 0x1b0}},
           o);
  }
  if (r(m, frame + 0x30, 1) & 4u) {
    state = r(m, p + 0x3dc, 1);
    bool reset = true;
    if (state == 6) {
      const auto e = p + std::uint32_t(sx(m, p + 0x346));
      if (r(m, e + 0x2c) != 0xffffffffu &&
          sx(m, e + 0x32, 1) <= sx(m, e + 0x33, 1))
        reset = false;
      else if (source_biped_physical_constraint_1_1(m, d))
        call(m, s, {.entry = 0xe22a0, .entry_esp = frame - 4, .ebx = d}, o);
    } else if (state == 4)
      call(m, s, {.entry = 0xe85c0, .entry_esp = frame - 4, .eax = d}, o);
    if (reset)
      call(m, s, {.entry = 0xe6f60, .entry_esp = frame - 4, .eax = d}, o);
  }
  const auto count = sx(m, p + 0x3aa, 1);
  if (count && !source_biped_physical_constraint_1_1(m, d))
    w(m, o, 0xe3ec0, p + 0x3aa, count + (count <= 0 ? 1 : -1), 1);
  const auto actual = object(m, d);
  state = r(m, actual + 0x3dc, 1);
  const bool special = state == 6 && r(m, actual + 0x3f8, 1) &&
                       (r(m, actual + 0x348, 2) & 0x2000u);
  const bool wanted = r(m, p + 0x148, 1) & 1u;
  if (r(m, p + 0x3dc, 1) != 4 && !special) {
    const bool present = r(m, p + 0x134) & 0x800000u;
    if ((!present && wanted) ||
        (present && !wanted && (r(m, frame + 0x30, 1) & 2u))) {
      w(m, o, 0xe3f66, p + 0x134,
        wanted ? r(m, p + 0x134) | 0x800000u : r(m, p + 0x134) & 0xff7fffffu);
      call(m, s,
           {.entry = 0x1c70d0, .entry_esp = frame - 12, .arguments = {d, 0}},
           o);
    }
  }
  if (!(r(m, p + 0x348, 2) & 0x4000u) && special &&
      !(r(m, p + 0x134) & 0x800000u)) {
    auto value =
        _mm_sub_ss(_mm_setzero_ps(), _mm_sub_ss(load(m, definition + 0x268),
                                                load(m, definition + 0x26c)));
    value = _mm_div_ss(
        value, _mm_mul_ss(load(m, r(m, 0x518444) + 4), load(m, 0x446df8)));
    value = _mm_add_ss(value, load(m, 0x46502c));
    const auto ticks = _mm_cvttss_si32(value);
    call(m, s,
         {.entry = 0x1c70d0, .entry_esp = frame - 12, .arguments = {d, 0}}, o);
    w(m, o, 0xe3fdd, p + 0x349, r(m, p + 0x349, 1) | 0x20u, 1);
    w(m, o, 0xe3fe4, p + 0x3aa, std::uint32_t(ticks), 1);
    w(m, o, 0xe3fea, p + 0x349, r(m, p + 0x349, 1) | 0x40u, 1);
  }
  const auto fraction =
      _mm_set_ss(f(source_biped_physical_fraction_1_1(m, d, frame - 4, fp, o)));
  w(m, o, 0xe4011, action + 9, (r(m, p + 0x134) >> 23) & 1u, 1);
  if (!(r(m, definition + 0x264) & 2u) &&
      (source_biped_airborne_restriction_1_1(m, d, fp) || special)) {
    auto v =
        _mm_sub_ss(load(m, definition + 0x268), load(m, definition + 0x26c));
    v = _mm_mul_ss(v,
                   _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, r(m, 0x518444) + 2)));
    v = _mm_mul_ss(v, fraction);
    w(m, o, 0xe4062, frame + 0x44, word(_mm_add_ss(v, load(m, frame + 0x44))));
  }
  w(m, o, 0xe4074, p + 0x349, r(m, p + 0x349, 1) | 8u, 1);
  w(m, o, (r(m, frame + 0x30, 1) & 1u) ? 0xe407f : 0xe4088, p + 0x348,
    (r(m, frame + 0x30, 1) & 1u) ? r(m, p + 0x348, 1) | 2u
                                 : r(m, p + 0x348, 1) & 0xfdu,
    1);
  if (r(m, p + 0xc0, 1) & 0x40u) {
    const auto c =
        r(m, r(m, 0x526248) + 0x44) + (r(m, p + 0xb4) & 65535) * 0xa0;
    if (!(r(m, c + 4) & 0x800u)) {
      bool moving = false;
      if (!r(m, p + 0x34b, 1)) {
        const auto x = load(m, physical + 0x138), y = load(m, physical + 0x13c),
                   z = load(m, physical + 0x140);
        moving = _mm_comigt_ss(
            _mm_add_ss(_mm_add_ss(_mm_mul_ss(x, x), _mm_mul_ss(y, y)),
                       _mm_mul_ss(z, z)),
            load(m, 0x465048));
      }
      const bool active = source_biped_physical_activation_1_1(m, d, fp);
      changed = active || moving;
      w(m, o, changed ? 0xe4137 : 0xe4131, frame + 0x24, changed, 1);
      const auto a = object(m, d);
      constexpr std::uint32_t sites[]{0xe415a, 0xe4160, 0xe4163};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, sites[i], a + 0x88 + 4 * i, r(m, frame + 0x3c + 4 * i));
      call(m, s,
           {.entry = 0x1c8430,
            .entry_esp = frame - 12,
            .eax = d,
            .ebx = frame + 0x3c,
            .arguments = {0, r(m, frame + 0x24)}},
           o);
    }
  } else {
    const auto a = object(m, d);
    const auto dt = load(m, r(m, 0x518444) + 4);
    const bool relocating = r(m, a + 4) & 0x100u;
    constexpr std::uint32_t temp[]{0xe41c0, 0xe41c6, 0xe41cc};
    for (unsigned i = 0; i < 3; ++i)
      w(m, o, temp[i], frame + 0x18 + 4 * i,
        word(_mm_add_ss(_mm_mul_ss(load(m, frame + 0x3c + 4 * i), dt),
                        load(m, p + 0x64 + 4 * i))));
    if (relocating)
      call(m, s, {.entry = 0xb96a0, .entry_esp = frame - 8, .arguments = {d}},
           o);
    bool valid = r(m, a + 0x14) != 0xffffffffu;
    if (!valid) {
      valid = true;
      for (unsigned i = 0; i < 3; ++i) {
        const auto v = load(m, frame + 0x18 + 4 * i);
        if (!_mm_comige_ss(v, load(m, 0x46594c)) ||
            !_mm_comige_ss(load(m, 0x465490), v))
          valid = false;
      }
    }
    if (valid) {
      constexpr std::uint32_t sites[]{0xe422f, 0xe4237, 0xe423a};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, sites[i], a + 0x64 + 4 * i, r(m, frame + 0x18 + 4 * i));
      const auto child = r(m, object(m, d) + 0xd4);
      if (child != 0xffffffffu)
        call(m, s,
             {.entry = 0xb6760,
              .entry_esp = frame - 12,
              .arguments = {child, 2}},
             o);
    }
    call(m, s,
         {.entry = 0x1c83b0,
          .entry_esp = frame - 12,
          .eax = d,
          .arguments = {0, 0}},
         o);
    if (relocating)
      call(m, s,
           {.entry = 0xb94f0, .entry_esp = frame - 12, .arguments = {d, 0}}, o);
    call(m, s, {.entry = 0xbc910, .entry_esp = frame - 4, .eax = d}, o);
    const auto zero = r(m, 0x46fc44), current = object(m, d);
    if (zero) {
      constexpr std::uint32_t sites[]{0xe429d, 0xe42a2, 0xe42a8};
      for (unsigned i = 0; i < 3; ++i)
        w(m, o, sites[i], current + 0x88 + 4 * i, r(m, zero + 4 * i));
    }
    call(m, s,
         {.entry = 0x1c8430,
          .entry_esp = frame - 12,
          .eax = d,
          .ebx = zero,
          .arguments = {0, 0}},
         o);
  }
  auto value = load(m, p + 0x2c4), zero = _mm_setzero_ps();
  auto bound = zero;
  bool adjust = false, lower = false;
  std::uint32_t control = 0;
  if (_mm_comieq_ss(value, zero) &&
      !(_mm_movemask_ps(_mm_cmpunord_ss(value, zero)) & 1) &&
      r(m, r(m, 0x4edf18) + 8) != 2)
    control = r(m, p + 0x148);
  if (control & 0x180u) {
    if (control & 0x80u) {
      value =
          _mm_sub_ss(load(m, p + 0x3a4), _mm_mul_ss(load(m, r(m, 0x518444) + 4),
                                                    load(m, 0x465080)));
      bound = load(m, 0x46503c);
      lower = true;
    } else {
      value =
          _mm_add_ss(_mm_mul_ss(load(m, r(m, 0x518444) + 4), load(m, 0x465080)),
                     load(m, p + 0x3a4));
      bound = load(m, 0x465030);
    }
    adjust = true;
  } else {
    value = load(m, p + 0x3a4);
    if (_mm_comigt_ss(value, zero)) {
      value = _mm_sub_ss(
          value, _mm_mul_ss(load(m, r(m, 0x518444) + 4), load(m, 0x46506c)));
      adjust = true;
      lower = true;
    } else if (_mm_comigt_ss(zero, value)) {
      value = _mm_add_ss(
          _mm_mul_ss(load(m, r(m, 0x518444) + 4), load(m, 0x46506c)), value);
      adjust = true;
    }
  }
  if (adjust) {
    w(m, o, 0xe437c, p + 0x3a4, word(value));
    const bool clamp =
        lower ? _mm_comige_ss(bound, value) : _mm_comige_ss(value, bound);
    if (clamp)
      w(m, o, 0xe4386, p + 0x3a4, word(bound));
  }
  return 1;
}
} // namespace halo2
