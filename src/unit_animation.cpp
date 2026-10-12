#include "halo2/unit_animation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu;
constexpr float value(std::uint32_t bits) { return std::bit_cast<float>(bits); }
// Keep each original scalar SSE rounding point, independent of host FMA flags.
float add(float a, float b) {
  volatile float r = a + b;
  return r;
}
float sub(float a, float b) {
  volatile float r = a - b;
  return r;
}
float mul(float a, float b) {
  volatile float r = a * b;
  return r;
}
float div(float a, float b) {
  volatile float r = a / b;
  return r;
}
void require(bool v, const char *s) {
  if (!v)
    throw std::runtime_error(s);
}
void extent(std::span<const std::byte> b, std::size_t n) {
  require(b.size() >= n, "Source unit animation packet is unavailable");
}
std::uint32_t get(std::span<const std::byte> b, std::size_t p, unsigned n = 4) {
  extent(b, p + n);
  std::uint32_t r = 0;
  for (unsigned i = 0; i < n; ++i)
    r |= std::uint32_t(std::to_integer<unsigned>(b[p + i])) << (i * 8);
  return r;
}
void put(std::span<std::byte> b, std::size_t p, std::uint32_t v,
         unsigned n = 4) {
  extent(b, p + n);
  for (unsigned i = 0; i < n; ++i)
    b[p + i] = std::byte((v >> (i * 8)) & 255);
}
float fp(std::span<const std::byte> b, std::size_t p) {
  return value(get(b, p));
}
void putf(std::span<std::byte> b, std::size_t p, float f) {
  put(b, p, std::bit_cast<std::uint32_t>(f));
}
bool valid(std::span<const std::byte> c) {
  return get(c, 0) != none && get(c, 6, 2) != 0xffff;
}
const UnitAnimationDefinition &clip(std::span<const std::byte> c,
                                    const UnitAnimationServices &s) {
  require(bool(s.definition),
          "Original animation graph/clip owner is unavailable");
  require(get(c, 6, 2) != 0xffff,
          "Original valid animation clip is unavailable");
  return s.definition(get(c, 0), get(c, 4));
}
float end(const UnitAnimationDefinition &d) {
  return sub(float(d.frame_count), value(0x38d1b717));
}
float step(std::span<const std::byte> c, const UnitAnimationServices &s) {
  require(std::isfinite(s.tick_seconds) && std::isfinite(fp(c, 0x18)) &&
              std::isfinite(fp(c, 0x1c)),
          "Unmeasured nonfinite animation clock input");
  return add(mul(mul(s.tick_seconds, fp(c, 0x18)), 30.f), fp(c, 0x1c));
}
void emit(UnitAnimationCallback cb, std::int16_t type, std::int16_t frame,
          std::uint32_t tag, std::uint32_t id, std::uint32_t flags,
          std::uint32_t marker, std::span<std::byte> m, std::span<std::byte> c,
          const UnitAnimationServices &s) {
  if (!cb.identity)
    return;
  require(bool(s.event),
          "Original unit animation event consumer is unavailable");
  std::array<std::byte, 20> p{};
  put(p, 0, std::uint16_t(type), 2);
  put(p, 2, std::uint16_t(frame), 2);
  put(p, 4, tag);
  put(p, 8, id);
  put(p, 12, flags);
  put(p, 16, marker);
  s.event(cb, float(frame), p, m, c);
}
bool admits(std::uint16_t f, std::uint16_t o, float from, float to) {
  return (!(f & 2) || (o & 0x2000)) && (!(f & 4) || (o & 0x1000)) &&
         (!(f & 0x10) || !(to < from)) && (!(f & 0x20) || !(to > from));
}
std::uint32_t traverse(const UnitAnimationDefinition &d, std::uint16_t options,
                       float from, float to, UnitAnimationCallback cb,
                       std::span<std::byte> m, std::span<std::byte> c,
                       const UnitAnimationServices &s) {
  const float lo = from <= to ? from : to, hi = from <= to ? to : from;
  std::uint32_t mask = 0;
  if (!(options & 0x80))
    for (const auto &e : d.timing)
      if (lo <= float(e.frame) && float(e.frame) < hi) {
        mask |= std::uint32_t(1) << (std::uint16_t(e.type) & 31);
        emit(cb, 0, e.frame, none, std::uint32_t(std::int32_t(e.type)), 0,
             0x0600008a, m, c, s);
      }
  if (!(options & 0x100))
    for (const auto &e : d.sounds)
      if (lo <= float(e.frame) && float(e.frame) < hi && e.reference != -1) {
        require(e.reference >= 0 &&
                    std::size_t(e.reference) < d.sound_references.size(),
                "Original animation sound reference is unavailable");
        const auto &r = d.sound_references[std::size_t(e.reference)];
        if (r.tag != none && cb.identity && admits(r.flags, options, from, to))
          emit(cb, 1, e.frame, r.tag, none, r.flags, e.marker, m, c, s);
      }
  if (!(options & 0x200))
    for (const auto &e : d.effects)
      if (lo <= float(e.frame) && float(e.frame) < hi && e.reference != -1) {
        require(e.reference >= 0 &&
                    std::size_t(e.reference) < d.effect_references.size(),
                "Original animation effect reference is unavailable");
        const auto &r = d.effect_references[std::size_t(e.reference)];
        if (r.tag != none && cb.identity && admits(r.flags, options, from, to))
          emit(cb, 2, e.frame, r.tag, none, r.flags, 0x0600008a, m, c, s);
      }
  return mask;
}
void future(std::span<std::byte> c, const UnitAnimationServices &s) {
  put(c, 0x16, 0, 2);
  if (get(c, 6, 2) == 0xffff)
    return;
  const auto &d = clip(c, s);
  const float current = fp(c, 0x1c),
              target = fp(c, 0x18) > 0 ? add(float(d.frame_count), 1.f) : 0.f;
  const float lo = current <= target ? current : target,
              hi = current <= target ? target : current;
  for (const auto &e : d.timing) {
    const float f = float(e.frame);
    if (lo == current ? (lo < f && f <= hi) : (lo <= f && f < hi))
      put(c, 0x16,
          get(c, 0x16, 2) | (std::uint32_t(1) << (std::uint16_t(e.type) & 31)),
          2);
  }
}
void clear(std::span<std::byte> c) {
  put(c, 0, none);
  put(c, 4, 0xffff, 2);
  put(c, 6, 0xffff, 2);
  putf(c, 0x1c, 0.f);
  put(c, 0x10, 0, 1);
  put(c, 0x11, 0, 1);
  put(c, 0x12, 0, 2);
  putf(c, 0x18, 1.f);
  put(c, 0x14, 0, 2);
  put(c, 0x16, 0, 2);
  put(c, 8, none);
  put(c, 0xc, 0xff, 1);
  put(c, 0xd, 0xff, 1);
  put(c, 0xe, 0xffff, 2);
}
void layers(std::span<std::byte> m, const UnitAnimationServices &s) {
  auto a = m.subspan(0, 32), b = m.subspan(32, 32);
  if (s.layer_synchronization && valid(b))
    require(std::isfinite(fp(m, 0x80)),
            "Unmeasured nonfinite animation layer weight");
  bool lower =
      !s.layer_synchronization || !valid(b) || fp(m, 0x80) < value(0x3ee66666);
  if (!lower && fp(m, 0x80) <= value(0x3f0ccccd))
    return;
  put(m, 0x6c, lower ? get(m, 0x6c, 1) & 0xdf : get(m, 0x6c, 1) | 0x20, 1);
  if (valid(a))
    put(a, 0x12, lower ? get(a, 0x12, 2) & 0xfc7f : get(a, 0x12, 2) | 0x380, 2);
  if (valid(b))
    put(b, 0x12, lower ? get(b, 0x12, 2) | 0x380 : get(b, 0x12, 2) & 0xfc7f, 2);
}
bool counter(std::span<std::byte> b) {
  const auto duration = get(b, 1, 1), flags = get(b, 3, 1);
  if (!duration || (flags & 2))
    return false;
  if (flags & 1) {
    const auto current = (get(b, 0, 1) + 1) & 255;
    put(b, 0, current, 1);
    if (std::int8_t(current) >= std::int8_t(duration)) {
      put(b, 0, duration, 1);
      put(b, 3, flags | 2, 1);
    }
  } else {
    const auto current = (get(b, 0, 1) - 1) & 255;
    put(b, 0, current, 1);
    if (std::int8_t(current) <= 0) {
      put(b, 0, 0, 1);
      put(b, 3, flags | 2, 1);
    }
  }
  return true;
}
} // namespace
void clamp_source_unit_animation_frame(std::span<std::byte> c, float frame,
                                       const UnitAnimationServices &s) {
  extent(c, 32);
  require(std::isfinite(frame), "Unmeasured nonfinite animation frame");
  const auto &d = clip(c, s);
  float f = frame < 0 ? 0.f : frame > end(d) ? end(d) : frame;
  putf(c, 0x1c, f);
  if ((get(c, 0x12, 2) & 1) && !(get(c, 0x11, 1) & 9))
    put(c, 0x11,
        step(c, s) >= end(d) ? get(c, 0x11, 1) | 2 : get(c, 0x11, 1) & 0xfd, 1);
  future(c, s);
}
void set_source_unit_animation_frame(std::span<std::byte> c, float requested,
                                     std::span<std::byte> m,
                                     UnitAnimationCallback cb,
                                     const UnitAnimationServices &s) {
  extent(c, 32);
  extent(m, 0x90);
  require(std::isfinite(requested), "Unmeasured nonfinite animation frame");
  auto packed = get(c, 4);
  const auto *definition = &clip(c, s);
  const float old = fp(c, 0x1c), old_end = end(*definition);
  float target = requested < 0 ? 0.f : requested;
  put(c, 0x14, 0, 2);
  put(c, 0x16, 0, 2);
  put(c, 0x11, get(c, 0x11, 1) & 0xf1, 1);
  auto mask = traverse(*definition, std::uint16_t(get(c, 0x12, 2)), old, target,
                       cb, m, c, s);
  if (get(c, 4) != packed)
    return;
  put(c, 0x14, get(c, 0x14, 2) | mask, 2);
  if (target >= old_end) {
    mask = traverse(*definition, std::uint16_t(get(c, 0x12, 2)), old,
                    value(0x7f7fffff), cb, m, c, s);
    if (get(c, 4) != packed)
      return;
    put(c, 0x14, get(c, 0x14, 2) | mask, 2);
    const auto options = get(c, 0x12, 2);
    if (options & 2) {
      if ((options & 0x20) && definition->loop_frame == 0) {
        require(bool(s.select_variant),
                "Original animation variant/RNG producer is unavailable");
        packed = s.select_variant(m, packed);
        put(c, 4, packed);
        if (get(c, 0x12, 1) & 8)
          putf(c, 0x18, 1.f);
        definition = &clip(c, s);
      }
      const float loop = float(definition->loop_frame);
      target = add(sub(target, old_end), loop);
      mask = traverse(*definition, std::uint16_t(get(c, 0x12, 2)), loop, target,
                      cb, m, c, s);
      if (get(c, 4) != packed)
        return;
      put(c, 0x14, get(c, 0x14, 2) | mask, 2);
      put(c, 0x11, get(c, 0x11, 1) | 4, 1);
    } else {
      put(c, 0x11, get(c, 0x11, 1) | 8, 1);
      target = end(*definition);
    }
  }
  if (get(c, 4) == packed)
    clamp_source_unit_animation_frame(c, target, s);
}
void advance_source_unit_animation_channel(std::span<std::byte> c,
                                           std::span<std::byte> m,
                                           UnitAnimationCallback cb,
                                           const UnitAnimationServices &s) {
  extent(c, 32);
  put(c, 0x14, 0, 2);
  put(c, 0x16, 0, 2);
  const auto delay = get(c, 0x10, 1);
  if (delay) {
    put(c, 0x10, delay - 1, 1);
    return;
  }
  if ((get(c, 0x12, 1) & 1) && !(get(c, 0x11, 1) & 9))
    set_source_unit_animation_frame(c, step(c, s), m, cb, s);
}
bool update_source_unit_animation_manager(std::span<std::byte> m, unsigned n,
                                          std::span<std::byte> saved,
                                          std::span<const std::byte> current,
                                          UnitAnimationCallback cb,
                                          const UnitAnimationServices &s) {
  extent(m, 0x90);
  bool result = true;
  for (unsigned i = 0; i < 3 && result; ++i) {
    auto c = m.subspan(i * 32, 32);
    if (valid(c)) {
      if (get(m, 0x68) == none)
        result = false;
      else if (!(get(m, 0x6c, 1) & 1))
        advance_source_unit_animation_channel(c, m, cb, s);
    }
  }
  auto third = m.subspan(64, 32);
  if (valid(third) && (get(third, 0x11, 1) & 0xa)) {
    if (!(clip(third, s).authored_flags & 2)) {
      require(bool(s.save_local_transition),
              "Original local transition bank producer is unavailable");
      s.save_local_transition(m, value(0x3e88b439), n, saved, current);
    }
    clear(third);
    if (valid(m.subspan(0, 32)) && (get(m, 0x12, 1) & 1))
      put(m, 0x11, get(m, 0x11, 1) & 0xfe, 1);
  }
  layers(m, s);
  if (s.layer_synchronization && valid(m.subspan(32, 32))) {
    const auto &a = clip(m.subspan(0, 32), s);
    float fraction = 0.f;
    if (a.frame_count > 0) {
      fraction = div(fp(m, 0x1c), float(a.frame_count));
      if (fraction < 0)
        fraction = 0;
      else if (fraction > 1)
        fraction = 1;
    }
    auto second = m.subspan(32, 32);
    const auto &b = clip(second, s);
    set_source_unit_animation_frame(second, mul(float(b.frame_count), fraction),
                                    m, cb, s);
  }
  return result;
}
bool advance_source_unit_animation_transitions(std::span<std::byte> m) {
  extent(m, 0x90);
  bool changed = counter(m.subspan(0x64, 4));
  if (get(m, 0x67, 1) & 2) {
    put(m, 0x65, 0, 1);
    put(m, 0x64, 0, 1);
    put(m, 0x67, 0, 1);
  }
  return counter(m.subspan(0x60, 4)) || changed;
}
} // namespace halo2
