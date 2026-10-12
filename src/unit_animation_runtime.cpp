#include "halo2/unit_animation_runtime.hpp"
#include "halo2/unit_animation_selection.hpp"
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
  return r(m, r(m, 0x4eb114) + (d & 65535u) * 16 + 8);
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
          "Required original animation FP exception owner unavailable");
    std::fegetenv(&env);
    _mm_setcsr(fp.mxcsr);
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fldcw %0" ::"m"(fp.x87_control_word) : "memory");
#else
    throw std::runtime_error(
        "Required original animation x87 backend unavailable");
#endif
  }
  ~Scope() {
    std::fesetenv(&env);
    _mm_setcsr(mx);
  }
};
std::uint32_t call(M &m, const SourceRuntimeOwner &owner, SourceRuntimeCall q,
                   const O &o) {
  if (q.entry == 0x1cfde0 || q.entry == 0x1d0790 ||
      q.entry == 0x1ceda0 || q.entry == 0x1d1b70 || q.entry == 0x111b60 ||
      q.entry == 0xbb240 || q.entry == 0x114bd0 || q.entry == 0x1cec20 ||
      q.entry == 0x1cfa90 || q.entry == 0x1c9de0) {
    SourcePhysicsFactoryFloatingPoint fp;
    fp.mxcsr = _mm_getcsr();
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fnstcw %0" : "=m"(fp.x87_control_word));
#else
    throw std::runtime_error("Required original animation FP backend unavailable");
#endif
    return source_call_unit_animation_selection_1_1(m, q, fp, owner, o);
  }
  if (!owner)
    throw std::runtime_error(
        "Required original unit animation owner unavailable");
  return owner(m, q, o);
}
} // namespace
bool source_advance_unit_transition_clocks_1_1(M &m, std::uint32_t d,
                                               const O &o) {
  const auto p = object(m, d), extension = p + std::uint32_t(sx(m, p + 0x33e));
  bool changed = false;
  for (unsigned i = 0; i < 5; ++i) {
    const auto clock = extension + 0x80 + 4 * i;
    const auto duration = r(m, clock + 1, 1);
    if (!duration)
      continue;
    const auto flags = r(m, clock + 3, 1);
    if (flags & 2u)
      continue;
    if (flags & 1u) {
      const auto next = (r(m, clock, 1) + 1u) & 255u;
      w(m, o, 0x1dcd2d, clock, next, 1);
      if (std::bit_cast<std::int8_t>(std::uint8_t(next)) >=
          std::bit_cast<std::int8_t>(std::uint8_t(duration))) {
        w(m, o, 0x1dcd34, clock, duration, 1);
        w(m, o, 0x1dcd36, clock + 3, flags | 2u, 1);
      }
    } else {
      const auto next = (r(m, clock, 1) - 1u) & 255u;
      w(m, o, 0x1dcd45, clock, next, 1);
      if (std::bit_cast<std::int8_t>(std::uint8_t(next)) <= 0) {
        w(m, o, 0x1dcd4c, clock, 0, 1);
        w(m, o, 0x1dcd4f, clock + 3, flags | 2u, 1);
      }
    }
    changed = true;
  }
  return changed;
}
bool source_unit_animation_completed_1_1(const M &m, std::uint32_t manager) {
  if (r(m, manager + 0x6c, 1) & 1u)
    return true;
  auto channel = manager + 0x40;
  if (r(m, channel) == 0xffffffffu || sx(m, channel + 6) == -1) {
    channel = manager;
    if (r(m, channel) == 0xffffffffu || sx(m, channel + 6) == -1)
      return false;
  }
  return !(r(m, channel + 0x12, 1) & 1u) ||
         (r(m, channel + 0x11, 1) & 9u);
}
bool source_decay_unit_animation_weights_1_1(M &m, std::uint32_t d,
                                              std::uint32_t esp, FP fp,
                                              const O &o) {
  Scope scope(fp);
  const auto p = object(m, d), definition = tag(m, r(m, p)),
             extension = p + std::uint32_t(sx(m, p + 0x33e)), frame = esp - 24;
  auto weight = load(m, extension + 0x40);
  if (r(m, extension + 0x38) != 0xffffffffu) {
    if (_mm_comigt_ss(load(m, 0x46504c), weight)) {
      weight = _mm_add_ss(weight, load(m, 0x46551c));
      const bool positive = _mm_comigt_ss(weight, _mm_setzero_ps());
      if (!positive)
        weight = _mm_setzero_ps();
      w(m, o, positive ? 0x10f4cd : 0x10f4a6, extension + 0x40, word(weight));
    } else {
      weight = _mm_add_ss(weight, load(m, 0x446df8));
      if (_mm_comigt_ss(weight, load(m, 0x465030)))
        weight = load(m, 0x465030);
      w(m, o, 0x10f4cd, extension + 0x40, word(weight));
    }
    return true;
  }
  weight = _mm_sub_ss(weight, load(m, 0x446ee4));
  if (!_mm_comigt_ss(weight, _mm_setzero_ps()))
    weight = _mm_setzero_ps();
  w(m, o, 0x10f509, extension + 0x40, word(weight));
  w(m, o, 0x10f511, frame + 0x10, 12);
  bool changed = false;
  for (unsigned i = 0; i < 12; ++i) {
    const auto authored = load(m, definition + 0x1e8);
    const auto rate = _mm_comigt_ss(authored, _mm_setzero_ps())
                          ? authored
                          : load(m, 0x4650f4);
    auto value = _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, extension + 0x50 + i, 1));
    value = _mm_mul_ss(value, load(m, 0x54f198));
    value = _mm_mul_ss(value, rate);
    value = _mm_mul_ss(value, load(m, 0x4651d0));
    w(m, o, 0x10f546, frame + 0x14, word(value));
    auto integer = _mm_cvttss_si32(load(m, frame + 0x14));
    if (integer < -128)
      integer = -128;
    else if (integer > 127)
      integer = 127;
    w(m, o, 0x10f56e, extension + 0x50 + i, std::uint32_t(integer), 1);
    changed |= integer != 0;
    w(m, o, 0x10f57a, frame + 0x10, 11 - i);
  }
  // Source re-fetches the salted object's current extension after the loop.
  const auto current = object(m, d);
  if (sx(m, current + std::uint32_t(sx(m, current + 0x33e)) + 0x36) != -1) {
    const bool zero =
        (_mm_movemask_ps(_mm_cmpeq_ss(load(m, extension + 0x40),
                                     _mm_setzero_ps())) & 1) != 0;
    if (!zero)
      w(m, o, 0x10f5a9, extension + 2, 255, 1);
    else if (!r(m, extension + 2, 1)) {
      const auto rng = r(m, 0x4ee9d8), seed = r(m, rng + 4) * 0x19660du + 0x3c6ef35fu;
      w(m, o, 0x10f5cd, rng + 4, seed);
      w(m, o, 0x10f5e0, extension + 2, (((seed >> 16) * 225u) >> 16) + 30u, 1);
    } else
      w(m, o, 0x10f5ed, extension + 2, r(m, extension + 2, 1) - 1u, 1);
  }
  return changed;
}
bool source_blend_unit_animation_weights_1_1(M &m, std::uint32_t d,
                                              std::uint32_t esp, FP fp,
                                              const O &o) {
  Scope scope(fp);
  const auto p = object(m, d), extension = p + std::uint32_t(sx(m, p + 0x33e)),
             frame = esp - 8;
  if (!_mm_comigt_ss(load(m, extension + 0x60), _mm_setzero_ps()))
    return false;
  const auto one = load(m, 0x465030);
  const auto age = _mm_add_ss(load(m, extension + 0x64), one);
  auto blend = _mm_mul_ss(load(m, extension + 0x60), age);
  w(m, o, 0x110302, extension + 0x64, word(age));
  w(m, o, 0x110307, frame, word(blend));
  if (_mm_comigt_ss(_mm_setzero_ps(), blend)) {
    blend = _mm_setzero_ps();
    w(m, o, 0x11031b, frame, word(blend));
  } else if (_mm_comigt_ss(blend, one)) {
    blend = one;
    w(m, o, 0x11031b, frame, word(blend));
  }
  const auto complement = _mm_sub_ss(one, blend), factor = load(m, 0x54f19c),
             multiplier = load(m, 0x4651d0);
  for (unsigned i = 0; i < 13; ++i) {
    auto previous = _mm_cvtsi32_ss(_mm_setzero_ps(), r(m, extension + 0x6e + i, 1));
    previous = _mm_mul_ss(previous, factor);
    auto value = _mm_mul_ss(previous, complement);
    if (sx(m, extension + 0x6c) == std::int32_t(i)) {
      value = _mm_sub_ss(load(m, extension + 0x68), previous);
      value = _mm_add_ss(_mm_mul_ss(value, blend), previous);
    }
    value = _mm_mul_ss(value, multiplier);
    w(m, o, 0x110373, frame + 4, word(value));
    const auto integer = std::uint32_t(_mm_cvttss_si32(load(m, frame + 4)));
    w(m, o, 0x11038c, extension + 0x6e + i, integer > 255u ? 255u : integer, 1);
  }
  const float value = f(r(m, frame)), one_value = f(r(m, 0x465030)),
              epsilon = f(r(m, 0x46504c));
  unsigned char done;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  asm volatile("flds %1\n\tfsubs %2\n\tfabs\n\tflds %3\n\tfcomip %%st(1),%%st\n\tfstp %%st(0)\n\tseta %0"
               : "=qm"(done) : "m"(value), "m"(one_value), "m"(epsilon)
               : "st", "cc");
#else
  throw std::runtime_error("Original animation weight x87 comparison unavailable");
#endif
  if (done) {
    w(m, o, 0x1103ad, extension + 0x60, 0);
    w(m, o, 0x1103b2, extension + 0x64, 0);
  }
  return true;
}
namespace {
bool valid_channel(const M &m, std::uint32_t p) {
  return r(m, p) != 0xffffffffu && sx(m, p + 6) != -1;
}
void reset_channels(M &m, std::uint32_t manager, const O &o) {
  // The existing complete1D0590 has no nested services. Its last48 bytes are
  // outside the reset's accesses; only the actual three channels are copied.
  std::array<std::byte, 144> channels{};
  m.read(manager, std::span(channels).first(96));
  reset_source_unit_animation_channels(channels, r(m, 0x465030), [&](const auto &s) {
    w(m, o, s.instruction, manager + std::uint32_t(s.offset), s.after, s.bytes);
  });
}
bool install_channel(M &m, std::uint32_t channel, std::uint32_t graph,
                     std::uint32_t flags, std::uint32_t packed,
                     std::uint32_t activity, std::uint32_t first,
                     std::uint32_t second, std::uint32_t last, const O &o) {
  if (graph == 0xffffffffu || (packed >> 16) == 65535u)
    return false;
  std::array<std::byte, 32> bytes;
  m.read(channel, bytes);
  return install_source_unit_animation_channel(
      bytes, 0, graph, flags, packed, activity, first, second, last,
      r(m, 0x465030), [&](const auto &s) {
        w(m, o, s.instruction, channel + std::uint32_t(s.offset), s.after, s.bytes);
      });
}
bool install_primary(M &m, std::uint32_t manager, std::uint32_t packed,
                     std::uint32_t activity, std::uint32_t flags,
                     std::uint32_t esp, FP fp, const SourceRuntimeOwner &owner,
                     const O &o) {
  // Complete1CEE00, including its original output slot for admitted variants.
  if (r(m, manager + 0x68) == 0xffffffffu)
    return false;
  if (flags & 0x10u) {
    const auto out = call(m, owner,
                          {.entry = 0x1ceda0,
                           .entry_esp = esp - 20,
                           .esi = esp + 8,
                           .edi = manager,
                           .arguments = {packed}}, o);
    packed = r(m, out);
  }
  if (!install_channel(m, manager, r(m, manager + 0x68), flags, packed,
                       activity, 0xffffffffu, 0xffffffffu, 0, o))
    return false;
  if (flags & 4u)
    source_clamp_unit_animation_frame_1_1(m, manager, 0, esp - 16, fp, owner, o);
  if (flags & 8u)
    w(m, o, 0x1cee69, manager + 0x18, r(m, 0x465030));
  return true;
}
} // namespace
std::uint32_t source_select_third_unit_animation_1_1(
    M &m, std::uint32_t manager, std::array<std::uint32_t, 4> ids,
    std::uint32_t packed_out, std::uint32_t activity_out, std::uint32_t esp,
    const SourceRuntimeOwner &owner, const O &o) {
  constexpr std::uint32_t none = 0xffffffffu;
  const auto frame = esp - 0x2c;
  w(m, o, 0x1d06cd, frame + 0x40, none, 2);
  w(m, o, 0x1d06d2, frame + 0x42, none, 2);
  auto activity = none;
  for (unsigned i = 0; i < 4; ++i)
    if (ids[3] == r(m, 0x4770e0 + 8 * i)) {
      activity = r(m, 0x4770e4 + 8 * i);
      break;
    }
  const auto enabled = r(m, 0x4770dc, 1);
  w(m, o, 0x1d06f8, activity_out, none);
  if (!enabled) {
    w(m, o, 0x1d077c, packed_out, r(m, frame + 0x40));
    return packed_out;
  }
  if (activity == none) {
    w(m, o, 0x1d0769, packed_out, r(m, frame + 0x40));
    return packed_out;
  }
  const auto resolved = call(m, owner,
                             {.entry = 0x1cfde0,
                              .entry_esp = frame - 28,
                              .eax = manager,
                              .ecx = frame + 0xc,
                              .edx = frame + 0x1c,
                              .arguments = {ids[0], ids[1], ids[2], activity, 0,
                                            frame + 0x40}}, o) & 255u;
  if (resolved) {
    w(m, o, 0x1d0733, activity_out, activity);
    w(m, o, 0x1d0738, packed_out, r(m, frame + 0x40));
  } else {
    w(m, o, 0x1d074a, frame + 0x40, none, 2);
    w(m, o, 0x1d074f, frame + 0x42, none, 2);
    w(m, o, 0x1d075a, packed_out, r(m, frame + 0x40));
  }
  return packed_out;
}
bool source_select_unit_animation_1_1(
    M &m, std::uint32_t manager, std::array<std::uint32_t, 4> ids,
    std::uint32_t flags, std::uint32_t installation, std::uint32_t esp, FP fp,
    const SourceRuntimeOwner &owner, const O &o) {
  Scope scope(fp);
  constexpr std::uint32_t none = 0xffffffffu;
  const auto frame = esp - 0x48;
  w(m, o, 0x1d0a7a, frame + 0x28, ids[0]);
  w(m, o, 0x1d0a82, frame + 0x54, 0, 1);
  w(m, o, 0x1d0a87, frame + 0x18, none, 2);
  w(m, o, 0x1d0a8c, frame + 0x1a, none, 2);
  w(m, o, 0x1d0a91, frame + 0x2c, ids[1]);
  w(m, o, 0x1d0a95, frame + 0x30, ids[2]);
  w(m, o, 0x1d0a99, frame + 0x34, ids[3]);
  w(m, o, 0x1d0a9d, frame + 0x13, 1, 1);
  if (valid_channel(m, manager)) {
    const auto definition = source_unit_animation_definition_1_1(
        m, tag(m, r(m, manager)), r(m, manager + 4), frame - 8, owner, o);
    w(m, o, 0x1d0ad0, frame + 0x13, !(r(m, definition + 0x18, 1) & 2u), 1);
  }
  if (flags & 0x10u)
    reset_channels(m, manager, o);
  w(m, o, 0x1d0aec, manager + 0x6e, r(m, manager + 0x6e, 1) & 0xfeu, 1);
  const auto resolved = call(m, owner,
                             {.entry = 0x1cfde0,
                              .entry_esp = frame - 28,
                              .eax = manager,
                              .ecx = frame + 0x38,
                              .edx = frame + 0x28,
                              .arguments = {ids[0], ids[1], ids[2], ids[3], flags,
                                            frame + 0x18}}, o) & 255u;
  auto update_identifiers = [&] {
    const bool changed = r(m, manager + 0x70) != r(m, frame + 0x28) ||
                         r(m, manager + 0x74) != r(m, frame + 0x2c) ||
                         r(m, manager + 0x78) != r(m, frame + 0x30);
    const std::array<std::uint32_t, 4> sites{0x1d0e10, 0x1d0e17, 0x1d0e1e, 0x1d0e2a};
    for (unsigned i = 0; i < 4; ++i)
      w(m, o, sites[i], manager + 0x70 + 4 * i, r(m, frame + 0x28 + 4 * i));
    if (!changed)
      return;
    if (r(m, manager + 0x68) != none && !(r(m, manager + 0x6c, 1) & 1u))
      call(m, owner,
           {.entry = 0x1d1b70,
            .entry_esp = frame - 16,
            .ecx = r(m, manager + 0x74),
            .edx = r(m, manager + 0x78),
            .edi = manager,
            .arguments = {r(m, manager + 0x70), 1, 1}}, o);
    if (valid_channel(m, manager + 0x40))
      return;
    if (valid_channel(m, manager + 0x40) && (r(m, manager + 0x52, 1) & 1u))
      w(m, o, 0x1d0e77, manager + 0x51, r(m, manager + 0x51, 1) & 0xfeu, 1);
    if (valid_channel(m, manager + 0x40))
      return;
    if (valid_channel(m, manager) && (r(m, manager + 0x12, 1) & 1u))
      w(m, o, 0x1d0e95, manager + 0x11, r(m, manager + 0x11, 1) & 0xfeu, 1);
    if (valid_channel(m, manager + 0x20) && (r(m, manager + 0x32, 1) & 1u))
      w(m, o, 0x1d0ea9, manager + 0x31, r(m, manager + 0x31, 1) & 0xfeu, 1);
  };
  auto fallback = [&] {
    if (!(flags & 4u))
      return false;
    w(m, o, 0x1d0ec3, frame + 0x54, 1, 1);
    update_identifiers();
    return true;
  };
  if (!resolved || sx(m, frame + 0x1a) == -1 || (flags & 8u))
    return fallback();
  w(m, o, 0x1d0b2f, frame + 0x14, none, 2);
  w(m, o, 0x1d0b34, frame + 0x16, none, 2);
  w(m, o, 0x1d0b39, frame + 0x50, 0, 1);
  if (!(flags & 0x200u)) {
    const auto output = call(m, owner,
                             {.entry = 0x1d0790,
                              .entry_esp = frame - 20,
                              .ebx = manager,
                              .arguments = {frame + 0x4c, r(m, frame + 0x28),
                                            r(m, frame + 0x34), frame + 0x50}}, o);
    const auto eligible = r(m, frame + 0x50, 1) & (~r(m, manager + 0x6c, 1) & 1u);
    w(m, o, 0x1d0b65, frame + 0x14, r(m, output));
    w(m, o, 0x1d0b6f, frame + 0x50, eligible, 1);
    const bool interrupted = valid_channel(m, manager) &&
                             (r(m, manager + 0x12, 1) & 1u) &&
                             !(r(m, manager + 0x11, 1) & 9u);
    w(m, o, 0x1d0ba5, frame + 0x50,
      eligible & (flags >> 10) & std::uint32_t(interrupted) & 1u, 1);
  }
  if (r(m, frame + 0x50, 1))
    w(m, o, 0x1d0bab, manager + 0x6e, r(m, manager + 0x6e, 1) | 1u, 1);
  else
    w(m, o, 0x1d0bb1, manager + 0x6e, r(m, manager + 0x6e, 1) & 0xfeu, 1);
  if (sx(m, frame + 0x16) == -1 && r(m, frame + 0x50, 1))
    return fallback();
  w(m, o, 0x1d0bd0, frame + 0x4c, 0);
  w(m, o, 0x1d0bd6, frame + 0x24, flags & 0x20u);
  if ((flags & 0x60u) && valid_channel(m, manager)) {
    const auto definition = source_unit_animation_channel_definition_1_1(
        m, manager, frame - 4, owner, o);
    const auto denominator = _mm_sub_ss(
        _mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)), load(m, 0x46504c));
    if (_mm_comigt_ss(denominator, load(m, 0x465048))) {
      auto value = load(m, manager + 0x1c);
      if (_mm_comigt_ss(_mm_setzero_ps(), value))
        value = _mm_setzero_ps();
      else if (_mm_comigt_ss(value, denominator))
        value = denominator;
      value = _mm_div_ss(value, denominator);
      w(m, o, 0x1d0c38, frame + 0x4c, word(value));
      if (flags & 0x40u) {
        value = _mm_sub_ss(load(m, 0x465030), value);
        w(m, o, 0x1d0c47, frame + 0x4c, word(value));
      }
      if (_mm_comigt_ss(_mm_setzero_ps(), value))
        w(m, o, 0x1d0c55, frame + 0x4c, 0);
      else if (_mm_comigt_ss(value, load(m, 0x465030)))
        w(m, o, 0x1d0c62, frame + 0x4c, r(m, 0x465030));
    }
  }
  w(m, o, 0x1d0c6b, frame + 0x1c, none, 2);
  w(m, o, 0x1d0c70, frame + 0x1e, none, 2);
  w(m, o, 0x1d0c75, frame + 0x20, none);
  if (!(flags & 0x100u)) {
    const auto output = source_select_third_unit_animation_1_1(
        m, manager, {r(m, frame + 0x28), r(m, frame + 0x2c),
                     r(m, frame + 0x30), r(m, frame + 0x34)},
        frame + 0x50, frame + 0x20, frame - 24, owner, o);
    w(m, o, 0x1d0c9d, frame + 0x1c, r(m, output));
  }
  reset_channels(m, manager, o);
  if (!install_primary(m, manager, r(m, frame + 0x18), r(m, frame + 0x44),
                       installation, frame - 28, fp, owner, o))
    return fallback();
  w(m, o, 0x1d0ccc, frame + 0x54, 1, 1);
  auto transition_allowed = [&](std::uint32_t channel) {
    if (!valid_channel(m, channel))
      return true;
    const auto definition = source_unit_animation_definition_1_1(
        m, tag(m, r(m, channel)), r(m, channel + 4), frame - 12, owner, o);
    return !(r(m, definition + 0x18, 1) & 1u);
  };
  w(m, o, 0x1d0cda, frame + 0x50,
    std::uint32_t(transition_allowed(manager)) & r(m, frame + 0x13, 1), 1);
  if (sx(m, frame + 0x16) != -1 && r(m, manager + 0x68) != none) {
    const auto output = call(m, owner,
                             {.entry = 0x1ceda0,
                              .entry_esp = frame - 8,
                              .esi = frame + 0x5c,
                              .edi = manager,
                              .arguments = {r(m, frame + 0x14)}}, o);
    if (install_channel(m, manager + 0x40, r(m, manager + 0x68), 0x15,
                        r(m, output), r(m, frame + 0x34), none, none, 3, o)) {
      source_clamp_unit_animation_frame_1_1(m, manager + 0x40, 0, frame - 8,
                                            fp, owner, o);
      if (r(m, manager + 0x12, 1) & 1u)
        w(m, o, 0x1d0d2c, manager + 0x11, r(m, manager + 0x11, 1) | 1u, 1);
      w(m, o, 0x1d0d37, frame + 0x50,
        r(m, frame + 0x50, 1) & std::uint32_t(transition_allowed(manager + 0x40)), 1);
      if (r(m, frame + 0x50, 1))
        call(m, owner, {.entry = 0x1d1a00, .entry_esp = frame - 4, .ebx = manager}, o);
    }
  }
  if (!valid_channel(m, manager + 0x40) && (flags & 0x60u) &&
      _mm_comigt_ss(load(m, frame + 0x4c), load(m, 0x465048))) {
    const auto definition = source_unit_animation_channel_definition_1_1(
        m, manager, frame - 4, owner, o);
    const auto value = _mm_mul_ss(
        _mm_sub_ss(_mm_cvtsi32_ss(_mm_setzero_ps(), sx(m, definition + 0x14)),
                   load(m, 0x46504c)), load(m, frame + 0x4c));
    w(m, o, 0x1d0d8e, frame - 4, word(value));
    source_clamp_unit_animation_frame_1_1(m, manager, word(value), frame - 8,
                                          fp, owner, o);
  }
  if (r(m, frame + 0x50, 1))
    w(m, o, 0x1d0da6, manager + 0x6c, r(m, manager + 0x6c, 1) & 0xefu, 1);
  else
    w(m, o, 0x1d0da0, manager + 0x6c, r(m, manager + 0x6c, 1) | 0x10u, 1);
  if (sx(m, frame + 0x1e) != -1 && r(m, manager + 0x68) != none &&
      install_channel(m, manager + 0x20, r(m, manager + 0x68), 0x3c0,
                      r(m, frame + 0x1c), r(m, frame + 0x20), none, none, 0, o))
    w(m, o, 0x1d0dde, manager + 0x80, 0);
  w(m, o, 0x1d0de6, manager + 0x6c, r(m, manager + 0x6c, 1) & 0xfeu, 1);
  update_identifiers();
  return r(m, frame + 0x54, 1) != 0;
}
std::uint32_t source_unit_animation_transition_1_1(
    const M &m, std::uint32_t old, std::uint32_t activity, std::uint8_t changed,
    std::uint8_t flag, std::uint32_t minimum, FP fp) {
  Scope scope(fp);
  auto result = load(m, 0x44c6b4);
  if (activity == 0x9000020 || activity == 0x900001f || activity == 0x90006b2)
    result = load(m, 0x465140);
  if (changed || flag)
    result = load(m, 0x44c6b4);
  auto moving = [](std::uint32_t a) {
    return a == 0xa000014 || a == 0x9000015 || a == 0x9000016 || a == 0xa000017;
  };
  if ((old == 0x400000c && moving(activity)) ||
      (moving(old) && (moving(activity) || activity == 0x400000c)))
    result = load(m, 0x44c6b4);
  if (!_mm_comigt_ss(result, _mm_set_ss(f(minimum))))
    result = _mm_set_ss(f(minimum));
  return word(result);
}
void source_refresh_unit_animation_channels_1_1(M &m, std::uint32_t d,
                                                std::uint32_t esp,
                                                const SourceRuntimeOwner &owner,
                                                const O &o) {
  const auto p = object(m, d), manager = p + std::uint32_t(sx(m, p + 0x12a)),
             extension = p + std::uint32_t(sx(m, p + 0x33e));
  for (const auto offset : {0x9cu, 0xdcu, 0xbcu})
    call(m, owner,
         {.entry = 0x1cec20,
          .entry_esp = esp - 20,
          .ecx = offset == 0xbc ? 0x400054bu : 0x7000101u,
          .edx = offset == 0xbc ? r(m, extension + 0x98) : 0x7000101u,
          .esi = extension + offset,
          .arguments = {manager}},
         o);
}
bool source_request_unit_animation_1_1(M &m,
                                       const SourceUnitAnimationRequest &q,
                                       std::uint32_t flags, std::uint32_t esp,
                                       FP fp, const SourceRuntimeOwner &owner,
                                       const O &o) {
  Scope scope(fp);
  const auto p = object(m, q.object),
             manager = p + std::uint32_t(sx(m, p + 0x12a)),
             extension = p + std::uint32_t(sx(m, p + 0x33e)), frame = esp - 20;
  const auto old_activity = r(m, manager + 0x7c),
             old_mode = r(m, manager + 0x70);
  const auto mode = q.identifiers[0], activity = q.identifiers[3];
  w(m, o, 0x110c31, frame + 0x10, old_mode);
  if (old_activity != 0xffffffffu) {
    if (old_activity == activity || activity == 0x7000101)
      flags |= 0x20u;
    else {
      std::uint32_t paired = 0xffffffffu;
      for (unsigned i = 0; i < 4; ++i)
        if (activity == r(m, 0x46fb24 + i * 8)) {
          paired = r(m, 0x46fb28 + i * 8);
          break;
        }
      if (old_activity == paired)
        flags |= 0x40u;
    }
  }
  if (r(m, p + 0x13c) != 0xffffffffu && !r(m, 0x517fe8, 1))
    flags |= 0x200u;
  bool admitted = true;
  if (mode != 0x7000101 || activity != mode)
    admitted = source_select_unit_animation_1_1(
        m, manager, q.identifiers, flags, 0x3f, frame - 24, fp, owner, o);
  if (!admitted)
    return false;
  const bool mode_changed = (old_mode != mode && mode != 0x7000101) || q.force;
  const bool activity_changed =
      (old_activity != activity && activity != 0x7000101) || q.force;
  if (old_mode == mode || mode == 0x7000101) {
    w(m, o, 0x110cd3, frame + 0x1c, 0, 1);
    if (q.force)
      w(m, o, 0x110cda, frame + 0x1c, 1, 1);
  } else
    w(m, o, 0x110cda, frame + 0x1c, 1, 1);
  if (mode_changed) {
    source_refresh_unit_animation_channels_1_1(m, q.object, frame - 4, owner,
                                               o);
    const bool alternate =
        call(m, owner,
             {.entry = 0x1cfa90, .entry_esp = frame - 4, .esi = manager}, o) &
        255u;
    w(m, o, alternate ? 0x110d17 : 0x110d1c, extension,
      alternate ? (r(m, extension, 1) | 2u) : (r(m, extension, 1) & 0xfdu), 1);
  } else if (!activity_changed)
    return true;
  const auto duration = source_unit_animation_transition_1_1(
      m, old_activity, activity, mode_changed, 0, q.minimum_transition_word,
      fp);
  w(m, o, 0x110d40, frame + 0x30, duration);
  if (_mm_comigt_ss(_mm_set_ss(f(duration)), load(m, 0x465048)) &&
      !(r(m, manager + 0x6c, 1) & 0x10u))
    call(m, owner,
         {.entry = 0xbb240,
          .entry_esp = frame - 8,
          .esi = q.object,
          .arguments = {duration}},
         o);
  call(m, owner,
       {.entry = 0x114bd0, .entry_esp = frame - 8, .arguments = {q.object}}, o);
  return true;
}
std::uint8_t source_update_unit_animation_1_1(M &m, std::uint32_t d,
                                              std::uint32_t action,
                                              std::uint32_t esp, FP fp,
                                              const SourceRuntimeOwner &owner,
                                              const O &o) {
  Scope scope(fp);
  const auto frame = esp - 0x90u, p = object(m, d),
             definition = tag(m, r(m, p)),
             manager = p + std::uint32_t(sx(m, p + 0x12a)),
             extension = p + std::uint32_t(sx(m, p + 0x33e));
  constexpr std::uint32_t sentinel = 0x7000101, none = 0xffffffffu;
  w(m, o, 0x112e5c, frame + 0x38, definition);
  w(m, o, 0x112e65, frame + 0x14, sentinel);
  w(m, o, 0x112e69, frame + 0x20, sentinel);
  w(m, o, 0x112e71, frame + 0x1c, p);
  w(m, o, 0x112e75, frame + 0x24, extension);
  w(m, o, 0x112e7b, frame + 0x11, 0, 1);
  w(m, o, 0x112e80, frame + 0x13, 0, 1);
  w(m, o, 0x112e85, frame + 0x48, 0, 1);
  w(m, o, 0x112e8a, frame + 0x3c, 0);
  w(m, o, 0x112e9e, frame + 0x12,
    source_advance_unit_transition_clocks_1_1(m, d, o),
    1);
  if (r(m, action) != none && r(m, action))
    w(m, o, 0x112ead, frame + 0x14, r(m, action));
  if (r(m, action + 4) != none && r(m, action + 4)) {
    w(m, o, 0x112ec7, frame + 0x20, r(m, action + 4));
    w(m, o, 0x112ecb, frame + 0x48, r(m, action + 8, 1), 1);
  }
  if (r(m, p + 0x14) == none) {
    if (r(m, manager + 0x70) == 0x20001b3)
      w(m, o, 0x112ee5, frame + 0x14, 0x20001b3);
    else if (r(m, p + 0x10a, 1) & 4u)
      w(m, o, 0x112efb, frame + 0x14, 0x6000086);
    else {
      const auto current = object(m, d),
                 bank = current + std::uint32_t(sx(m, current + 0x346));
      if (r(m, bank + 0x36, 2))
        w(m, o, 0x112f21, frame + 0x20, sentinel);
      else {
        if (r(m, frame + 0x14) == sentinel)
          w(m, o, 0x112f4d, frame + 0x14,
            call(m, owner,
                 {.entry = 0x1106b0,
                  .entry_esp = frame - 8,
                  .eax = d,
                  .arguments = {0},
                  .dl = std::uint8_t(r(m, action + 9, 1))},
                 o));
        if (r(m, p + 0x1f4, 1))
          w(m, o, 0x112f5b, frame + 0x14, 0x7000039);
        if (r(m, extension + 0x2c) != none)
          w(m, o, 0x112f6b, frame + 0x14, r(m, extension + 0x2c));
      }
    }
  }
  if (r(m, manager) != none && sx(m, manager + 6) != -1) {
    const auto current = object(m, d);
    source_update_unit_animation_manager_1_1(
        m, manager, 0xc04f0, d, std::uint32_t(sx(m, current + 0x110)) >> 5,
        current + std::uint32_t(sx(m, current + 0x10e)),
        current + std::uint32_t(sx(m, current + 0x112)), frame - 24, fp, owner,
        o);
    w(m, o, 0x112fc9, frame + 0x4c, r(m, manager + 0x70));
    w(m, o, 0x112fd4, frame + 0x34, r(m, manager + 0x7c));
    if (r(m, manager + 0x14, 2))
      call(m, owner,
           {.entry = 0xe8050,
            .entry_esp = frame - 12,
            .eax = r(m, manager + 0x7c),
            .esi = d,
            .arguments = {r(m, manager + 0x14, 2), r(m, manager + 0x16, 2)}},
           o);
    if (!(r(m, manager + 0x6c, 1) & 1u) && (r(m, manager + 0x12, 1) & 1u) &&
        !(r(m, manager + 0x11, 1) & 9u)) {
      const auto animation = source_unit_animation_channel_definition_1_1(
          m, manager, frame - 4, owner, o);
      const bool multiple_frames = sx(m, animation + 0x14) > 1;
      if (multiple_frames)
        w(m, o, 0x113027, frame + 0x12, 1, 1);
      if (multiple_frames && (r(m, manager + 0x11, 1) & 10u)) {
        const auto flags = r(m, manager + 0x12, 2);
        if ((flags & 0x4000u) && (flags & 2u))
          w(m, o, 0x113043, frame + 0x11, 0, 1);
        else if ((flags & 0x4000u) && !(r(m, extension + 1, 1) & 1u)) {
          w(m, o, 0x11305b, manager + 0x6c, r(m, manager + 0x6c, 2) | 1u, 2);
          w(m, o, 0x11305f, frame + 0x11, 0, 1);
        } else {
          const auto previous = r(m, frame + 0x34);
          w(m, o, 0x113084, frame + 0x44, sentinel);
          w(m, o, 0x113088, frame + 0x40, sentinel);
          call(m, owner,
               {.entry = 0xe8130,
                .entry_esp = frame - 16,
                .ebx = d,
                .arguments = {previous, frame + 0x44, frame + 0x40}},
               o);
          if (r(m, frame + 0x44) != sentinel ||
              r(m, frame + 0x40) != sentinel) {
            w(m, o, 0x1130a8, frame + 0x14, r(m, frame + 0x44));
            w(m, o, 0x1130ac, frame + 0x20, r(m, frame + 0x40));
            w(m, o, 0x1130b0, frame + 0x13, 1, 1);
          }
          if (previous == 0x800001c || previous == 0xa00001d) {
            if (!r(m, p + 0xaa, 1) &&
                !(call(m, owner,
                       {.entry = 0xc0b80, .entry_esp = frame - 4, .ecx = d},
                       o) &
                  255u)) {
              const auto marker = sx(m, tag(m, r(m, object(m, d))) + 0x254);
              if (marker != -1) {
                w(m, o, 0x113158, frame + 0x28, 0);
                w(m, o, 0x11315c, frame + 0x2c, 0);
                w(m, o, 0x113160, frame + 0x30, 0, 1);
                w(m, o, 0x113164, frame + 0x31, 0, 1);
                call(m, owner,
                     {.entry = 0x1cad70,
                      .entry_esp = frame - 4,
                      .eax = manager,
                      .edi = frame + 0x28},
                     o);
                if (call(m, owner,
                         {.entry = 0x110660,
                          .entry_esp = frame - 4,
                          .ecx = std::uint32_t(marker),
                          .edx = frame + 0x28},
                         o) &
                    255u) {
                  const auto choices =
                      call(m, owner,
                           {.entry = 0x110630,
                            .entry_esp = frame - 8,
                            .edx = frame + 0x28,
                            .arguments = {std::uint32_t(marker) + 1}},
                           o);
                  const auto variant = call(m, owner,
                                            {.entry = 0x13fb90,
                                             .entry_esp = frame - 8,
                                             .arguments = {choices}},
                                            o) -
                                       1;
                  call(m, owner,
                       {.entry = 0xbb050,
                        .entry_esp = frame - 8,
                        .eax = d,
                        .arguments = {frame + 0x5c}},
                       o);
                  call(m, owner,
                       {.entry = 0x1cad90,
                        .entry_esp = frame - 12,
                        .eax = manager,
                        .esi = frame + 0x50,
                        .arguments = {0, variant}},
                       o);
                  call(m, owner,
                       {.entry = 0x143ce0,
                        .entry_esp = frame - 8,
                        .eax = frame + 0x50,
                        .ecx = frame + 0x5c,
                        .edx = frame + 0x50,
                        .arguments = {frame + 0x5c}},
                       o);
                  const auto node = call(m, owner,
                                         {.entry = 0xb9ac0,
                                          .entry_esp = frame - 8,
                                          .eax = d,
                                          .arguments = {std::uint32_t(marker)}},
                                         o);
                  constexpr std::uint32_t stores[]{0x1131e1, 0x1131f2,
                                                   0x113203};
                  for (unsigned i = 0; i < 3; ++i)
                    w(m, o, stores[i], frame + 0x28 + i * 4,
                      word(_mm_sub_ss(load(m, node + 0x28 + i * 4),
                                      load(m, frame + 0x50 + i * 4))));
                } else {
                  constexpr std::uint32_t stores[]{0x1132ac, 0x1132b0,
                                                   0x1132b4};
                  for (unsigned i = 0; i < 3; ++i)
                    w(m, o, stores[i], frame + 0x28 + 4 * i,
                      r(m, r(m, 0x46fc44) + 4 * i));
                }
                const bool changed = r(m, frame + 0x20) != r(m, frame + 0x34) ||
                                     r(m, frame + 0x14) != r(m, frame + 0x4c);
                call(m, owner,
                     {.entry = 0xe6e70,
                      .entry_esp = frame - 8,
                      .ecx = frame + 0x28,
                      .esi = d,
                      .arguments = {std::uint32_t(changed)}},
                     o);
                w(m, o, 0x1132e0, frame + 0x11,
                  r(m, frame + 0x20) != r(m, frame + 0x34), 1);
              }
            }
          } else if (previous == 0x700005c || previous == 0x700005d) {
            constexpr std::uint32_t stores[]{0x113224, 0x113239, 0x11325a};
            constexpr std::uint32_t offsets[]{0x40, 0, 0x20};
            for (unsigned i = 0; i < 3; ++i) {
              const auto channel = manager + offsets[i];
              if (r(m, channel) != none && sx(m, channel + 6) != -1 &&
                  (r(m, channel + 0x12, 1) & 1u))
                w(m, o, stores[i], channel + 0x11, r(m, channel + 0x11, 1) | 1u,
                  1);
            }
          } else if (previous == 0x900001f || previous == 0x9000020 ||
                     previous == 0x90006b2) {
            if (!r(m, p + 0xaa, 1)) {
              call(m, owner,
                   {.entry = 0xe5930, .entry_esp = frame - 4, .eax = d}, o);
              w(m, o, 0x11328d, frame + 0x11, 1, 1);
            }
          } else if (previous == 0xe0000c2) {
            call(m, owner,
                 {.entry = 0x11cc90,
                  .entry_esp = frame - 8,
                  .eax = d,
                  .arguments = {r(m, frame + 0x14)}},
                 o);
            w(m, o, 0x113317, frame + 0x13, 1, 1);
          } else if (previous == 0xd000042)
            w(m, o, 0x11331e, frame + 0x11, 0, 1);
          else if (previous == 0xc000043) {
            bool disposable = false;
            if (r(m, definition + 0xbc) & 2u)
              disposable =
                  call(m, owner,
                       {.entry = 0x110db0, .entry_esp = frame - 4, .eax = d},
                       o) != 0xd000042 ||
                  (r(m, p + 0xc1, 1) & 1u);
            if (!(call(m, owner,
                       {.entry = 0xa8420, .entry_esp = frame - 4, .eax = d},
                       o) &
                  255u) &&
                disposable)
              call(m, owner,
                   {.entry = 0xd81c0, .entry_esp = frame - 8, .arguments = {d}},
                   o);
            else {
              w(m, o, 0x113376, extension, r(m, extension, 1) | 4u, 1);
              w(m, o, 0x113379, manager + 0x6c, r(m, manager + 0x6c, 1) | 1u,
                1);
            }
          }
          if (!(call(m, owner,
                     {.entry = 0x1127d0, .entry_esp = frame - 4, .ecx = d}, o) &
                255u)) {
            w(m, o, 0x113397, frame + 0x11, 1, 1);
            if (r(m, frame + 0x20) == sentinel)
              w(m, o, 0x11339e, frame + 0x20, 0x400000c);
          }
          const auto parent = r(m, p + 0x14);
          if (parent != none &&
              (call(m, owner,
                    {.entry = 0xc83d0, .entry_esp = frame - 4, .ecx = d}, o) &
               255u)) {
            const auto row = r(m, tag(m, r(m, object(m, parent))) + 0x1cc) +
                             std::uint32_t(sx(m, p + 0x1fc)) * 0xb0;
            if ((r(m, row) & 0x800u) && sx(m, row + 0x3e) != -1) {
              w(m, o, 0x11340d, frame + 0x3c, 4);
              w(m, o, 0x113415, frame + 0x14, sentinel);
              w(m, o, 0x11341d, frame + 0x20, 0x400000c);
            }
          }
        }
      }
    }
  }
  const auto mode = r(m, frame + 0x14), activity = r(m, frame + 0x20);
  const bool valid = r(m, manager) != none && sx(m, manager + 6) != -1;
  const bool changed =
      (mode != r(m, manager + 0x70) && mode != sentinel) ||
      (activity != r(m, manager + 0x7c) && activity != sentinel);
  w(m, o, 0x113447, frame + 0x1b, 0, 1);
  if (r(m, frame + 0x11, 1) || !valid)
    w(m, o, 0x113476, frame + 0x1b, 1, 1);
  bool install = false;
  if (changed)
    install = r(m, frame + 0x13, 1) ||
              (call(m, owner,
                    {.entry = 0x111b60,
                     .entry_esp = frame - 16,
                     .arguments = {d, activity, r(m, frame + 0x48, 1)},
                     .argument_masks = {0xffffffffu, 0xffffffffu, 255u}},
                    o) &
               255u);
  install = install || r(m, frame + 0x1b, 1);
  if (install) {
    const bool admitted = source_request_unit_animation_1_1(
        m, {d, {mode, sentinel, sentinel, activity}, 0, 0},
        r(m, frame + 0x3c) | 0x401u, frame - 32, fp, owner, o);
    if (!admitted &&
        ((activity == sentinel && r(m, manager + 0x7c) == 0x400000c) ||
         activity == 0x400000c) &&
        !(r(m, manager + 0x6e, 1) & 1u))
      call(m, owner,
           {.entry = 0x1d0590, .entry_esp = frame - 4, .eax = manager}, o);
    if (r(m, manager + 0x68) != none && r(m, manager) != none &&
        sx(m, manager + 6) != -1 &&
        !source_unit_animation_completed_1_1(m, manager))
      w(m, o, 0x113528, frame + 0x12, 1, 1);
  } else {
    const auto serial = r(m, 0x5662bc) + 1;
    w(m, o, 0x11353c, 0x5662bc, serial);
    if (!((serial ^ d) & 15u)) {
      if (r(m, manager + 0x68) != none && !(r(m, manager + 0x6c, 1) & 1u))
        call(m, owner,
             {.entry = 0x1d1b70,
              .entry_esp = frame - 16,
              .ecx = r(m, manager + 0x74),
              .edx = r(m, manager + 0x78),
              .edi = manager,
              .arguments = {r(m, manager + 0x70), 1, 1}},
             o);
      for (unsigned i = 0; i < 4; ++i) {
        const auto weapon = r(m, p + 0x218 + 4 * i);
        if (weapon != none)
          call(m, owner,
               {.entry = 0x10a240,
                .entry_esp = frame - 4,
                .eax = weapon,
                .ecx = d},
               o);
      }
    }
  }
  auto active = [&](std::uint32_t channel) {
    return r(m, channel) != none && sx(m, channel + 6) != -1;
  };
  auto advance = [&](std::uint32_t channel) {
    if (r(m, manager + 0x68) != none && !(r(m, manager + 0x6c, 1) & 1u))
      source_advance_unit_animation_channel_1_1(m, channel, manager, 0xc04f0, d,
                                                frame - 16, fp, owner, o);
  };
  auto completed = [&](std::uint32_t channel, std::uint32_t site) {
    if (active(channel) && (r(m, channel + 0x12, 1) & 1u) &&
        !(r(m, channel + 0x11, 1) & 9u))
      w(m, o, site, frame + 0x12, 1, 1);
  };
  auto events = [&](std::uint32_t channel, std::uint32_t channel_activity) {
    if (r(m, channel + 0x14, 2))
      call(m, owner,
           {.entry = 0xe8050,
            .entry_esp = frame - 12,
            .eax = channel_activity,
            .esi = d,
            .arguments = {r(m, channel + 0x14, 2), r(m, channel + 0x16, 2)}},
           o);
  };
  if (!(r(m, manager + 0x6c, 1) & 1u)) {
    const auto channel = extension + 0xdc;
    if (active(channel)) {
      advance(channel);
      if ((r(m, channel + 0x11, 1) & 10u) && (r(m, extension, 1) & 0x80u)) {
        call(m, owner,
             {.entry = 0x1c9de0, .entry_esp = frame - 4, .eax = channel}, o);
        w(m, o, 0x1135fa, extension, r(m, extension, 1) & 0x7fu, 1);
        call(m, owner, {.entry = 0x115af0, .entry_esp = frame - 4, .eax = d},
             o);
      }
      completed(channel, 0x11361a);
    }
  }
  if (!(r(m, manager + 0x6c, 1) & 1u)) {
    const auto channel = extension + 0x9c;
    if (active(channel)) {
      advance(channel);
      const auto current = r(m, channel + 8);
      w(m, o, 0x11366e, frame + 0x38, current);
      events(channel, current);
      if (r(m, channel + 0x11, 1) & 14u) {
        const auto flags = r(m, channel + 0x12, 2);
        if (!((flags & 0x4000u) && (flags & 2u))) {
          if (flags & 0x4000u)
            call(m, owner,
                 {.entry = 0x11cc90,
                  .entry_esp = frame - 8,
                  .eax = d,
                  .arguments = {r(m, manager + 0x70)}},
                 o);
          call(m, owner,
               {.entry = 0xe8130,
                .entry_esp = frame - 16,
                .ebx = d,
                .arguments = {current, 0, 0}},
               o);
          if (r(m, channel + 8) == r(m, frame + 0x38) &&
              !(call(m, owner,
                     {.entry = 0x112780, .entry_esp = frame - 4, .eax = d}, o) &
                255u)) {
            call(m, owner,
                 {.entry = 0x1c9de0, .entry_esp = frame - 4, .eax = channel},
                 o);
            if (r(m, p + 0xaa, 1) != 1)
              call(m, owner,
                   {.entry = 0xbb240,
                    .entry_esp = frame - 8,
                    .esi = d,
                    .arguments = {0x3e88b439}},
                   o);
          }
        }
      }
      completed(channel, 0x11371a);
    }
  }
  if (!(r(m, manager + 0x6c, 1) & 1u)) {
    const auto channel = extension + 0xbc;
    if (active(channel)) {
      advance(channel);
      const auto current = r(m, channel + 8);
      events(channel, current);
      if (r(m, channel + 0x11, 1) & 14u) {
        call(m, owner,
             {.entry = 0xe8130,
              .entry_esp = frame - 16,
              .ebx = d,
              .arguments = {current, 0, 0}},
             o);
        if (r(m, channel + 8) == current &&
            !(call(m, owner,
                   {.entry = 0x112780, .entry_esp = frame - 4, .eax = d}, o) &
              255u)) {
          call(m, owner,
               {.entry = 0x1c9de0, .entry_esp = frame - 4, .eax = channel}, o);
          if (r(m, p + 0xaa, 1) != 1)
            call(m, owner,
                 {.entry = 0xbb240,
                  .entry_esp = frame - 8,
                  .esi = d,
                  .arguments = {0x3e08b439}},
                 o);
        }
      }
      completed(channel, 0x1137f2);
    }
  }
  const auto result =
      source_decay_unit_animation_weights_1_1(m, d, frame - 4, fp, o);
  return std::uint8_t(
      r(m, frame + 0x12, 1) | result |
      source_blend_unit_animation_weights_1_1(m, d, frame - 4, fp, o));
}
} // namespace halo2
