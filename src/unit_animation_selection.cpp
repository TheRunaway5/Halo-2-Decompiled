#include "halo2/unit_animation_selection.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none = 0xffffffffu, current = 0x07000101u,
                        default_id = 0x07000001u, default_weapon = 0x030000d9u;
void require(bool ok, const char *why) {
  if (!ok)
    throw std::runtime_error(why);
}
std::uint32_t read(std::span<const std::byte> b, std::size_t at,
                   unsigned n = 4) {
  require(n <= 4 && at <= b.size() && n <= b.size() - at,
          "Truncated original animation selection storage");
  std::uint32_t v = 0;
  for (unsigned i = 0; i < n; ++i)
    v |= std::uint32_t(std::to_integer<unsigned>(b[at + i])) << (8 * i);
  return v;
}
std::uint32_t word(const SourceAnimationGraphMemory &m, std::uint32_t p) {
  require(bool(m.read), "Original animation graph backing unavailable");
  return read(m.read(p, 4), 0);
}
void write(std::span<std::byte> b, std::size_t at, unsigned n, std::uint32_t v,
           std::uint32_t ip, const UnitAnimationSelectionObserver &o = {}) {
  const auto before = read(b, at, n);
  for (unsigned i = 0; i < n; ++i)
    b[at + i] = std::byte((v >> (8 * i)) & 255);
  if (o)
    o({ip, at, n, before, read(b, at, n)});
}
bool valid(std::span<const std::byte> m, std::size_t at) {
  return read(m, at) != none && read(m, at + 6, 2) != 0xffff;
}
float number(std::uint32_t v) { return std::bit_cast<float>(v); }
std::uint32_t bits(float v) { return std::bit_cast<std::uint32_t>(v); }
// Explicit single-precision stores retain original SSE operation boundaries.
float subtract(float a, float b) {
  volatile float v = a - b;
  return v;
}
float divide(float a, float b) {
  volatile float v = a / b;
  return v;
}
float multiply(float a, float b) {
  volatile float v = a * b;
  return v;
}
const UnitAnimationDefinition &
definition(std::span<const std::byte> m, std::size_t at,
           const UnitAnimationSelectionServices &s) {
  require(bool(s.definition),
          "Original animation definition owner unavailable");
  return s.definition(read(m, at), read(m, at + 4));
}
bool authored_transition(std::span<const std::byte> m, std::size_t at,
                         const UnitAnimationSelectionServices &s) {
  return !valid(m, at) || !(definition(m, at, s).authored_flags & 1);
}
void clamp(std::span<std::byte> m, std::size_t at, std::uint32_t frame,
           const UnitAnimationSelectionServices &s) {
  require(bool(s.clamp_frame), "Original1c9f50 frame owner unavailable");
  s.clamp_frame(m, at, frame);
}
std::uint32_t variant(std::span<std::byte> m, std::size_t at,
                      std::uint32_t packed,
                      const UnitAnimationSelectionServices &s) {
  require(bool(s.variant), "Original1ceda0 variant/RNG owner unavailable");
  return s.variant(m, at, packed);
}
bool install_primary(std::span<std::byte> m, std::uint32_t packed,
                     std::uint32_t activity, std::uint32_t flags,
                     const UnitAnimationSelectionServices &s,
                     const UnitAnimationSelectionObserver &o) {
  // Complete1cee00, with actual lower variant owner required only if called.
  if (read(m, 0x68) == none)
    return false;
  if (flags & 0x10)
    packed = variant(m, 0, packed, s);
  if (!install_source_unit_animation_channel(m, 0, read(m, 0x68), flags, packed,
                                             activity, none, none, 0,
                                             s.one_bits, o))
    return false;
  if (flags & 4)
    clamp(m, 0, 0, s);
  if (flags & 8)
    write(m, 0x18, 4, s.one_bits, 0x1cee69, o);
  return true;
}
} // namespace

std::uint32_t
find_source_animation_record(std::uint32_t bank, std::uint32_t key,
                             std::uint32_t stride,
                             const SourceAnimationGraphMemory &m) {
  require(stride >= 4, "Invalid original animation record stride");
  // 1e0f50 uses signed comparisons for count and StringID, and wraps its
  // 32-bit index arithmetic. Backing validation belongs to the host resolver.
  std::int32_t low = 0;
  auto high = std::bit_cast<std::int32_t>(word(m, bank) - 1u);
  bool last = false;
  while (!last && low <= high) {
    last = low == high;
    auto sum =
        std::bit_cast<std::int32_t>(std::uint32_t(low) + std::uint32_t(high));
    auto mid =
        sum >= 0 ? sum / 2 : std::int32_t(-((-std::int64_t(sum) + 1) / 2));
    const auto p = word(m, bank + 4) + std::uint32_t(mid) * stride;
    const auto actual = word(m, p);
    if (actual == key)
      return p;
    if (std::bit_cast<std::int32_t>(actual) > std::bit_cast<std::int32_t>(key))
      high = std::bit_cast<std::int32_t>(std::uint32_t(mid) - 1u);
    else if (std::bit_cast<std::int32_t>(actual) <
             std::bit_cast<std::int32_t>(key))
      low = std::bit_cast<std::int32_t>(std::uint32_t(mid) + 1u);
  }
  return 0;
}
std::uint32_t
find_source_animation_weapon(std::uint32_t graph, std::uint32_t mode,
                             std::uint32_t weapon_class, std::uint32_t weapon,
                             const SourceAnimationGraphMemory &m) {
  auto p = find_source_animation_record(graph + 0x34, mode, 0x14, m);
  if (p)
    p = find_source_animation_record(p + 4, weapon_class, 0x14, m);
  if (p)
    p = find_source_animation_record(p + 4, weapon, 0x34, m);
  return p;
}
SourceAnimationIdentifiers
normalize_source_animation_request(std::span<const std::byte> m,
                                   SourceAnimationIdentifiers in,
                                   const SourceAnimationGraphMemory &g) {
  require(m.size() >= 0x90, "Truncated source animation manager");
  auto out = in;
  if (in[0] == current) {
    out[0] = read(m, 0x70);
    if (out[0] == none)
      out[0] = 0x06000086;
  }
  if (out[0] == default_id)
    out[0] = 0x06000086;
  if (in[1] == current)
    out[1] = read(m, 0x74);
  if (out[1] == default_id || out[1] == none)
    out[1] = 0x07000083;
  if (in[2] == current)
    out[2] = read(m, 0x78);
  if (out[2] == default_id || out[2] == none)
    out[2] = default_weapon;
  require(bool(g.graph_address),
          "Original animation graph identity unavailable");
  out[1] = source_animation_weapon_class(g.graph_address(read(m, 0x68)),
                                          out[1], out[2], g);
  if (in[3] == current) {
    out[3] = read(m, 0x7c);
    if (out[3] == none)
      out[3] = 0x0400000c;
  }
  if (out[3] == default_id)
    out[3] = 0x0400000c;
  return out;
}
std::uint32_t source_animation_weapon_class(
    std::uint32_t graph, std::uint32_t weapon_class, std::uint32_t weapon,
    const SourceAnimationGraphMemory &g) {
  const auto p = find_source_animation_record(graph + 0x54, weapon, 8, g);
  if (p) {
    const auto value = word(g, p + 4);
    if (value != none && value != default_weapon && weapon_class != 0x0400054b)
      weapon_class = value;
  }
  return weapon_class;
}
std::uint32_t
next_source_animation_weapon(std::span<std::byte> state,
                             std::array<std::uint32_t, 3> &out,
                             const SourceAnimationGraphMemory &g) {
  require(state.size() >= 28, "Truncated source animation iterator");
  std::uint32_t result = 0;
  while (read(state, 0x10) < 8) {
    auto counter = read(state, 0x10);
    if (!read(state, 0x14)) {
      if (!(counter & 4) || read(state, 4) != default_weapon) {
        out[0] = (counter & 4) ? default_weapon : read(state, 4);
        write(state, 0x14, 4,
              find_source_animation_record(read(state, 0) + 0x34, out[0], 0x14,
                                           g),
              0x29c6f7);
        write(state, 0x18, 4, 0, 0x29c6fa);
      }
    }
    if (!read(state, 0x14)) {
      write(state, 0x10, 4, (counter + 4) & ~3u, 0x29c7b2);
      write(state, 0x18, 4, 0, 0x29c7b5);
    } else {
      if (!read(state, 0x18)) {
        if (!(counter & 2) || read(state, 8) != default_weapon) {
          out[1] = (counter & 2) ? default_weapon : read(state, 8);
          write(state, 0x18, 4,
                find_source_animation_record(read(state, 0x14) + 4, out[1],
                                             0x14, g),
                0x29c744);
        }
        if (!read(state, 0x18)) {
          counter = (counter + 2) & ~1u;
          write(state, 0x10, 4, counter, 0x29c79f);
          if (!(counter & 2))
            write(state, 0x14, 4, 0, 0x29c7a4);
          continue;
        }
      }
      if (!(counter & 1) || read(state, 0xc) != default_weapon) {
        out[2] = (counter & 1) ? default_weapon : read(state, 0xc);
        result = find_source_animation_record(read(state, 0x18) + 4, out[2],
                                              0x34, g);
      }
      ++counter;
      write(state, 0x10, 4, counter, 0x29c783);
      if (!(counter & 1)) {
        write(state, 0x18, 4, 0, 0x29c78a);
        if (!(counter & 2))
          write(state, 0x14, 4, 0, 0x29c78f);
      }
    }
    if (result)
      return result;
  }
  return result;
}

void reset_source_unit_animation_channels(
    std::span<std::byte> m, std::uint32_t one,
    const UnitAnimationSelectionObserver &o) {
  require(m.size() >= 0x90, "Truncated source animation manager");
  struct Op {
    std::uint32_t ip;
    unsigned at, n;
    bool one;
  };
  constexpr Op rows[]{{0x1d05a0, 0x00, 4, false}, {0x1d05a2, 0x04, 2, false},
                      {0x1d05a6, 0x06, 2, false}, {0x1d05aa, 0x1c, 4, false},
                      {0x1d05af, 0x10, 1, false}, {0x1d05b2, 0x11, 1, false},
                      {0x1d05b5, 0x12, 2, false}, {0x1d05b9, 0x18, 4, true},
                      {0x1d05be, 0x14, 2, false}, {0x1d05c2, 0x16, 2, false},
                      {0x1d05c6, 0x08, 4, false}, {0x1d05c9, 0x0c, 1, false},
                      {0x1d05cc, 0x0d, 1, false}, {0x1d05cf, 0x0e, 2, false},
                      {0x1d05d3, 0x20, 4, false}, {0x1d05d6, 0x24, 2, false},
                      {0x1d05da, 0x26, 2, false}, {0x1d05de, 0x3c, 4, false},
                      {0x1d05e3, 0x30, 1, false}, {0x1d05e6, 0x31, 1, false},
                      {0x1d05e9, 0x32, 2, false}, {0x1d05ed, 0x38, 4, true},
                      {0x1d05f2, 0x34, 2, false}, {0x1d05f6, 0x36, 2, false},
                      {0x1d05fa, 0x28, 4, false}, {0x1d05fd, 0x2c, 1, false},
                      {0x1d0600, 0x2d, 1, false}, {0x1d0603, 0x2e, 2, false},
                      {0x1d0607, 0x40, 4, false}, {0x1d060a, 0x44, 2, false},
                      {0x1d060e, 0x46, 2, false}, {0x1d0612, 0x5c, 4, false},
                      {0x1d0617, 0x50, 1, false}, {0x1d061a, 0x51, 1, false},
                      {0x1d061d, 0x52, 2, false}, {0x1d0621, 0x58, 4, true},
                      {0x1d0626, 0x54, 2, false}, {0x1d062a, 0x56, 2, false},
                      {0x1d062e, 0x48, 4, false}, {0x1d0631, 0x4c, 1, false},
                      {0x1d0634, 0x4d, 1, false}, {0x1d0637, 0x4e, 2, false}};
  for (auto r : rows) {
    const unsigned field = r.at & 0x1f;
    const bool sentinel = field == 0 || field == 4 || field == 6 ||
                          field == 8 || field == 0xc || field == 0xd ||
                          field == 0xe;
    write(m, r.at, r.n, r.one ? one : sentinel ? none : 0, r.ip, o);
  }
}
bool install_source_unit_animation_channel(
    std::span<std::byte> m, std::size_t at, std::uint32_t graph,
    std::uint32_t flags, std::uint32_t packed, std::uint32_t activity,
    std::uint32_t first, std::uint32_t second, std::uint32_t last,
    std::uint32_t one, const UnitAnimationSelectionObserver &o) {
  require(at <= m.size() && 32 <= m.size() - at,
          "Truncated source animation channel");
  if (graph == none || (packed >> 16) == 0xffff)
    return false;
  auto w = [&](unsigned p, unsigned n, std::uint32_t v, std::uint32_t ip) {
    write(m, at + p, n, v, ip, o);
  };
  w(4, 2, none, 0x1c9e78);
  w(6, 2, none, 0x1c9e7c);
  w(0x10, 1, 0, 0x1c9e85);
  w(0x11, 1, 0, 0x1c9e88);
  w(0x14, 2, 0, 0x1c9e8b);
  w(0x16, 2, 0, 0x1c9e8f);
  w(8, 4, activity, 0x1c9e97);
  w(0xc, 1, first, 0x1c9e9e);
  w(0x12, 2, flags, 0x1c9ea5);
  w(0xd, 1, second, 0x1c9ea9);
  w(0, 4, graph, 0x1c9eb2);
  w(0x1c, 4, 0, 0x1c9ebb);
  w(4, 4, packed, 0x1c9ecb);
  w(0x18, 4, one, 0x1c9ece);
  const auto signed_last = (last & 0x80) ? (last & 255) | 0xff00u : last & 255;
  w(0xe, 2, signed_last, 0x1c9ed3);
  w(0x10, 1, (flags >> 14) & 2, 0x1c9ed7);
  return true;
}

bool select_source_unit_animation(std::span<std::byte> m,
                                  const UnitAnimationSelectionRequest &r,
                                  const UnitAnimationSelectionServices &s,
                                  const UnitAnimationSelectionObserver &o) {
  require(m.size() >= 0x90, "Truncated source animation manager");
  const bool old_transition =
      !valid(m, 0) || !(definition(m, 0, s).authored_flags & 2);
  const auto flags = r.selection_flags;
  if (flags & 0x10)
    reset_source_unit_animation_channels(m, s.one_bits, o);
  auto mask = [&](std::size_t at, std::uint32_t v, std::uint32_t ip) {
    write(m, at, 1, read(m, at, 1) & v, ip, o);
  };
  auto set = [&](std::size_t at, std::uint32_t v, std::uint32_t ip) {
    write(m, at, 1, read(m, at, 1) | v, ip, o);
  };
  mask(0x6e, 0xfe, 0x1d0aec);
  require(bool(s.resolve_request),
          "Original1cfde0 graph/resource owner unavailable");
  auto found = s.resolve_request(m, r.identifiers, flags);
  bool result = false;
  bool update_identifiers = false;
  if (!found.admitted || (found.packed >> 16) == 0xffff || (flags & 8)) {
    result = update_identifiers = bool(flags & 4);
  } else {
    UnitAnimationSecondarySelection layer{none, 0};
    std::uint8_t eligible = 0;
    if (!(flags & 0x200)) {
      require(bool(s.secondary), "Original1d0790 secondary owner unavailable");
      layer = s.secondary(m, found.requested[0], found.requested[3]);
      eligible = layer.eligible & (~read(m, 0x6c, 1) & 1);
      const bool interrupted =
          valid(m, 0) && (read(m, 0x12, 1) & 1) && !(read(m, 0x11, 1) & 9);
      eligible &= (flags >> 10) & (interrupted ? 1u : 0u) & 1u;
      if (eligible)
        set(0x6e, 1, 0x1d0bab);
      else
        mask(0x6e, 0xfe, 0x1d0bb1);
    } else {
      mask(0x6e, 0xfe, 0x1d0bb1);
    }
    if ((layer.packed >> 16) == 0xffff && eligible) {
      result = update_identifiers = bool(flags & 4);
    } else {
      float fraction = 0;
      const auto one = number(s.one_bits), zero = number(s.zero_bits),
                 subtractor = number(s.frame_subtractor_bits);
      if ((flags & 0x60) && valid(m, 0)) {
        float denominator =
            subtract(float(definition(m, 0, s).frame_count), subtractor);
        if (denominator > zero) {
          auto frame = number(read(m, 0x1c));
          if (0 > frame)
            frame = 0;
          else if (frame > denominator)
            frame = denominator;
          fraction = divide(frame, denominator);
          if (flags & 0x40)
            fraction = subtract(one, fraction);
          if (0 > fraction)
            fraction = 0;
          else if (fraction > one)
            fraction = one;
        }
      }
      UnitAnimationThirdSelection third{none, none};
      if (!(flags & 0x100)) {
        require(bool(s.third),
                "Original1d06c0 third-channel owner unavailable");
        third = s.third(m, found.requested);
      }
      reset_source_unit_animation_channels(m, s.one_bits, o);
      if (!install_primary(m, found.packed, found.resolved[3],
                           r.installation_flags, s, o)) {
        result = update_identifiers = bool(flags & 4);
      } else {
        result = update_identifiers = true;
        bool transition = authored_transition(m, 0, s) && old_transition;
        if ((layer.packed >> 16) != 0xffff && read(m, 0x68) != none) {
          auto packed = variant(m, 0x40, layer.packed, s);
          if (install_source_unit_animation_channel(
                  m, 0x40, read(m, 0x68), 0x15, packed, found.requested[3],
                  none, none, 3, s.one_bits, o)) {
            clamp(m, 0x40, 0, s);
            if (read(m, 0x12, 1) & 1)
              set(0x11, 1, 0x1d0d2c);
            transition = authored_transition(m, 0x40, s) && transition;
            if (transition) {
              require(bool(s.root_transform_delta),
                      "Original1d1a00 root transform owner unavailable");
              s.root_transform_delta(m);
            }
          }
        }
        if (!valid(m, 0x40) && (flags & 0x60) && fraction > zero) {
          const auto frame = multiply(
              subtract(float(definition(m, 0, s).frame_count), subtractor),
              fraction);
          clamp(m, 0, bits(frame), s);
        }
        if (transition)
          mask(0x6c, 0xef, 0x1d0da6);
        else
          set(0x6c, 0x10, 0x1d0da0);
        if ((third.packed >> 16) != 0xffff && read(m, 0x68) != none &&
            install_source_unit_animation_channel(m, 0x20, read(m, 0x68), 0x3c0,
                                                  third.packed, third.activity,
                                                  none, none, 0, s.one_bits, o))
          write(m, 0x80, 4, 0, 0x1d0ddb, o);
        mask(0x6c, 0xfe, 0x1d0de6);
      }
    }
  }
  if (update_identifiers) {
    bool changed = false;
    for (unsigned i = 0; i < 3; ++i)
      changed |= read(m, 0x70 + 4 * i) != found.requested[i];
    constexpr std::uint32_t ips[]{0x1d0e10, 0x1d0e17, 0x1d0e1e, 0x1d0e2a};
    for (unsigned i = 0; i < 4; ++i)
      write(m, 0x70 + 4 * i, 4, found.requested[i], ips[i], o);
    if (changed) {
      if (read(m, 0x68) != none && !(read(m, 0x6c, 1) & 1)) {
        require(bool(s.resources), "Original1d1b70 resource owner unavailable");
        s.resources(
            m, {found.requested[0], found.requested[1], found.requested[2]}, 1,
            1);
      }
      if (!valid(m, 0x40)) {
        // Keep the original repeated third-channel checks and stores. A
        // source callback may have changed the manager before this stage.
        if (read(m, 0x40) != none && read(m, 0x46, 2) != 0xffff &&
            (read(m, 0x52, 1) & 1))
          mask(0x51, 0xfe, 0x1d0e77);
        if (!valid(m, 0x40)) {
          if (valid(m, 0) && (read(m, 0x12, 1) & 1))
            mask(0x11, 0xfe, 0x1d0e95);
          if (valid(m, 0x20) && (read(m, 0x32, 1) & 1))
            mask(0x31, 0xfe, 0x1d0ea9);
        }
      }
    }
  }
  return result;
}
} // namespace halo2
